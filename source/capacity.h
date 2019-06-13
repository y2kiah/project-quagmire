// Storage first block size
#define INIT_TRANSIENT_BLOCK_MEGABYTES      		64
#define INIT_FRAMESCOPED_BLOCK_MEGABYTES        	64

// File search recursion
#define MAX_FILE_RECURSION_DEPTH                    10

// Logger
// maximum number of messages that can build up in concurrent queue before it is flushed (per frame)
#define LOGGER_CAPACITY				                50

// Game Input
#define PLATFORMINPUT_EVENTSQUEUE_CAPACITY			64
#define PLATFORMINPUT_EVENTSPOPQUEUE_CAPACITY		128
#define PLATFORMINPUT_MOTIONEVENTSQUEUE_CAPACITY	128
#define PLATFORMINPUT_MOTIONEVENTSPOPQUEUE_CAPACITY	256
//#define GAMEINPUT_CALLBACKS_CAPACITY				10
#define GAMEINPUT_MAX_JOYSTICKS						64
#define GAMEINPUT_MAX_AXES							256

// Assets
#define ASSET_PACKS_CAPACITY						16

// Scene
// TODO: should these be smaller and we would have multiple spatial stores?
#define SCENE_MAX_ENTITIES							USHRT_MAX-1
#define SCENE_MAX_CAMERAS							64
/**
 * TODO: make multiple active cameras supported
 * This is the max number of active cameras for any one frame of a rendered scene. This
 * number includes cameras needed for rendering all viewports and shadow frustums. The
 * frustum culling results are stored in a 4-byte bitset, hence this limitation.
 */
#define SCENE_MAX_ACTIVE_CAMERAS					32
#define SCENE_MAX_LIGHTS							1024
#define SCENE_MAX_SCREEN_SHAKE_PRODUCERS			1024