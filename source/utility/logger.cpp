#include "logger.h"

namespace logger {
	
	// on platform side _log is pointed to the log function defined in platform_logger
	// on game side _log is set to the pointer passed in the platform api
	LogFunc* _log = nullptr;

	#define pass_args(f)	va_list args;\
							va_start(args, s);\
							f;\
							va_end(args);


	void critical(const char* s, ...) {
		pass_args(_log(_Category_Default, Priority_Critical, s, args));
	}

	void error(const char* s, ...) {
		pass_args(_log(_Category_Default, Priority_Error, s, args));
	}

	void warn(const char* s, ...) {
		pass_args(_log(_Category_Default, Priority_Warn, s, args));
	}

	void info(const char* s, ...) {
		pass_args(_log(_Category_Default, Priority_Info, s, args));
	}

	#if defined(QUAGMIRE_DEBUG_LOG) && QUAGMIRE_DEBUG_LOG != 0
	void debug(const char* s, ...) {
		pass_args(_log(_Category_Default, Priority_Debug, s, args));
	}
	#else
	void debug(const char* s, ...) {}
	#endif

	void verbose(const char* s, ...) {
		pass_args(_log(_Category_Default, Priority_Verbose, s, args));
	}

	void critical(Category c, const char* s, ...) {
		pass_args(_log(c, Priority_Critical, s, args));
	}

	void error(Category c, const char* s, ...) {
		pass_args(_log(c, Priority_Error, s, args));
	}

	void warn(Category c, const char* s, ...) {
		pass_args(_log(c, Priority_Warn, s, args));
	}

	void info(Category c, const char* s, ...) {
		pass_args(_log(c, Priority_Info, s, args));
	}

	#if defined(QUAGMIRE_DEBUG_LOG) && QUAGMIRE_DEBUG_LOG != 0
	void debug(Category c, const char* s, ...) {
		pass_args(_log(c, Priority_Debug, s, args));
	}
	#else
	void debug(Category c, const char* s, ...) {}
	#endif

	void verbose(Category c, const char* s, ...) {
		pass_args(_log(c, Priority_Verbose, s, args));
	}

	void test(const char* s, ...) {
		pass_args(_log(Category_Test, _Priority_Default, s, args));
	}

	void out(Category c, Priority p, const char* s, ...) {
		pass_args(_log(c, p, s, args));
	}

	void out(Category c, const char* s, ...) {
		pass_args(_log(c, _Priority_Default, s, args));
	}

	void out(const char* s, ...) {
		pass_args(_log(_Category_Default, _Priority_Default, s, args));
	}

}