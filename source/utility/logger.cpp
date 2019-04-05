namespace logger {

	Mode gLoggingMode = Mode_Deferred_Thread_Safe;
	// TODO: pass in buffer space, currently this mallocs its own
	global ConcurrentQueue gMessageQueue(sizeof(LogMessage), QUAGMIRE_LOGGER_CAPACITY);


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
		//gMessageQueue.try_pop_all(m_popArray);
		//for (auto& li : m_popArray) {
		//	write(li);
		//}
		//m_popArray.clear();
	}

	/**
	 * Setting the mode will also flush the queue. This call is not thread safe, call this only
	 * from the same thread that normally calls flush.
	 */
	void set_mode(Mode mode) {
		flush();
		gLoggingMode = mode;
	}

	void log(Category c, Priority p, const char *s, va_list args) {
		// write formatted string
		//int len = _vscprintf(s, args);
		/*std::string str(len, 0);
		vsnprintf_s(&str[0], len+1, len, s, args);
		*/
		if (m_priority[c] >= p) {
			if (m_mode == Mode_Deferred_Thread_Safe) {
				m_q.push({ c, p, std::move(str) });
			}
			else {
				write({ c, p, std::move(str) });
			}
		}
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
		pass_args(log(defaultCategory, Priority_Critical, s, args));
	}

	void Logger::error(const char *s, ...) {
		pass_args(log(defaultCategory, Priority_Error, s, args));
	}

	void Logger::warn(const char *s, ...) {
		pass_args(log(defaultCategory, Priority_Warn, s, args));
	}

	void Logger::info(const char *s, ...) {
		pass_args(log(defaultCategory, Priority_Info, s, args));
	}

	#if defined(QUAGMIRE_DEBUG_LOG) && QUAGMIRE_DEBUG_LOG != 0
	void Logger::debug(const char *s, ...) {
		pass_args(log(defaultCategory, Priority_Debug, s, args));
	}
	#else
	void Logger::debug(const char *s, ...) {}
	#endif

	void Logger::verbose(const char *s, ...) {
		pass_args(log(defaultCategory, Priority_Verbose, s, args));
	}

	void Logger::critical(Category c, const char *s, ...) {
		pass_args(log(c, Priority_Critical, s, args));
	}

	void Logger::error(Category c, const char *s, ...) {
		pass_args(log(c, Priority_Error, s, args));
	}

	void Logger::warn(Category c, const char *s, ...) {
		pass_args(log(c, Priority_Warn, s, args));
	}

	void Logger::info(Category c, const char *s, ...) {
		pass_args(log(c, Priority_Info, s, args));
	}

	#if defined(QUAGMIRE_DEBUG_LOG) && QUAGMIRE_DEBUG_LOG != 0
	void Logger::debug(Category c, const char *s, ...) {
		pass_args(log(c, Priority_Debug, s, args));
	}
	#else
	void Logger::debug(Category c, const char *s, ...) {}
	#endif

	void Logger::verbose(Category c, const char *s, ...) {
		pass_args(log(c, Priority_Verbose, s, args));
	}

	void Logger::test(const char *s, ...) {
		pass_args(log(Category_Test, categoryDefaultPriority[Category_Test], s, args));
	}

	void Logger::log(Category c, Priority p, const char *s, ...) {
		pass_args(log(c, p, s, args));
	}

	void Logger::log(Category c, const char *s, ...) {
		pass_args(log(c, categoryDefaultPriority[c], s, args));
	}

	void Logger::log(const char *s, ...) {
		pass_args(log(defaultCategory, categoryDefaultPriority[defaultCategory], s, args));
	}

}