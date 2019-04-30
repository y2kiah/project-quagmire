#ifndef _INPUT_SYSTEM_H
#define _INPUT_SYSTEM_H

#include <SDL_events.h>
#include "capacity.h"
#include "utility/concurrent_queue.h"
#include "utility/dense_handle_map_16.h"
#include "utility/fixed_timestep.h"
#include "platform_input.h"

namespace input {

	/**
	 * Input Cursor types
	 */
	enum InputMouseCursor : u8 {
		Cursor_Arrow = 0,
		Cursor_Hand,
		Cursor_Wait,
		Cursor_IBeam,
		Cursor_Crosshair,
		_InputMouseCursorCount
	};

	/**
	 * Input Mapping types
	 */
	enum InputMappingType : u8 {
		Type_Action = 0,	// Actions are single-time events
		Type_State,			// States are on/off
		Type_Axis			// Axis are ranges of motion normalized to [-1,1]
	};

	/**
	 * Input Mapping Binding types
	 */
	enum InputMappingBindEvent : u8 {
		Bind_Down = 0,		// Bind to DOWN event of keyboard key, mouse or joystick button
		Bind_Up				// Bind to UP event of keyboard key, mouse or joystick button
	};

	/**
	 * Input Mapping Axis Curve types
	 */
	enum InputMappingAxisCurve : u8 {
		Curve_Linear = 0,
		Curve_SCurve
	};

	/**
	 * Input Context Options
	 */
	enum InputContextOptions : u8 {
		CaptureTextInput  = 0x1,		// true if text input events should be captured by this context
		RelativeMouseMode = 0x2,		// **Currently Unused** true for relative mouse mode vs. regular "GUI" mode
		ShowMouseCursor   = 0x4,		// **Currently Unused** true to show cursor specified by cursorIndex
		EatKeyboardEvents = 0x8,		// true to eat all keyboard events, preventing pass-down to lower contexts
		EatMouseEvents    = 0x10,		// prevent mouse events from passing down
		EatJoystickEvents = 0x20		// prevent joystick events from passing down
	};

	enum InputStateTransition : u8 {
		State_Activate = 0,
		State_Deactivate
	};


	/**
	 * Input mappings map raw input events or position data to Actions, States or Ranges. These
	 * are high-level game input events obtained from configuration data.
	 *	Actions: Single-time events, not affected by key repeat, mapped to a single input event
	 *			 e.g. {keydown-G} or {keyup-G}.
	 *	States:  Binary on/off flag mapped to two input events e.g. {on:keydown-G, off:keyup-G}
	 *			 or {on:keydown-G, off:keydown-G}. The first example would be typical of WASD
	 *			 type movement state, the second example would behave like a toggle.
	 *	Axis:	 Uses position information of joysticks, throttles, rudder pedals, head
	 *			 tracking gear, even mouse movement if desired.
	 */
	struct InputMapping {
		InputMappingType		type        = Type_Action;	// type of this mapping
		InputMappingBindEvent	bindIn      = Bind_Down;	// event to start the action or state
		InputMappingBindEvent	bindOut     = Bind_Up;		// event to end the state
		InputMappingAxisCurve	curve       = Curve_SCurve;	// curve type of axis
		u32						device      = 0;			// instanceID of the device, comes through event "which"
		// keyboard, mouse button, joystick button, mouse wheel events
		u32						keycode     = 0;			// keyboard virtual key code, mouse or joystick button
		u16						modifier    = 0;			// keyboard modifier, SDL_Keymod, defaults to 0 (KMOD_NONE), for actions only
		u8						mouseWheel  = 0;			// 0=false, 1=true is a mouse wheel binding
		u8						clicks      = 1;			// number of mouse clicks for binding (2==double-click)
		// mouse motion, joystick motion, joystick hat motion, ball motion events
		u8						axis        = 0;			// index of the joystick axis
		u8						deadzone    = 0;			// deadzone for axis
		u8						curvature   = 0;			// curvature of axis
		u8						saturationX = 100;			// saturation of the x axis
		u8						saturationY = 100;			// saturation of the y axis
		u8						relativeMotion = 0;			// 0=false, 1=true motion is relative not absolute
		u8						invert      = 0;			// 0=false, 1=true axis is inverted
		u8						slider      = 0;			// 0=false, 1=true axis is a slider
		f32						sensitivity = 1.0f;			// sensitivity multiplier, mainly for mouse movement in relative mode
		// all events
		h32						mappingId   = null_h32;		// the id handle of this mapping
		h32						contextId   = null_h32;		// the id handle of the context
		char					name[32]    = {};			// display name of the mapping
	};

	/**
	 * Mapped action for a frame.
	 */
	struct MappedAction {
		InputMapping*	inputMapping;
		h32				mappingId;
		f32				x;
		f32				y;			// mouse clicks include normalized position here
		i32				xRaw;
		i32				yRaw;
		bool			handled;	// flag set to true when event has been handled by a callback
	};

	/**
	 * Mapped active state for a frame. All input timings are quantized to the update frame
	 * rate, therefor the first frame a state becomes active includes the full frame timestep.
	 */
	struct MappedState {
		InputMapping*	inputMapping;
		h32				mappingId;
		f64				totalMs;		// total millis the state has been active
		i64				startCounts;	// clock counts when state began
		i64				totalCounts;	// currentCounts - startCounts + countsPerTick
		i64				startFrame;		// frame number when state began
		i32				totalFrames;	// currentFrame - startFrame + 1
		bool			handled;		// flag set to true when event has been handled by a callback
	};

	/**
	 * Axis motion for a frame. Motion events are accumulated for the frame to get relative, and
	 * the last absolute position value is taken for the frame.
	 */
	struct AxisMotion {
		u32				device;			// instanceID of the device that owns this axis, mouse is always 0 (x) and 1 (y)
		u32				axis;			// axis number on the device
		f32				posMapped;		// absolute position of axis mapped to curve
		f32				relMapped;		// relative motion of the axis since last frame mapped to curve
		i32				posRaw;			// raw value from device, not normalized or mapped to curve, may be useful but use posMapped by default
		i32				relRaw;			// relative raw value of the axis
		const char*		deviceName;		// name of the device
	};

	/**
	 * MappedAxis matches up AxisMotion with a valid InputMapping for a frame.
	 */
	struct MappedAxis {
		InputMapping*	inputMapping;
		AxisMotion*		axisMotion;
		h32				mappingId;
		bool			handled;		// flag set to true when event has been handled by a callback
	};

	/**
	 * Container holding all mapped input for a frame, plus text input
	 */
	struct FrameMappedInput {
		u16				actionsSize;
		u16				statesSize;
		u16				axesSize;
		u16				motionSize;

		MappedAction	actions[GAMEINPUT_ACTION_CAPACITY];	// actions mapped to an active InputMapping for the frame
		MappedState		states[GAMEINPUT_STATE_CAPACITY];	// states mapped to an active InputMapping for the frame
		MappedAxis		axes[GAMEINPUT_AXIS_CAPACITY];		// axes mapped to an active InputMapping for the frame
		AxisMotion		motion[GAMEINPUT_AXIS_CAPACITY];	// holds accumulated motion for the mouse and joysticks

		char			textInput[32];			// text input buffer
		char			textComposition[32];	// text editing buffer

		u8				textInputSize;
		u8				textCompositionSize;
		u8				textInputHandled;		// flag set to true when text input has been handled by a callback
		u8				_padding;
		i32				cursorPos;				// text editing cursor position
		i32				selectionLength;		// text editing selection length (if any)
	};

	struct InputContext {
		h32				contextId;		// the id handle of this context
		u8				options;		// all input context options
		u8				priority;		// input context priority
		u8				_padding[2];
		// stores input mapping to actions, states, axes
		h32				inputMappings[INPUTCONTEXT_MAPPINGS_CAPACITY];
		char			name[32];		// display name of the context
		//u32			cursorIndex;	// lookup into input system's cursor table
	};

	/**
	 * Active Input Context record
	 */
	struct ActiveInputContext {
		h32				contextId;
		u16				priority;
		u8				active;
		u8				_padding;
	};

	struct CallbackPriority {
		h32				callbackId;
		i32				priority;
	};

	
	typedef void InputCallbackFunc(FrameMappedInput&);
	typedef bool InputActionFunc(MappedAction&, InputContext&);
	typedef bool InputStateFunc(MappedState&, InputContext&);
	typedef bool InputAxisFunc(MappedAxis&, InputContext&);
	typedef bool InputTextFunc(FrameMappedInput&, InputContext&);
	
	DenseHandleMap16Typed(InputMapping, HandleMap_InputMapping, 0);
	DenseHandleMap16Typed(InputContext, HandleMap_InputContext, 1);
	DenseHandleMap16Typed(InputCallbackFunc*, HandleMap_InputCallback, 2);


	struct GameInput {
		HandleMap_InputMapping		inputMappings;	// collection of input mappings (actions,states,axes)
		HandleMap_InputContext		inputContexts;	// collection of input contexts
		HandleMap_InputCallback		callbacks;		// map storing all registered callbacks

		DenseHandleMap16Buffer(InputMapping, inputMappingsBuffer, GAMEINPUT_MAPPINGS_CAPACITY);
		DenseHandleMap16Buffer(InputContext, inputContextsBuffer, GAMEINPUT_CONTEXTS_CAPACITY);
		DenseHandleMap16Buffer(InputCallbackFunc*, callbacksBuffer, GAMEINPUT_CALLBACKS_CAPACITY);

		// active input contexts sorted by priority
		ActiveInputContext	activeInputContexts[GAMEINPUT_CONTEXTS_CAPACITY];
		// callback order sorted by priority
		CallbackPriority	callbackPriorityList[GAMEINPUT_CALLBACKS_CAPACITY];
		
		FrameMappedInput	frameMappedInput;	// per-frame mapped input buffer

		// table of mouse cursors
		SDL_Cursor*			cursors[_InputMouseCursorCount];
		// list of opened joysticks
		SDL_Joystick*		joysticks[GAMEINPUT_JOYSTICKS_CAPACITY];
		u8					joysticksSize;


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
		void updateFrameTick(const UpdateInfo& ui, PlatformInput& platformInput);


		// Input Mappings

		/**
		 * Slow function to get an input mapping id from its name, systems use this at startup
		 * and store the handle for later use
		 * @name		mapping name found in config/inputcontexts.json
		 * @return	Id of the mapping, null_h32 if name not found within context
		 */
		h32 getInputMappingHandle(const char* name, h32 contextId);
		

		/**
		 * Get index of active state in frame states array, -1 if not present.
		 */
		int findActiveState(h32 mappingId);


		// Input Contexts

		/**
		 * Create a context and get back its handle
		 */
		h32 createContext(u8 options, u8 priority, bool makeActive = false);

		/**
		 * Slow function to get an input context id from its name, systems use this at startup
		 * and store the handle for later use
		 * @name		context name found in config/inputcontexts.json
		 * @return	Id of the context, null_h32 if name not found
		 */
		h32 getInputContextHandle(const char* name);

		/**
		 * Set the InputContext, returns true on success
		 */
		bool setContextActive(h32 contextId, bool active = true, i8 priority = -1);

		// Callbacks
		
		h32 registerCallback(i32 priority, InputCallbackFunc* func);

		bool unregisterCallback(h32 callbackId);

		/**
		 * Convenience function for handling a single active mapped action event
		 * @param	callback	returns true if input was handled, false if not handled
		 * @return	true if a mapped action is found and handled by the callback
		 */
		bool handleInputAction(h32 mappingId,
							   FrameMappedInput& mappedInput,
							   InputActionFunc* callback);

		/**
		 * Convenience function for handling a single active mapped state event
		 * @param	callback	returns true if input was handled, false if not handled
		 * @return	true if a mapped state is found and handled by the callback
		 */
		bool handleInputState(h32 mappingId,
							  FrameMappedInput& mappedInput,
							  InputStateFunc* callback);

		/**
		 * Convenience function for handling a single active mapped axis event
		 * @param	callback	returns true if input was handled, false if not handled
		 * @return	true if a mapped axis is found and handled by the callback
		 */
		bool handleInputAxis(h32 mappingId,
							 FrameMappedInput& mappedInput,
							 InputAxisFunc* callback);

		/**
		 * Convenience function for handling text input events. Returns false if the context
		 *	asked for is not active or does not capture text input.
			*	Otherwise returns the result of callback.
			* @param	contextId	which input context you are capturing text for
			* @param	mappedInput	member wstring textInput contains the input text
			* @param	callback	returns true if text input was consumed, false if not consumed
			* @return	true if text input was consumed by the callback
			*/
		bool handleTextInput(h32 contextId,
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
		void mapFrameInputs(const UpdateInfo& ui);
		void mapFrameMotion(const UpdateInfo& ui);
	};

}

#endif