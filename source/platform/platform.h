#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <SDL_render.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include "../utility/types.h"
#include "../utility/logger.h"
#include "../game.h"

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
	int				cpuCount;		// number of logical CPU cores
	int				systemRAM;		// amount of system RAM in MB
};

struct SDLApplication {
	WindowData		windowData = {};
	SystemInfo		systemInfo = {};
	Environment		environment = {};
	int				numDisplays = 0;
	DisplayData		displayData[50] = {};
};

struct PlatformApi {
	logger::LogFunc* log;
};

struct GameMemory {
	Game*			gameState;
	size_t			gameStateSize;

	void*			transient;
	size_t			transientSize;

	void*			frameScoped;
	size_t			frameScopedSize;
	
	bool			initialized;

	PlatformApi		platform;
};

namespace logger {
	void log(Category, Priority, const char*, va_list);
}
//bool getWindowInfo(SDL_Window* window, SDL_SysWMinfo* info);
//bool getEnvironmentInfo(Environment* env);

//void yieldThread();

//void showErrorBox(const char* text, const char* caption);

//void setWindowIcon(const WindowData *windowData);


#endif