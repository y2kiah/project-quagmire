#include "game_input.h"


#define JOYSTICK_INVERSE_MAX_RAW	1.0f / 32768.0f

namespace input {

	void GameInput::updateFrameTick(const UpdateInfo& ui, PlatformInput& platformInput)
	{
		platformInput.eventsQueue.try_pop_all_push(platformInput.popEvents._q);
		platformInput.motionEventsQueue.try_pop_all_push(platformInput.popMotionEvents._q);
// TEMP instead of processing
//platformInput.popEvents.clear();
platformInput.popMotionEvents.clear();

		// clear previous frame actions and axis mappings
		frameMappedInput.activeActionCount = 0;
		frameMappedInput.activeAxisCount = 0;

		// clear handled flag of frame's text input
		// TODO: does the input string itself need to persist many frames for compositing, or should we clear it too?
		frameMappedInput.textInputHandled = 0;

		// Remove active states where the context is no longer active, swap and pop.
		// Apply this filter to bindings where key down is active. Don't apply it to toggles or
		// bindings where key up is active.
		u16 s = 0;
		while (s < frameMappedInput.activeStateCount)
		{
			InputState& state = states._states[frameMappedInput.activeStates[s]];
			bool hasDownUpBinding = (state.binding.bindIn == Bind_Down && state.binding.bindOut == Bind_Up);

			if (hasDownUpBinding
				&& !contexts._contexts[state.context].active)
			{
				frameMappedInput.activeStates[s] =
					frameMappedInput.activeStates[--frameMappedInput.activeStateCount-1];
			}
			else {
				++s;
			}
		}

		// loop over active states, add to totalCounts, totalMS and totalFrames, reset handled flag
		for (s = 0; s < frameMappedInput.activeStateCount; ++s)
		{
			InputState& state = states._states[frameMappedInput.activeStates[s]];
			++state.mapping.totalFrames;
			state.mapping.totalCounts += ui.deltaCounts;
			state.mapping.totalMs += ui.deltaMs;
			state.handled = 0;
		}

		// map inputs using active contexts
		mapFrameInputs(ui, platformInput.popEvents);
//		mapFrameMotion(ui, platformInput.popMotionEvents);

		// TEMP output mapped inputs
		for (u16 a = 0; a < frameMappedInput.activeActionCount; ++a) {
			logger::info(
					logger::Category_Input,
					"action \"%s\" active",
					InputActionNames[frameMappedInput.activeActions[a]]);
		}
		for (s = 0; s < frameMappedInput.activeStateCount; ++s) {
			logger::info(
					logger::Category_Input,
					"state \"%s\" active",
					InputStateNames[frameMappedInput.activeStates[s]]);
		}

		// invoke all callbacks in priority order, passing this frame's mapped input
		//for (auto& cp : callbackPriorityList) {
		//	auto& cb = callbacks[cp.callbackId];
		//	cb(frameMappedInput);
		//}
	}


	void GameInput::mapFrameInputs(
		const UpdateInfo& ui,
		DenseQueue_InputEvent& events)
	{
		// Find the highest priority active context that eats events, these block mappings at lower priority.
		// For example, a menu pops up with a higher context that blocks mouse look camera control.
		u8 highestPriorityKeyboardEvent = 0xFF;
		u8 highestPriorityMouseEvent = 0xFF;
		u8 highestPriorityJoystickEvent = 0xFF;
		u8 highestPriorityTextEvent = 0xFF;

		for (u8 c = 0; c < _InputContextsCount; ++c) {
			InputContext& context = contexts._contexts[c];
			if (context.active) {
				if (context.priority <= highestPriorityKeyboardEvent && context.options & EatKeyboardEvents) {
					highestPriorityKeyboardEvent = context.priority;
				}
				if (context.priority <= highestPriorityMouseEvent && context.options & EatMouseEvents) {
					highestPriorityMouseEvent = context.priority;
				}
				if (context.priority <= highestPriorityJoystickEvent && context.options & EatJoystickEvents) {
					highestPriorityJoystickEvent = context.priority;
				}
				if (context.priority <= highestPriorityTextEvent && context.options & CaptureTextInput) {
					highestPriorityTextEvent = context.priority;
				}
			}
		}

		// process events timestamped up to the current simulation frame time
		while (!events.empty()
				&& events.front()->timeStampCounts <= ui.virtualTime)
		{
			InputEvent& evt = *events.pop_fifo();

			// handle key and button events
			if (evt.eventType == Event_Keyboard ||
				evt.eventType == Event_Joystick ||
				evt.eventType == Event_Mouse)
			{
				u8 highestActionContextPriority = 0xFF;
				u8 mappedActionIndex = 0xFF;

				// for each action, find the one in the highest priority active
				// context with a matching binding 
				for (u8 a = 0; a < _InputActionsCount; ++a) {
					InputAction& action = actions._actions[a];
					InputContext& context = contexts._contexts[action.context];
					
					if (context.active
						&& context.priority < highestActionContextPriority
						// make sure event wasn't eaten by a higher priority context
						&& (evt.eventType != Event_Keyboard || context.priority <= highestPriorityKeyboardEvent)
						&& (evt.eventType != Event_Joystick || context.priority <= highestPriorityJoystickEvent)
						&& (evt.eventType != Event_Mouse    || context.priority <= highestPriorityMouseEvent))
					{
						InputActionBinding& binding = action.binding;
						bool matched = false;

						if ((binding.bind == Bind_Down && evt.evt.type == SDL_KEYDOWN) ||
							(binding.bind == Bind_Up   && evt.evt.type == SDL_KEYUP))
						{
							u16 mod = evt.evt.key.keysym.mod & ~(KMOD_NUM | KMOD_CAPS | KMOD_MODE); // ignore these mods
							matched = ((u32)evt.evt.key.keysym.sym == binding.keycode
										&& mod == binding.modifier);
						}
						else if ((binding.bind == Bind_Down && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
								 (binding.bind == Bind_Up   && evt.evt.type == SDL_MOUSEBUTTONUP))
						{
							matched = (evt.evt.button.button == binding.keycode
										&& evt.evt.button.clicks == binding.clicks);
						}
						else if ((binding.bind == Bind_Down && evt.evt.type == SDL_JOYBUTTONDOWN) ||
								 (binding.bind == Bind_Up   && evt.evt.type == SDL_JOYBUTTONUP))
						{
							matched = ((u32)evt.evt.jbutton.which == binding.device
										&& evt.evt.jbutton.button == binding.keycode);
						}
						else if (evt.evt.type == SDL_MOUSEWHEEL)
						{
							matched = (binding.bind == Bind_MouseWheelDown  && evt.evt.wheel.y < 0) ||
									  (binding.bind == Bind_MouseWheelUp    && evt.evt.wheel.y > 0) ||
									  (binding.bind == Bind_MouseWheelLeft  && evt.evt.wheel.x < 0) ||
									  (binding.bind == Bind_MouseWheelRight && evt.evt.wheel.x > 0);
						}

						// found a matching action mapping for the event
						if (matched) {
							highestActionContextPriority = context.priority;
							mappedActionIndex = a;
						}
					}
				}

				u8 highestStateContextPriority = 0xFF;
				u8 mappedStateIndex = 0xFF;
				u8 mappedActiveStateIndex = 0xFF;

				// for each state, find the one in the highest priority active
				// context with a matching binding 
				for (u8 s = 0; s < _InputStatesCount; ++s) {
					InputState& state = states._states[s];
					InputContext& context = contexts._contexts[state.context];
					
					if (context.active
						&& context.priority <= highestActionContextPriority
						&& context.priority < highestStateContextPriority
						// make sure event wasn't eaten by a higher priority context
						&& (evt.eventType != Event_Keyboard || context.priority <= highestPriorityKeyboardEvent)
						&& (evt.eventType != Event_Joystick || context.priority <= highestPriorityJoystickEvent)
						&& (evt.eventType != Event_Mouse    || context.priority <= highestPriorityMouseEvent))
					{
						InputStateBinding& binding = state.binding;
						bool matched = false;

						u8 stateActiveIndex = 0xFF;
						for (u8 act = 0; act < frameMappedInput.activeStateCount; ++act) {
							if (frameMappedInput.activeStates[act] == s) {
								stateActiveIndex = act;
								break;
							}
						}

						// state is currently inactive
						if (stateActiveIndex == 0xFF)
						{
							if ((binding.bindIn == Bind_Down && evt.evt.type == SDL_KEYDOWN) ||
								(binding.bindIn == Bind_Up   && evt.evt.type == SDL_KEYUP) &&
								// prevent repeat key events from changing toggle states
								(binding.bindIn != binding.bindOut || !evt.evt.key.repeat))
							{
								matched = ((u32)evt.evt.key.keysym.sym == binding.keycode);
											//&& evt.evt.key.keysym.mod == binding.modifier);
								// TODO: states cannot use modifiers currently, make sure this is ok
								// might want to support modifiers but check whether modifier "matters" or not - if there is a matching key binding
								// with the modifier, then it matters, otherwise it doesn't
							}
							else if ((binding.bindIn == Bind_Down && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
									 (binding.bindIn == Bind_Up   && evt.evt.type == SDL_MOUSEBUTTONUP))
							{
								matched = (evt.evt.button.button == binding.keycode);
							}
							else if ((binding.bindIn == Bind_Down && evt.evt.type == SDL_JOYBUTTONDOWN) ||
									 (binding.bindIn == Bind_Up   && evt.evt.type == SDL_JOYBUTTONUP))
							{
								matched = ((u32)evt.evt.jbutton.which == binding.device
											&& evt.evt.jbutton.button == binding.keycode);
							}
						}
						// state is currently active
						else {
							if ((binding.bindOut == Bind_Down && evt.evt.type == SDL_KEYDOWN) ||
								(binding.bindOut == Bind_Up   && evt.evt.type == SDL_KEYUP))
							{
								matched = ((u32)evt.evt.key.keysym.sym == binding.keycode);
											//&& evt.evt.key.keysym.mod == binding.modifier);
								// TODO: maybe need to look for sym and mod keys separately here and make state inactive for either one
								// if the modifier OR the key is lifted in either order, the state should be deactivated
							}
							else if ((binding.bindOut == Bind_Down && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
									 (binding.bindOut == Bind_Up   && evt.evt.type == SDL_MOUSEBUTTONUP))
							{
								matched = (evt.evt.button.button == binding.keycode);
							}
							else if ((binding.bindOut == Bind_Down && evt.evt.type == SDL_JOYBUTTONDOWN) ||
									 (binding.bindOut == Bind_Up   && evt.evt.type == SDL_JOYBUTTONUP))
							{
								matched = ((u32)evt.evt.jbutton.which == binding.device
											&& evt.evt.jbutton.button == binding.keycode);
							}
						}

						// found a matching state mapping for the event
						if (matched) {
							highestStateContextPriority = context.priority;
							mappedStateIndex = s;
							mappedActiveStateIndex = stateActiveIndex;
						}
					}
				}

				// Now process the mapped events, actions and states with overlapping bindings can
				// exist at the same level, and both will be processed, but a higher priority on
				// one will always take precendence over the other.

				// found an action mapping, make it active
				if (mappedActionIndex != 0xFF
					&& highestActionContextPriority <= highestStateContextPriority)
				{
					InputAction& action = actions._actions[mappedActionIndex];
					
					action.mapping.gameTime = ui.gameTime;
					action.mapping.frame = ui.frame;
					if (evt.evt.type == SDL_MOUSEBUTTONDOWN || evt.evt.type == SDL_MOUSEBUTTONUP) {
						action.mapping.xRaw = evt.evt.button.x;
						action.mapping.yRaw = evt.evt.button.y;
					}
					else {
						action.mapping.xRaw = action.mapping.yRaw = 0;
					}
					action.handled = 0;

					frameMappedInput.activeActions[++frameMappedInput.activeActionCount] =
						(InputActionIndex)mappedActionIndex;
				}
				// found a state mapping
				if (mappedStateIndex != 0xFF
					&& highestStateContextPriority <= highestActionContextPriority)
				{
					// make state active
					if (mappedActiveStateIndex == 0xFF) {
						InputState& state = states._states[mappedStateIndex];
						state.mapping.startCounts = ui.gameTime;
						state.mapping.startFrame = ui.frame;
						state.handled = 0;
						
						frameMappedInput.activeStates[++frameMappedInput.activeStateCount] =
							(InputStateIndex)mappedStateIndex;
					}
					// make state inactive
					else {
						// make inactive, remove from active states by swap and pop
						frameMappedInput.activeStates[mappedActiveStateIndex] =
							frameMappedInput.activeStates[--frameMappedInput.activeStateCount-1];
						break;
					}
				}
			}
			// handle text input events if there is any active context that captures text input
			else if (evt.eventType == Event_TextInput
					 && highestPriorityTextEvent != 0xFF
					 && highestPriorityTextEvent <= highestPriorityKeyboardEvent)
			{
				if (evt.evt.type == SDL_TEXTINPUT) {
					memcpy(frameMappedInput.textInput, evt.evt.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
				}
				else if (evt.evt.type == SDL_TEXTEDITING) {
					memcpy(frameMappedInput.textComposition, evt.evt.edit.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
					frameMappedInput.cursorPos = evt.evt.edit.start;
					frameMappedInput.selectionLength = evt.evt.edit.length;
				}
			}
		}
	}


/*	void GameInput::mapFrameMotion(const UpdateInfo& ui)
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
		// TODO: for now I'm doing this here, but eventually this should probably move to a higher level function
		contexts.inGame.active = 1;
		contexts.playerFPS.active = 1;

	//	inputMappings.init(GAMEINPUT_MAPPINGS_CAPACITY, inputMappingsBuffer);
	//	assert(sizeof(inputMappingsBuffer) == HandleMap_InputMapping::getTotalBufferSize(GAMEINPUT_MAPPINGS_CAPACITY));
		
	//	inputContexts.init(GAMEINPUT_CONTEXTS_CAPACITY, inputContextsBuffer);
	//	assert(sizeof(inputContextsBuffer) == HandleMap_InputContext::getTotalBufferSize(GAMEINPUT_CONTEXTS_CAPACITY));

	//	callbacks.init(GAMEINPUT_CALLBACKS_CAPACITY, callbacksBuffer);
	//	assert(sizeof(callbacksBuffer) == HandleMap_InputCallback::getTotalBufferSize(GAMEINPUT_CALLBACKS_CAPACITY));

	}


	void GameInput::deinit()
	{
//		inputMappings.deinit();
//		inputContexts.deinit();
//		callbacks.deinit();
	}

}