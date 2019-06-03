#ifndef _GAME_SCREEN_SHAKE_SYSTEM_H
#define _GAME_SCREEN_SHAKE_SYSTEM_H

#include "../../game.h"
#include "screen_shake_components.h"


namespace game {

	struct ScreenShakeSystem {

		void updateFrameTick(
			Game& game,
			Scene& scene,
			const UpdateInfo& ui);
		
		void renderFrameTick(
			Game& game,
			Scene& scene,
			r32 interpolation,
			const i64 realTime,
			const i64 countsPassed);

		void init(
			Game& game);
	};

	/**
	 * Adds a new ScreenShakeProducer component to an existing entity.
	 * @param turbulence  starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe
	 * @param totalTimeToLiveMS  total time the shaker is active, turbulence goes from max to 0 linearly by time
	 * @param radius  radius of the effective area in feet
	 * @return  id of the new ScreenShakeProducer component
	 */
	ComponentId addScreenShakeProducerToEntity(
		EntityId entityId,
		SceneNodeId sceneNodeId,
		r32 turbulence,
		r32 totalTimeToLiveMS,
		r32 radius);
	
	/**
	 * Some screen shake effects are not centered at a point in 3d space, and simply need
	 * to be added to the player so that they always take effect regardless of position.
	 * @param turbulence  starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe
	 * @param totalTimeToLiveMS  total time the shaker is active, turbulence goes from max to 0 linearly by time
	 * @return  id of the new ScreenShakeProducer component
	 */
	ComponentId addScreenShakeProducerToPlayer(
		EntityId playerId,
		r32 turbulence,
		r32 totalTimeToLiveMS = 0.0f);

	/**
	 * Makes a camera shakable by adding a ScreenShakeNode and SceneNode to the CameraInstance
	 * entity. Sets the camera instance scene node to point to the newly created shake-enabled
	 * node. If the camera already contains a ScreenShakeNode, nothing is added and the
	 * existing component id is returned.
	 * 
	 * @return  id of the ScreenShakeNode component
	 */
	ComponentId addScreenShakeNodeToCamera(
		Game& game,
		Scene& scene,
		ComponentId cameraInstanceId);
	
}

#endif