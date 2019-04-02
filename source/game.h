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
	// Id_T						sceneId = NullId_T;
	// game::PlayerControlSystem	player;
	// game::SkySystem				sky;
	// game::TerrainSystem			terrain;
	// game::DevCameraSystem		devCamera;
	// game::DevConsoleSystem		devConsole;
	// game::ScreenShakeSystem		screenShaker;

	//uint16_t					gameComponentStoreIds[MAX_GAME_COMPONENTS] = {};

	~Game();
};


void gameUpdateFrameTick(Game& game, Engine& engine, UpdateInfo& ui);

void gameRenderFrameTick(Game& gGame, Engine& engine, float interpolation,
						 int64_t realTime, int64_t countsPassed);

Game* make_game(Engine& engine, SDLApplication& app);
