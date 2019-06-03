#ifndef _SCREEN_SHAKE_COMPONENTS_H
#define _SCREEN_SHAKE_COMPONENTS_H

#include "../../scene/scene.h"
#include "../../scene/entity.h"


/**
 * Causes shake on nearby ScreenShakeNodes
 */
struct ScreenShakeProducer {
	SceneNodeId	sceneNodeId;		// center position of the shake producer
	r32			startTurbulence;	// starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe
	r32			turbulence;			// current turbulence level
	r32			totalTimeToLiveMS;	// total time the shaker is active, turbulence goes from startTurbulence to 0 linearly over this time
	r32			radius;				// radius of the effective area in feet
	r32			shakeFreqHz;		// frequency of shake oscillations in Hz, somewhere in the ballpark of 10.0 tends to work well
	r32			maxAngle;			// max possible shake angle for yaw, pitch and roll
};

/**
 * Pairs with a parent SceneNode and receives shake from nearby ScreenShakeProducers.
 */
struct ScreenShakeNode {
	SceneNodeId	sceneNodeId;		// SceneNode for the CameraInstance to base shake angles on
	SceneNodeId	baseSceneNodeId;	// movement node of the CameraInstance (camera transform without shake applied)
									// shake SceneNode rebased on this every frame so shake doesn't wander from view direction
	ComponentId	cameraInstanceId;	// CameraInstance that this shake node controls
	r32			prevTurbulence;		// previous effective turbulence used for interpolation
	r32			nextTurbulence;		// next effective turbulence used for interpolation
	r32			prevNoiseTime;		// previous time for perlin noise used for interpolation
	r32			nextNoiseTime;		// next time for perlin noise used for interpolation
	r32			prevMaxAngle;		// previous max shake angle in degrees used for interpolation
	r32			nextMaxAngle;		// next max shake angle in degrees used for interpolation
};


#endif