#ifndef _PLATFORM_INPUT_H
#define _PLATFORM_INPUT_H

#include <SDL_events.h>
#include "../utility/concurrent_queue.h"

namespace input {

	enum InputEventType : u8 {
		Event_Keyboard = 0,
		Event_Mouse,
		Event_Joystick,
		Event_TextInput
	};

	struct InputEvent {
		i64						timeStampCounts;
		union {
			SDL_Event			evt;
			struct {
				u8				_padding[55];
				InputEventType	eventType;	// the last byte of SDL_Event is just padding so we use it for our own flag
			};
		};
	};
	static_assert(sizeof(SDL_Event) == 56 && sizeof(InputEvent) == 64, "SDL_Event size is not 56, check the InputEvent union");

	ConcurrentQueueTyped(InputEvent, ConcurrentQueue_InputEvent);
	DenseQueueTyped(InputEvent, DenseQueue_InputEvent);

	/**
	 * platform input events are pushed on the input thread and popped on the game update thread
	 */
	struct alignas(64) PlatformInput {
		ConcurrentQueue_InputEvent	eventsQueue;
		ConcurrentQueue_InputEvent	motionEventsQueue;
		DenseQueue_InputEvent		popEvents;
		DenseQueue_InputEvent		popMotionEvents;
	};

}

#endif