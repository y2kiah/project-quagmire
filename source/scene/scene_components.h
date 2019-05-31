#ifndef _SCENE_COMPONENTS_H
#define _SCENE_COMPONENTS_H

#include "../math/qmath.h"
#include "../utility/dense_queue.h"


typedef h32 SceneNodeId;

/**
 * SceneNode tracks the transform relative to a parent node and contains ids forming an
 * intrusive hierarchical tree. The SceneGraph is traversed starting at the root to get the
 * worldspace position of each node.
 */
struct SceneNode {
	u32			numChildren;		// number of children contained

	// Flags
	u8			positionDirty;		// position needs recalc
	u8			orientationDirty;	// orientation needs recalc
	u8			_padding[2];

	// transform vars
	dvec3	    translationLocal;	// translation relative to parent
	// TODO: was dquat in griffin, still needed?
	dquat	    rotationLocal;		// local rotation quaternion relative to parent

	dvec3	    positionWorld;		// position in world space
	// TODO: was dquat in griffin, still needed?
	dquat	    orientationWorld;	// orientation in world space

	// support scale??

	// intrusive tree vars
	SceneNodeId	firstChild;			// first child node index
	SceneNodeId	nextSibling;		// next sibling node index
	SceneNodeId	prevSibling;		// previous sibling node index
	SceneNodeId	parent;				// parent node index
};

/**
 * ModelInstance is a component that goes along with the SceneNode to make an
 * entity represent a unique instance of a model object in the scene.
 */
struct ModelInstance {
	SceneNodeId	sceneNodeId;		// scene node containing the root of the model instance
	h32			modelId;			// resource handle to the model resource
};

/**
 * The CameraInstance is a component that pairs with a SceneNode to make an entity represent
 * a camera in the scene. The cameraId is obtained from the scene by calling createCamera.
 * The sceneNodeId references the camera's transform in the scene graph, while the movement
 * component referenced by movementId has its own sceneNodeId, which may reference a
 * different node. One example is to enable camera shake where the camera's node is a child
 * of the movement node.
 */
struct CameraInstance {
	SceneNodeId	sceneNodeId;		// scene node containing the camera instance
	h32			movementId;			// movement component controlling the camera
	u32			cameraId;			// id of the referenced camera
	u8			_padding[4];
	char		name[32];			// name of the camera
};

/**
 * The LightInstance component that goes along with the SceneNode to make a light in the
 * scene. A CameraInstance component is also created for the entity when the shadowCaster
 * flag is set. If positionViewspace.w is 1.0, it is a position so the light is treated as
 * a point light or spot light. If positionViewspace.w is 0.0, it is a directional light at
 * infinite distance, and the position represents to direction from the scene to the light
 * (in viewspace coordinates). The spot direction is set in worldspace coordinates from the
 * light to the scene for directional lights and spot lights. The distinction between point
 * light and spot light is made with the isSpotLight flag. 
 */
struct LightInstance {
	SceneNodeId	sceneNodeId;			// scene node containing the light instance
	vec4		positionViewspace;		// position of light in viewSpace
	vec3		directionViewspace;		// direction spotlight is pointing
	vec3		ambient;				// ambient light color
	vec3		diffuseSpecular;		// diffuse/specular light color
	r32			attenuationConstant;	// constant added to denominator, usually 1.0
	r32			linearAttenuation;		// constant multiplied by inverse distance
	r32			quadraticAttenuation;	// constant multiplied by inverse square of distance
	r32			spotAngleCutoff;		// spotlight angle cutoff, dot product comparison (% from 90deg)
	r32			spotEdgeBlendPct;		// spotlight edge blend, in % of spot radius
	r32			volumeRadius;			// radius from light origin of containing geometry
	u8			isSpotLight;			// 1 if spot light
	u8			isPointLight;			// 1 if point light
	u8			isDirectionalLight;		// 1 if directional light
	u8			isShadowCaster;			// 1 if light casts shadow
};

/**
 * The Movement component is present for all SceneNodes that aren't static. This structure
 * contains prev/next values so the render loop can interpolate between them to get the
 * final rendered position, which is then set in the SceneNode. IT IS UP TO YOU to use this
 * component correctly for movement, in other words you have to set the prev/next values and
 * dirty flags on each update tick so the movement system behaves correctly. It is always an
 * option to NOT use this component to achieve movement in special cases, but you would have
 * to handle the interpolation yourself in a renderTick handler and set the SceneNode values
 * directly.
 */
struct Movement {
	SceneNodeId	sceneNodeId;			// scene node controlled by this movement component

	// flags, TODO: consider converting to bit flags
	u8			translationDirty;		// position needs recalc
	u8			rotationDirty;			// orientation needs recalc
	u8			prevTranslationDirty;	// previous value of translationDirty
	u8			prevRotationDirty;		// previous value of rotationDirty
	u8			_padding[4];
	
	// movement vars
	dvec3		prevTranslation;		// previous local translation
	dvec3		nextTranslation;		// next local translation
	dquat		prevRotation;			// previous local rotation
	dquat		nextRotation;			// next local rotation
};

/**
 * All entities that can be rendered have a RenderCullInfo component. This stores the data
 * needed to keep track of its indexing in the space partitioning structure, and the data
 * needed to perform frustum culling.
 */
// TODO: this appears to be the right component for storage in the spatial grid 
struct RenderCullInfo {
	SceneNodeId	sceneNodeId;			// scene node related to this render culling information
	u32			visibleFrustumBits;		// bits representing visibility in frustums
	u32			minWorldAABB[3];		// AABB integer lower coords in worldspace
	u32			maxWorldAABB[3];		// AABB integer upper coords in worldspace
	r32			viewspaceBSphere[4];	// bounding sphere x,y,z,r in viewspace
};


#endif