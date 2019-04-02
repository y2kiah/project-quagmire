#include "build_config.h"
#include "main.h"
//#include <application/Engine.h>
//#include <game/Game.h>
//#include <input/InputSystem.h>
//#include <render/Render.h>
//#include <resource/ResourceLoader.h>
//#include <utility/profile/Profile.h>
//#include <utility/debug.h>
//#include <utility/Logger.h>
//#include <tests/Test.h>
#include "platform/platform.cpp"
#include "platform/win32_timer.cpp"
#include "engine.cpp"
#include "game.cpp"


struct GameContext {
	SDLApplication* app = nullptr;
	std::atomic<bool> done = false;
	//*game
	//*engine
};

/**
* Game Update-Render Thread, runs the main rendering frame loop and the inner
* fixed-timestep game update loop
*/
int gameProcess(void* ctx)
{
	GameContext& gameContext = *(GameContext*)ctx;
	SDLApplication& app = *(gameContext.app);
	Timer timer;
	uint64_t frame = 0;

	// gl context made current on the main loop thread
	SDL_GL_MakeCurrent(app.windowData.window, app.windowData.glContext);
	
	int64_t realTime = timer.start();

	FixedTimestep update(1000.0f / 30.0f, timer.countsPerMs);
	
	for (frame = 0; !gameContext.done; ++frame)
	{
		int64_t countsPassed = timer.queryCountsPassed();
		realTime = timer.stopCounts;

		float interpolation = update.tick(realTime, countsPassed, frame, 1.0f,
			[](UpdateInfo& ui) {
				/*logger.verbose("Update virtualTime=%lu: gameTime=%ld: deltaCounts=%ld: countsPerMs=%ld\n",
						virtualTime, gameTime, deltaCounts, Timer::timerFreq() / 1000);*/

				//engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_Update);

				//engineUpdateFrameTick(engine, game, ui);
			});

		//SDL_Delay(1000);
		/*logger.verbose("Render realTime=%lu: interpolation=%0.3f: threadIdHash=%lu\n",
				realTime, interpolation, std::this_thread::get_id().hash());*/

		//engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_OpenGL_Render);

//		engineRenderFrameTick(engine, game, interpolation, realTime, countsPassed);

		SDL_GL_SwapWindow(app.windowData.window);

		yieldThread();
		
		//logger.error(Logger::Category_Error, "%s", e.what());
	}

	// NOTE: be sure to wait on important futures here for processes that must finish before exit (e.g. save game)

	SDL_GL_MakeCurrent(nullptr, 0); // quitting, make no gl context current on the game thread
	glGetError(); // clear GL error from SDL call above

	return 0;
}


void initApplication(SDLApplication& app)
{
	app.systemInfo.cpuCount = SDL_GetCPUCount();
	app.systemInfo.systemRAM = SDL_GetSystemRAM();
}


bool initWindow(
	SDLApplication& app,
	const char* appName)
{
	// initialize all subsystems except audio
	if (SDL_Init(SDL_INIT_EVERYTHING & ~SDL_INIT_AUDIO) < 0) {
		showErrorBox(SDL_GetError(), "Error");
		return false;
	}

	// enable logging
	// Logger takes over the priority filtering from SDL, so we just set the max level for SDL
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	// turn off text input to start
	SDL_StopTextInput();

	// get number of displays
	app.numDisplays = SDL_GetNumVideoDisplays();
	if (app.numDisplays <= 0) {
		//log (SDL_GetError());
		return false;
	}

	// get all display modes for each display
	for (int d = 0; d < app.numDisplays; ++d) {
		if (SDL_GetDisplayBounds(d, &(app.displayData[d].bounds)) != 0) {
			//log (SDL_GetError());
			return false;
		}

		if (SDL_GetDesktopDisplayMode(d, &app.displayData[d].displayMode) != 0) {
			//log (SDL_GetError());
			return false;
		}
	}
	
	// Request opengl 4.4 context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//WGL_NV_gpu_affinity
	//AMD_gpu_association

	// Turn on double buffering, Z buffer, and stencil buffer
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// Turn on antialiasing
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

	// Request sRGB capable framebuffer
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1); // or, opt for gamma control in fragment shader?

	int width = 1600;
	int height = 900;
	//int width = 1920;
	//int height = 1080;

	SDL_Window* window = SDL_CreateWindow(
		appName,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height,
		SDL_WINDOW_OPENGL);
		//0, 0,
		//SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);

	if (window == nullptr) {
		//log (SDL_GetError());
		return false;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(window);

	if (glContext == nullptr) {
		//log (SDL_GetError());
		return false;
	}

	app.windowData.window = window;
	app.windowData.glContext = glContext;
	app.windowData.width = width;
	app.windowData.height = height;
	getWindowInfo(window, &app.windowData.wmInfo);
	getEnvironmentInfo(&app.environment);

	setWindowIcon(&app.windowData);

	SDL_DisableScreenSaver();

	return true;
}


bool initOpenGL(SDLApplication& app)
{
	// Initialize GLEW
	glewExperimental = true; // Needed in core profile
	GLenum err = glewInit();
	if (err != GLEW_OK) {
//		logger.critical(Logger::Category_Error, "Failed to initialize GLEW\n");
//		logger.critical(Logger::Category_Error, (const char *)glewGetErrorString(err));
		return false;
	}
	glGetError(); // clear any error created by GLEW init

//	logger.debug(Logger::Category_Video,
//				 "OpenGL Information:\n  Vendor: %s\n  Renderer: %s\n  Version: %s\n  Shading Language Version: %s\n",
//				 glGetString(GL_VENDOR),
//				 glGetString(GL_RENDERER),
//				 glGetString(GL_VERSION),
//				 glGetString(GL_SHADING_LANGUAGE_VERSION));

	//PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)SDL_GL_GetProcAddress("glGetStringi");

	// Use the post-3.0 method for querying extensions
	int numExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	// TODO: rewrite this when we have a string library
	size_t totalLen = 0;
	for (int i = 0; i < numExtensions; ++i) {
		totalLen += strlen((const char*)glGetStringi(GL_EXTENSIONS, i)) + 1;
	}
	char* glExtensions = (char*)malloc(totalLen);
	char* dStr = glExtensions;
	for (int i = 0; i < numExtensions; ++i) {
		const char* extStr = (const char*)glGetStringi(GL_EXTENSIONS, i);
		size_t extStrLen = strlen(extStr);
		strncpy_s(dStr, totalLen, extStr, extStrLen-1);
		dStr += extStrLen;
		*dStr = ' ';
		++dStr;
	}
	glExtensions[totalLen-1] = '\0';
	//logger.debug(Logger::Category_Video, "OpenGL Extensions: %s\n", glExtensions.c_str());

	// give the extensions string to the SOIL library
	//SOIL_set_gl_extensions_string(glExtensions.c_str());
	free(glExtensions);


	// This makes our buffer swap syncronized with the monitor's vertical refresh
	SDL_GL_SetSwapInterval(1);

	// Enable multisampling
	//glEnable(GL_MULTISAMPLE);

	// Enable sRGB frame buffer
	//glEnable(GL_FRAMEBUFFER_SRGB); // or, opt for gamma control in fragment shader?

	// Enable back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Enable seamless filtering across cubemap faces to avoid edge seams
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// Set the clear color to black and clear the screen
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	SDL_GL_SwapWindow(app.windowData.window);

	ASSERT_GL_ERROR;

	return true;
}


void quitApplication(SDLApplication& app)
{
	SDL_GL_DeleteContext(app.windowData.glContext);
	SDL_DestroyWindow(app.windowData.window);

	SDL_Quit();
}


int main(int argc, char *argv[])
{
	initHighPerfTimer();
//	logger.setAllPriority(Logger::Priority_Verbose);
	
	SDLApplication app;
	initApplication(app);
//	EnginePtr enginePtr;
//	GamePtr gamePtr;
	
	GameContext gameContext;
	gameContext.app = &app;

	initWindow(app, PROGRAM_NAME);
	initOpenGL(app);

	// enginePtr = make_engine(app);
	// Engine& engine = *enginePtr;

	// gamePtr = make_game(engine, app);
	// Game& game = *gamePtr;

	// run tests at startup
	//struct Something {
	//	u32 a, b, c, d;
	//};
	typedef id_t Something;
	Something s = { 1, 1, 1, 1 };
	handle_map testMap(sizeof(Something), 100, 1);
	Something* items = (Something*)testMap.items;
	id_t h1 = testMap.insert();
	id_t h2 = testMap.insert(&s);
	Something* s3 = nullptr;
	id_t h3 = testMap.insert(&s, (void**)&s3);
	*s3 = { 3, 3, 3, 3 }; 
	testMap.erase(h3);

	auto component_insert = [](handle_map& hm, Something&& val)->id_t {
		return hm.insert((void*)&val);
	};

	id_t h4 = component_insert(testMap, { 4, 4, 4, 4 });
	
	testMap.erase(h4);
	testMap.insert();
	component_insert(testMap, { 5, 5, 5, 5 });
	testMap.defragment([](void*a, void*b)->bool {
		return a && b && ((Something*)a)->index > ((Something*)b)->index;
	});

	testMap.clear();

	// test::TestRunner tests;
	// tests.registerAllTests();
	// tests.runAllTests();

	SDL_GL_MakeCurrent(nullptr, 0); // make no gl context current on the input thread
	glGetError(); // clear GL error from SDL call above

	// This starts the game thread. OpenGL context is transferred to this thread
	// after OpenGL is initialized on the OS thread. This thread joins after SDL_QUIT.
	SDL_Thread *gameThread = SDL_CreateThread(gameProcess, "GameThread", (void *)&gameContext);

	/**
	* OS-Input Thread, runs the platform message loop
	*/
	while (!gameContext.done) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			// send to the input system to handle the event
			bool handled = false;//engine.inputSystem->handleMessage(event);
			
			if (!handled) {
				switch (event.type) {
					case SDL_QUIT: {
						gameContext.done = true; // OS and game threads read this to exit
						break;
					}

					case SDL_WINDOWEVENT:
					case SDL_SYSWMEVENT:
						break;

					default: {
						//logger.verbose(Logger::Category_Input, "event type=%d\n", event.type);
					}
				}
			}
		}
		
		// run tasks with Thread_OS_Input thread affinity
		//engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_OS_Input);
		
		// run thread_pool deferred task check (when_any, when_all)
		//   engine.taskPool.checkDeferredTasks();

		// flush the logger queue, writing out all of the messages
		//logger.flush();

		yieldThread();
	}

	int gameResult = 0;
	SDL_WaitThread(gameThread, &gameResult); // wait for the game thread to join

	//showErrorBox(e.what(), "Error");

	// gl context made current on the OS/Input thread for destruction
	SDL_GL_MakeCurrent(app.windowData.window, app.windowData.glContext);

	//assert(gamePtr.use_count() == 1 && "game has reference count remaining");
	//assert(enginePtr.use_count() == 1 && "engine has reference count remaining");
	//gamePtr.reset();
	//enginePtr.reset(); // must delete the engine on the GL thread

	quitApplication(app);
	
	//logger.deinit();

	return 0;
}
