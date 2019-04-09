#ifndef _LOGGER_H
#define _LOGGER_H

#include <cstdarg>
#include "types.h"
#include "nstring.h"

namespace logging {
	/**
	 * Logging mode. Normally you'd set Immediate_Thread_Unsafe only for early initialization
	 * before multiple threads are running, and Deferred_Thread_Safe while multiple threads are
	 * running. If you use Immediate mode, SDL logging must have already been initialized before
	 * calling any logging function. With Deferred mode, SDL must be initialized before the
	 * first flush.
	 */
	enum Mode : u8 {
		Mode_Deferred_Thread_Safe = 0,	// (default) messages are queued and must be flushed, thread safe
		Mode_Immediate_Thread_Unsafe	// message written immediately, not thread safe
	};
	
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
		_Category_Count
	};

	enum Priority : u8 {
		Priority_Off = 0,
		Priority_Critical,
		Priority_Error,
		Priority_Warn,
		Priority_Info,
		Priority_Debug,
		Priority_Verbose
	};
	
	struct LogMessage {
		//char*		message;
		Category	category;
		Priority	priority;
		u64			id;

		u8			_padding[6];

		fstring254	message; // TODO: don't really want to use a fixed len string here, we want a string ring-buffer allowing variable length strings
	};
	static_assert_aligned_size(LogMessage,8);

	// TODO: consider giving Logger a name (union w entityId) to differentiate logging between various systems and individual entities
	// for later output filtering.
	// consider decoupling categories from SDL categories and just use one SDL cat to output
	// TODO: keep an entity/gameplay log history file specific to save-game, with save game diffs stacking up. If an old save game is deleted, prepend old diffs into history
	struct Logger {
		Category	defaultCategory = Category_System;
		Priority	categoryDefaultPriority[_Category_Count] = {
						Priority_Info,		// Category_Application
						Priority_Critical,	// Category_Error
						Priority_Warn,		// Category_Assert
						Priority_Critical,	// Category_System
						Priority_Critical,	// Category_Audio
						Priority_Critical,	// Category_Video
						Priority_Critical,	// Category_Render
						Priority_Critical,	// Category_Input
						Priority_Verbose	// Category_Test
					};

		// Functions
	
		void critical(const char *s, ...);
		void error(const char *s, ...);
		void warn(const char *s, ...);
		void info(const char *s, ...);
		void debug(const char *s, ...);
		void verbose(const char *s, ...);

		void critical(Category c, const char *s, ...);
		void error(Category c, const char *s, ...);
		void warn(Category c, const char *s, ...);
		void info(Category c, const char *s, ...);
		void debug(Category c, const char *s, ...);
		void verbose(Category c, const char *s, ...);
		void test(const char *s, ...);

		void out(const char *s, ...);
		void out(Category c, const char *s, ...);
		void out(Category c, Priority p, const char *s, ...);
		void _out(Category c, Priority p, const char *s, va_list args);

		/**
		* Sets the priority for a category. This call is not thread safe, call this only
		* from the same thread that normally calls flush.
		*/
		void setPriority(Category c, Priority p);

		/**
		* Sets the priority for all categories. This call is not thread safe, call this only
		* from the same thread that normally calls flush.
		*/
		void setAllPriorities(Priority p);
	};

}

#endif