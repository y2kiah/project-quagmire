#include <SDL2/SDL_log.h>
#include "../capacity.h"
#include "logger.h"
#include "concurrent_queue.h"

namespace logger {

	Mode gLoggingMode = Mode_Deferred_Thread_Safe;
	// TODO: pass in buffer space, currently this mallocs its own
	global ConcurrentQueue gMessageQueue(sizeof(LogMessage), QUAGMIRE_LOGGER_CAPACITY);
	int gPopArrayLen = 0;
	LogMessage gPopArray[QUAGMIRE_LOGGER_CAPACITY] = {};


	void write(const LogMessage& m)
	{
		SDL_LogPriority sdlPriority = (SDL_LogPriority)(m.priority == 0 ? 0 : SDL_NUM_LOG_PRIORITIES - m.priority);
		SDL_LogMessage(m.category, sdlPriority, "%s", m.message);
	}

	/**
	 * Empties the thread-safe queue and writes all messages. Call this at least once per frame
	 * while in Deferred_Thread_Safe mode. Also may want to call before exiting to flush messages
	 * logged after the main loop has exited.
	 */
	void flush()
	{
		gPopArrayLen = gMessageQueue.try_pop_all(gPopArray);
		for (int i = 0; i < gPopArrayLen; ++i) {
			write(gPopArray[i]);
		}
		gPopArrayLen = 0;
	}

	/**
	 * Setting the mode will also flush the queue. This call is not thread safe, call this only
	 * from the same thread that normally calls flush.
	 */
	void set_mode(Mode mode) {
		flush();
		gLoggingMode = mode;
	}


	// Logger Functions

	void Logger::setPriority(Category c, Priority p)
	{
		categoryDefaultPriority[c] = p;
	}

	void Logger::setAllPriorities(Priority p)
	{
		memset(categoryDefaultPriority, p, sizeof(categoryDefaultPriority));
	}

	#define pass_args(f)	va_list args;\
							va_start(args, s);\
							f;\
							va_end(args);


	void Logger::critical(const char *s, ...) {
		pass_args(_log(defaultCategory, Priority_Critical, s, args));
	}

	void Logger::error(const char *s, ...) {
		pass_args(_log(defaultCategory, Priority_Error, s, args));
	}

	void Logger::warn(const char *s, ...) {
		pass_args(_log(defaultCategory, Priority_Warn, s, args));
	}

	void Logger::info(const char *s, ...) {
		pass_args(_log(defaultCategory, Priority_Info, s, args));
	}

	#if defined(QUAGMIRE_DEBUG_LOG) && QUAGMIRE_DEBUG_LOG != 0
	void Logger::debug(const char *s, ...) {
		pass_args(_log(defaultCategory, Priority_Debug, s, args));
	}
	#else
	void Logger::debug(const char *s, ...) {}
	#endif

	void Logger::verbose(const char *s, ...) {
		pass_args(_log(defaultCategory, Priority_Verbose, s, args));
	}

	void Logger::critical(Category c, const char *s, ...) {
		pass_args(_log(c, Priority_Critical, s, args));
	}

	void Logger::error(Category c, const char *s, ...) {
		pass_args(_log(c, Priority_Error, s, args));
	}

	void Logger::warn(Category c, const char *s, ...) {
		pass_args(_log(c, Priority_Warn, s, args));
	}

	void Logger::info(Category c, const char *s, ...) {
		pass_args(_log(c, Priority_Info, s, args));
	}

	#if defined(QUAGMIRE_DEBUG_LOG) && QUAGMIRE_DEBUG_LOG != 0
	void Logger::debug(Category c, const char *s, ...) {
		pass_args(_log(c, Priority_Debug, s, args));
	}
	#else
	void Logger::debug(Category c, const char *s, ...) {}
	#endif

	void Logger::verbose(Category c, const char *s, ...) {
		pass_args(_log(c, Priority_Verbose, s, args));
	}

	void Logger::test(const char *s, ...) {
		pass_args(_log(Category_Test, categoryDefaultPriority[Category_Test], s, args));
	}

	void Logger::log(Category c, Priority p, const char *s, ...) {
		pass_args(_log(c, p, s, args));
	}

	void Logger::log(Category c, const char *s, ...) {
		pass_args(_log(c, categoryDefaultPriority[c], s, args));
	}

	void Logger::log(const char *s, ...) {
		pass_args(_log(defaultCategory, categoryDefaultPriority[defaultCategory], s, args));
	}

	void Logger::_log(Category c, Priority p, const char *s, va_list args)
	{
		if (logPriority >= p) {
			//int len = _vscprintf(s, args);
			// write formatted string
			vsnprintf_s(&str[0], len+1, len, s, args);
			if (gLoggingMode == Mode_Deferred_Thread_Safe) {
				gMessageQueue.push(LogMessage{ c, p, std::move(str) });
			}
			else {
				write({ c, p, std::move(str) });
			}
		}
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