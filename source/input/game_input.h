#ifndef _GAME_INPUT_H
#define _GAME_INPUT_H

#include <SDL_events.h>
#include "capacity.h"
#include "utility/concurrent_queue.h"
#include "utility/fixed_timestep.h"
#include "platform_input.h"

namespace input {

	// TODO: would love some macro or metaprogramming magic to combine all of these
	// enum / string table definitions like in griffin (without Boost)

	/**
	 * Input Contexts
	 */

	enum InputContextIndex : u8 {
		InputContext_InGame = 0,
		InputContext_DevConsole,
		InputContext_DevCamera,
		InputContext_PlayerFPS,
		_InputContextsCount
	};

	const char* InputContextNames[_InputContextsCount] = {
		"System Controls",
		"Dev Console Controls",
		"Dev Camera Controls",
		"Player Movement"
	};

	enum InputContextOptions : u16 {
		CaptureTextInput        = 0x1,		// text input events are captured by this context
		SetRelativeMouseMode    = 0x2,		// this context sets relative mouse mode
		UnsetRelativeMouseMode  = 0x4,		// this context unsets relative mouse mode
		ShowMouseCursor         = 0x8,		// **Currently Unused** true to show cursor specified by cursorIndex
		EatKeyboardEvents       = 0x10,		// true to eat all keyboard events, preventing pass-down to lower contexts
		EatMouseEvents          = 0x20,		// prevent mouse button/wheel events from passing down
		EatJoystickEvents       = 0x40,		// prevent joystick button events from passing down
		EatMouseMotionEvents    = 0x80,		// prevent mouse motion events from passing down
		EatJoystickMotionEvents = 0x100		// prevent joystick motion events from passing down
	};

	// TODO: consider reintroducing the activeInputContexts list sorted in priority order, otherwise just get rid of
	// the priority concept and define them in the order needed for proper cascading of mappings
	struct InputContext {
		u16					options;		// all input context options
		u8					priority;		// input context priority
		u8					toolsOnly;		// 1 if this is for tools and not the shipping game
		u8					active;			// 1 if the context is currently active
		u8					_padding;
		// stores input mapping to actions, states, axes
		// TODO: need inputMappings length
//		h32				inputMappings[INPUTCONTEXT_MAPPINGS_CAPACITY];
		//u32			cursorIndex;		// lookup into input system's cursor table
	};

	#define DefineContext(priority, toolsOnly, options) \
		{ options, priority, toolsOnly, 0, 0 }


	struct GameInputContexts {
		union {
			struct {
				InputContext inGame;
				InputContext devConsole;
				InputContext devCamera;
				InputContext playerFPS;
			};
			InputContext _contexts[_InputContextsCount] = {
				DefineContext(0, 0, 0), // inGame
				DefineContext(1, 1, CaptureTextInput | EatKeyboardEvents), // devConsole
				DefineContext(2, 1, SetRelativeMouseMode), // devCamera
				DefineContext(2, 0, SetRelativeMouseMode), // playerFPS
			};
		};
	};


	/**
	 * Input Actions
	 */

	enum InputActionIndex : u8 {
		InputAction_Exit = 0,
		InputAction_CaptureMouse,
		InputAction_ToggleDevCamera,
		InputAction_ToggleDevConsole,
		InputAction_Player_Jump,
		InputAction_Player_Use,
		InputAction_Player_Reload,
		InputAction_DevCam_SpeedScrollIncrease,
		InputAction_DevCam_SpeedScrollDecrease,
		_InputActionsCount
	};

	const char* InputActionNames[_InputActionsCount] = {
		"Exit",
		"Capture Mouse",
		"Toggle Dev Camera",
		"Toggle Dev Console",
		"Jump",
		"Use",
		"Reload",
		"DevCam Speed Increase",
		"DevCam Speed Decrease"
	};

	enum InputBindEvent : u8 {
		Bind_Down = 0,			// Bind to DOWN event of keyboard key, mouse or joystick button
		Bind_Up,				// Bind to UP event of keyboard key, mouse or joystick button
		Bind_MouseWheelDown,	// Bind to mouse wheel -y event
		Bind_MouseWheelUp,		// Bind to mouse wheel +y event
		Bind_MouseWheelLeft,	// Bind to mouse wheel -x event
		Bind_MouseWheelRight	// Bind to mouse wheel +x event
	};

	enum InputMouseClicks : u8 {
		Mouse_ClicksNA = 0,
		Mouse_SingleClick = 1,	// for mouse buttons only, trigger event on single click
		Mouse_DoubleClick = 2	// for mouse buttons only, trigger event on double click
	};

	/**
	 * InputActionBinding maps raw input events to game Actions events. Actions bind to keyboard,
	 * mouse button, joystick button, and mouse wheel events.
	 * Actions are single-time events, not affected by key repeat, mapped to a single input event
	 * e.g. {keydown-G} or {keyup-G}.
	 */
	struct InputActionBinding {
		u32					device;			// instance_id of the device, comes through event "which", keyboard is always 0
		u32					keycode;		// keyboard virtual key code, mouse or joystick button
		u16					modifier;		// keyboard modifier, SDL_Keymod, defaults to 0 (KMOD_NONE)
		InputBindEvent		bind;			// event to trigger the action
		InputMouseClicks	clicks;			// for mouse buttons only, single or double click
		u32					_padding;
	};

	/**
	 * Mapped action for a frame.
	 */
	struct MappedAction {
		i64					gameTime;		// gameTime in counts when most recent action was triggered
		u64					frame;			// frame number when most recent action was triggered
		f32					x;				// mouse clicks include normalized and raw position
		f32					y;
		i32					xRaw;
		i32					yRaw;
	};

	struct InputAction {
		InputActionBinding	binding;
		MappedAction		mapping;
		InputContextIndex	context;
		u8					handled;		// flag set to 0 on new event, can be set to 1 when event handled by a callback
		u8					active;			// flag set to 1 when action is active for a frame
		u8					activeIndex; 	// index into the frameMappedInput.activeActions array, or 0xFF when inactive
		u32					_padding;
	};
	static_assert_aligned_size(InputAction,8);

	#define DefineAction(context, keycode, modifier, bind) \
		{ { 0, keycode, modifier, bind, Mouse_ClicksNA },\
		  {}, context, 0, 0, 0xFF, 0 }

	#define ActionMouseClick(context, clicks) \
		{ { 0, 0, 0, Bind_Down, clicks },\
		  {}, context, 0, 0, 0xFF, 0 }

	#define ActionMouseWheel(context, bind) \
		{ { 0, 0, 0, bind, Mouse_ClicksNA },\
		  {}, context, 0, 0, 0xFF, 0 }

	struct GameInputActions {
		union {
			struct {
				InputAction	exit;
				InputAction	captureMouse;
				InputAction	toggleDevCamera;
				InputAction	toggleDevConsole;
				InputAction	player_jump;
				InputAction	player_use;
				InputAction	player_reload;
				InputAction	devCam_speedIncrease;
				InputAction	devCam_speedDecrease;
			};
			InputAction _actions[_InputActionsCount] = {
				DefineAction(InputContext_InGame,    SDLK_ESCAPE,    KMOD_NONE,  Bind_Down),	// exit
				DefineAction(InputContext_InGame,    SDLK_LALT,      KMOD_NONE,  Bind_Up),		// captureMouse
				DefineAction(InputContext_InGame,    SDLK_c,         KMOD_LCTRL, Bind_Down),	// toggleDevCamera
				DefineAction(InputContext_InGame,    SDLK_BACKQUOTE, KMOD_NONE,  Bind_Down),	// toggleDevConsole
				DefineAction(InputContext_PlayerFPS, SDLK_SPACE,     KMOD_NONE,  Bind_Down),	// player_jump
				DefineAction(InputContext_PlayerFPS, SDLK_f,         KMOD_NONE,  Bind_Down),	// player_use
				DefineAction(InputContext_PlayerFPS, SDLK_r,         KMOD_NONE,  Bind_Down),	// player_reload
				ActionMouseWheel(InputContext_DevCamera, Bind_MouseWheelUp),					// devCam_speedIncrease
				ActionMouseWheel(InputContext_DevCamera, Bind_MouseWheelDown)					// devCam_speedDecrease
			};
		};
	};


	/**
	 * Input States
	 */

	enum InputStateIndex : u8 {
		InputState_Pause = 0,
		InputState_Player_MoveForward,
		InputState_Player_MoveBackward,
		InputState_Player_MoveLeft,
		InputState_Player_MoveRight,
		InputState_Player_Sprint,
		InputState_Player_Walk,
		InputState_Player_Crouch,
		InputState_DevCamera_MoveForward,
		InputState_DevCamera_MoveBackward,
		InputState_DevCamera_MoveLeft,
		InputState_DevCamera_MoveRight,
		InputState_DevCamera_MoveUp,
		InputState_DevCamera_MoveDown,
		InputState_DevCamera_RollLeft,
		InputState_DevCamera_RollRight,
		InputState_DevCamera_Speed,
		_InputStatesCount
	};

	const char* InputStateNames[_InputStatesCount] = {
		"Pause",
		"Move Forward",
		"Move Backward",
		"Move Left",
		"Move Right",
		"Sprint",
		"Walk",
		"Crouch",
		"DevCam Move Forward",
		"DevCam Move Backward",
		"DevCam Move Left",
		"DevCam Move Right",
		"DevCam Move Up",
		"DevCam Move Down",
		"DevCam Roll Left",
		"DevCam Roll Right",
		"DevCam Speed"
	};

	/**
	 * InputStateBinding maps raw input events to game States. States bind to keyboard, mouse
	 * button, joystick button, and mouse wheel events.
	 * States are binary on/off flags mapped to two input events:
	 * e.g. {on:keydown-W, off:keyup-W} would be a typical of WASD movement state
	 * while {on:keydown-C, off:keydown-C} would behave like a "crouch" toggle.
	 */
	struct InputStateBinding {
		u32					device;			// instance_id of the device, comes through event "which", keyboard is always 0
		u32					keycode;		// keyboard virtual key code, mouse or joystick button
		u16					modifier;		// keyboard modifier, SDL_Keymod, defaults to 0 (KMOD_NONE)
		InputBindEvent		bindIn;			// event to start the state
		InputBindEvent		bindOut;		// event to end the state
		InputMouseClicks	clicks;			// for mouse buttons only, single or double click
		u8					_padding[3];
	};

	/**
	 * Mapped active state for a frame. All input timings are quantized to the update frame
	 * rate, therefor the first frame a state becomes active includes the full frame timestep.
	 */
	struct MappedState {
		f64					totalMs;		// total millis the state has been active
		i64					startCounts;	// clock counts when state began
		i64					totalCounts;	// currentCounts - startCounts + countsPerTick
		u64					startFrame;		// frame number when state began
		u32					totalFrames;	// currentFrame - startFrame + 1
	};

	struct InputState {
		InputStateBinding	binding;
		MappedState			mapping;
		InputContextIndex	context;
		u8					handled;		// flag set to 0 on new event, can be set to 1 when event handled by a callback
		u8					active;			// flag set to 1 when action is active for a frame
		u8					activeIndex; 	// index into the frameMappedInput.activeStates array, or 0xFF when inactive
	};
	static_assert_aligned_size(InputState,8);

	#define DefineState(context, keycode, modifier, binddown, bindup, clicks) \
		{ { 0, keycode, modifier, binddown, bindup, clicks, {} },\
		  {}, context, 0, 0, 0xFF }

	#define StatePress(context, keycode) \
		{ { 0, keycode, KMOD_NONE, Bind_Down, Bind_Up, Mouse_ClicksNA, {} },\
		  {}, context, 0, 0, 0xFF }

	#define StateToggle(context, keycode) \
		{ { 0, keycode, KMOD_NONE, Bind_Down, Bind_Down, Mouse_ClicksNA, {} },\
		  {}, context, 0, 0, 0xFF }

	struct GameInputStates {
		union {
			struct {
				InputState	pause;
				InputState	player_moveForward;
				InputState	player_moveBackward;
				InputState	player_moveLeft;
				InputState	player_moveRight;
				InputState	player_sprint;
				InputState	player_walk;
				InputState	player_crouch;
				InputState	devCam_moveForward;
				InputState	devCam_moveBackward;
				InputState	devCam_moveLeft;
				InputState	devCam_moveRight;
				InputState	devCam_moveUp;
				InputState	devCam_moveDown;
				InputState	devCam_rollLeft;
				InputState	devCam_rollRight;
				InputState	devCam_speed;
			};
			InputState _states[_InputStatesCount] = {
				StateToggle(InputContext_InGame,    SDLK_p ),		// pause
				StatePress( InputContext_PlayerFPS, SDLK_w ),		// player_moveForward
				StatePress( InputContext_PlayerFPS, SDLK_s ),		// player_moveBackward
				StatePress( InputContext_PlayerFPS, SDLK_a ),		// player_moveLeft
				StatePress( InputContext_PlayerFPS, SDLK_d ),		// player_moveRight
				StatePress( InputContext_PlayerFPS, SDLK_LSHIFT ),	// player_sprint
				StatePress( InputContext_PlayerFPS, SDLK_LCTRL ),	// player_walk
				StateToggle(InputContext_PlayerFPS, SDLK_c ),		// player_crouch
				StatePress( InputContext_DevCamera, SDLK_w ),		// devCam_moveForward
				StatePress( InputContext_DevCamera, SDLK_s ),		// devCam_moveBackward
				StatePress( InputContext_DevCamera, SDLK_a ),		// devCam_moveLeft
				StatePress( InputContext_DevCamera, SDLK_d ),		// devCam_moveRight
				StatePress( InputContext_DevCamera, SDLK_x ),		// devCam_moveUp
				StatePress( InputContext_DevCamera, SDLK_z ),		// devCam_moveDown
				StatePress( InputContext_DevCamera, SDLK_q ),		// devCam_rollLeft
				StatePress( InputContext_DevCamera, SDLK_e ),		// devCam_rollRight
				StatePress( InputContext_DevCamera, SDLK_LSHIFT )	// devCam_speed
			};
		};
	};


	/**
	 * Input Axes
	 */

	enum InputAxisIndex : u8 {
		InputAxis_Player_MouseLookX = 0,
		InputAxis_Player_MouseLookY,
		InputAxis_DevCam_MouseLookX,
		InputAxis_DevCam_MouseLookY,
		_InputAxisCount
	};

	const char* InputAxisNames[_InputAxisCount] = {
		"Mouse Look X",
		"Mouse Look Y",
		"DevCam Mouse Look X",
		"DevCam Mouse Look Y"
	};

	enum InputAxisMotion : u8 {
		Motion_Absolute = 0,
		Motion_Relative
	};

	enum InputAxisInvert : u8 {
		Axis_NotInverted = 0,	// Axis_NotInverted is the default setting
		Axis_Inverted			// Axis_Inverted multiplies the range mapping by -1
	};

	enum InputAxisRange : u8 {
		Range_NegToPos = 0,		// Range_NegToPos maps range from -1 to 1
		Range_Pos				// Range_Pos maps 0 to 1, sliders use Range_Pos
	};

	enum InputAxisCurve : u8 {
		Curve_Linear = 0,
		Curve_SCurve
	};

	/**
	 * InputAxisBinding maps raw input motion events to game Axis relative motion or absolute
	 * position. An axis tracks the movement of joystick, throttle, rudder pedals, head tracker,
	 * trackball and mouse.
	 */
	struct InputAxisBinding {
		u32					device;			// instance_id of the device, comes through event "which", mouse is always 0
		u8					axis;			// index of the joystick axis, mouse is always 0 (x) and 1 (y)
		u8					deadzone;		// deadzone for axis
		u8					curvature;		// curvature of axis
		InputAxisMotion		motion;			// motion is absolute or relative
		i16					sensitivity;	// sensitivity multiplier, mainly for mouse movement in relative mode
		u8					saturationX;	// saturation of the x axis
		u8					saturationY;	// saturation of the y axis
		InputAxisInvert		invert;			// invert the axis or not
		InputAxisRange		range;			// determines -1 to 1, or 0 to 1 range for the axis
		InputAxisCurve		curve;			// curve type of axis
		u8					_padding;
		const char*			deviceName;		// name of the device
	};

	/**
	 * Mapped active axis for a frame. AxisMotion accumulates the relative motion for an axis and
	 * records the last absolute position, then it is mapped to an active game axis and curves and
	 * other attributes are applied to get the final mapped value.
	 */
	struct MappedAxis {
		i64					gameTime;		// gameTime in counts when most recent action was triggered
		u64					frame;			// frame number when most recent action was triggered
		f32					posMapped;		// absolute position of axis mapped to curve
		f32					relMapped;		// relative motion of the axis since last frame mapped to curve
		i32					posRaw;			// raw value from device, not normalized or mapped to curve, may be useful but use posMapped by default
		i32					relRaw;			// relative raw value of the axis
	};

	struct InputAxis {
		InputAxisBinding	binding;
		MappedAxis			mapping;
		InputContextIndex	context;
		u8					handled;		// flag set to 0 on new event, can be set to 1 when event handled by a callback
		u8					active;			// flag set to 1 when action is active for a frame
		u8					activeIndex; 	// index into the frameMappedInput.activeAxes array, or 0xFF when inactive
		u32					_padding;
	};
	static_assert_aligned_size(InputAxis,8);

	/**
	 * Axis motion for a frame. Motion events are accumulated for the frame to get relative, and
	 * the last absolute position value is taken for the frame.
	 */
	struct AxisMotion {
		u32					device;			// instance_id of the device that owns this axis, mouse is always 0 (x) and 1 (y)
		u8					axis;			// axis number on the device
		u8					_padding[3];
		i32					posRaw;			// raw value from device, not normalized or mapped to curve, may be useful but use posMapped by default
		i32					relRaw;			// relative raw value of the axis
	};

	#define DefineAxis(context, axis, invert) \
		{ { 0, axis, 0, 0, Motion_Absolute, 1, 100, 100, invert, Range_NegToPos, Curve_SCurve, 0, nullptr },\
		  {}, context, 0, 0, 0xFF, 0 }

	#define AxisSlider(context, axis, invert) \
		{ { 0, axis, 0, 0, Motion_Absolute, 100, 100, invert, Range_Pos, Curve_Linear, 0, nullptr },\
		  {}, context, 0, 0, 0xFF, 0 }

	#define AxisRelative(context, axis, sensitivity, invert) \
		{ { 0, axis, 0, 0, Motion_Relative, sensitivity, 100, 100, invert, Range_NegToPos, Curve_Linear, 0, nullptr },\
		  {}, context, 0, 0, 0xFF, 0 }

	struct GameInputAxes {
		union {
			struct {
				InputAxis	player_mouseLookX;
				InputAxis	player_mouseLookY;
				InputAxis	devCam_mouseLookX;
				InputAxis	devCam_mouseLookY;
			};
			InputAxis _axes[_InputAxisCount] = {
				AxisRelative(InputContext_PlayerFPS, 0, 15, Axis_NotInverted), // player_mouseLookX
				AxisRelative(InputContext_PlayerFPS, 1, 10, Axis_NotInverted), // player_mouseLookY
				AxisRelative(InputContext_DevCamera, 0, 15, Axis_NotInverted), // devCam_mouseLookX
				AxisRelative(InputContext_DevCamera, 1, 10, Axis_NotInverted)  // devCam_mouseLookY
			};
		};
	};


	/**
	 * Container holding all mapped input for a frame, plus text input
	 */
	struct FrameMappedInput {
		union {
			struct {
				AxisMotion	mouseXMotion;
				AxisMotion	mouseYMotion;
				AxisMotion	axisMotion[GAMEINPUT_MAX_AXES];
			};
			AxisMotion		mouseAndAxisMotion[2 + GAMEINPUT_MAX_AXES];
		};

		char				textInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];		// text input buffer
		char				textComposition[SDL_TEXTINPUTEVENT_TEXT_SIZE];	// text editing buffer

		i32					cursorPos;				// text editing cursor position
		i32					selectionLength;		// text editing selection length (if any)
		
		u8					textInputSize;
		u8					textCompositionSize;
		u8					textInputHandled;		// flag set to true when text input has been handled by a callback

		u8					activeActionCount;		// how many actions are active this frame
		u8					activeStateCount;		// how many states are active this frame
		u8					activeAxisCount;		// how many axes are active this frame
		u8					axisMotionCount;		// how many axes had motion this frame

		InputActionIndex	activeActions[_InputActionsCount];	// action events mapped to active InputActions for the frame
		InputStateIndex		activeStates[_InputStatesCount];	// state events mapped to active InputStates for the frame
		InputAxisIndex		activeAxes[_InputAxisCount];		// axis events mapped to active InputAxis' for the frame
	};

	/**
	 * Track the highest priority active context with the corresponding options. You must use the
	 * setContextActive function to activate/deactivate contexts for this data to remain accurate.
	 */
	struct HighestPriority {
		u8 captureTextInput       = 0xFF;
		u8 setRelativeMouseMode   = 0xFF;
		u8 unsetRelativeMouseMode = 0xFF;
		u8 keyboardEvent          = 0xFF;
		u8 mouseEvent             = 0xFF;
		u8 joystickEvent          = 0xFF;
		u8 mouseMotionEvent       = 0xFF;
		u8 joystickMotionEvent    = 0xFF;
	};

	struct CallbackPriority {
		h32					callbackId;
		i32					priority;
	};

	
	typedef void InputCallbackFunc(FrameMappedInput&);
	typedef bool InputActionFunc(MappedAction&, InputContext&);
	typedef bool InputStateFunc(MappedState&, InputContext&);
	typedef bool InputAxisFunc(MappedAxis&, InputContext&);
	typedef bool InputTextFunc(FrameMappedInput&, InputContext&);
	
//	DenseHandleMap16Typed(InputCallbackFunc*, HandleMap_InputCallback, 2);


	struct GameInput {
		GameInputContexts	contexts;
		GameInputActions	actions;
		GameInputStates		states;
		GameInputAxes		axes;

//		HandleMap_InputCallback		callbacks;		// map storing all registered callbacks

//		DenseHandleMap16Buffer(InputCallbackFunc*, callbacksBuffer, GAMEINPUT_CALLBACKS_CAPACITY);

		// active input contexts sorted by priority
//		ActiveInputContext	activeContexts[_InputContextsCount];
		// callback order sorted by priority
//		CallbackPriority	callbackPriorityList[GAMEINPUT_CALLBACKS_CAPACITY];
		
		FrameMappedInput	frameMappedInput = {};	// per-frame mapped input buffer
		
		HighestPriority		highestPriority;

		/**
		 * Initialize the system, detect devices, etc.
		 */
		void init();
		
		/**
		 * Frees cursors, joysticks and any other resources
		 */
		void deinit();

		/**
		 * Executed in the update fixed timestep loop
		 */
		void updateFrameTick(const UpdateInfo& ui,
							 PlatformInput& platformInput,
							 u32 windowWidth,
							 u32 windowHeight);


		// Input Contexts

		/**
		 * Set the InputContext, returns true on success
		 */
		void setContextActive(InputContextIndex ctx, bool active = true);

		// Callbacks
		
		h32 registerCallback(i32 priority, InputCallbackFunc* func);

		bool unregisterCallback(h32 callbackId);

		/**
		 * Convenience function for handling a single active mapped action event
		 * @param	callback	returns true if input was handled, false if not handled
		 * @return	true if a mapped action is found and handled by the callback
		 */
		bool handleInputAction(InputActionIndex action,
							   FrameMappedInput& mappedInput,
							   InputActionFunc* callback);

		/**
		 * Convenience function for handling a single active mapped state event
		 * @param	callback	returns true if input was handled, false if not handled
		 * @return	true if a mapped state is found and handled by the callback
		 */
		bool handleInputState(InputStateIndex state,
							  FrameMappedInput& mappedInput,
							  InputStateFunc* callback);

		/**
		 * Convenience function for handling a single active mapped axis event
		 * @param	callback	returns true if input was handled, false if not handled
		 * @return	true if a mapped axis is found and handled by the callback
		 */
		bool handleInputAxis(InputAxisIndex axis,
							 FrameMappedInput& mappedInput,
							 InputAxisFunc* callback);

		/**
		 * Convenience function for handling text input events. Returns false if the context
		 *	asked for is not active or does not capture text input.
			*	Otherwise returns the result of callback.
			* @param	context		which input context you are capturing text for
			* @param	mappedInput	member wstring textInput contains the input text
			* @param	callback	returns true if text input was consumed, false if not consumed
			* @return	true if text input was consumed by the callback
			*/
		bool handleTextInput(InputContextIndex context,
							 FrameMappedInput& mappedInput,
							 InputTextFunc* callback);

		// Input Modes

		/**
		 * Text editing mode
		 */
		void startTextInput();
		void stopTextInput();
		bool textInputActive();

		/**
		 * Mouse movement mode
		 */
		void startRelativeMouseMode();
		void stopRelativeMouseMode();
		bool relativeMouseModeActive();

		/**
		 * Translate input events into mapped into for one frame
		 */
		void mapFrameInputs(const UpdateInfo& ui,
							DenseQueue_InputEvent& events,
							f32 inverseWindowWidth,
							f32 inverseWindowHeight);

		void mapFrameMotion(const UpdateInfo& ui,
							DenseQueue_InputEvent& motionEvents,
							f32 inverseWindowWidth,
							f32 inverseWindowHeight);
	};

}

#endif