
#include <atomic>
#include "../capacity.h"
#include "platform.h"
#include "platform_api.h"
#include "input/platform_input.h"
#include "utility/logger.h"
#include <SDL_filesystem.h>
#include <SDL_cpuinfo.h>

#ifdef _WIN32

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

void logLastError()
{
	LPSTR pBuffer = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		0,
		GetLastError(),
		0,
		(LPSTR)&pBuffer,
		0,
		0);
	logger::error(logger::Category_System, pBuffer);
	LocalFree(pBuffer);
}

void setWindowIcon(
	const WindowData* windowData)
{
	HWND hWnd = windowData->wmInfo.info.win.window;
	HINSTANCE handle = GetModuleHandle(nullptr);

// TODO: find a way to do this without a .rc file or just use SDL same as on linux
//	HICON icon = LoadIcon(handle, MAKEINTRESOURCE(IDI_ICON1));
//	if (icon != nullptr) {
//		SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR)icon);
//		SetClassLongPtr(hWnd, GCLP_HICONSM, (LONG_PTR)icon);
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

	info.systemRAM = SDL_GetSystemRAM();

	// TODO: call IsProcessorFeaturePresent on features we need to check for

	return info;
}

PlatformBlock* platformAllocate(
	size_t minimumSize)
{
	SIZE_T size = ((minimumSize / app.systemInfo.allocationGranularity) + 1) * app.systemInfo.allocationGranularity;
	void* memory = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	
	// if we can't allocate memory, time to panic (for now)
	if (!memory) {
		// TODO: before exiting, we could try to detect and free up memory and retry the allocation
		showLastErrorAndQuit();
	}

	PlatformBlock* block = (PlatformBlock*)memory;
	block->memoryBlock.base = (void*)((uintptr_t)memory + sizeof(PlatformBlock));
	block->memoryBlock.size = size - sizeof(PlatformBlock);
	block->next = &gameContext.platformMemory.sentinel;

	SDL_LockMutex(gameContext.platformMemory.lock);
	
	block->prev = gameContext.platformMemory.sentinel.prev;
	block->prev->next = block;
	block->next->prev = block;

	gameContext.platformMemory.totalSize += size;
	++gameContext.platformMemory.numBlocks;

	SDL_UnlockMutex(gameContext.platformMemory.lock);

	return block;
}

void platformDeallocate(
	PlatformBlock* block)
{
	SDL_LockMutex(gameContext.platformMemory.lock);

	size_t size = block->memoryBlock.size + sizeof(PlatformBlock);
	gameContext.platformMemory.totalSize -= size;
	--gameContext.platformMemory.numBlocks;
	
	block->prev->next = block->next;
	block->next->prev = block->prev;
	
	// check the integrity of the list
	PlatformBlock& sentinel = gameContext.platformMemory.sentinel;
	assert((gameContext.platformMemory.numBlocks > 0
			&& sentinel.prev != &sentinel
			&& sentinel.next == &sentinel
			&& gameContext.platformMemory.totalSize > 0)
		   ||
		   (gameContext.platformMemory.numBlocks == 0
			&& sentinel.prev == &sentinel
			&& sentinel.next == &sentinel
			&& gameContext.platformMemory.totalSize == 0));

	SDL_UnlockMutex(gameContext.platformMemory.lock);
	
	BOOL result = VirtualFree(block, 0, MEM_RELEASE);
	assert(result);
}


PlatformFindAllFilesResult platformFindAllFiles(
	const char* relSearchPath,
	bool recursive = false,
	u8 maxDepth = MAX_FILE_RECURSION_DEPTH,
	FindAllFilesCallbackFunc* callback = nullptr,
	void* userData = nullptr)
{
	PlatformFindAllFilesResult result{};
	assert(callback);
	WIN32_FIND_DATAA ffd;

	HANDLE hFind = FindFirstFileExA(
		relSearchPath,
		FindExInfoBasic,
		&ffd,
		FindExSearchNameMatch,
		NULL,
		FIND_FIRST_EX_LARGE_FETCH);

	if (hFind == INVALID_HANDLE_VALUE) 
	{
		logLastError();
		return result;
	}
	
	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			callback(
				ffd.cFileName,
				0,
				true,
				userData);
			++result.numDirectories;

			if (recursive) {
				assert(maxDepth != 1 && "max recursion depth reached, raise the limit or reduce nested directories.");
				if (maxDepth != 1) {
					PlatformFindAllFilesResult rr =
						platformFindAllFiles(
							ffd.cFileName,
							true,
							maxDepth - 1,
							callback,
							userData);
					result.numFiles += rr.numFiles;
					result.numDirectories += rr.numDirectories;
				}
			}
		}
		else {
			LARGE_INTEGER fileSize;
			fileSize.LowPart = ffd.nFileSizeLow;
			fileSize.HighPart = ffd.nFileSizeHigh;
			assert(fileSize.QuadPart <= UINT_MAX && "individual files must be under 4GB");

			callback(
				ffd.cFileName,
				(u32)fileSize.QuadPart,
				false,
				userData);
			++result.numFiles;
		}
	}
	while (FindNextFileA(hFind, &ffd) != 0);
	
	FindClose(hFind);

	return result;
}


PlatformApi& platformApi()
{
	assert(_platformApi);
	return *_platformApi;
}



// NOT _WIN32
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

SystemInfo platformGetSystemInfo()
{
	SystemInfo info{};
	/*SYSTEM_INFO si{};
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
	*/

	info.logicalProcessorCount = SDL_GetCPUCount();
	info.systemRAM = SDL_GetSystemRAM();

	// TODO: call IsProcessorFeaturePresent on features we need to check for

	return info;
}



#endif


void initGameContext()
{
	gameContext.app = &app;
	
	loadGameCode(gameContext.gameCode);
	
	// Note: not asserting on full for the event concurrent queues, if the game stops processing
	// events, the queue will fill up quickly. We will simply ignore inputs in that case.
	gameContext.input.eventsQueue.init(
			PLATFORMINPUT_EVENTSQUEUE_CAPACITY,
			allocArrayOfType(platformMemory, input::InputEvent, PLATFORMINPUT_EVENTSQUEUE_CAPACITY),
			0);
	gameContext.input.motionEventsQueue.init(
			PLATFORMINPUT_MOTIONEVENTSQUEUE_CAPACITY,
			allocArrayOfType(platformMemory, input::InputEvent, PLATFORMINPUT_MOTIONEVENTSQUEUE_CAPACITY),
			0);
	gameContext.input.popEvents.init(
			PLATFORMINPUT_EVENTSPOPQUEUE_CAPACITY,
			allocArrayOfType(platformMemory, input::InputEvent, PLATFORMINPUT_EVENTSPOPQUEUE_CAPACITY));
	gameContext.input.popMotionEvents.init(
			PLATFORMINPUT_MOTIONEVENTSPOPQUEUE_CAPACITY,
			allocArrayOfType(platformMemory, input::InputEvent, PLATFORMINPUT_MOTIONEVENTSPOPQUEUE_CAPACITY));

	// gameContext.input should be stored on a cache line boundary due to the concurrent queues
	// contained within, to prevent possible false sharing
	assert(is_aligned(&gameContext.input.eventsQueue, 64) && "ConcurrentQueue not stored on a cache line boundary");
	assert(is_aligned(&gameContext.input.motionEventsQueue, 64) && "ConcurrentQueue not stored on a cache line boundary");
}


void deinitGameContext()
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
	api.deallocate = &platformDeallocate;
	api.findAllFiles = &platformFindAllFiles;

	return api;
}
