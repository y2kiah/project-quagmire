#include <SDL_log.h>
#include <atomic>
#include "../capacity.h"
#include "common.h"
#include "logger.h"
#include "concurrent_queue.h"

namespace logger {

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

	struct LogMessage {
		//char*		message;
		Category	category;
		Priority	priority;
		u64			id;

		u8			_padding[6];

		fstring254	message; // TODO: don't really want to use a fixed len string here, we want a string ring-buffer allowing variable length strings
	};
	static_assert_aligned_size(LogMessage,8);

	// TODO: consider giving loggers a name (union w entityId) to differentiate logging between various systems and individual entities
	// for later output filtering.
	// consider decoupling categories from SDL categories and just use one SDL cat to output
	// TODO: keep an entity/gameplay log history file specific to save-game, with save game diffs stacking up. If an old save game is deleted, prepend old diffs into history

    Mode loggingMode = Mode_Deferred_Thread_Safe;
	
	global ConcurrentQueue messageQueue;

	int popArrayLen = 0;
	LogMessage popArray[LOGGER_CAPACITY] = {};

	Category defaultCategory = Category_System;
	Priority categoryDefaultPriority[_Category_Count] = {
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


	void write(const LogMessage& m)
	{
		SDL_LogPriority sdlPriority = (SDL_LogPriority)(m.priority == 0 ? 0 : SDL_NUM_LOG_PRIORITIES - m.priority);
		SDL_LogMessage(m.category, sdlPriority, "%lu %s", m.id, m.message.c_str);
	}

	/**
	 * Empties the thread-safe queue and writes all messages. Call this at least once per frame
	 * while in Deferred_Thread_Safe mode. Also may want to call before exiting to flush messages
	 * logged after the main loop has exited.
	 */
	void flush()
	{
		popArrayLen = messageQueue.try_pop_all(popArray, countof(popArray));
		for (int i = 0; i < popArrayLen; ++i) {
			write(popArray[i]);
		}
		popArrayLen = 0;
	}

	/**
	 * Setting the mode will also flush the queue. This call is not thread safe, call this only
	 * from the same thread that normally calls flush.
	 */
	void setMode(Mode mode) {
		flush();
		loggingMode = mode;
	}

	/**
	 * Sets the priority for a category. This call is not thread safe, call this only
	 * from the same thread that normally calls flush.
	 */
	void setPriority(Category c, Priority p)
	{
		categoryDefaultPriority[c] = p;
	}

	/**
	 * Sets the priority for all categories. This call is not thread safe, call this only
	 * from the same thread that normally calls flush.
	 */
	void setAllPriorities(Priority p)
	{
		memset(categoryDefaultPriority, p, sizeof(categoryDefaultPriority));
	}

    /**
	 * In Mode_Deferred_Thread_Safe, enqueues message to be flushed later.
	 * In Mode_Immediate_Thread_Unsafe, writes the message.
	 */
    void log(Category c, Priority p, const char* s, va_list args)
	{
		va_list argsCopy;
		va_copy(argsCopy, args);

		if (c == _Category_Default) { c = defaultCategory; }
		if (p == _Priority_Default) { p = categoryDefaultPriority[c]; }

		static std::atomic<u64> id{0};
		if (categoryDefaultPriority[c] >= p) {
			// write formatted string
			int len = _vscprintf(s, args);
			LogMessage msg = { c, p, id++, {}, {} };
			_vsnprintf_s(msg.message.c_str, 254, _TRUNCATE, s, argsCopy);
			msg.message.sizeB = (u8)min(len, 254);
			if (loggingMode == Mode_Deferred_Thread_Safe) {
				if (!messageQueue.push(&msg)) {
					flush();
					messageQueue.push(&msg);
				}
			}
			else {
				write(msg);
			}
		}
		va_end(argsCopy);
	}

	static_assert(Category_Application == (u8)SDL_LOG_CATEGORY_APPLICATION
			   && Category_Error       == (u8)SDL_LOG_CATEGORY_ERROR
			   && Category_Assert      == (u8)SDL_LOG_CATEGORY_ASSERT
			   && Category_System      == (u8)SDL_LOG_CATEGORY_SYSTEM
			   && Category_Audio       == (u8)SDL_LOG_CATEGORY_AUDIO
			   && Category_Video       == (u8)SDL_LOG_CATEGORY_VIDEO
			   && Category_Render      == (u8)SDL_LOG_CATEGORY_RENDER
			   && Category_Input       == (u8)SDL_LOG_CATEGORY_INPUT
			   && Category_Test        == (u8)SDL_LOG_CATEGORY_TEST,
			   "Logger Category mismatch with SDL");
}

#include "logger.cpp"