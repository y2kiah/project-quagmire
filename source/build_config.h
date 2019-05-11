#define QUAGMIRE_DEVELOPMENT	1	// set 1 to enable development tools and hot reloading in the build
#define QUAGMIRE_SLOWCHECKS		1	// set 1 to run slow code like asserts and other dev-time tasks
#define QUAGMIRE_LOG_ASSERTS	0	// set 1 to log failed asserts rather than hard stop when SLOWCHECKS is enabled, could be useful during play testing if you prefer not to crash
#define QUAGMIRE_MEMPROFILE		0	// set 1 to enable memory profiling
#define QUAGMIRE_DEBUG_LOG		1	// set 1 to enable debug level logging TODO: is this necessary?
#define QUAGMIRE_ALLOW_MALLOC   0   // set 1 to allow calls to Q_malloc for ease of development, 0 to assert for production readiness 

#define GL_GLEXT_PROTOTYPES
#define GLEW_STATIC
#define GLEW_NO_GLU
#define NANOVG_GL3_IMPLEMENTATION // TODO: remove if we don't use NanoVG

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// TODO: ??
#define _VERTEX_
#define _TESS_CONTROL_
#define _TESS_EVAL_
#define _GEOMETRY_
#define _FRAGMENT_

#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
#define SDL_ASSERT_LEVEL 3
#else
#define SDL_ASSERT_LEVEL 1
#endif