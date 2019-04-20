#ifndef _LOGGER_H
#define _LOGGER_H

#include <cstdarg>
#include "types.h"
#include "nstring.h"

namespace logger {
	
	/**
	 * Logging categories. Each category can target a different write destination and priority
	 * level. Mirroring SDL, by default the Application category is enabled at the Info level,
	 * the Assert category is enabled at the Warn level, Test is enabled at the Verbose level,
	 * and all other categories are enabled at the Critical level.
	 */
	enum Category : u8 {
		Category_Application = 0,	// gameplay code
		Category_Error,				// platform errors
		Category_Assert,			// asserts are logged when QUAGMIRE_LOG_ASSERTS is defined
		Category_System,			// platform code
		Category_Audio,				// audio system
		Category_Video,				// video playback
		Category_Render,			// renderer
		Category_Input,				// input system
		Category_Test,				// test code
		_Category_Count,
		_Category_Default
	};

	enum Priority : u8 {
		Priority_Off = 0,
		Priority_Critical,
		Priority_Error,
		Priority_Warn,
		Priority_Info,
		Priority_Debug,
		Priority_Verbose,
		_Priority_Default
	};
	
	// function type for the log api that spans the dll boundary
	typedef void LogFunc(Category, Priority, const char*, va_list);

	// Logging functions

	void critical(const char* s, ...);
	void error(const char* s, ...);
	void warn(const char* s, ...);
	void info(const char* s, ...);
	void debug(const char* s, ...);
	void verbose(const char* s, ...);
	void critical(Category c, const char* s, ...);
	void error(Category c, const char* s, ...);
	void warn(Category c, const char* s, ...);
	void info(Category c, const char* s, ...);
	void debug(Category c, const char* s, ...);
	void verbose(Category c, const char* s, ...);
	void test(const char* s, ...);
	void out(Category c, Priority p, const char* s, ...);
	void out(Category c, const char* s, ...);
	void out(const char* s, ...);

}

#endif