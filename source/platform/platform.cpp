
static void getPreferencesPath_utf8(char* dst)
{
	char* path = SDL_GetPrefPath(PROGRAM_NAME, PROGRAM_NAME);
	strcpy_s(dst, MAXPATH, path);
	SDL_free(path);
}

// TODO: rewrite this when we have a way to convert char* to wchar_t* without wstring
/*static wchar_t* getPreferencesPathW()
{
	std::wstring ws;

	wchar_t* s = getPreferencesPath();
	ws.assign(s.begin(), s.end());

	return ws;
}*/


#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Windows.h"
#undef min
#undef max

static void getCurrentWorkingDirectory(char* dst)
{
	GetCurrentDirectoryA(MAXPATH, dst);
}

// TODO: rewrite this when we have a way to convert char* to wchar_t* without wstring
/*static wchar_t* getCurrentWorkingDirectoryW()
{
	std::wstring ws;

	wchar_t currentDir[LONG_MAX_PATH];
	GetCurrentDirectory(LONG_MAX_PATH, currentDir);
	ws = currentDir;

	return ws;
}*/

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

std::string getCurrentWorkingDirectory()
{
	std::string s;
	auto c = SDL_GetBasePath();
	s.assign(c);
	SDL_free(c);
	return s;
}

std::wstring getCurrentWorkingDirectoryW()
{
	std::wstring ws;

	auto s = getCurrentWorkingDirectory();
	ws.assign(s.begin(), s.end());

	return ws;
}

void yieldThread()
{
	// TODO: look for SDL yield or posix yield
	//std::this_thread::yield();
}

void showErrorBox(char *text, char *caption)
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
	getCurrentWorkingDirectory(env->currentWorkingDirectory);
	
	return env->preferencesPath != nullptr
		&& env->currentWorkingDirectory != nullptr;
}
