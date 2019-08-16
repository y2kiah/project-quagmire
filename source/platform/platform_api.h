#ifndef _PLATFORM_API_H
#define _PLATFORM_API_H

#include <SDL_render.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <SDL_joystick.h>
#include <SDL_mouse.h>
#include "../utility/common.h"
#include "../utility/memory.h"
#include "../utility/logger.h"

/*class FileSystemWatcher {
public:
	void onFileRenamed(std::function<void(const std::wstring&, const std::wstring&)> f);
	void onFileModified(std::function<void(const std::wstring&)> f);
	void onFileAdded(std::function<void(const std::wstring&)> f);
	void onFileRemoved(std::function<void(const std::wstring&)> f);

};*/

#define PROGRAM_NAME "Project Quagmire"

#define MAXPATH		1024


// TODO: switch these to fstring?
struct Environment {
	char preferencesPath[MAXPATH] = {};
	char currentWorkingDirectory[MAXPATH] = {};
};

struct DisplayData {
	SDL_Rect		bounds;			// bounds of the display
	SDL_DisplayMode	displayMode;	// desktop display mode
};

struct WindowData {
	SDL_Window*		window;			// SDL window handle
	SDL_GLContext	glContext;		// OpenGL context handle
	u32				width;			// width px of the window
	u32				height;			// height px of the window
	SDL_SysWMinfo	wmInfo;			// system-dependent window information
};

struct SystemInfo {
	u32				logicalProcessorCount;
	u32				systemRAM;		// amount of system RAM in MB
	
	size_t			availPhysBytes;	// available physical memory in MB
	size_t			availVirtBytes;	// available virtual memory in MB
	
	u32				pageSize;
	u32				allocationGranularity;
	
	void*			minimumApplicationAddress;
	void*			maximumApplicationAddress;
	
	u64				activeProcessorMask;
	u16				processorArchitecture;
	u16				processorLevel;
	u16				processorRevision;
};

struct JoystickInfo {
	SDL_Joystick*	joysticks[GAMEINPUT_MAX_JOYSTICKS];
	u32				numJoysticks;
	u32				totalAxes;
};

enum InputMouseCursor : u8 {
	Cursor_Arrow = 0,
	Cursor_Hand,
	Cursor_Wait,
	Cursor_IBeam,
	Cursor_Crosshair,
	_InputMouseCursorCount
};

struct SDLApplication {
	WindowData		windowData = {};
	SystemInfo		systemInfo = {};
	Environment		environment = {};
	JoystickInfo	joystickInfo = {};
	int				numDisplays = 0;
	DisplayData		displayData[50] = {};
	SDL_Cursor*		cursors[_InputMouseCursorCount];
};

typedef PlatformBlock* PlatformAllocateFunc(size_t);
typedef void PlatformDeallocateFunc(PlatformBlock*);


// Find all files

typedef void FindAllFilesCallbackFunc(
	const char* filePath,
	u32 sizeBytes,
	bool isDirectory,
	void* userData);

struct PlatformFindAllFilesResult {
	u32	numFiles;
	u32	numDirectories;
};

typedef PlatformFindAllFilesResult PlatformFindAllFilesFunc(
	const char* relSearchPath,
	bool recursive,
	u8 maxDepth,
	FindAllFilesCallbackFunc* callback,
	void* userData);

// File information

struct PlatformFileInfo {
	u64		lastWrite;
};


// Watch for file / directory changes

typedef u64 PlatformFileChangeHandle;

enum PlatformWatchEventType : u8 {
	Platform_WatchEvent_Change = 0,	// includes creating, renaming or deleting a file or directory, and modifying last write time
	Platform_WatchEvent_Timeout		// intermittentTimeout occurs without an event, useful for detecting thread exit condition
};

typedef int FileChangeCallbackFunc(
	PlatformWatchEventType changeType,
	u32 handleIndex,
	PlatformFileChangeHandle handle,
	void* userData,
	MemoryArena& taskMemory);

typedef int PlatformRunDirectoryWatchLoop(
	PlatformFileChangeHandle* handles,
	const u32* numHandles,
	FileChangeCallbackFunc* onChangeCallback,
	u32 intermittentTimeoutMS,
	void* userData,
	MemoryArena& taskMemory);


// Platform API function pointers passed to game dll

struct PlatformApi {
	logger::LogFunc*				log;
	PlatformAllocateFunc*			allocate;
	PlatformDeallocateFunc*			deallocate;
	PlatformFindAllFilesFunc*		findAllFiles;
	PlatformRunDirectoryWatchLoop*	watchDirectory;
};

struct Game;

struct GameMemory {
	MemoryArena		gameState;
	MemoryArena		transient;
	MemoryArena		frameScoped;
	Game*			game;
	bool			initialized;
};

namespace logger {
	void log(Category, Priority, const char*, va_list);
}

PlatformApi& platformApi();


#endif