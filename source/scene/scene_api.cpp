#include "scene_api.h"


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

	SceneNode& node = scene.components.sceneNodes[sceneNodeId]->component;
	SceneNodeId childId = node.firstChild;
	if (childId != null_h32) {
		for (;;) {
			SceneNode& child = scene.components.sceneNodes[sceneNodeId]->component;
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
	std::vector<SceneNodeId>& outDescendants)
{
	DenseQueueTyped(SceneNodeId, SceneNodeIdQueue);
	const size_t bfsQueueSize = scene.components.sceneNodes.length();
	SceneNodeIdQueue bfsQueue(bfsQueueSize, bfsQueueBuffer);
	
	SceneNode& node = scene.components.sceneNodes[sceneNodeId]->component;
	
	bfsQueue.push(sceneNodeId);

	while (!bfsQueue.empty()) {
		SceneNodeId thisId = bfsQueue.front();

		SceneNode& node = scene.components.sceneNodes[thisId]->component;
		SceneNodeId childId = node.firstChild;
		for (u32 c = 0; c < node.numChildren; ++c) {
			SceneNode& child = scene.components.sceneNodes[childId]->component;
			outDescendants.push_back(childId);
			bfsQueue.push(childId);
			childId = child.nextSibling;
		}

		bfsQueue.pop();
	}
}


SceneNodeId scene_addEntity(
	Scene& scene,
	EntityId entityId,
	const dvec3& translationLocal,
	const dquat& rotationLocal,
	SceneNodeId parentNodeId)
{
	// TODO: make sure entity isn't already in a scene (has a scenenode)

	// get the parent node where we're inserting this component
	auto& parentNode = (parentNodeId == null_h32)
		? scene.root
		: scene.components.sceneNodes[parentNodeId]->component;

	// add a SceneNode component to the entity
	Scene::Components::SceneNodeComponentRecord cr{
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

	SceneNodeId nodeId = scene.components.sceneNodes.insert(&cr);
	entity_addComponent(
		scene.entities[entityId]->sceneComponents,
		nodeId);

	if (nodeId != null_h32) {
		// push new node to the front of parent't list
		// set the prevSibling of the former first child
		if (parentNode.firstChild != null_h32) {
			scene.components.sceneNodes[parentNode.firstChild]->component.prevSibling = nodeId;
		}

		// make the new node the first child of its parent
		parentNode.firstChild = nodeId;
		++parentNode.numChildren;
	}

	return nodeId;
}


bool scene_removeNode(
	Scene& scene,
	SceneNodeId sceneNodeId,
	bool cascade,
	EntityIdQueue* outRemovedEntities)
{
	if (!scene.components.sceneNodes.has(sceneNodeId)) {
		return false;
	}

	Scene::Components::SceneNodeComponentRecord& thisNodeCR =
		*scene.components.sceneNodes[sceneNodeId];
	Entity& entity = *scene.entities[thisNodeCR.entityId];
	SceneNode& node = thisNodeCR.component;
	SceneNode& parentNode = (node.parent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->component;

	// remove from scene graph
	// if this was the firstChild, set the new one
	if (node.prevSibling == null_h32) {
		parentNode.firstChild = node.nextSibling;
		scene.components.sceneNodes[parentNode.firstChild]->component.prevSibling = null_h32;
	}
	// fix the node's sibling linked list
	else {
		scene.components.sceneNodes[node.prevSibling]->component.nextSibling = node.nextSibling;
	}

	--parentNode.numChildren;

	if (node.numChildren > 0) {
		// remove all descendants
		if (cascade) {
			handleBuffer.clear();
			collectDescendants(scene, sceneNodeId, handleBuffer);
			if (outRemovedEntities != nullptr) {
				EntityIdQueue& rmvEnt = *outRemovedEntities;
				for (auto descSceneNodeId : handleBuffer) {
					EntityId entityIdOfRemoved = scene.components.sceneNodes[descSceneNodeId].entityId;
					if (entityIdOfRemoved != thisNodeCR.entityId) {
						rmvEnt.push_back(entityIdOfRemoved);
					}
					scene_removeNode(scene, descSceneNodeId, true, outRemovedEntities);
				}
			}
			else {
				for (auto descSceneNodeId : handleBuffer) {
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
				thisNodeCR.entityId);
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

	SceneNode& node = scene.components.sceneNodes[sceneNodeId]->component;
	// check to make sure we're not trying to move into the current parent
	if (node.parent == moveToParent) {
		return false;
	}

	SceneNode& currentParent = (node.parent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->component;
	
	SceneNode& newParent = (moveToParent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->component;

	// remove from current parrent
	// if this was the firstChild, set the new one
	if (node.prevSibling == null_h32) {
		currentParent.firstChild = node.nextSibling;
		scene.components.sceneNodes[currentParent.firstChild]->component.prevSibling = null_h32;
	}
	// fix the node's sibling linked list
	else {
		scene.components.sceneNodes[node.prevSibling]->component.nextSibling = node.nextSibling;
	}

	--currentParent.numChildren;

	// move to new parent
	node.parent = moveToParent;
	node.nextSibling = newParent.firstChild;
	node.prevSibling = null_h32;
	if (newParent.firstChild != null_h32) {
		scene.components.sceneNodes[newParent.firstChild]->component.prevSibling = sceneNodeId;
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

	SceneNode& node = scene.components.sceneNodes[siblingToMove]->component;
	// check to make sure we're not trying to move into the current parent
	if (node.parent == moveToParent) {
		return false;
	}

	SceneNode& currentParent = (node.parent == null_h32)
		? scene.root
		: scene.components.sceneNodes[node.parent]->component;

	bool allMoved = true;

	// move each child to the new parent
	SceneNodeId childId = currentParent.firstChild;
	
	while (childId != null_h32) {
		auto& childCR = *scene.components.sceneNodes[childId];
		
		// move the child as long as it isn't owned by the excluded entity
		// also make sure we're not trying to move a node into itself
		if ((excludeEntityId == null_h32 || childCR.entityId != excludeEntityId)
			&& childId != moveToParent)
		{
			auto& child = childCR.component;
			allMoved = allMoved && scene_moveNode(scene, childId, moveToParent);
			childId = child.nextSibling;
		}
	}

	assert(currentParent.numChildren == 0 && "numChildren > 0 but all siblings should have moved");

	return allMoved;
}


u32 scene_createCamera(
	Scene& scene,
	const CameraParameters& cameraParams,
	bool makeActive)
{
	CameraPtr camPtr = nullptr;

	if (cameraParams.cameraType == Camera_Perspective) {
		// add a perspective view camera
		camPtr = std::make_shared<CameraPersp>(cameraParams.viewportWidth, cameraParams.viewportHeight,
											   cameraParams.verticalFieldOfViewDegrees,
											   cameraParams.nearClipPlane, cameraParams.farClipPlane);
	}
	else if (cameraParams.cameraType == Camera_Ortho) {
		// add an orthographic view camera
		camPtr = std::make_shared<CameraOrtho>(0.0f, static_cast<float>(cameraParams.viewportWidth),
											   static_cast<float>(cameraParams.viewportHeight), 0.0f,
											   cameraParams.nearClipPlane, cameraParams.farClipPlane);
	}

	camPtr->calcMatrices();
	cameras.push_back(camPtr);
	u32 newCameraId = static_cast<u32>(cameras.size() - 1);

	if (makeActive) {
		setActiveCamera(newCameraId);
	}

	return newCameraId;
}


// TODO: take a viewport index
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
