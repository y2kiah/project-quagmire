
#include "platform.h"
#include <SDL_filesystem.h>

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

#ifdef _WIN32

#include "Windows.h"

void yieldThread()
{
	SwitchToThread();
}

void showErrorBox(const char *text, const char *caption)
{
	MessageBoxA(NULL, text, caption, MB_OK | MB_ICONERROR | MB_TOPMOST);
}

void setWindowIcon(const WindowData* windowData)
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

#endif


bool getWindowInfo(SDL_Window* window, SDL_SysWMinfo* info)
{
	SDL_VERSION(&info->version);
	if (SDL_GetWindowWMInfo(window, info) != SDL_TRUE) {
		// log (SDL_GetError());
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
