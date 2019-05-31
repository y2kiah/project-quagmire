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

struct PlatformApi {
	logger::LogFunc*		log;
	PlatformAllocateFunc*	allocate;
	PlatformDeallocateFunc*	deallocate;
};

struct Game;

struct GameMemory {
	MemoryArena		gameState;
	MemoryArena		transient;
	MemoryArena		frameScoped;
	Game*			game;		// Note: using void* here to avoid having to declare Game struct on the platform side
	bool			initialized;
};

namespace logger {
	void log(Category, Priority, const char*, va_list);
}


#endif