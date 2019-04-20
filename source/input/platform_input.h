#ifndef _PLATFORM_INPUT_H
#define _PLATFORM_INPUT_H

#include <SDL_events.h>
#include "utility/concurrent_queue.h"

namespace input {

	enum InputEventType : u8 {
		Event_Keyboard = 0,
		Event_Mouse,
		Event_Joystick,
		Event_TextInput
	};

	struct InputEvent {
		i64				timeStampCounts;
		SDL_Event		evt;
		InputEventType	eventType;
		u8				_padding[7];
	};

	/**
	 * platform input events are pushed on the input thread and popped on the game update thread
	 */
	struct PlatformInput {
		ConcurrentQueue		eventsQueue;
		ConcurrentQueue		motionEventsQueue;
		DenseQueue			popEvents;
		DenseQueue			popMotionEvents;
	};

}

#endif