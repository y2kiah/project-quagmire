
#include <atomic>
#include "../capacity.h"
#include "platform.h"
#include "input/platform_input.h"
#include "utility/logger.h"
#include <SDL_filesystem.h>
#include <SDL_cpuinfo.h>

#ifdef _WIN32

#include "Windows.h"

typedef u8 GameUpdateAndRenderFunc(
		GameMemory* gameMemory,
		PlatformApi* platform,
		input::PlatformInput* input,
		SDLApplication* app,
		i64 realTime,
		i64 countsPassed,
		i64 countsPerMs,
		u64 frame);

typedef u8 GameOnLoadFunc(
		GameMemory* gameMemory,
		PlatformApi* platformApi,
		SDLApplication* app);

typedef void GameOnExitFunc(
		GameMemory* gameMemory,
		PlatformApi* platformApi,
		SDLApplication* app);

struct GameCode
{
	HMODULE		gameDLL;
	FILETIME	dllLastWriteTime;

	GameUpdateAndRenderFunc*	updateAndRender = nullptr;
	GameOnLoadFunc*				onLoad = nullptr;
	GameOnExitFunc*				onExit = nullptr;
	//game_get_sound_samples *getSoundSamples;

	bool isValid = false;
};


void yieldThread()
{
	SwitchToThread();
}

void platformSleep(
	unsigned long ms)
{
	Sleep(ms);
}

void showErrorBox(
	const char *text,
	const char *caption)
{
	// TODO: need to convert from UTF-8 text to wchar_t here
	MessageBoxA(NULL, text, caption, MB_OK | MB_ICONERROR | MB_TOPMOST);
}

void showLastErrorAndQuit()
{
	LPWSTR pBuffer = nullptr;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		0,
		GetLastError(),
		0,
		(LPWSTR)&pBuffer,
		0,
		0);
	
	MessageBoxW(NULL, pBuffer, L"Unrecoverable Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
	exit(1);
}

void setWindowIcon(
	const WindowData* windowData)
{
	HWND hWnd = windowData->wmInfo.info.win.window;
	HINSTANCE handle = GetModuleHandle(nullptr);

// TODO: find a way to do this without a .rc file or just use SDL same as on linux
//	HICON icon = LoadIcon(handle, MAKEINTRESOURCE(IDI_ICON1));
//	if (icon != nullptr) {
//		SetClassLongPtr(hWnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
//		SetClassLongPtr(hWnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(icon));
//	}
}

FILETIME fileGetLastWriteTime(
	const char *filename)
{
	FILETIME lastWriteTime = {};
	
	WIN32_FILE_ATTRIBUTE_DATA status;
	if (GetFileAttributesExA(filename, GetFileExInfoStandard, &status)) {
		lastWriteTime = status.ftLastWriteTime;
	}

	return lastWriteTime;
}

void unloadGameCode(
	GameCode& gameCode)
{
	if (gameCode.gameDLL) {
		FreeLibrary(gameCode.gameDLL);
		gameCode.gameDLL = 0;
	}
	
	gameCode.isValid = false;
	gameCode.updateAndRender = nullptr;
}

bool loadGameDLL(
	GameCode& gameCode,
	const char* gameDLLPath)
{
	gameCode.gameDLL = LoadLibraryA(gameDLLPath);
	if (gameCode.gameDLL) {
		gameCode.updateAndRender = (GameUpdateAndRenderFunc*)GetProcAddress(gameCode.gameDLL, "gameUpdateAndRender");
		gameCode.onLoad = (GameOnLoadFunc*)GetProcAddress(gameCode.gameDLL, "onLoad");
		gameCode.onExit = (GameOnExitFunc*)GetProcAddress(gameCode.gameDLL, "onExit");

		gameCode.isValid = (gameCode.updateAndRender);
	}
	return gameCode.isValid;
}

/**
 * @returns true if new code was loaded
 */
bool loadGameCode(
	GameCode& gameCode)
{
	bool loaded = false;
	const char *gameDLLPath    = "game.dll";
	const char *newGameDLLPath = "./build/game.dll";
	const char *lockFilePath   = "./build/lock.tmp";
	
	// load on startup
	if (!gameCode.isValid) {
		if (loadGameDLL(gameCode, gameDLLPath)) {
			gameCode.dllLastWriteTime = fileGetLastWriteTime(gameDLLPath);
			loaded = true;
		}
	}
	// try reload from new dll in build directory
	else {
		FILETIME lockFileTime = fileGetLastWriteTime(lockFilePath);
		if (!lockFileTime.dwLowDateTime && !lockFileTime.dwHighDateTime)
		{
			FILETIME newDLLFileTime = fileGetLastWriteTime(newGameDLLPath);

			if ((newDLLFileTime.dwLowDateTime || newDLLFileTime.dwHighDateTime)
				&& CompareFileTime(&newDLLFileTime, &gameCode.dllLastWriteTime) != 0)
			{
				logger::info("new game.dll detected...");
				unloadGameCode(gameCode);
				
				CopyFileA(newGameDLLPath, gameDLLPath, FALSE);

				if (loadGameDLL(gameCode, gameDLLPath)) {
					gameCode.dllLastWriteTime = newDLLFileTime;
					loaded = true;
				}

				logger::info(gameCode.isValid ? "loaded!" : "ERROR LOADING");
			}
		}
	}

	if (!gameCode.isValid) {
		gameCode.updateAndRender = nullptr;
		gameCode.onLoad = nullptr;
		gameCode.onExit = nullptr;
	}

	return loaded;
}

SystemInfo platformGetSystemInfo()
{
	SystemInfo info{};
	SYSTEM_INFO si{};
	GetSystemInfo(&si);

	info.pageSize = si.dwPageSize;
	info.allocationGranularity = si.dwAllocationGranularity;
	
	info.minimumApplicationAddress = si.lpMinimumApplicationAddress;
	info.maximumApplicationAddress = si.lpMaximumApplicationAddress;
	
	info.activeProcessorMask = si.dwActiveProcessorMask;
	info.logicalProcessorCount = si.dwNumberOfProcessors;
	info.processorArchitecture = si.wProcessorArchitecture;
	info.processorLevel = si.wProcessorLevel;
	info.processorRevision = si.wProcessorRevision;

	info.cpuCount = SDL_GetCPUCount(); // TODO: is this needed?
	info.systemRAM = SDL_GetSystemRAM();

	// TODO: call IsProcessorFeaturePresent on features we need to check for

	return info;
}

MemoryBlock* platformAllocate(
	size_t minimumSize)
{
	SIZE_T size = ((minimumSize / app.systemInfo.allocationGranularity) + 1) * app.systemInfo.allocationGranularity;
	void* memory = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	
	// if we can't allocate memory, time to panic (for now)
	if (!memory) {
		// TODO: before exiting, we could try to detect and free up memory and retry the allocation
		showLastErrorAndQuit();
	}

	MemoryBlock* block = (MemoryBlock*)memory;
	block->base = (void*)((uintptr_t)memory + sizeof(MemoryBlock));
	block->size = size - sizeof(MemoryBlock);

	return block;
}


#else

#include <unistd.h>
#ifdef _SC_PRIORITY_SCHEDULING
#include <sched.h>

void yieldThread()
{
	sched_yield();
}

#else

#include <thread>
void yieldThread()
{
	std::this_thread::yield();
}

#endif

void showErrorBox(const char* text, const char* caption)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, nullptr);
}

void setWindowIcon(WindowData *windowData)
{
	assert("use SDL here");
	//	SDL_Surface* icon = IMG_Load(const char *file);
	//	SDL_SetWindowIcon(window, icon);
	//	SDL_FreeSurface(icon);
}

struct GameCode
{
	void*		gameDLL;
	time_t		dllLastWriteTime;

	GameUpdateAndRender *updateAndRender = nullptr;
	//game_get_sound_samples *getSoundSamples;

	bool isValid = false;
};

GameCode
loadGameCode(
	//win32_state *State,
	char *gameDLLName)
{
	GameCode gameCode = {};
	
	struct stat fileStatus;
	if (stat(gameDLLName, &fileStatus) == 0) {
		gameCode.dllLastWriteTime = fileStatus.st_mtime;
	}
		
	// 	while (State->TempDLLNumber < 1024)
	// 	{
	// 		Win32BuildEXEPathFileName(State, "game.dll", State->TempDLLNumber,
	// 								  sizeof(tempDLLName), tempDLLName);
			
	// 		if (CopyFileA(gameDLLName, tempDLLName, FALSE)) {
	// 			break;
	// 		}
	// 		else {
	// 			++State->TempDLLNumber;
	// 		}
	// 	}

	if (gameCode.dllLastWriteTime) {
	//	gameCode.gameDLL = dlopen(gameDLLName, RTLD_LAZY);
		gameCode.gameDLL = dlopen("game.so", RTLD_LAZY);
		if (gameCode.gameDLL) {
			gameCode.updateAndRender = (GameUpdateAndRender*)dlsym(gameCode.gameDLL, "gameUpdateAndRender");
			
			gameCode.isValid = (gameCode.updateAndRender);
		}
	}
	
	if (!gameCode.isValid) {
		gameCode.updateAndRender = nullptr;
	}
	
	return gameCode;
}

void
unloadGameCode(GameCode& gameCode)
{
	if (gameCode.gameDLL) {
		dlclose(gameCode.gameDLL);
		gameCode.gameDLL = 0;
	}
	
	gameCode.isValid = false;
	gameCode.updateAndRender = nullptr;
}


#endif


struct GameContext {
	SDLApplication*			app = nullptr;
	GameCode				gameCode = {};
	GameMemory				gameMemory = {};
	input::PlatformInput	input = {};
	std::atomic_bool		done; // TODO: consider using SDL atomics instead of std to avoid the template
};


void createGameContext(GameContext& gameContext)
{
	gameContext.app = &app;
	
	loadGameCode(gameContext.gameCode);
	
	// Note: not asserting on full for the event concurrent queues, if the game stops processing
	// events, the queue will fill up quickly. We will simply ignore inputs in that case.
	gameContext.input.eventsQueue.init(PLATFORMINPUT_EVENTSQUEUE_CAPACITY, nullptr, 0);
	gameContext.input.popEvents.init(PLATFORMINPUT_EVENTSPOPQUEUE_CAPACITY);
	gameContext.input.motionEventsQueue.init(PLATFORMINPUT_MOTIONEVENTSQUEUE_CAPACITY, nullptr, 0);
	gameContext.input.popMotionEvents.init(PLATFORMINPUT_MOTIONEVENTSPOPQUEUE_CAPACITY);

	// gameContext.input should be stored on a cache line boundary due to the concurrent queues
	// contained within, to prevent possible false sharing
	assert(is_aligned(&gameContext.input.eventsQueue, 64) && "ConcurrentQueue not stored on a cache line boundary");
	assert(is_aligned(&gameContext.input.motionEventsQueue, 64) && "ConcurrentQueue not stored on a cache line boundary");
}


void destroyGameContext(GameContext& gameContext)
{
	gameContext.input.eventsQueue.deinit();
	gameContext.input.popEvents.deinit();
	gameContext.input.motionEventsQueue.deinit();
	gameContext.input.popMotionEvents.deinit();
}


static void getPreferencesPath_utf8(char* dst)
{
	char* path = SDL_GetPrefPath(PROGRAM_NAME, PROGRAM_NAME);
	if (path) {
		_strcpy_s(dst, MAXPATH, path);
		SDL_free(path);
	}
}

static void getCurrentWorkingDirectory_utf8(char* dst, int max)
{
	char* path = SDL_GetBasePath();
	if (path) {
		_strcpy_s(dst, max, path);
		SDL_free(path);
	}
}

bool getWindowInfo(SDL_Window* window, SDL_SysWMinfo* info)
{
	SDL_VERSION(&info->version);
	if (SDL_GetWindowWMInfo(window, info) != SDL_TRUE) {
		logger::error(SDL_GetError());
		return false;
	}
	return true;
}

bool getEnvironmentInfo(Environment* env)
{
	// TODO: probably need to convert this UTF8 string to wchar_t for windows
	getPreferencesPath_utf8(env->preferencesPath);
	getCurrentWorkingDirectory_utf8(env->currentWorkingDirectory, sizeof(env->currentWorkingDirectory));
	
	return strlen(env->preferencesPath) 
		&& strlen(env->currentWorkingDirectory);
}

PlatformApi createPlatformApi()
{
	PlatformApi api{};
	
	api.log = &logger::log;
	api.allocate = &platformAllocate;

	return api;
}
