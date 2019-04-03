#define PROGRAM_NAME "Project Quagmire"

#include <cstdlib>
#include <cmath>
#include <atomic>
#include <SDL.h>
#include <SDL_syswm.h>
#include <GL/glew.h>
#include "utility/types.h"
#include "utility/nstring.h"
#include "utility/handle_map.h"
#include "utility/dense_queue.h"
#include "utility/concurrent_queue.h"
#include "platform/platform.h"
#include "platform/timer.h"
#include "platform/fixed_timestep.h"
#include "engine.h"
#include "game.h"
