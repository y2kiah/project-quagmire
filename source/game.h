#ifndef _GAME_H
#define _GAME_H

#include "utility/common.h"
#include "utility/memory.h"
#include "utility/nstring.h"
#include "utility/dense_handle_map_16.h"
#include "utility/dense_handle_map_32.h"
#include "utility/dense_queue.h"
#include "utility/concurrent_queue.h"
#include "utility/logger.h"
#include "utility/fixed_timestep.h"
#include "input/game_input.h"
#include "math/qmath.h"
#include "scene/camera.h"

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

	FixedTimestep		simulationUpdate = {};

	//ThreadPoolPtr					threadPool		= nullptr;
	input::GameInput	gameInput;
	//resource::ResourceLoaderPtr		resourceLoader	= nullptr;
	//render::RenderSystemPtr			renderSystem	= nullptr;
	//render::ShaderManagerPtr		shaderManager	= nullptr;
	//render::ModelManagerPtr			modelManager	= nullptr;
	//scene::SceneManagerPtr			sceneManager	= nullptr;

	//Id_t							engineLuaState	= NullId_t;

	//#ifdef QUAGMIRE_DEVELOPMENT
	//tools::GriffinToolsManagerPtr	toolsManager	= nullptr;
	//Id_t							toolsLuaState	= NullId_t;
	//#endif

	// Game Systems

	// Id_t						sceneId = NullId_t;
	// game::PlayerControlSystem	player;
	// game::SkySystem				sky;
	// game::TerrainSystem			terrain;
	// game::DevCameraSystem		devCamera;
	// game::DevConsoleSystem		devConsole;
	// game::ScreenShakeSystem		screenShaker;

	//uint16_t					gameComponentStoreIds[MAX_GAME_COMPONENTS] = {};
};

#endif