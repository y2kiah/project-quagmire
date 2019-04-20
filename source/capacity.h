// Transient Storage
// TODO: temporary, these should be replaced by a linked list of 4K blocks, allowing memory to grow without too much waste
#define FRAMESCOPED_MEGABYTES	64
#define TRANSIENT_MEGABYTES		64

// Logger
#define LOGGER_CAPACITY		50		// maximum number of messages that can build up in concurrent queue before it is flushed (per frame)

// Game Input
#define PLATFORMINPUT_EVENTSQUEUE_CAPACITY			100
#define PLATFORMINPUT_MOTIONEVENTSQUEUE_CAPACITY	150
#define GAMEINPUT_MAPPINGS_CAPACITY					64
#define GAMEINPUT_CONTEXTS_CAPACITY					32
#define GAMEINPUT_CALLBACKS_CAPACITY				10
#define GAMEINPUT_ACTION_CAPACITY					64
#define GAMEINPUT_STATE_CAPACITY					64
#define GAMEINPUT_AXIS_CAPACITY						64
#define GAMEINPUT_JOYSTICKS_CAPACITY				64
#define INPUTCONTEXT_MAPPINGS_CAPACITY				64
