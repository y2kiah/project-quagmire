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
#include "math/noise.h"

#include "render/texture_gl.h" // TODO: replace several of these with renderer_gl
//#include "render/mesh_gl.h"
//#include "render/shader_gl.h"

#include "asset/asset.h"
#include "scene/camera.h"
#include "game/screen_shake/screen_shake_system.h"

//#define MAX_GAME_COMPONENTS		32

//enum : uint16_t {
	//ScreenShakerComponentTypeId
//};

// TODO: move these render assets to the renderer_gl.h file
namespace render {
	struct RenderAssets {
		Texture2D_HandleMap			textures2D;
		TextureCubeMap_HandleMap	texturesCubeMap;
	};
}

/**
* The Game structure contains all memory for the game state other than data stored in the
* component store. This is allocated on the heap as a single block and kept with a unique_ptr.
* Avoid allocating sub-objects on the heap whenever possible, try to keep everything in the
* game within this block.
*/
struct Game {
	// Core Systems

	FixedTimestep				simulationUpdate = {};

	//ThreadPoolPtr					threadPool		= nullptr;
	input::GameInput			gameInput;
	AssetStore					assetStore;
	render::RenderAssets		renderAssets;
	//render::ShaderManagerPtr		shaderManager	= nullptr;
	//render::ModelManagerPtr			modelManager	= nullptr;
	//scene::SceneManagerPtr			sceneManager	= nullptr;
	// TODO: SparseHandleMap16WithBuffer for scenes
	
	Scene						gameScene;


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
	game::ScreenShakeSystem		screenShaker;

	//uint16_t					gameComponentStoreIds[MAX_GAME_COMPONENTS] = {};

	struct Components
	{
		ComponentStore(ScreenShakeNode,     ScreenShakeNodeMap,     ComponentId, 0, shakeNodes,     SCENE_MAX_CAMERAS)
		ComponentStore(ScreenShakeProducer, ScreenShakeProducerMap, ComponentId, 1, shakeProducers, SCENE_MAX_SCREEN_SHAKE_PRODUCERS)
	}
	components;
};

#endif