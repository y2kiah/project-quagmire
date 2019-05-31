#ifndef _SCENE_API_H
#define _SCENE_API_H

#include "scene.h"


/**
 * Add a SceneNode component to the entity and incorporate it into the scene graph as a
 * child of the parentNode. If component already exists in entity, the node is moved to
 * the new parent and position.
 * 
 * @param entityId	entity to which a SceneNode Component is added
 * @param translationLocal	local position relative to the parent node
 * @param rotationLocal	local rotation relative to the parent node
 * @param parentNodeId	SceneNode component of parent node, NullId_T for the root node
 * @return ComponentId of the SceneNode added to the entity
 */
SceneNodeId scene_addEntity(
	Scene& scene,
	EntityId entityId,
	const dvec3& translationLocal,
	const dquat& rotationLocal,
	SceneNodeId parentNodeId);


/**
 * Removes the SceneNode component from the entity and also fixes up the scene graph.
 * Use this function when you want to remove a specific SceneNode or its entire branch.
 * 
 * @param sceneNodeId	ComponentId of the SceneNode to remove
 * @param cascade	If true the node's descendants are given to the node's parent not
 *			destroyed. If false the node's descendants are removed.
 * @param removedEntities	vector to push the removed descendant entity ids, not
 *			including the top removed entity, or nullptr if you don't care. The caller
 *			is responsible for the vector, this function only uses push_back.
 * @return true if removed, false if SceneNode component not present
 */
bool scene_removeNode(
	Scene& scene,
	SceneNodeId sceneNodeId,
	bool cascade,
	EntityIdQueue* outRemovedEntities = nullptr);


/**
 * Removes all SceneNode components from the entity and also fixes up the scene graph.
 * Use this function for entities with many SceneNode branches (not just a single branch
 * from one root node) when all branches should be removed.
 * 
 * @param sceneNodeId	ComponentId of the SceneNode to remove
 * @param cascade	If true the node's descendants are given to the node's parent not
 *			destroyed. If false the node's descendants are removed.
 * @param removedEntities	vector to push the removed descendant entity ids, not
 *			including the top removed entity, or nullptr if you don't care. The caller
 *			is responsible for the vector, this function only uses push_back.
 * @return true if removed, false if SceneNode component not present
 */
bool scene_removeEntity(
	Scene& scene,
	EntityId entityId,
	bool cascade,
	EntityIdQueue* outRemovedEntities = nullptr);


// TODO: may need a toggle to control switching entity owner to the new parent's entity
/**
 * Moves a sceneNode referenced by sceneNodeId from its current parent to a new parent.
 * Take care not to move the node to a parent deeper in its own branch, as that would
 * break the tree, and the case is not checked for performance reasons. This function
 * does cascade so all descendants of sceneNodeId move with it.
 * 
 * @param sceneNodeId	id of the scene node to move
 * @param moveToParent	the new parent id
 * @return true if the node is moved, false if a given id is invalid or if
 *		moveToParent is already the current parent
 */
bool scene_moveNode(
	Scene& scene,
	SceneNodeId sceneNodeId,
	SceneNodeId moveToParent);


// TODO: may need a toggle to control switching entity owner to the new parent's entity
/**
 * Moves the sceneNode referenced by siblingToMove, and each of its siblings by getting
 * the full list from the current parent, and moving each to the front of the new
 * parent's child list. Take care not to move the siblings to a parent deeper in their
 * own branch, as that would break the tree, and the case is not checked for performance
 * reasons. This function does cascade so all descendants of the nodes move with them.
 * 
 * @param siblingToMove	id of the scene node to move, along with all siblings
 * @param moveToParent	the new parent id
 * @param excludeEntityId	pass this to prevent moving any children owned by the
 *	specified entity, effectively moving only scene children owned by other entities
 * @return true if the nodes are moved, false if a given id is invalid or if
 *		moveToParent is already the current parent
 */
bool scene_moveAllSiblings(
	Scene& scene,
	SceneNodeId siblingToMove,
	SceneNodeId moveToParent,
	EntityId excludeEntityId);


u32 scene_createCamera(
	Scene& scene,
	const CameraParameters& cameraParams,
	bool makeActive);


void scene_setActiveCamera(
	Scene& scene,
	u32 cameraId);


#endif