#include "build_config.h"
#include "capacity.h"
#include "main.h"
//#include <input/InputSystem.h>
//#include <render/Render.h>
//#include <resource/ResourceLoader.h>
//#include <utility/profile/Profile.h>
//#include <tests/Test.h>

// enable dedicated graphics for NVIDIA and AMD
extern "C" {
	_export unsigned long NvOptimusEnablement = 0x00000001;
	_export int AmdPowerXpressRequestHighPerformance = 1;
}

static SDLApplication app;
static GameContext gameContext{};
static MemoryArena platformMemory = makeMemoryArena();
static PlatformApi* _platformApi = nullptr;

#include "utility/platform_logger.cpp"
#include "platform/timer.cpp"
#include "platform/platform.cpp"
#include "input/platform_input.cpp"
#include "utility/memory_arena.cpp"
#include "utility/memory_heap.cpp"


bool initApplication()
{
	// initialize all subsystems except audio
	if (SDL_Init(SDL_INIT_EVERYTHING & ~SDL_INIT_AUDIO) < 0) {
		showErrorBox(SDL_GetError(), "Error");
		return false;
	}

	// enable logging
	// Logger takes over the priority filtering from SDL, so we just set the max level for SDL
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	app.systemInfo = platformGetSystemInfo();
	
	logger::messageQueue.init(
		sizeof(logger::LogMessage),
		LOGGER_CAPACITY,
		allocArrayOfType(platformMemory, logger::LogMessage, LOGGER_CAPACITY),
		0);
	//logger::setMode(logger::Mode_Immediate_Thread_Unsafe);
	logger::setAllPriorities(logger::Priority_Verbose);

	logger::info(logger::Category_System,
				"System Information\n"
				"  pageSize:              %u\n"
				"  allocationGranularity: %u\n"
				"  logicalProcessorCount: %u\n"
				"  systemRAM:             %u",
				app.systemInfo.pageSize,
				app.systemInfo.allocationGranularity,
				app.systemInfo.logicalProcessorCount,
				app.systemInfo.systemRAM);

	// turn off text input to start
	SDL_StopTextInput();

	return true;
}


bool initWindow(
	const char* appName)
{
	// get number of displays
	app.numDisplays = SDL_GetNumVideoDisplays();
	if (app.numDisplays <= 0) {
		logger::error(SDL_GetError());
		return false;
	}

	// get all display modes for each display
	for (int d = 0; d < app.numDisplays; ++d) {
		if (SDL_GetDisplayBounds(d, &(app.displayData[d].bounds)) != 0) {
			logger::error(SDL_GetError());
			return false;
		}

		if (SDL_GetDesktopDisplayMode(d, &app.displayData[d].displayMode) != 0) {
			logger::error(SDL_GetError());
			return false;
		}
	}
	
	// Request opengl 4.4 context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
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
		logger::error(SDL_GetError());
		return false;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(window);

	if (glContext == nullptr) {
		logger::error(SDL_GetError());
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


bool initOpenGL()
{
	// Initialize GLEW
	glewExperimental = true; // Needed in core profile
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		logger::critical(logger::Category_Error, "Failed to initialize GLEW\n");
		logger::critical(logger::Category_Error, (const char *)glewGetErrorString(err));
		return false;
	}
	glGetError(); // clear any error created by GLEW init

	logger::info(logger::Category_Video,
				"OpenGL Information\n"
				"  Vendor:       %s\n"
				"  Renderer:     %s\n"
				"  Version:      %s\n"
				"  Shd Lang Ver: %s",
				glGetString(GL_VENDOR),
				glGetString(GL_RENDERER),
				glGetString(GL_VERSION),
				glGetString(GL_SHADING_LANGUAGE_VERSION));

	//PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)SDL_GL_GetProcAddress("glGetStringi");

	// Use the post-3.0 method for querying extensions
	int numExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	logger::debug(logger::Category_Video, "OpenGL Extensions:\n");
	for (int i = 0; i < numExtensions; ++i) {
		const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
		logger::debug(logger::Category_Video, "%s ", ext);
	}

	// give the extensions string to the SOIL library
	//SOIL_set_gl_extensions_string(glExtensions.c_str());

	// use adaptive vertical refresh, if available
	if (SDL_GL_SetSwapInterval(-1) == -1) {
		// buffer swap syncronized with the monitor's vertical refresh
		if (SDL_GL_SetSwapInterval(1) == -1) {
			// immediate buffer swap
			SDL_GL_SetSwapInterval(0);
		}
	}

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


void quitApplication()
{
	input::deinitPlatformInput();

	if (app.windowData.glContext) {
		SDL_GL_DeleteContext(app.windowData.glContext);
	}
	if (app.windowData.window) {
		SDL_DestroyWindow(app.windowData.window);
	}

	SDL_Quit();
}


/**
 * Game Update-Render Thread, runs the main rendering frame loop and the inner
 * fixed-timestep game update loop
 */
int gameProcess(void* ctx)
{
	gameContext.done = false;
	Timer timer;
	u64 frame = 0;

	if (!gameContext.gameCode.onLoad(
			&gameContext.gameMemory,
			_platformApi,
			gameContext.app))
	{
		gameContext.done = true;
		return 1;
	}

	// gl context made current on the main loop thread
	SDL_GL_MakeCurrent(app.windowData.window, app.windowData.glContext);
	
	FixedTimestep gameCodeHotLoad;

	i64 realTime = timer.start();
	
	for (frame = 0; !gameContext.done; ++frame)
	{
		i64 countsPassed = timer.queryCountsPassed();
		realTime = timer.stopCounts;

		if (gameContext.gameCode.isValid) {
			gameContext.done =
				gameContext.gameCode.updateAndRender(
						&gameContext.gameMemory,
						_platformApi,
						&gameContext.input,
						gameContext.app,
						realTime,
						countsPassed,
						timer.countsPerMs,
						frame);
		}

		SDL_GL_SwapWindow(app.windowData.window);

		#if defined(QUAGMIRE_DEVELOPMENT) && QUAGMIRE_DEVELOPMENT != 0
		// check for new game code to load
		gameCodeHotLoad.tick(500.0f, realTime, countsPassed, timer.countsPerMs, frame, 1.0f,
			[](UpdateInfo& ui, void* ctx) {
				if (loadGameCode(gameContext.gameCode)) {
					gameContext.gameCode.onLoad(
							&gameContext.gameMemory,
							_platformApi,
							gameContext.app);
				}
			}, ctx);
		#endif

		yieldThread();
	}

	gameContext.gameCode.onExit(
			&gameContext.gameMemory,
			_platformApi,
			gameContext.app);

	// NOTE: be sure to wait on important futures here for processes that must finish before exit (e.g. save game)

	SDL_GL_MakeCurrent(nullptr, 0); // quitting, make no gl context current on the game thread
	glGetError(); // clear GL error from SDL call above

	return 0;
}


int main(int argc, char *argv[])
{
	initHighPerfTimer();
	
	PlatformApi platformApi = createPlatformApi();
	_platformApi = &platformApi;

	logger::_log = &logger::log;

	if (!initApplication() ||
		!initWindow(PROGRAM_NAME) ||
		!input::initPlatformInput() ||
		!initOpenGL())
	{
		quitApplication();
		return 1;
	}

	initGameContext();

	// run tests at startup
	TemporaryMemory tkn = beginTemporaryMemory(platformMemory);
	using Something = h64;
	Something s = { 1, 1, 1, 1 };
	DenseHandleMap32 testMap(sizeof(Something), 100, 1, allocArrayOfType(platformMemory, Something, 100));
	Something* items = (Something*)testMap.items;
	h64 h1 = testMap.insert();
	h64 h2 = testMap.insert(&s);
	Something* s3 = nullptr;
	h64 h3 = testMap.insert(&s, (void**)&s3);
	*s3 = { 3, 3, 3, 1 }; 
	testMap.erase(h3);

	auto component_insert = [](DenseHandleMap32& hm, Something&& val)->h64 {
		return hm.insert((void*)&val);
	};

	h64 h4 = component_insert(testMap, { 4, 4, 4, 1 });
	
	testMap.erase(h4);
	testMap.insert();
	component_insert(testMap, { 5, 5, 5, 1 });
	testMap.defragment([](void*a, void*b)->bool {
		return a && b && ((Something*)a)->index > ((Something*)b)->index;
	});

	testMap.clear();
	endTemporaryMemory(tkn);

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
			i64 eventTimestamp = timer_queryCounts();
			bool handled = input::handleMessage(event, eventTimestamp);
			
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
						//logger::verbose(Logger::Category_Input, "event type=%d\n", event.type);
					}
				}
			}
		}
		
		// run tasks with Thread_OS_Input thread affinity
		//engine.threadPool->executeFixedThreadTasks(ThreadAffinity::Thread_OS_Input);
		
		// run thread_pool deferred task check (when_any, when_all)
		//   engine.taskPool.checkDeferredTasks();

		//logger::verbose("Input frame\n");

		// flush the logger queue, writing out all of the messages
		logger::flush();

		//yieldThread();
		platformSleep(1);
	}

	int gameResult = 0;
	SDL_WaitThread(gameThread, &gameResult); // wait for the game thread to join

	// gl context made current on the OS/Input thread for destruction
	SDL_GL_MakeCurrent(app.windowData.window, app.windowData.glContext);

	//assert(gamePtr.use_count() == 1 && "game has reference count remaining");
	//assert(enginePtr.use_count() == 1 && "engine has reference count remaining");
	//gamePtr.reset();
	//enginePtr.reset(); // must delete the engine on the GL thread

	deinitGameContext();
	quitApplication();

	return 0;
}
