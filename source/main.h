#include <cstdlib>
#include <cmath>
#include <cstdarg>
#include <atomic>
#include <SDL.h>
#include <SDL_syswm.h>
#include <GL/glew.h>
#include "utility/common.h"
#include "utility/nstring.h"
#include "utility/handle_map.h"
#include "utility/dense_queue.h"
#include "utility/concurrent_queue.h"
#include "utility/logger.h"
#include "platform/platform.h"
#include "platform/timer.h"
#include "input/platform_input.h"
#include "game.h"
