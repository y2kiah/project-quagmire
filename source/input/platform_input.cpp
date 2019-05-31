#include "platform_input.h"
#include "../utility/logger.h"

namespace input {

	bool handleMessage(const SDL_Event& event, i64 timestamp)
	{
		PlatformInput& input = gameContext.input;
		bool handled = false;
		
		InputEvent evt{};
		evt.timeStampCounts = timestamp;
		evt.evt = event;

		switch (event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				if (event.key.repeat == 0) {
					//logger::verbose(logger::Category_Input,
					//			"key event=%d: state=%d: key=%d: repeat=%d: realTime=%lu: name=%s\n",
					//			event.type, event.key.state, event.key.keysym.sym, event.key.repeat, timestamp, SDL_GetKeyName(event.key.keysym.sym));

					evt.eventType = Event_Keyboard;
					auto pushed = input.eventsQueue.push(&evt);
					if (!pushed) {
						logger::info(logger::Category_Input, "missed input, eventsQueue is full");
					}
				}
				handled = true;
				break;
			}

			case SDL_TEXTEDITING: {
				/*logger::verbose(logger::Category_Input,
							"key event=%d: text=%s: length=%d: start=%d: windowID=%d: realTime=%lu\n",
							event.type, event.edit.text, event.edit.length, event.edit.start, event.edit.windowID, timestamp);*/

				evt.eventType = Event_TextInput;
				auto pushed = input.eventsQueue.push(&evt);
				if (!pushed) {
					logger::info(logger::Category_Input, "missed input, eventsQueue is full");
				}

				handled = true;
				break;
			}
			case SDL_TEXTINPUT: {
				/*logger::verbose(logger::Category_Input,
							"key event=%d: text=%s: windowID=%d: realTime=%lu\n",
							event.type, event.text.text, event.text.windowID, timestamp);*/

				evt.eventType = Event_TextInput;
				auto pushed = input.eventsQueue.push(&evt);
				if (!pushed) {
					logger::info(logger::Category_Input, "missed input, eventsQueue is full");
				}

				handled = true;
				break;
			}

			case SDL_MOUSEMOTION: {
				//logger::verbose(logger::Category_Input,
				//			"mouse motion event=%d: which=%d: state=%d: window=%d: x,y=%d,%d: xrel,yrel=%d,%d: realTime=%lu\n",
				//			event.type, event.motion.which, event.motion.state, event.motion.windowID,
				//			event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, timestamp);

				evt.eventType = Event_Mouse;
				auto pushed = input.motionEventsQueue.push(&evt);
				handled = true;
				if (!pushed) {
					logger::info(logger::Category_Input, "missed input, motionEventsQueue is full");
				}
				break;
			}
			case SDL_JOYAXISMOTION:
				/*logger::verbose(logger::Category_Input,
							"joystick motion event=%d: which=%d: axis=%d: value=%d: realTime=%lu\n",
							event.type, event.jaxis.which, event.jaxis.axis, event.jaxis.value, timestamp);*/
			case SDL_JOYBALLMOTION:
			case SDL_JOYHATMOTION: {
				evt.eventType = Event_Joystick;
				auto pushed = input.motionEventsQueue.push(&evt);
				handled = true;
				if (!pushed) {
					logger::info(logger::Category_Input, "missed input, motionEventsQueue is full");
				}
				break;
			}

			case SDL_MOUSEWHEEL: {
//				logger::verbose(logger::Category_Input,
//							"mouse wheel event=%d: which=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
//							event.type, event.wheel.which, event.wheel.windowID,
//							event.wheel.x, event.wheel.y, timestamp);

				evt.eventType = Event_Mouse;
				input.eventsQueue.push(&evt);
				handled = true;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
//				logger::verbose(logger::Category_Input,
//							"mouse button event=%d: which=%d: button=%d: state=%d: clicks=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
//							event.type, event.button.which, event.button.button, event.button.state,
//							event.button.clicks, event.button.windowID, event.button.x, event.button.y, timestamp);

				evt.eventType = Event_Mouse;
				auto pushed = input.eventsQueue.push(&evt);
				if (!pushed) {
					logger::info(logger::Category_Input, "missed input, eventsQueue is full");
				}
				handled = true;
				break;
			}

			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED: {
				i32 instance_id = event.jdevice.which;
				handled = true;
				break;
			}
			
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP: {
//				logger::verbose(logger::Category_Input,
//							"joystick button event=%d: which=%d: button=%d: state=%d: realTime=%lu\n",
//							event.type, event.jbutton.which, event.jbutton.button, event.jbutton.state, timestamp);
				
				evt.eventType = Event_Joystick;
				auto pushed = input.eventsQueue.push(&evt);
				if (!pushed) {
					logger::info(logger::Category_Input, "missed input, eventsQueue is full");
				}
				handled = true;
				break;
			}

			case SDL_FINGERMOTION:
			case SDL_FINGERDOWN:
			case SDL_FINGERUP: {
				handled = true;
				break;
			}
		}
		return handled;
	}


	bool initPlatformInput() {
		// Initialize the mouse cursors table
		app.cursors[Cursor_Arrow]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		app.cursors[Cursor_Hand]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		app.cursors[Cursor_Wait]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
		app.cursors[Cursor_IBeam]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
		app.cursors[Cursor_Crosshair] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
		
		// Get number of input devices
		int numJoysticks = min(SDL_NumJoysticks(), GAMEINPUT_MAX_JOYSTICKS);
		app.joystickInfo.numJoysticks = numJoysticks;
		
		// Initialize mouse motion
		int numPointingDevices = 1; //SDL_GetNumInputDevices();
		
		//app.joystickInfo.mouseXMotion = 
		//		AxisMotion& m = frameMappedInput.motion[motionAxis];
		//		app.joystickInfo.
		//		m.device = 0;
		//		m.deviceName = "mouse";//SDL_GetInputDeviceName();
		//		m.axis = axis;
		//		++motionAxis;
		
		// Initialize the joysticks
		//int motionAxis = 0;
		for (int j = 0; j < numJoysticks; ++j) {
			// Open joystick
			SDL_Joystick* joy = SDL_JoystickOpen(j);
			
			if (joy != nullptr) {
				app.joystickInfo.joysticks[j] = joy;
				int numAxes = SDL_JoystickNumAxes(joy);
				app.joystickInfo.totalAxes += numAxes;

				// for each axis, create an AxisMotion struct
				/*for (int axis = 0; axis < numAxes; ++axis) {
					AxisMotion& m = frameMappedInput.motion[motionAxis];
					m.device = SDL_JoystickInstanceID(joy);
					m.deviceName = SDL_JoystickName(joy);
					m.axis = (u8)axis;
					
					++motionAxis;
					if (motionAxis == GAMEINPUT_MAX_AXES) {
						break;
					}
				}*/
				
				char guid[33] = {};
				SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof(guid));

				logger::debug(logger::Category_Input, "Opened Joystick %d", j);
				logger::debug(logger::Category_Input, "  Name: %s", SDL_JoystickNameForIndex(j));
				logger::debug(logger::Category_Input, "  Number of Axes: %d", SDL_JoystickNumAxes(joy));
				logger::debug(logger::Category_Input, "  Number of Buttons: %d", SDL_JoystickNumButtons(joy));
				logger::debug(logger::Category_Input, "  Number of Hats: %d", SDL_JoystickNumHats(joy));
				logger::debug(logger::Category_Input, "  Number of Balls: %d", SDL_JoystickNumBalls(joy));
				logger::debug(logger::Category_Input, "  Instance ID: %d", SDL_JoystickInstanceID(joy));
				logger::debug(logger::Category_Input, "  GUID: %s", guid);
				
			}
			else {
				logger::warn(logger::Category_Input, "Couldn't open Joystick %d", j);
			}
		}
		// The device_index passed as an argument refers to the N'th joystick presently recognized by SDL on the system.
		// It is NOT the same as the instance ID used to identify the joystick in future events.
		// See SDL_JoystickInstanceID() for more details about instance IDs.
		
		return true;
	}


	void deinitPlatformInput()
	{
		// close all joysticks
		for (u32 j = 0; j < app.joystickInfo.numJoysticks; ++j) {
			SDL_Joystick* joy = app.joystickInfo.joysticks[j];
			SDL_JoystickClose(joy);
			app.joystickInfo.joysticks[j] = nullptr;
		}

		// free all cursors
		for (u32 c = 0; c < _InputMouseCursorCount; ++c) {
			SDL_FreeCursor(app.cursors[c]);
			app.cursors[c] = nullptr;
		}
	}
}