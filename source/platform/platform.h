/*class FileSystemWatcher {
public:
	void onFileRenamed(std::function<void(const std::wstring&, const std::wstring&)> f);
	void onFileModified(std::function<void(const std::wstring&)> f);
	void onFileAdded(std::function<void(const std::wstring&)> f);
	void onFileRemoved(std::function<void(const std::wstring&)> f);

};*/

#define MAXPATH		1024


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
	int				numDisplays = 0;
	DisplayData		displayData[50] = {};
	Environment		environment = {};
};


bool getWindowInfo(SDL_Window* window, SDL_SysWMinfo* info);
bool getEnvironmentInfo(Environment* env);

void yieldThread();

void showErrorBox(const char* text, const char* caption);

void setWindowIcon(const WindowData *windowData);


#if defined(_MSC_VER)
    #define _strncpy_s(dest,destsz,src,count)   strncpy_s(dest,destsz,src,count)
#else
    #define _strncpy_s(dest,destsz,src,count)   strncpy(dest,src,count)
#endif