#include "scene_api.h"
#include "../math/qmath.h"


/**
 * Iterates through the child linked list and returns the last child (where nextSibling
 * is NullId_T). This is a O(n) linear operation but most lists besides root should be
 * pretty short.
 * @param sceneNodeId	the parent node of the last child to look for
 * @return scene node id of the last child, or NullId_T if there are no children
 */
SceneNodeId getLastImmediateChild(
	Scene& scene,
	SceneNodeId sceneNodeId)
{
	if (!scene.components.sceneNodes.has(sceneNodeId)) {
		return null_h32;
	}

	SceneNode& node = scene.components.sceneNodes[sceneNodeId]->data;
	SceneNodeId childId = node.firstChild;
	if (childId != null_h32) {
		for (;;) {
			SceneNode& child = scene.components.sceneNodes[sceneNodeId]->data;
			if (child.nextSibling == null_h32) {
				break;
			}
			childId = child.nextSibling;
		}
	}

	return childId;
}


/**
 * Starts at sceneNodeId and push all nodes in its descendant tree into outDescendants.
 * The caller is responsible for the vector outDescendants, it is not cleared before
 * pushing the nodes.
 * 
 * @param sceneNodeId	the parent node of the descendants to collect
 * @param outDescendants	vector to push_back the descendant SceneNodeIds into
 */
void collectDescendants(
	Scene& scene,
	SceneNodeId sceneNodeId,
	SceneNodeIdQueue& outDescendants,
	MemoryArena& frameScoped)
{
	// get some temporary storage for the traversal queue
	ScopedTemporaryMemory temp = scopedTemporaryMemory(frameScoped);

	const size_t bfsQueueSize = scene.components.sceneNodes.length();
	SceneNodeId* bfsQueueBuffer = allocArrayOfType(frameScoped, SceneNodeId, bfsQueueSize);
	SceneNodeIdQueue bfsQueue(bfsQueueSize, bfsQueueBuffer);
	
	SceneNode& node = scene.components.sceneNodes[sceneNodeId]->data;
	
	bfsQueue.push(sceneNodeId);

	while (!bfsQueue.empty()) {
		SceneNodeId thisId = *bfsQueue.front();

		SceneNode& node = scene.components.sceneNodes[thisId]->data;
		SceneNodeId childId = node.firstChild;
		for (u32 c = 0; c < node.numChildren; ++c) {
			SceneNode& child = scene.components.sceneNodes[childId]->data;
			outDescendants.push(childId);
			bfsQueue.push(childId);
			childId = child.nextSibling;
		}

		bfsQueue.pop_fifo();
	}
}


SceneNodeId scene_addSceneNodeToEntity(
	Scene& scene,
	EntityId entityId,
	const dvec3& translationLocal,
	const dquat& rotationLocal,
	SceneNodeId parentNodeId)
{
	// get the parent node where we're inserting this component
	SceneNode& parentNode = (parentNodeId == null_h32)
		? scene.root
		: scene.components.sceneNodes[parentNodeId]->data;

	// add a SceneNode component to the entity
	Scene::Components::SceneNodeComponent snc{
		SceneNode{
			0,															// numChildren
			0,															// positionDirty
			0,															// orientationDirty
			{},															// padding
			translationLocal,											// translationLocal
			rotationLocal,												// rotationLocal
			parentNode.positionWorld + translationLocal,				// positionWorld
			normalize(parentNode.orientationWorld * rotationLocal),     // orientationWorld
			null_h32,													// firstChild
			parentNode.firstChild,										// nextSibling
			null_h32,													// prevSibling
			parentNodeId												// parent
		},
		entityId
	};

	SceneNodeId nodeId = scene.components.sceneNodes.insert(&snc);
	entity_addComponent(
		scene.entities[entityId]->sceneComponents,
		nodeId);

	if (nodeId != null_h32) {
		// push new node to the front of parent't list
		// set the prevSibling of the former first child
		if (parentNode.firstChild != null_h32) {
			scene.components.sceneNodes[parentNode.firstChild]->data.prevSibling = nodeId;
		}

		// make the new node the first child of its parent
		parentNode.firstChild = nodeId;
		++parentNode.numChildren;
	}

	return nodeId;
}


NewEntityResult scene_createNewEntity(
	Scene& scene,
	bool inScene,
	bool movable,
	SceneNodeId parentNode)
{
	NewEntityResult result{};

	Entity* entity = nullptr;
	result.entityId = scene.entities.insert(nullptr, &entity);

	if (inScene) {
		result.sceneNodeId = scene_addSceneNodeToEntity(
			scene,
			result.entityId,
			dvec3{},
			dquat{},
			parentNode);
	
		if (movable) {
			Scene::Components::MovementComponent mc{};
			mc.entityId = result.entityId;
			mc.data.sceneNodeId = result.sceneNodeId;

			result.movementId = scene.components.movement.insert(&mc);
			entity_addComponent(entity->sceneComponents, result.movementId);
		}
	}

	return result;
}


bool scene_removeNode(
	Scene& scene,
	SceneNodeId sceneNodeId,
	bool cascade,
	EntityIdQueue* outRemovedEntities,
	MemoryArena& frameScoped)
{
	if (!scene.components.sceneNodes.has(sceneNodeId)) {
		return false;
	}

	Scene::Components::SceneNodeComponent& thisCmp =
		*scene.components.sceneNodes[sceneNodeId];
	Entity& entity = *scene.entities[thisCmp.entityId];
	SceneNode& node = thisCmp.data;
	SceneNode& parentNode = (node.parent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->data;

	// remove from scene graph
	// if this was the firstChild, set the new one
	if (node.prevSibling == null_h32) {
		parentNode.firstChild = node.nextSibling;
		scene.components.sceneNodes[parentNode.firstChild]->data.prevSibling = null_h32;
	}
	// fix the node's sibling linked list
	else {
		scene.components.sceneNodes[node.prevSibling]->data.nextSibling = node.nextSibling;
	}

	--parentNode.numChildren;

	if (node.numChildren > 0) {
		// remove all descendants
		if (cascade) {
			ScopedTemporaryMemory temp = scopedTemporaryMemory(frameScoped);
			
			const size_t bufferSize = scene.components.sceneNodes.length();
			SceneNodeId* handleQueueBuffer = allocArrayOfType(frameScoped, SceneNodeId, bufferSize);
			SceneNodeIdQueue handleQueue(bufferSize, handleQueueBuffer);
			
			collectDescendants(scene, sceneNodeId, handleQueue, frameScoped);
			
			if (outRemovedEntities != nullptr) {
				EntityIdQueue& rmvEnt = *outRemovedEntities;
				while (!handleQueue.empty())
				{
					SceneNodeId descSceneNodeId = *handleQueue.pop_fifo();
					EntityId entityIdOfRemoved = scene.components.sceneNodes[descSceneNodeId]->entityId;
					if (entityIdOfRemoved != thisCmp.entityId) {
						rmvEnt.push(entityIdOfRemoved);
					}
					scene_removeNode(scene, descSceneNodeId, true, outRemovedEntities);
				}
			}
			else {
				while (!handleQueue.empty())
				{
					SceneNodeId descSceneNodeId = *handleQueue.pop_fifo();
					scene_removeNode(scene, descSceneNodeId, true, nullptr);
				}
			}
		}
		// don't cascade the delete, give the node's children to its parent,
		// excluding nodes that are directly owned by this entity
		else {
			scene_moveAllSiblings(
				scene,
				node.firstChild,
				node.parent,
				thisCmp.entityId);
		}
	}

	// remove this SceneNode component from the store and entity
	bool removed =
		entity_removeComponent(entity.sceneComponents, sceneNodeId)
		&& scene.components.sceneNodes.erase(sceneNodeId);
	
	assert(removed);
	return removed;
}


bool scene_removeEntity(
	Scene& scene,
	EntityId entityId,
	bool cascade,
	EntityIdQueue* outRemovedEntities)
{
	if (!scene.entities.has(entityId)) {
		return false;
	}
	Entity& entity = *scene.entities[entityId];

	bool removed = false;

	for (SceneNodeId sceneNodeId =
			entity_getFirstComponent(
				entity.sceneComponents,
				Scene::Components::SceneNodeComponentType);
		sceneNodeId != null_h32;)
	{
		removed = removed &&
			scene_removeNode(scene, sceneNodeId, cascade, outRemovedEntities);
	}

	return removed;
}


bool scene_moveNode(
	Scene& scene,
	SceneNodeId sceneNodeId,
	SceneNodeId moveToParent)
{
	assert(sceneNodeId != moveToParent && "can't move a node into itself");

	if (!scene.components.sceneNodes.has(sceneNodeId)
		|| !scene.components.sceneNodes.has(moveToParent))
	{
		return false;
	}

	SceneNode& node = scene.components.sceneNodes[sceneNodeId]->data;
	// check to make sure we're not trying to move into the current parent
	if (node.parent == moveToParent) {
		return false;
	}

	SceneNode& currentParent = (node.parent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->data;
	
	SceneNode& newParent = (moveToParent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->data;

	// remove from current parrent
	// if this was the firstChild, set the new one
	if (node.prevSibling == null_h32) {
		currentParent.firstChild = node.nextSibling;
		scene.components.sceneNodes[currentParent.firstChild]->data.prevSibling = null_h32;
	}
	// fix the node's sibling linked list
	else {
		scene.components.sceneNodes[node.prevSibling]->data.nextSibling = node.nextSibling;
	}

	--currentParent.numChildren;

	// move to new parent
	node.parent = moveToParent;
	node.nextSibling = newParent.firstChild;
	node.prevSibling = null_h32;
	if (newParent.firstChild != null_h32) {
		scene.components.sceneNodes[newParent.firstChild]->data.prevSibling = sceneNodeId;
	}
	newParent.firstChild = sceneNodeId;
			
	++newParent.numChildren;

	return true;
}


bool scene_moveAllSiblings(
	Scene& scene,
	SceneNodeId siblingToMove,
	SceneNodeId moveToParent,
	EntityId excludeEntityId)
{
	if (!scene.components.sceneNodes.has(siblingToMove)
		|| !scene.components.sceneNodes.has(moveToParent))
	{
		return false;
	}

	SceneNode& node = scene.components.sceneNodes[siblingToMove]->data;
	// check to make sure we're not trying to move into the current parent
	if (node.parent == moveToParent) {
		return false;
	}

	SceneNode& currentParent = (node.parent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->data;

	bool allMoved = true;

	// move each child to the new parent
	SceneNodeId childId = currentParent.firstChild;
	
	while (childId != null_h32) {
		Scene::Components::SceneNodeComponent& childCmp =
			*scene.components.sceneNodes[childId];
		
		// move the child as long as it isn't owned by the excluded entity
		// also make sure we're not trying to move a node into itself
		if ((excludeEntityId == null_h32 || childCmp.entityId != excludeEntityId)
			&& childId != moveToParent)
		{
			SceneNode& child = childCmp.data;
			allMoved = allMoved && scene_moveNode(scene, childId, moveToParent);
			childId = child.nextSibling;
		}
	}

	assert(currentParent.numChildren == 0 && "numChildren > 0 but all siblings should have moved");

	return allMoved;
}


EntityId scene_createCamera(
	Scene& scene,
	const CameraParameters& params,
	const char *name,
	bool shakable,
	SceneNodeId parentNode)
{
	NewEntityResult newIds = scene_createNewEntity(scene, true, true, parentNode);
	
	Scene::Components::CameraInstanceComponent cam{};
	cam.entityId = newIds.entityId;
	cam.data.sceneNodeId = newIds.sceneNodeId;
	cam.data.movementId = newIds.movementId;
	strcpy_s(cam.data.name, sizeof(cam.data.name), name);

	if (params.cameraType == Camera_Perspective) {
		cam.data.camera = makePerspectiveCamera(
			(params.viewportWidth / params.viewportHeight),
			params.fovDegreesVertical,
			params.nearClip,
			params.farClip,
			dvec3{},
			quat{});
	}
	else if (params.cameraType == Camera_Ortho) {
		assert(false && "not implemented");
		/*makeOrthoCamera(
			0.0f,
			params.viewportWidth,
			0.0f,
			params.viewportHeight,
			params.nearClip,
			params.farClip);*/
	}
	
	//bool makePrimary = (s.cameras.size() == 0); // if this is the first camera in the scene, make it primary
	
	ComponentId camInstId = scene.components.cameraInstances.insert(&cam);

	if (shakable) {
		game::addScreenShakeNodeToCamera(*_game, scene, camInstId);
	}

	return cam.entityId;
}


// TODO: take a viewport param
void scene_setActiveCamera(
	Scene& scene,
	u32 cameraId)
{
	activeRenderCamera = cameraId;
}


// TODO: not sure this belongs in scene_api
void createGameScene(
	Scene& scene)
{
	// TODO: init component stores
	
	
	//bfsQueue.reserve(RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE);
	//handleBuffer.reserve(RESERVE_SCENEGRAPH_TRAVERSAL_QUEUE);

	scene.root = {};
	scene.root.rotationLocal = dquat_default;
	scene.root.orientationWorld = dquat_default;
}
