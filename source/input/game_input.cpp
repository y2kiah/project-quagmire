#include "game_input.h"


#define JOYSTICK_INVERSE_MAX_RAW	1.0f / 32768.0f

namespace input {

	void GameInput::updateFrameTick(const UpdateInfo& ui, PlatformInput& platformInput)
	{
		platformInput.eventsQueue.try_pop_all_push(platformInput.popEvents._q);
		platformInput.motionEventsQueue.try_pop_all_push(platformInput.popMotionEvents._q);
// TEMP instead of processing
platformInput.popEvents.clear();
platformInput.popMotionEvents.clear();

		// clear previous frame actions and axis mappings
		frameMappedInput.actionsSize = 0;
		frameMappedInput.axesSize = 0;

		// clear handled flag of frame's text input, the input string itself may persist many frames
		frameMappedInput.textInputHandled = false;

		// remove active states that are no longer mappings in any active context
/*		frameMappedInput.states.erase(
			std::remove_if(frameMappedInput.states.begin(), frameMappedInput.states.end(),
				[&](const MappedState& state) {
					for (const auto& ac : activeInputContexts) {
						if (!ac.active) { continue; }
						auto& context = inputContexts[ac.contextId];
				
						// Apply this filter to states where keypress down is active. Don't apply it to toggles
						// or bindings where the key being unpressed is active. Also don't filter states where
						// the mappingId is found in the active context.
						bool hasDownUpBinding = (state.inputMapping->bindIn == Bind_Down && state.inputMapping->bindOut == Bind_Up);
						if (!hasDownUpBinding ||
							std::find(context.inputMappings.begin(), context.inputMappings.end(), state.mappingId) != context.inputMappings.end())
						{
							return false;
						}
					}
					return true;
				}),
			frameMappedInput.states.end());

		// loop over active states, add to totalCounts, totalMS and totalFrames, reset handled flag
		for (auto& state : frameMappedInput.states) {
			state.totalFrames += 1;
			state.totalCounts += ui.deltaCounts;
			state.totalMs += ui.deltaMs;
			state.handled = false;
		}

		// map inputs using active contexts
		mapFrameInputs(ui);
		mapFrameMotion(ui);

		// TEMP output mapped inputs
		// for (const auto& s : frameMappedInput.states) {
		// 	logger.debug(Logger::Category_Input, "state \"%s\" active", s.inputMapping->name);
		// }
		// for (const auto& a : frameMappedInput.actions) {
		// 	logger.debug(Logger::Category_Input, "action \"%s\" triggered", a.inputMapping->name);
		// }

		// invoke all callbacks in priority order, passing this frame's mapped input
		for (auto& cp : callbackPriorityList) {
			auto& cb = callbacks[cp.callbackId];
			cb(frameMappedInput);
		}
*/
	}


/*	void GameInput::mapFrameInputs(const UpdateInfo& ui)
	{
		// process this frame's input events
		for (auto& evt : popEvents) {							// for each input event
			// if timestamp is greater than frame time, we're done
			if (evt.timeStampCounts > ui.virtualTime) {
				break;
			}
			
			bool matched = false;

			for (const auto& ac : activeInputContexts) {		// for each active context
				if (!ac.active) { continue; }

				auto& context = inputContexts[ac.contextId];
				
				// handle key and button events
				if (evt.eventType == Event_Keyboard ||
					evt.eventType == Event_Joystick ||
					evt.eventType == Event_Mouse)
				{
					for (Id_t mappingId : context.inputMappings) {	// check all input mappings for a match
						const auto& mapping = inputMappings[mappingId];

						// check for action mappings
						if (mapping.type == Type_Action) {
							MappedAction ma{};

							if ((mapping.bindIn == Bind_Down && evt.evt.type == SDL_KEYDOWN) ||
								(mapping.bindIn == Bind_Up   && evt.evt.type == SDL_KEYUP))
							{
								u16 mod = evt.evt.key.keysym.mod & ~(KMOD_NUM | KMOD_CAPS | KMOD_MODE); // ignore these mods
								matched = (evt.evt.key.keysym.sym == mapping.keycode &&
										mod == mapping.modifier);
							}
							else if ((mapping.bindIn == Bind_Down && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
									(mapping.bindIn == Bind_Up   && evt.evt.type == SDL_MOUSEBUTTONUP))
							{
								matched = (evt.evt.button.button == mapping.keycode &&
										evt.evt.button.clicks == mapping.clicks);
								if (matched) {
									ma.xRaw = evt.evt.button.x;
									ma.yRaw = evt.evt.button.y;
								}
							}
							else if ((mapping.bindIn == Bind_Down && evt.evt.type == SDL_JOYBUTTONDOWN) ||
									(mapping.bindIn == Bind_Up   && evt.evt.type == SDL_JOYBUTTONUP))
							{
								matched = (evt.evt.jbutton.which == mapping.device &&
										evt.evt.jbutton.button == mapping.keycode);
							}
							else if (evt.evt.type == SDL_MOUSEWHEEL)
							{
								matched = mapping.mouseWheel == 1 &&
										((mapping.axis == 0 && mapping.bindIn == Bind_Up   && evt.evt.wheel.x > 0) ||
										(mapping.axis == 0 && mapping.bindIn == Bind_Down && evt.evt.wheel.x < 0) ||
										(mapping.axis == 1 && mapping.bindIn == Bind_Up   && evt.evt.wheel.y > 0) ||
										(mapping.axis == 1 && mapping.bindIn == Bind_Down && evt.evt.wheel.y < 0));
							}

							// found a matching action mapping for the event
							if (matched) {
								ma.mappingId = mappingId;
								ma.inputMapping = &mapping;
								
								frameMappedInput.actions.push_back(std::move(ma));
								break; // skip checking the rest of the mappings
							}
						}

						// check for state mappings
						else if (mapping.type == Type_State) {
							auto stateIndex = findActiveState(mappingId);
							bool stateActive = (stateIndex != -1);

							if (!stateActive) {
								if ((mapping.bindIn == Bind_Down && evt.evt.type == SDL_KEYDOWN) ||
									(mapping.bindIn == Bind_Up   && evt.evt.type == SDL_KEYUP) &&
									(mapping.bindIn != mapping.bindOut || !evt.evt.key.repeat)) // prevent repeat key events from changing toggle states
								{
									matched = (evt.evt.key.keysym.sym == mapping.keycode);
												//&& evt.evt.key.keysym.mod == mapping.modifier);
									// TODO: states cannot use modifiers currently, make sure this is ok
									// might want to support modifiers but check whether modifier "matters" or not - if there is a matching key mapping
									// with the modifier, then it matters, otherwise it doesn't
								}
								else if ((mapping.bindIn == Bind_Down && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
										(mapping.bindIn == Bind_Up   && evt.evt.type == SDL_MOUSEBUTTONUP))
								{
									matched = (evt.evt.button.button == mapping.keycode);
								}
								else if ((mapping.bindIn == Bind_Down && evt.evt.type == SDL_JOYBUTTONDOWN) ||
										(mapping.bindIn == Bind_Up   && evt.evt.type == SDL_JOYBUTTONUP))
								{
									matched = (evt.evt.jbutton.which == mapping.device &&
											evt.evt.jbutton.button == mapping.keycode);
								}
							}
							else { // stateActive
								if ((mapping.bindOut == Bind_Down && evt.evt.type == SDL_KEYDOWN) ||
									(mapping.bindOut == Bind_Up   && evt.evt.type == SDL_KEYUP))
								{
									matched = (evt.evt.key.keysym.sym == mapping.keycode);
												//&& evt.evt.key.keysym.mod == mapping.modifier);
									// TODO: maybe need to look for sym and mod keys separately here and make state inactive for either one
									// if the modifier OR the key is lifted in either order, the state should be deactivated
								}
								else if ((mapping.bindOut == Bind_Down && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
										(mapping.bindOut == Bind_Up   && evt.evt.type == SDL_MOUSEBUTTONUP))
								{
									matched = (evt.evt.button.button == mapping.keycode);
								}
								else if ((mapping.bindOut == Bind_Down && evt.evt.type == SDL_JOYBUTTONDOWN) ||
										(mapping.bindOut == Bind_Up   && evt.evt.type == SDL_JOYBUTTONUP))
								{
									matched = (evt.evt.jbutton.which == mapping.device &&
											evt.evt.jbutton.button == mapping.keycode);
								}
							}

							// found a new active state mapping with this event
							if (matched && !stateActive) {
								MappedState ms{};
								ms.mappingId = mappingId;
								ms.inputMapping = &mapping;
								ms.startCounts = ui.gameTime;
								ms.startFrame = ui.frame;

								frameMappedInput.states.push_back(std::move(ms));
								break; // skip checking the rest of the mappings
							}
							// make state inactive
							else if (matched && stateActive) {
								// make inactive, remove from states list by swap with last and pop_back
								if (stateIndex != frameMappedInput.states.size() - 1) {
									std::swap(frameMappedInput.states.at(stateIndex), frameMappedInput.states.back());
								}
								frameMappedInput.states.pop_back();
								break;
							}
						}
					}

					// found a mapping, move on to the next input event
					if (matched) { break; }
				}
				// handle text input events
				else if (context.options[CaptureTextInput] && evt.eventType == Event_TextInput) {
					if (evt.evt.type == SDL_TEXTINPUT) {
						frameMappedInput.textInput += converter.from_bytes(evt.evt.text.text);
					}
					if (evt.evt.type == SDL_TEXTEDITING) {
						frameMappedInput.textComposition = converter.from_bytes(evt.evt.edit.text);
						frameMappedInput.cursorPos = evt.evt.edit.start;
						frameMappedInput.selectionLength = evt.evt.edit.length;
					}
					break;
				}

				// didn't find a match, check if this context eats input events or passes them down
				if (context.options[EatMouseEvents]    && evt.eventType == Event_Mouse)    { break; }
				if (context.options[EatJoystickEvents] && evt.eventType == Event_Joystick) { break; }
				if (context.options[EatKeyboardEvents] &&
					(evt.eventType == Event_Keyboard || evt.eventType == Event_TextInput)) { break; }
			}
		}
	}


	void GameInput::mapFrameMotion(const UpdateInfo& ui)
	{
		f32 inverseWindowWidth  = 1.0f / app->getPrimaryWindow().width;
		f32 inverseWindowHeight = 1.0f / app->getPrimaryWindow().height;
		bool relativeMode = relativeMouseModeActive();

		// reset previous frame's relative motion
		for (auto& motion : frameMappedInput.motion) {
			motion.relRaw = 0;
			motion.relMapped = 0.0f;
		}

		// process all motion events, these go into AxisMotion struct
		auto frameSize = popMotionEvents.size();
		for (int e = 0; e < frameSize; ++e) {
			const auto& motionEvt = popMotionEvents[e];
			
			// if timestamp is greater than frame time, we're done
			if (motionEvt.timeStampCounts > ui.virtualTime) {
				frameSize = e;
				break;
			}

			switch (motionEvt.evt.type) {
				case SDL_MOUSEMOTION: {
					auto& mouseX = frameMappedInput.motion[0];
					auto& mouseY = frameMappedInput.motion[1];
					
					mouseX.posRaw = motionEvt.evt.motion.x;
					mouseX.relRaw += motionEvt.evt.motion.xrel;
					mouseX.posMapped = mouseX.posRaw * inverseWindowWidth;
					mouseX.relMapped += motionEvt.evt.motion.xrel * inverseWindowWidth;

					mouseY.posRaw = motionEvt.evt.motion.y;
					mouseY.relRaw += motionEvt.evt.motion.yrel;
					mouseY.posMapped = mouseY.posRaw * inverseWindowHeight;
					mouseY.relMapped += motionEvt.evt.motion.yrel * inverseWindowHeight;
					break;
				}
				case SDL_JOYAXISMOTION: {
					auto it = std::find_if(frameMappedInput.motion.begin(), frameMappedInput.motion.end(),
						[&motionEvt](const AxisMotion& m){
							return (m.device == motionEvt.evt.jaxis.which &&
									m.axis == motionEvt.evt.jaxis.axis);
						});

					if (it != frameMappedInput.motion.end()) {
						auto& motion = *it;
						auto newRaw = motionEvt.evt.jaxis.value;
						motion.relRaw = newRaw - motion.posRaw;
						motion.posRaw = newRaw;
						
						motion.posMapped = motion.posRaw * JOYSTICK_INVERSE_MAX_RAW;
						motion.relMapped = motion.relRaw * JOYSTICK_INVERSE_MAX_RAW;
					}
					break;
				}
			}
		}

		// erase up to the last event for this frame
		popMotionEvents.erase(popMotionEvents.begin(), popMotionEvents.begin() + frameSize);

		// all AxisMotion is aggregated for frame, now map to active InputMappings
		for (auto& motion : frameMappedInput.motion) {		// for each axis
			bool matched = false;
			bool isMouse = (motion.device == 0);

			for (const auto& ac : activeInputContexts) {		// for each active context
				// skip contexts that aren't active
				if (!ac.active) { continue; }

				auto& context = inputContexts[ac.contextId];

				for (Id_t mappingId : context.inputMappings) {	// check all input mappings for a match
					const auto& mapping = inputMappings[mappingId];

					// check for axis mappings
					matched = (mapping.type == Type_Axis &&
							mapping.device == motion.device &&
							mapping.axis == motion.axis &&
							((mapping.relativeMotion == 1) == relativeMode) &&
							(motion.relRaw != 0 || !relativeMode));
					if (matched) {
						// found a matching axis mapping for the event, apply mapping parameters
						if (isMouse) {
							motion.relMapped *= mapping.sensitivity * (mapping.invert == 1 ? -1.0f : 1.0f);
						}
						else {
							// TODO: this new stuff needs testing
							auto getModifiedPosition = [](f32 posMapped, const InputMapping& mapping) -> f32 {
								// deadzone
								f32 deadzone = mapping.deadzone * 0.01f;
								f32 newPos = (posMapped - (posMapped >= 0 ? deadzone : -deadzone)) / (1.0f - deadzone);

								// "curved" relative is (curve(posMapped) - curve(posMapped-relMapped)), not curve(relMapped)
								//mapping.curve
								//mapping.curvature

								//mapping.saturationX
								//mapping.saturationY
								//mapping.slider

								// inversion
								newPos *= (mapping.invert == 1 ? -1.0f : 1.0f);
								
								return newPos;
							};
							
							// get new motion after curves and modifications
							// "curved" relative is (curve(posMapped) - curve(posMapped-relMapped)), not just curve(relMapped)
							f32 newPos = getModifiedPosition(motion.posMapped, mapping);
							motion.relMapped = newPos - getModifiedPosition(motion.posMapped - motion.relMapped, mapping);
							motion.posMapped = newPos;
						}
						
						MappedAxis ma{};
						ma.mappingId = mappingId;
						ma.inputMapping = &mapping;
						ma.axisMotion = &motion;

						frameMappedInput.axes.push_back(std::move(ma));
						break; // skip checking the rest of the mappings
					}
				}

				if (matched || // found a mapping, move on to the next input event
					(context.options[EatMouseEvents] && isMouse) || // didn't find a match, check if this context eats input events or passes them down
					(context.options[EatJoystickEvents] && !isMouse))
				{
					break;
				}
			}
		}
	}

	Id_t GameInput::registerCallback(i32 priority, InputCallbackFunc* func)
	{
		Id_t cbId = callbacks.insert(std::move(func));
		
		callbackPriorityList.push_back({ cbId, priority });
		
		std::stable_sort(callbackPriorityList.begin(), callbackPriorityList.end(),
			[](const CallbackPriority& a, const CallbackPriority& b){
				return (a.priority < b.priority);
			});
		
		return cbId;
	}

	bool GameInput::unregisterCallback(Id_t callbackId)
	{
		for (int c = 0; c < callbackPriorityList.size(); ++c) {
			if (callbackPriorityList[c].callbackId == callbackId) {
				callbackPriorityList.erase(callbackPriorityList.begin() + c);
				break;
			}
		}
		return (callbacks.erase(callbackId) == 1);
	}

	bool GameInput::handleInputAction(Id_t mappingId, FrameMappedInput& mappedInput,
									  InputActionFunc* callback)
	{
		InputMapping mapping = inputMappings[mappingId];
		assert(mapping.type == Type_Action && "wrong handler for mapping type");

		for (auto& mi : mappedInput.actions) {
			if (mi.mappingId == mappingId && !mi.handled) {
				auto& context = inputContexts[mi.inputMapping->contextId];
				mi.handled = callback(mi, context);
				return mi.handled;
			}
		}
		return false;
	}

	bool GameInput::handleInputState(Id_t mappingId, FrameMappedInput& mappedInput,
									 InputStateFunc* callback)
	{
		InputMapping mapping = inputMappings[mappingId];
		assert(mapping.type == Type_State && "wrong handler for mapping type");

		for (auto& mi : mappedInput.states) {
			if (mi.mappingId == mappingId && !mi.handled) {
				auto& context = inputContexts[mi.inputMapping->contextId];
				mi.handled = callback(mi, context);
				return mi.handled;
			}
		}
		return false;
	}

	bool GameInput::handleInputAxis(Id_t mappingId, FrameMappedInput& mappedInput,
									InputAxisFunc* callback)
	{
		InputMapping mapping = inputMappings[mappingId];
		assert(mapping.type == Type_Axis && "wrong handler for mapping type");

		for (auto& mi : mappedInput.axes) {
			if (mi.mappingId == mappingId && !mi.handled) {
				auto& context = inputContexts[mi.inputMapping->contextId];
				mi.handled = callback(mi, context);
				return mi.handled;
			}
		}
		return false;
	}

	bool GameInput::handleTextInput(Id_t contextId, FrameMappedInput& mappedInput,
									InputTextFunc* callback)
	{
		assert(inputContexts.isValid(contextId) && "invalid input context");

		// there must be some text input left to consume
		if (!mappedInput.textInputHandled && mappedInput.textInput.length() > 0) {
			// find if the context is active and captures text input
			for (auto& ac : activeInputContexts) {
				if (ac.active && ac.contextId == contextId) {
					auto& context = inputContexts[contextId];
					if (context.options[CaptureTextInput]) {
						mappedInput.textInputHandled = callback(mappedInput, context);
						mappedInput.textInputHandled;
					}
				}
			}
		}
		return false;
	}


	Id_t GameInput::getInputMappingHandle(const char* name, Id_t contextId)
	{
		const auto& context = inputContexts[contextId];

		for (Id_t m : context.inputMappings) {
			const auto& mapping = inputMappings[m];
			if (strncmp(mapping.name, name, sizeof(mapping.name)) == 0) {
				return mapping.mappingId;
			}
		}
		return NullId_t;
	}


	int GameInput::findActiveState(Id_t mappingId)
	{
		for (size_t s = 0; s < frameMappedInput.states.size(); ++s) {
			if (frameMappedInput.states[s].mappingId.value == mappingId.value) {
				return static_cast<int>(s);
			}
		}
		return -1;
	}


	Id_t GameInput::createContext(u8 options, u8 priority, bool makeActive)
	{
		//auto f = tss_([optionsMask, priority](ThreadSafeState& tss_) {
		//	return tss_.inputContexts.emplace(optionsMask, priority);
		//});
		//return f;

		auto contextId = inputContexts.emplace(optionsMask, priority);
		activeInputContexts.push_back({ contextId, makeActive, priority, {} });
		std::stable_sort(activeInputContexts.begin(), activeInputContexts.end(),
						[](const ActiveInputContext& i, const ActiveInputContext& j) {
							return i.priority < j.priority;
						});

		return contextId;
	}


	bool GameInput::setContextActive(Id_t contextId, bool active, i8 priority)
	{
		if (inputContexts.isValid(contextId)) {
			for (auto& ac : activeInputContexts) {
				if (ac.contextId == contextId) {
					ac.active = active;
					if (priority >= 0) {
						ac.priority = (u8)priority;
					}
					break;
				}
			}

			if (priority >= 0) {
				// re-sort the contexts if the priority was set explicitly
				std::stable_sort(activeInputContexts.begin(), activeInputContexts.end(),
								[](const ActiveInputContext& i, const ActiveInputContext& j) {
					return i.priority < j.priority;
				});
			}

			return true;
		}
		return false;
	}


	Id_t GameInput::getInputContextHandle(const char* name)
	{
		for (const auto& i : inputContexts.getItems()) {
			if (strncmp(i.name, name, sizeof(i.name)) == 0) {
				return i.contextId;
			}
		}
		return NullId_t;
	}


	void GameInput::startTextInput()
	{
		// clear text input
		frameMappedInput.textInput[0] = '\0';
		frameMappedInput.textInputSize = 0;
		// TODO: what about re-entering a textbox and defaulting text input and cursor position to the existing values?
		// for example we want to be able to remove characters that were entered previously

		SDL_StartTextInput();
	}


	void GameInput::stopTextInput()
	{
		SDL_StopTextInput();
	}


	bool GameInput::textInputActive()
	{
		return (SDL_IsTextInputActive() == SDL_TRUE);
	}


	void GameInput::startRelativeMouseMode()
	{
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}


	void GameInput::stopRelativeMouseMode()
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}


	bool GameInput::relativeMouseModeActive()
	{
		return (SDL_GetRelativeMouseMode() == SDL_TRUE);
	}
*/

	void GameInput::init()
	{
		inputMappings.init(GAMEINPUT_MAPPINGS_CAPACITY, inputMappingsBuffer);
		assert(sizeof(inputMappingsBuffer) == HandleMap_InputMapping::getTotalBufferSize(GAMEINPUT_MAPPINGS_CAPACITY));
		
		inputContexts.init(GAMEINPUT_CONTEXTS_CAPACITY, inputContextsBuffer);
		assert(sizeof(inputContextsBuffer) == HandleMap_InputContext::getTotalBufferSize(GAMEINPUT_CONTEXTS_CAPACITY));

		callbacks.init(GAMEINPUT_CALLBACKS_CAPACITY, callbacksBuffer);
		assert(sizeof(callbacksBuffer) == HandleMap_InputCallback::getTotalBufferSize(GAMEINPUT_CALLBACKS_CAPACITY));

		// the data structure that holds all of the metadata queried here should use the reflection
		// system, and be made available to Lua script

		// Initialize the mouse cursors table
		cursors[Cursor_Arrow]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		cursors[Cursor_Hand]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		cursors[Cursor_Wait]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
		cursors[Cursor_IBeam]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
		cursors[Cursor_Crosshair] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);


		// Get number of input devices
		int numJoysticks = SDL_NumJoysticks();
		assert(numJoysticks <= GAMEINPUT_JOYSTICKS_CAPACITY && "not enough joysticks supported");
		joysticksSize = (u8)numJoysticks;
		int motionAxis = 0;

		// Initialize the mouse
		int numPointingDevices = 1; //SDL_GetNumInputDevices();
		// mouse is always index 0 and 1 (x and y) in AxisMotion array
		{ // this scope would be a loop in future SDL versions that support multiple mice
			int numAxes = 2;
			for (int axis = 0; axis < numAxes; ++axis) {
				AxisMotion& m = frameMappedInput.motion[motionAxis];
				m.device = 0;
				m.deviceName = "mouse";//SDL_GetInputDeviceName();
				m.axis = axis;
				++motionAxis;
			}
		}
		
		// Initialize the joysticks
		for (int j = 0; j < numJoysticks; ++j) {
			// Open joystick
			SDL_Joystick* joy = SDL_JoystickOpen(j);
			
			if (joy != nullptr) {
				joysticks[j] = joy;

				// for each axis, create an AxisMotion struct
				int numAxes = SDL_JoystickNumAxes(joy);
				for (int axis = 0; axis < numAxes; ++axis) {
					AxisMotion& m = frameMappedInput.motion[motionAxis];
					m.device = SDL_JoystickInstanceID(joy);
					m.deviceName = SDL_JoystickName(joy);
					m.axis = axis;
					
					++motionAxis;
					if (motionAxis == GAMEINPUT_AXIS_CAPACITY) {
						assert(false && "not enough axes supported");
						break;
					}
				}
				
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

	}


	void GameInput::deinit()
	{
		// close all joysticks
		for (int j = 0; j < joysticksSize; ++j) {
			SDL_Joystick* joy = joysticks[j];
			if (SDL_JoystickGetAttached(joy)) {
				SDL_JoystickClose(joy);
			}
		}

		// free all cursors
		for (int c = 0; c < _InputMouseCursorCount; ++c) {
			SDL_FreeCursor(cursors[c]);
		}

		inputMappings.deinit();
		inputContexts.deinit();
		callbacks.deinit();
	}

}