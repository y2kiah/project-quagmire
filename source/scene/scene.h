#ifndef _SCENE_H_
#define _SCENE_H_

#include "../utility/common.h"
#include "../math/math_core.h"
#include "../math/vec3.h"
#include "../utility/dense_handle_map_16.h"


/**
 * SpatialHandle represents an entry into a single cell of the SpatialMap. Each cell of SpatialMap
 * holds an embedded linked list of SpatialValue's. The front of the list is stored in the
 * spatialCells array.
 */
typedef h32 SpatialHandle;

struct SpatialCell {
	u8 x, y, z;

	bool operator==(SpatialCell& c) { return x == c.x && y == c.y && z == c.z; }
};

/**
 * SpatialKey is used for lookups into the spatial hash map. The key holds two cells, a start and
 * end, which represents a 3D block of cells that fall within the range (inclusive).
 */
struct SpatialKey {
	SpatialCell cs, ce;
};

struct SpatialValue {
	// TODO: determine where this will point, maybe just a u16 index into SpatialHashAABBData SoA data
	h32				entityOrComponentId;
	SpatialHandle	next;	// handle to next SpatialValue in the grid cell
	SpatialCell		cell;
	// TODO: should we store a copy of position and radius here for an eventual frustum check, we could iterate
	// the values in a cell adding them to an AoS of data and then do 4 checks at a time against the frustum using sse
	// but... dynamic objects will have to update all of the copies for each cell they exist in
};

struct SpatialHashAABBData {
	r32*	x;
	r32*	y;
	r32*	z;
};


DenseHandleMap16Typed(SpatialValue, SpatialValueMap, 0);
SpatialValueMap spatialMap; // TODO: needs a home and init

const int gridSizeX = 256;
const int gridSizeY = 16;
const int gridSizeZ = 256;

f32 spatialGridSizeX = 1000.0f;
f32 spatialGridSizeY = 10000.0f;
f32 spatialGridSizeZ = 1000.0f;
const int SpatialMapSize = gridSizeX * gridSizeY * gridSizeZ;

// TODO: should be in game state memory, not on the stack
SpatialHandle spatialCells[SpatialMapSize];


SpatialCell getSpatialCell(
	r32 x, r32 y, r32 z)
{
	return SpatialCell{
		// TODO: can use simd here
		// note: trunc behavior for negative numbers here is fine since we are clamping to 0 anyway
		(u8)clamp((i32)(x / spatialGridSizeX), 0, gridSizeX-1),
		(u8)clamp((i32)(y / spatialGridSizeY), 0, gridSizeY-1),
		(u8)clamp((i32)(z / spatialGridSizeZ), 0, gridSizeZ-1)
	};
}

SpatialKey getSpatialKeyForAABB(
	vec3& vs,
	vec3& ve)
{
	return SpatialKey{
		getSpatialCell(vs.x, vs.y, vs.z),
		getSpatialCell(ve.x, ve.y, ve.z)
	};
}

u32 getSpatialIndex(SpatialCell& cell)
{
	return (cell.y * gridSizeX * gridSizeZ) + (cell.z * gridSizeX) + cell.x;
}

void addToSpatialCell(
	SpatialCell& cell,
	h32 entityOrComponentId)
{
	SpatialValue val;
	val.entityOrComponentId = entityOrComponentId;
	val.cell = cell;
	u32 idx = getSpatialIndex(cell);
	SpatialHandle front = spatialCells[idx];
	
	val.next = front;
	SpatialHandle newFront = spatialMap.insert(&val);
	spatialCells[idx] = newFront;
}

void addToSpatialMap(
	SpatialKey& key,
	h32 entityOrComponentId)
{
	if (key.cs == key.ce) {
		addToSpatialCell(key.cs, entityOrComponentId);
	}
	else {
		for (u8 y = key.cs.y; y <= key.ce.y; ++y) {
			for (u8 z = key.cs.z; z <= key.ce.z; ++z) {
				for (u8 x = key.cs.x; x <= key.ce.x; ++x) {
					addToSpatialCell(
						{ x, y, z },
						entityOrComponentId);
				}
			}
		}
	}
}

void removeFromSpatialCell(
	SpatialCell& cell,
	h32 entityOrComponentId)
{
	u32 idx = getSpatialIndex(cell);
	SpatialHandle curr = spatialCells[idx];

	SpatialHandle prev = null_h32;
	SpatialValue* prevVal = nullptr;

	while (curr != null_h32) {
		SpatialValue* val = spatialMap[curr];
		
		if (val->entityOrComponentId == entityOrComponentId)
		{
			if (prevVal) {
				prevVal->next = val->next;
			}
			else {
				spatialCells[idx] = val->next;
			}

			spatialMap.erase(curr);
			break;
		}
		
		prev = curr;
		prevVal = val;
		curr = val->next;
	}
}

void removeFromSpatialMap(
	SpatialKey& key,
	h32 entityOrComponentId)
{
	if (key.cs == key.ce) {
		removeFromSpatialCell(key.cs, entityOrComponentId);
	}
	else {
		for (u8 y = key.cs.y; y <= key.ce.y; ++y) {
			for (u8 z = key.cs.z; z <= key.ce.z; ++z) {
				for (u8 x = key.cs.x; x <= key.ce.x; ++x) {
					removeFromSpatialCell(
						{ x, y, z },
						entityOrComponentId);
				}
			}
		}
	}
}

/*
bitwise quadtree pseudo code
-- build the slots
each bucket stores 
1024*1024

spatial hash, external chaining pseudo code
-------------------------------------------
-- build the table --
define a world size for each cell in a 256x256 table, for example 1000ft x 1000ft
the hash key combines those grid coords to a single 16-bit 1 dimensional has key 
4 bytes per slot, an h32 to the first handle in a linked list of objects stored in a SparseHandleMap16
OR (see discussion below) directly store a handle to the collision component which includes
objects in the handle map store:
	-?? copy of the worldspace aabb (or whatever local coord space is appropriate) from the collision component pointed to,
		this would give faster collision tests by eliminating the extra lookup / cache miss, but also means every time the object
		moves it will have to iterate the lists of all cells it belongs to and update the copy, which may negate the gains made.
	- the hash table index (16-bit hash key) that the linked list originates from (important for clearing the value when the list is empty)
	- a 16 bit vertical layer mask (from vertical worldspace coordinate), for quick collision checks later with a bitwise and
	- link to next h32 in linked list (a 16 bit index directly into the sparse array, we don't need the full 32 bit handle)
	- h32 to the entity or collision component representing the actual object, this should contain the actual bounding volume(s) to be checked against
when an object's aabb spans a grid cell boundary, it must exist in ALL slots from (sx,sz) to (ex,ez), this means there might be multiple
linked list entries for a single object in the world, one for each grid cell it currently overlaps.
when an object is removed from the world, it must iterate all grid linked lists looking for its own entry and remove its own entry from each
(the entity will know its own collision h32 handle to compare with)

the x16 are vertical layers, since objects are most likely distributed horizontally, we can get away with fewer vertical slices

things to consider, 1) do we really need the full h32 handle from hash storage to the sparse array, could this just be the 16 bit index?
would we ever store and use the handle produced by inserting a value into the sparsehandlemap16, or is the important thing just that there
is the embedded freelist?
2) if we enforce the maximum size for all collision objects in the game to never exceed the grid size, then an object could never occupy
more than 4 cells at a time, so we could discard the need for it to exist in multiple linked lists and just store it in one list based
on the origin coordinates. large objects would just have to be broken up before coming into the game. for collision checks, we have to
check the grid our volume occupies, plus the neighboring cells depending on which side of the center of each cell the volume falls on.

-- get the key
get aabb of object, (sx,sy,sz),(ex,ey,ez)
floor each component, right shift down to fit # slots




*/


enum CameraType : u8 {
	Camera_Perspective = 0,
	Camera_Ortho,
	Camera_Stereo
};

struct CameraParams {
	r32		    nearClipPlane;
	r32		    farClipPlane;
	u32 	    viewportWidth;
	u32 	    viewportHeight;
	r32		    verticalFieldOfViewDegrees;
	CameraType	cameraType;
	u8		    _padding_end[3];
};


/**
 * TODO: make multiple active cameras supported
 * This is the max number of active cameras for any one frame of a rendered scene. This
 * number includes cameras needed for rendering all viewports and shadow frustums. The
 * frustum culling results are stored in a 4-byte bitset, hence this limitation.
 */
#define SCENE_MAX_ACTIVE_CAMERAS	32

/*
struct Scene
{
	EntityManagerPtr	entityManager;
	SceneGraphPtr		sceneGraph;
	CameraList			cameras;
	
	// contains Lua state?
	// contains layer id for RenderEntry???

	i32				    activeRenderCamera = -1;
	bool				active = false;
	std::string			name;

	// Functions

	void getVisibleEntities();

	u32 createCamera(const CameraParams& cameraParams, bool makeActive = false);
	
	u32 getActiveCamera() const
	{
		return activeRenderCamera;
	}

	void setActiveCamera(u32 cameraId)// TODO: take a viewport index
	{
		activeRenderCamera = cameraId;
	}

	explicit Scene(const std::string& _name, bool _active);
	~Scene();
};


class SceneManager
{
	// Public Functions

	explicit SceneManager();
	~SceneManager();

	SceneId createScene(const std::string& name, bool makeActive);

	void updateActiveScenes();
	void renderActiveScenes(r32 interpolation);

	void frustumCullActiveScenes();

	// Private Variables

	handle_map<Scene> m_scenes;
};
*/

#endif