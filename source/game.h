#ifndef _GAME_H
#define _GAME_H

#include "utility/common.h"
#include "utility/nstring.h"
#include "utility/handle_map.h"
#include "utility/dense_queue.h"
#include "utility/concurrent_queue.h"
#include "utility/logger.h"
#include "utility/fixed_timestep.h"

//#define MAX_GAME_COMPONENTS		32

//enum : uint16_t {
	//ScreenShakerComponentTypeId
//};

/**
* The Game structure contains all memory for the game state other than data stored in the
* component store. This is allocated on the heap as a single block and kept with a unique_ptr.
* Avoid allocating sub-objects on the heap whenever possible, try to keep everything in the
* game within this block.
*/
struct Game {
	// Core Systems

	FixedTimestep simulationUpdate = {};

	//ThreadPoolPtr					threadPool		= nullptr;
//	InputSystem*			inputSystem		= nullptr;
	//resource::ResourceLoaderPtr		resourceLoader	= nullptr;
	//render::RenderSystemPtr			renderSystem	= nullptr;
	//render::ShaderManagerPtr		shaderManager	= nullptr;
	//render::ModelManagerPtr			modelManager	= nullptr;
	//scene::SceneManagerPtr			sceneManager	= nullptr;

	//Id_T							engineLuaState	= NullId_T;

	//#ifdef QUAGMIRE_DEVELOPMENT
	//tools::GriffinToolsManagerPtr	toolsManager	= nullptr;
	//Id_T							toolsLuaState	= NullId_T;
	//#endif

	// Game Systems

	// Id_T						sceneId = NullId_T;
	// game::PlayerControlSystem	player;
	// game::SkySystem				sky;
	// game::TerrainSystem			terrain;
	// game::DevCameraSystem		devCamera;
	// game::DevConsoleSystem		devConsole;
	// game::ScreenShakeSystem		screenShaker;

	//uint16_t					gameComponentStoreIds[MAX_GAME_COMPONENTS] = {};
};

#endif