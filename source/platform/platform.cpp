#include <atomic>
#include "../capacity.h"
#include "platform.h"
#include "platform_api.h"
#include "../input/platform_input.h"
#include "../utility/logger.h"
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
		gameContext.gameCode.onUnload(
			&gameContext.gameMemory,
			_platformApi,
			gameContext.app);

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
		gameCode.updateAndRender =
			(GameUpdateAndRenderFunc*)GetProcAddress(
				gameCode.gameDLL, "gameUpdateAndRender");

		gameCode.onLoad =
			(GameOnLoadFunc*)GetProcAddress(
				gameCode.gameDLL, "onLoad");

		gameCode.onUnload =
			(GameOnUnloadFunc*)GetProcAddress(
				gameCode.gameDLL, "onUnload");

		gameCode.onExit =
			(GameOnExitFunc*)GetProcAddress(
				gameCode.gameDLL, "onExit");

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
		gameCode.onUnload = nullptr;
		gameCode.onExit = nullptr;
	}

	return loaded;
}

void updateMemoryStatus(
	SystemInfo& info)
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	info.availPhysBytes = status.ullAvailPhys;
	info.availVirtBytes = status.ullAvailVirtual;
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
	updateMemoryStatus(info);

	// TODO: call IsProcessorFeaturePresent on features we need to check for

	return info;
}

PlatformBlock* platformAllocate(
	size_t minimumSize)
{
	assert(minimumSize < UINT_MAX); // we don't use 8 bytes to store size, no block should ever be larger than 4GB

	SIZE_T size = ((minimumSize / app.systemInfo.allocationGranularity) + 1) * app.systemInfo.allocationGranularity;
	SIZE_T minAllocSize = ((MEMORY_MIN_PLATFORM_ALLOC_SIZE / app.systemInfo.allocationGranularity) + 1) * app.systemInfo.allocationGranularity;
	size = max(size, minAllocSize);
	void* memory = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	
	// if we can't allocate memory, time to panic (for now)
	if (!memory) {
		// TODO: before exiting, we could try to detect and free up memory and retry the allocation
		showLastErrorAndQuit();
	}

	PlatformBlock* block = (PlatformBlock*)memory;
	block->memoryBlock.base = (void*)((uintptr_t)memory + sizeof(PlatformBlock));
	block->memoryBlock.size = (u32)(size - sizeof(PlatformBlock));
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


PlatformFileChangeHandle platformBeginDirectoryWatch(
	const char* relWatchPath)
{
	PlatformFileChangeHandle result = 0;

	HANDLE dwChangeHandle = FindFirstChangeNotificationA(
			relWatchPath,				   // directory to watch
			TRUE,						   // watch subtree
			FILE_NOTIFY_CHANGE_FILE_NAME | // watch file, directory, and last write time changes
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_LAST_WRITE);
	
	if (dwChangeHandle != INVALID_HANDLE_VALUE) {
		result = (PlatformFileChangeHandle)dwChangeHandle;
	}
	return result;
}

void platformEndDirectoryWatch(
	PlatformFileChangeHandle handle)
{
	HANDLE dwChangeHandle = (HANDLE)handle;
	FindCloseChangeNotification(dwChangeHandle);
}

/**
 * Blocking call that begins a directory watch loop. Call this in a dedicated thread to monitor a
 * directory and its subpaths for file and directory changes.
 * 
 * The onChangeCallback is called when an event or an intermittent timeout occurs. A low timeout
 * period is important for responding to the program exit condition in a timely manner, although
 * simply not waiting for the thread to join and allowing the OS to clean up is a fine option.
 * 
 * The loop exits when onChangeCallback returns a non-zero value, causing this function to return.
 * The onChangeCallback runs in the same thread from which this function is called, so be careful
 * about locking any shared state passed through the userData pointer. onChangeCallback may be used
 * to directly run tasks or may queue up tasks to be run on another worker thread.
 * 
 * This functions returns non-zero on failure, and zero for any other exit condition.
 */
int platformRunDirectoryWatchLoop(
	PlatformFileChangeHandle* handles,
	const u32* numHandles,
	FileChangeCallbackFunc* onChangeCallback,
	u32 intermittentTimeoutMS,
	void* userData,
	MemoryArena& taskMemory)
{
	int result = 0;
	assert(onChangeCallback);
	assert(intermittentTimeoutMS <= 1000 && "a low timeout (<= ~33ms) is recommended");

	HANDLE* dwChangeHandles = (HANDLE*)handles;

	bool done = false;
	while (!done)
	{
		DWORD dwWaitStatus = 
			WaitForMultipleObjects(
				*numHandles,
				dwChangeHandles,
				FALSE,
				intermittentTimeoutMS);

		if (dwWaitStatus == WAIT_TIMEOUT) {
			done = (
				onChangeCallback(
					Platform_WatchEvent_Timeout,
					0, 0,
					userData,
					taskMemory) != 0);
		}
		else {
			u32 index = dwWaitStatus - WAIT_OBJECT_0;

			done = (
				onChangeCallback(
					Platform_WatchEvent_Change,
					index, handles[index],
					userData,
					taskMemory) != 0);

			if (!done) {
				done = (FindNextChangeNotification(dwChangeHandles[index]) == FALSE);
			}
		}
	}

	return result;
}


// NOT _WIN32
#else

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <ctime>

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

void platformSleep(
	unsigned long ms)
{
	usleep(ms * 1000);
}

void showErrorBox(const char* text, const char* caption)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, nullptr);
}

void setWindowIcon(WindowData *windowData)
{
	// TODO: enable this
	//SDL_Surface* icon = IMG_Load(const char *file);
	//SDL_SetWindowIcon(window, icon);
	//SDL_FreeSurface(icon);
}

time_t fileGetLastWriteTime(
	const char *filename)
{
	time_t lastWriteTime = time(nullptr);
	struct stat fileStatus;
	if (stat(filename, &fileStatus) == 0) {
		lastWriteTime = fileStatus.st_mtime;
	}

	return lastWriteTime;
}

void unloadGameCode(
	GameCode& gameCode)
{
	if (gameCode.gameModule) {
		gameContext.gameCode.onUnload(
			&gameContext.gameMemory,
			_platformApi,
			gameContext.app);

		//dlclose(gameCode.gameModule);
		SDL_UnloadObject(gameCode.gameModule);
		gameCode.gameModule = 0;
	}
	
	gameCode.isValid = false;
	gameCode.updateAndRender = nullptr;
}

bool loadGameModule(
	GameCode& gameCode,
	const char* gameModulePath)
{
	//gameCode.gameModule = dlopen(gameModulePath, RTLD_LAZY);
	gameCode.gameModule = SDL_LoadObject(gameModulePath);
	if (gameCode.gameModule) {
		//gameCode.updateAndRender = (GameUpdateAndRenderFunc*)dlsym(gameCode.gameModule, "gameUpdateAndRender");
		//gameCode.onLoad   = (GameOnLoadFunc*)dlsym(gameCode.gameModule, "onLoad");
		//gameCode.onUnload = (GameOnUnloadFunc*)dlsym(gameCode.gameModule, "onUnload");
		//gameCode.onExit   = (GameOnExitFunc*)dlsym(gameCode.gameModule, "onExit");
		gameCode.updateAndRender =
			(GameUpdateAndRenderFunc*)SDL_LoadFunction(
				gameCode.gameModule, "gameUpdateAndRender");

		gameCode.onLoad =
			(GameOnLoadFunc*)SDL_LoadFunction(
				gameCode.gameModule, "onLoad");

		gameCode.onUnload =
			(GameOnUnloadFunc*)SDL_LoadFunction(
				gameCode.gameModule, "onUnload");

		gameCode.onExit =
			(GameOnExitFunc*)SDL_LoadFunction(
				gameCode.gameModule, "onExit");

		gameCode.isValid = (gameCode.updateAndRender);
	}
	return gameCode.isValid;
}

bool loadGameCode(
	GameCode& gameCode)
{
	bool loaded = false;
	const char *gameModulePath    = "./game.so";
	const char *newGameModulePath = "./build/game.so";
	const char *lockFilePath      = "./build/lock.tmp";
	
	// load on startup
	if (!gameCode.isValid) {
		if (loadGameModule(gameCode, gameModulePath)) {
			gameCode.moduleLastWriteTime = fileGetLastWriteTime(gameModulePath);
			loaded = true;
		}
	}
	// try reload from new .so in build directory
	else {
		time_t lockFileTime = fileGetLastWriteTime(lockFilePath);
		if (lockFileTime == time(nullptr))
		{
			time_t newModuleFileTime = fileGetLastWriteTime(newGameModulePath);

			if (newModuleFileTime != time(nullptr)
				&& difftime(newModuleFileTime, gameCode.moduleLastWriteTime) != 0)
			{
				logger::info("new game.so detected...");
				unloadGameCode(gameCode);
				
				execl("/bin/cp", "-p", newGameModulePath, gameModulePath, (char*)0);

				if (loadGameModule(gameCode, gameModulePath)) {
					gameCode.moduleLastWriteTime = newModuleFileTime;
					loaded = true;
				}

				logger::info(gameCode.isValid ? "loaded!" : "ERROR LOADING");
			}
		}
	}

	if (!gameCode.isValid) {
		gameCode.updateAndRender = nullptr;
		gameCode.onLoad = nullptr;
		gameCode.onUnload = nullptr;
		gameCode.onExit = nullptr;
	}

	return loaded;
}

void updateMemoryStatus(
	SystemInfo& info)
{
	FILE* f = fopen("/proc/meminfo", "r");
	if (f == nullptr) {
		showErrorBox("Cannot access /proc/meminfo", "Error");
		exit(EXIT_FAILURE);
	}

	const char* MEM_TOTAL_PREFIX  = "MemTotal:";
	const char* MEM_FREE_PREFIX   = "MemFree:";
	const char* MEM_CACHED_PREFIX = "Cached:";

	info.availPhysBytes = 0;

	char* linePtr = nullptr;
	char lineBuf[256] = {};

	while ((linePtr = fgets(lineBuf, sizeof(lineBuf), f)) != 0)
	{
		size_t value = 0;

		if (sscanf(lineBuf, "%*s %lld kB", &value) != 1) {
			continue;
		}

		if (strncmp(lineBuf, MEM_FREE_PREFIX, sizeof(MEM_FREE_PREFIX) - 1) == 0) {
			info.availPhysBytes += (value * 1024);
		}
		else if (strncmp(lineBuf, MEM_CACHED_PREFIX, sizeof(MEM_CACHED_PREFIX) - 1) == 0) {
			info.availPhysBytes += (value * 1024);
		}
	}

	info.availVirtBytes = info.availPhysBytes;

	fclose(f);
}

SystemInfo platformGetSystemInfo()
{
	SystemInfo info{};

	info.pageSize = getpagesize();
	info.allocationGranularity = info.pageSize;
	
	// get minimum mmap address
	info.minimumApplicationAddress = 0;
	FILE* f = fopen("/proc/sys/vm/mmap_min_addr", "r");
	if (f == nullptr) {
		showErrorBox("/proc/sys/vm/mmap_min_addr", "Error");
		exit(EXIT_FAILURE);
	}
	char lineBuf[256] = {};
	uintptr_t minAddr = 0;
	if (fgets(lineBuf, sizeof(lineBuf), f) != 0) {
		if (sscanf(lineBuf, "%lld", &minAddr) == 1) {
			info.minimumApplicationAddress = (void*)minAddr;
		}
	}
	fclose(f);
	
	// get max virtual address
	rlimit addressSpace{};
	if (getrlimit(RLIMIT_AS, &addressSpace) == 0) {
		info.maximumApplicationAddress = (void*)((uintptr_t)addressSpace.rlim_cur + (uintptr_t)info.minimumApplicationAddress);
	}
	
	info.activeProcessorMask = 0;
	info.logicalProcessorCount = SDL_GetCPUCount();
	info.processorArchitecture = 0;
	info.processorLevel = 0;
	info.processorRevision = 0;
	
	info.systemRAM = SDL_GetSystemRAM();
	updateMemoryStatus(info);

	// TODO: get cpu features we need to check for

	return info;
}

PlatformBlock* platformAllocate(
	size_t minimumSize)
{
	assert(minimumSize < UINT_MAX); // we don't use 8 bytes to store size, no block should ever be larger than 4GB

	size_t size = ((minimumSize / app.systemInfo.allocationGranularity) + 1) * app.systemInfo.allocationGranularity;
	size_t minAllocSize = ((MEMORY_MIN_PLATFORM_ALLOC_SIZE / app.systemInfo.allocationGranularity) + 1) * app.systemInfo.allocationGranularity;
	size = max(size, minAllocSize);
	void* memory = mmap(
					nullptr, // kernel chooses page-aligned address
					size,    // length in bytes
					PROT_READ | PROT_WRITE, // pages may be read and written
					MAP_PRIVATE      // private copy-on-write mapping, not visible to other processes
					| MAP_ANONYMOUS  // not backed by file, contents are initialized to zero
					| MAP_NORESERVE, // do not reserve swap space for this mapping
					-1, 0);
	
	// if we can't allocate memory, time to panic (for now)
	if (memory == MAP_FAILED) {
		// TODO: before exiting, we could try to detect and free up memory and retry the allocation
		showErrorBox("Out of memory", "Error");
		exit(EXIT_FAILURE);
	}

	PlatformBlock* block = (PlatformBlock*)memory;
	block->memoryBlock.base = (void*)((uintptr_t)memory + sizeof(PlatformBlock));
	block->memoryBlock.size = (u32)(size - sizeof(PlatformBlock));
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
			&& sentinel.next != &sentinel
			&& gameContext.platformMemory.totalSize > 0)
		   ||
		   (gameContext.platformMemory.numBlocks == 0
			&& sentinel.prev == &sentinel
			&& sentinel.next == &sentinel
			&& gameContext.platformMemory.totalSize == 0));

	SDL_UnlockMutex(gameContext.platformMemory.lock);
	
	int result = munmap(block, size);
	assert(result == 0);
}

// END NOT WIN32
#endif


bool initGameContext()
{
	gameContext.app = &app;
	
	if (!loadGameCode(gameContext.gameCode)) {
		return false;
	}
	
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
	
	return true;
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
	api.findAllFiles = nullptr;//&platformFindAllFiles;
	api.watchDirectory = nullptr;//&platformRunDirectoryWatchLoop;

	return api;
}

PlatformApi& platformApi()
{
	assert(_platformApi);
	return *_platformApi;
}
