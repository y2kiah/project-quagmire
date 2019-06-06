#include "screen_shake_system.h"


void game::ScreenShakeSystem::updateFrameTick(
	Game& game,
	Scene& scene,
	const UpdateInfo& ui)
{
	// for each ScreenShakeNode (receiver)
	for (u16 n = 0;
		n < game.components.shakeNodes.length();
		++n)
	{
		ScreenShakeNode& shakeNode = game.components.shakeNodes.item(n).data;
		SceneNode& shakeSceneNode = scene.components.sceneNodes[shakeNode.sceneNodeId]->data;

		shakeNode.prevTurbulence = shakeNode.nextTurbulence;
		shakeNode.nextTurbulence = 0;

		shakeNode.prevMaxAngle = shakeNode.nextMaxAngle;
		shakeNode.nextMaxAngle = 0;

		r32 shakeFreqHz = 0;

		// for each ScreenShakeProducer component, total turbulence, angle, freq for the receiver
		for (u16 p = 0;
			p < game.components.shakeProducers.length();
			++p)
		{
			ScreenShakeProducer& producer = game.components.shakeProducers.item(p).data;
			SceneNode& producerSceneNode = scene.components.sceneNodes[producer.sceneNodeId]->data;

			r32 producerRadiusSq = producer.radius * producer.radius;

			vec3 nodeToProducer = make_vec3(shakeSceneNode.positionWorld - producerSceneNode.positionWorld);
			r32 distSq = dot(nodeToProducer, nodeToProducer);

			// determine the effectiveness based on distance ratio to radius of the producer
			r32 effectiveTurbulence = (producerRadiusSq == 0.0f)
				? producer.turbulence
				: producer.turbulence * max((producerRadiusSq - distSq) / producerRadiusSq, 0.0f);

			// accumulate effective turbulence level from all producers
			shakeNode.nextTurbulence += effectiveTurbulence;

			// take the max angle and shake freq encountered
			shakeNode.nextMaxAngle = (producer.maxAngle > shakeNode.nextMaxAngle
										? producer.maxAngle : shakeNode.nextMaxAngle);

			shakeFreqHz = (producer.shakeFreqHz > shakeFreqHz ? producer.shakeFreqHz : shakeFreqHz);
		}
//		shakeFreqHz = 10.0f;
//		shakeNode.nextTurbulence = 0.7;
//		shakeNode.nextMaxAngle = 2.0f;

		// set noise time interpolation values
		shakeNode.prevNoiseTime = shakeNode.nextNoiseTime;
		if (shakeNode.prevNoiseTime > 256.0f) {
			shakeNode.prevNoiseTime -= 256.0f;
		}
		shakeNode.nextNoiseTime = shakeNode.prevNoiseTime + (ui.deltaT * shakeFreqHz);
	}

	// for each ScreenShakeProducer component
	u16 p = 0;
	while (p < game.components.shakeProducers.length())
	{
		Game::Components::ScreenShakeProducerComponent& cmp =
			game.components.shakeProducers.item(p);
		
		ScreenShakeProducer& producer = cmp.data;

		// decrease the turbulence linearly by time, unless TTL is 0
		// (which means it stays constant forever, probably controlled externally)
		r32 turbulenceFalloff = (producer.totalTimeToLiveMS == 0) ? 0
			: ui.deltaMs / producer.totalTimeToLiveMS;

		producer.turbulence -= turbulenceFalloff;

		// remove expired components (consider adding an autoremove flag to control this)
		if (producer.turbulence <= 0.0f) {
			ComponentId thisCmpId = game.components.shakeProducers.getHandleForInnerIndex(p);
			game.components.shakeProducers.erase(thisCmpId);
			Entity& entity = *scene.entities[cmp.entityId];
			entity_removeComponent(entity.gameComponents, thisCmpId);
		}
		else {
			++p;
		}
	}
}


void game::ScreenShakeSystem::renderFrameTick(
	Game& game,
	Scene& scene,
	r32 interpolation,
	const i64 realTime,
	const i64 countsPassed)
{
	for (u16 n = 0;
		n < game.components.shakeNodes.length();
		++n)
	{
		ScreenShakeNode& shakeNode = game.components.shakeNodes.item(n).data;

		r32 turbulence = mix(shakeNode.prevTurbulence, shakeNode.nextTurbulence, interpolation);
		r32 turbSq = turbulence * turbulence;

		r32 noiseTime = mix(shakeNode.prevNoiseTime, shakeNode.nextNoiseTime, interpolation);
		r32 maxAngle  = mix(shakeNode.prevMaxAngle, shakeNode.nextMaxAngle, interpolation);
		
		r32 mult = maxAngle * DEG_TO_RADf * turbSq;

		r32 yawAngle   = mult * perlinNoise2(noiseTime, 0.0f);
		r32 pitchAngle = mult * perlinNoise2(noiseTime, 11.0f);
		r32 rollAngle  = mult * perlinNoise2(noiseTime, 23.0f);

		assert(shakeNode.cameraInstanceId != null_h32 && "a screen shake entity must contain a camera instance");
		assert(shakeNode.baseSceneNodeId != shakeNode.sceneNodeId
				&& "camera shake SceneNode should be a child of the movement SceneNode, not the same node");
		
		dvec3 angles{ (r64)pitchAngle, (r64)yawAngle, (r64)rollAngle };
		
		SceneNode& shakeSceneNode = scene.components.sceneNodes[shakeNode.sceneNodeId]->data;
		shakeSceneNode.rotationLocal = dquat_fromEulerAngles(angles);
		shakeSceneNode.orientationDirty = 1;
	}
}


void game::ScreenShakeSystem::init(
	Game& game)
{
	//playerfpsInputContextId = game.player.playerfpsInputContextId;

/*
	// get playerfps input mapping ids
	{
		auto ctx = engine.inputSystem->getInputContextHandle("playerfps");
		playerfpsInputContextId = ctx;

		lookYId = engine.inputSystem->getInputMappingHandle("Mouse Look Y", ctx);

		assert(forwardId != NullId_T && backId != NullId_T &&
			leftId != NullId_T && rightId != NullId_T &&
			sprintId != NullId_T && walkId != NullId_T &&
			crouchId != NullId_T &&
			lookXId != NullId_T && lookYId != NullId_T &&
			"playerfps input mappings changed");
	}

	// playerfps input handlers
	{
		using namespace input;

		// set the callback priority to the same as input context priority
		int priority = engine.inputSystem->getContext(playerfpsInputContextId).priority;

		engine.inputSystem->registerCallback(priority, [&engine, &game, this](FrameMappedInput& mi) {

			auto& scene = engine.sceneManager->getScene(game.sceneId);

			engine.inputSystem->handleInputState(forwardId, mi, [this](MappedState& ms, InputContext& c) {
				moveForward = glm::min(moveForward + 1, 1);
				return true;
			});
		});
	}
*/
}


ComponentId game::addScreenShakeNodeToCamera(
	Game& game,
	Scene& scene,
	ComponentId cameraInstanceId)
{
	Scene::Components::CameraInstanceComponent& cam =
		*scene.components.cameraInstances[cameraInstanceId];
	
	Entity& entity = *scene.entities[cam.entityId];
	CameraInstance& camInst = cam.data;

	// create a new scene node with a parent of the original camera node
	SceneNodeId shakeSceneNodeId = scene_addSceneNodeToEntity(
		scene,
		cam.entityId,
		dvec3{},
		dquat{},
		camInst.sceneNodeId);

	// add a shakenode component to control the new scene node
	Game::Components::ScreenShakeNodeComponent shakeNode{};
	shakeNode.entityId = cam.entityId;
	shakeNode.data.sceneNodeId = shakeSceneNodeId;
	shakeNode.data.cameraInstanceId = cameraInstanceId;
	shakeNode.data.baseSceneNodeId = camInst.sceneNodeId;
	ComponentId shakeNodeId = game.components.shakeNodes.insert(&shakeNode);
	
	entity_addComponent(entity.gameComponents, shakeNodeId);

	// put the camera onto the new shakable scene node, the camera's movement component remains
	// unchanged and still points to the original scene node
	camInst.sceneNodeId = shakeSceneNodeId;

	return shakeNodeId;
}