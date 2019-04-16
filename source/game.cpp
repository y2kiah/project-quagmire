
#include "build_config.h"
#include "capacity.h"
#include <cstdlib>
#include <cmath>
#include <cstdarg>
#include <atomic>
#include <SDL_render.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include "game.h"
#include "platform/platform.h"
#include "input/platform_input.h"

#include "utility/logger.cpp"

struct SimulationUpdateContext {
	Game&					game;
	input::PlatformInput&	input;
	GameMemory*				gameMemory;
};

logging::Logger logger;

/**
 * Runs the simulation logic at a fixed frame rate. Keep a "previous" and "next" value for
 * any state that needs to be interpolated smoothly in the renderFrameTick loop. The sceneNode
 * position and orientation are interpolated automatically, but other values like color that
 * need smooth interpolation for rendering should be handled manually.
 */
void gameUpdateFrameTick(UpdateInfo& ui, void* _ctx)
{
	SimulationUpdateContext& simContext = *(SimulationUpdateContext*)_ctx;

	logger.verbose("Update virtualTime=%lu: gameTime=%ld: deltaCounts=%ld: countsPerMs=%ld\n",
				   ui.virtualTime, ui.gameTime, ui.deltaCounts, ui.countsPerMs);

	//engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_Update);

	// if all systems operate on 1(+) frame-old-data, can all systems be run in parallel?
	// should this list become a task flow graph?

	// TEMP instead of popping
	simContext.input.eventsQueue.clear();
	simContext.input.motionEventsQueue.clear();
	//gameMemory.
//	inputSystem->updateFrameTick(ui);

	//	ResourceLoader
	//	AISystem
	//	PhysicsSystem
	//	CollisionSystem
	//	ResourcePredictionSystem
	//	etc.

//	game.player.updateFrameTick(game, engine, ui);

//	game.devCamera.updateFrameTick(game, engine, ui);

//	game.terrain.updateFrameTick(game, engine, ui);

	// TODO: consider running this less frequently, and spread the load with other systems that
	//	don't run every frame by offsetting the frame that it runs on
//	game.sky.updateFrameTick(game, engine, ui);

//	game.screenShaker.updateFrameTick(game, engine, ui);
	
//	engine.sceneManager->updateActiveScenes();
}


/**
 * Runs at the "full" variable frame rate of the render loop, often bound to vsync at 60hz. For
 * smooth animation, state must be kept from the two most recent update ticks, and interpolated
 * in this loop for final rendering.
 */
void gameRenderFrameTick(GameMemory* gameMemory, float interpolation,
						 int64_t realTime, int64_t countsPassed)
{
	logger.verbose("Render realTime=%lu: interpolation=%0.3f\n", realTime, interpolation);

//	engine.resourceLoader->executeCallbacks();

//	game.screenShaker.renderFrameTick(game, engine, interpolation, realTime, countsPassed);

//	game.devConsole.renderFrameTick(game, engine, interpolation, realTime, countsPassed);

//	engine.sceneManager->renderActiveScenes(interpolation, engine);

//	engine.renderSystem->renderFrame(interpolation, engine);
}


/**
 * Create and init the systems of the griffin engine, and do dependency injection
 */
void makeCoreSystems(GameMemory* gameMemory)
{
	Game& game = *(Game*)gameMemory->gameState;
	
	/**
	 * Create thread pool, one worker thread per logical core
	 */
//	{
//		engine.threadPool = make_shared<thread_pool>(app.getSystemInfo().cpuCount);
//		task_base::s_threadPool = engine.threadPool;
//	}

//	auto scriptPtr  = make_shared<script::ScriptManager>();
//	auto inputPtr   = make_shared<input::InputSystem>();
//	auto loaderPtr  = make_shared<resource::ResourceLoader>();

	/**
	 * Build the Lua scripting system
	 */
//	{
//		using namespace script;

		// init.lua configures the startup settings
//		engine.engineLuaState = scriptPtr->createState("scripts/initState.lua"); // throws on error

		// add system API functions to Lua


//		engine.scriptManager = scriptPtr;
//	}

	/**
	 * Build the tools manager, QUAGMIRE_DEVELOPMENT only
	 */
	#ifdef QUAGMIRE_DEVELOPMENT
	{
//		using namespace tools;

//		engine.toolsLuaState = scriptPtr->createState("scripts/initState.lua");

//		auto toolsPtr = make_shared<GriffinToolsManager>(engine.toolsLuaState);

		// executes tools build scripts, and starts http server on a new thread
//		toolsPtr->init(scriptPtr);

//		engine.toolsManager = toolsPtr;
	}
	#endif

	/**
	 * Build the input system
	 */
	{
//		using namespace input;

		// inject dependencies into the InputSystem
//		inputPtr->app = &app;

//		inputPtr->initialize();

		// inject dependencies to the InputSystem C API
//		setInputSystemPtr(inputPtr);

		// InputSystem.lua contains initInputSystem function
//		scriptPtr->doFile(engine.engineLuaState, "scripts/InputSystem.lua"); // throws on error

		// invoke Lua function to init InputSystem
//		scriptPtr->callLuaGlobalFunction(engine.engineLuaState, "initInputSystem");

		// move input system into application
//		engine.inputSystem = inputPtr;
	}

	/**
	 * Build the resource system
	 */
	{
//		using namespace resource;

		// Build resource caches
		//   Permanent Cache
//		auto permanentCachePtr = make_shared<ResourceCache>(Cache_Permanent, RESERVE_RESOURCECACHE_PERMANENT, 0 /* Infinite */);
//		loaderPtr->registerCache(permanentCachePtr, (CacheType)permanentCachePtr->getItemTypeId());

		//   Materials Cache
//		auto materialsCachePtr = make_shared<ResourceCache>(Cache_Materials, RESERVE_RESOURCECACHE_MATERIALS, 256 * 1024 * 1024 /* 256 MB */);
//		loaderPtr->registerCache(materialsCachePtr, (CacheType)materialsCachePtr->getItemTypeId());
		
		//   Models Cache
//		auto modelsCachePtr = make_shared<ResourceCache>(Cache_Models, RESERVE_RESOURCECACHE_MODELS, 256 * 1024 * 1024 /* 256 MB */);
//		loaderPtr->registerCache(modelsCachePtr, (CacheType)modelsCachePtr->getItemTypeId());

		//   Scripts Cache
//		auto scriptsCachePtr = make_shared<ResourceCache>(Cache_Scripts, RESERVE_RESOURCECACHE_SCRIPTS, 16 * 1024 * 1024 /* 16 MB */);
//		loaderPtr->registerCache(scriptsCachePtr, (CacheType)scriptsCachePtr->getItemTypeId());

		// Build resource sources
//		auto fileSystemSourcePtr = IResourceSourcePtr((IResourceSource*)(new FileSystemSource()));
//		loaderPtr->registerSource(fileSystemSourcePtr);

		// inject loader dependencies into other system
//		render::setResourceLoaderPtr(loaderPtr);
//		scene::setResourceLoaderPtr(loaderPtr);
		#ifdef QUAGMIRE_DEVELOPMENT
//		tools::setResourceLoaderPtr(loaderPtr);
		#endif

		// add Lua APIs


//		engine.resourceLoader = loaderPtr;
	}

	/**
	 * Build the render system
	 */
	{
//		using namespace render;

//		auto renderSystemPtr = make_shared<RenderSystem>();
//		auto shaderManagerPtr = make_shared<ShaderManager_GL>();
//		auto modelManagerPtr = make_shared<ModelManager_GL>();

//		engine.renderSystem = renderSystemPtr;
//		engine.shaderManager = shaderManagerPtr;
//		engine.modelManager = modelManagerPtr;

//		renderSystemPtr->init(app.getPrimaryWindow().width, app.getPrimaryWindow().height, engine);
//		renderSystemPtr->loadGlobalFonts();
//		shaderManagerPtr->loadUbershaderCode("shaders/ads.glsl");
	}

	/**
	 * Build the scene manager
	 */
	{
//		using namespace scene;

//		auto scenePtr = make_shared<SceneManager>();

		// inject dependencies
//		setRenderSystemPtr(engine.renderSystem);
//		setSceneManagerPtr(scenePtr); // to the Scene C API

		// Scene.lua contains scene functions
//		scriptPtr->doFile(engine.engineLuaState, "scripts/Scene.lua"); // throws on error

//		engine.sceneManager = scenePtr;
	}
}


/**
 * Create and init the initial game state and game systems and do dependency injection
 */
void makeGame(GameMemory* gameMemory)
{
	Game& game = *(Game*)gameMemory->gameState;

	//logging::setMode(logging::Mode_Immediate_Thread_Unsafe);
	logger.setAllPriorities(logging::Priority_Info);
	
//	GamePtr gamePtr = std::make_shared<Game>();
//	Game& game = *gamePtr;

	// InputSystem.lua contains initInputSystem function
//	engine.scriptManager->doFile(engine.engineLuaState, "scripts/game/initGame.lua"); // throws on error

	// invoke Lua function to init the game
//	engine.scriptManager->callLuaGlobalFunction(engine.engineLuaState, "initGame");

	makeCoreSystems(gameMemory);

	// set up game scene
	{
//		using namespace griffin::scene;

//		game.sceneId = engine.sceneManager->createScene("Game World", true);
//		auto& scene = engine.sceneManager->getScene(game.sceneId);
		
		// set up terrain system
//		game.terrain.init(game, engine, app);
//		render::g_pGame = gamePtr.get(); // TEMP

		// set up sky system
//		game.sky.init(game, engine, app);
		// TODO: scene should have a setSkybox convenience function or something like that
		//scene.setSkybox(game.sky.skyBoxCubeMap);
		// temp, will be part of scene as above
//		engine.renderSystem->setSkyboxTexture(game.sky.skyBoxCubeMap);

		// set up input control systems
//		game.player.init(game, engine, app);
//		game.devCamera.init(game, engine, app);

		// set up development tools
//		game.devConsole.init(game, engine, app);
		
		// set up positionalEffect systems
//		game.screenShaker.init(game, engine, app);

		// ...
	}

	// startup active input contexts
//	engine.inputSystem->setContextActive(game.player.playerfpsInputContextId);

//	return game;
	gameMemory->initialized = true;
}


/**
 * Releases all systems on the OpenGL thread
 */
void destroyGame(GameMemory* gameMemory)
{
//	terrain.deinit();

	// Destroy the scene manager
//	scene::setRenderSystemPtr(render::RenderSystemPtr());
//	scene::setSceneManagerPtr(scene::SceneManagerPtr());
//	sceneManager.reset();

	// Destroy the render system
//	renderSystem.reset();

	// Destroy the resource system
//	render::setResourceLoaderPtr(ResourceLoaderPtr());
//	scene::setResourceLoaderPtr(ResourceLoaderPtr());
	#ifdef QUAGMIRE_DEVELOPMENT
//	tools::setResourceLoaderPtr(ResourceLoaderPtr());
	#endif
//	resourceLoader.reset();

	// Destroy the input system
//	setInputSystemPtr(input::InputSystemPtr());
//	inputSystem.reset();

	// Destroy the tools system
	#ifdef QUAGMIRE_DEVELOPMENT
//	toolsManager.reset();
	#endif

	// Destroy the scripting system
//	scriptManager.reset();

	// Destroy the thread pool
//	threadPool.reset();
//	task_base::s_threadPool.reset();
}


extern "C" {
_export
void
gameUpdateAndRender(
	GameMemory* gameMemory,
	input::PlatformInput* input,
	i64 realTime,
	i64 countsPassed,
	i64 countsPerMs,
	u64 frame)
{
	if (!gameMemory->initialized) {
		makeGame(gameMemory);
	}

	Game& game = *(Game*)gameMemory->gameState;
	SimulationUpdateContext ctx = { game, *input, gameMemory };
	
	float interpolation = game.simulationUpdate.tick(
			1000.0f / 30.0f,	// deltaMS, run at 30fps
			realTime,
			countsPassed,
			countsPerMs,
			frame,
			1.0f,				// game speed, 1.0=normal
			gameUpdateFrameTick,
			&ctx);

	//SDL_Delay(1000);

	//engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_OpenGL_Render);

	gameRenderFrameTick(gameMemory, interpolation, realTime, countsPassed);

	logging::flush();
}
}
