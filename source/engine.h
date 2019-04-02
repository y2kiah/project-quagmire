
struct Engine {
	//ThreadPoolPtr					threadPool		= nullptr;
//	InputSystem*			inputSystem		= nullptr;
	//resource::ResourceLoaderPtr		resourceLoader	= nullptr;
	//render::RenderSystemPtr			renderSystem	= nullptr;
	//render::ShaderManagerPtr		shaderManager	= nullptr;
	//render::ModelManagerPtr			modelManager	= nullptr;
	//scene::SceneManagerPtr			sceneManager	= nullptr;

	//Id_T							engineLuaState	= NullId_T;

	//#ifdef GRIFFIN_TOOLS_BUILD
	//tools::GriffinToolsManagerPtr	toolsManager	= nullptr;
	//Id_T							toolsLuaState	= NullId_T;
	//#endif

	~Engine();
};

struct Game;

void engineUpdateFrameTick(Engine& engine, Game& game, UpdateInfo& ui);
void engineRenderFrameTick(Engine& engine, Game& game, float interpolation,
						   int64_t realTime, int64_t countsPassed);

Engine* make_engine(SDLApplication& app);