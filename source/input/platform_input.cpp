#include "platform_input.h"
//#include <codecvt>

namespace input {

	bool handleMessage(PlatformInput& input, const SDL_Event& event, i64 timestamp)
	{
		bool handled = false;
		
		switch (event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				if (event.key.repeat == 0) {
					/*logger.verbose(Logger::Category_Input,
								"key event=%d: state=%d: key=%d: repeat=%d: realTime=%lu: name=%s\n",
								event.type, event.key.state, event.key.keysym.sym, event.key.repeat, timestamp, SDL_GetKeyName(event.key.keysym.sym));*/

					InputEvent evt = { timestamp, event, Event_Keyboard, {} };
					input.eventsQueue.push((void*)&evt);
				}
				handled = true;
				break;
			}

			case SDL_TEXTEDITING: {
				/*logger.verbose(Logger::Category_Input,
							"key event=%d: text=%s: length=%d: start=%d: windowID=%d: realTime=%lu\n",
							event.type, event.edit.text, event.edit.length, event.edit.start, event.edit.windowID, timestamp);*/

				InputEvent evt = { timestamp, event, Event_TextInput, {} };
				input.eventsQueue.push((void*)&evt);

				handled = true;
				break;
			}
			case SDL_TEXTINPUT: {
				/*logger.verbose(Logger::Category_Input,
							"key event=%d: text=%s: windowID=%d: realTime=%lu\n",
							event.type, event.text.text, event.text.windowID, timestamp);*/

				InputEvent evt = { timestamp, event, Event_TextInput, {} };
				input.eventsQueue.push((void*)&evt);

				handled = true;
				break;
			}

			case SDL_MOUSEMOTION: {
				/*logger.verbose(Logger::Category_Input,
							"mouse motion event=%d: which=%d: state=%d: window=%d: x,y=%d,%d: xrel,yrel=%d,%d: realTime=%lu\n",
							event.type, event.motion.which, event.motion.state, event.motion.windowID,
							event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, timestamp);*/

				InputEvent evt = { timestamp, event, Event_Mouse, {} };
				input.motionEventsQueue.push((void*)&evt);
				handled = true;
				break;
			}
			case SDL_JOYAXISMOTION:
				/*logger.verbose(Logger::Category_Input,
							"joystick motion event=%d: which=%d: axis=%d: value=%d: realTime=%lu\n",
							event.type, event.jaxis.which, event.jaxis.axis, event.jaxis.value, timestamp);*/
			case SDL_JOYBALLMOTION:
			case SDL_JOYHATMOTION: {
				InputEvent evt = { timestamp, event, Event_Joystick, {} };
				input.motionEventsQueue.push((void*)&evt);
				handled = true;
				break;
			}

			case SDL_MOUSEWHEEL: {
//				logger.verbose(Logger::Category_Input,
//							"mouse wheel event=%d: which=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
//							event.type, event.wheel.which, event.wheel.windowID,
//							event.wheel.x, event.wheel.y, timestamp);

				InputEvent evt = { timestamp, event, Event_Mouse, {} };
				input.eventsQueue.push((void*)&evt);
				handled = true;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
//				logger.verbose(Logger::Category_Input,
//							"mouse button event=%d: which=%d: button=%d: state=%d: clicks=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
//							event.type, event.button.which, event.button.button, event.button.state,
//							event.button.clicks, event.button.windowID, event.button.x, event.button.y, timestamp);

				InputEvent evt = { timestamp, event, Event_Mouse, {} };
				input.eventsQueue.push((void*)&evt);
				handled = true;
				break;
			}

			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED: {

				handled = true;
				break;
			}
			
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP: {
//				logger.verbose(Logger::Category_Input,
//							"joystick button event=%d: which=%d: button=%d: state=%d: realTime=%lu\n",
//							event.type, event.jbutton.which, event.jbutton.button, event.jbutton.state, timestamp);
				
				InputEvent evt = { timestamp, event, Event_Joystick, {} };
				input.eventsQueue.push((void*)&evt);
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

}