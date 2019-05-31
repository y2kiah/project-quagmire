#ifndef _SCENE_H_
#define _SCENE_H_

#include "../capacity.h"
#include "../utility/common.h"
#include "../math/math_core.h"
#include "../math/vec3.h"
#include "../utility/dense_handle_map_16.h"
#include "geometry.h"
#include "entity.h"
#include "scene_components.h"


const i16 gridSizeX = 256;
const i16 gridSizeY = 16;
const i16 gridSizeZ = 256;

const r32 spatialGridSizeY = 10000.0f;
const r32 spatialGridSizeXZ = 1000.0f;

// scalar for Y axis into XZ space (used for frustum-sphere culling of spatial grid cells)
const r32 spatialGridSize_XZ_Y_ratio = spatialGridSizeXZ / spatialGridSizeY;
// radius for the bounding sphere containing a grid cell
const r32 spatialGridCellRadius = SQRT_2 * spatialGridSizeXZ * 0.5f;

const vec3 invSpatialGridSizeXYZ = vec3{
	1.0f / spatialGridSizeXZ,
	1.0f / spatialGridSizeY,
	1.0f / spatialGridSizeXZ
};

const int SpatialGridSize = gridSizeX * gridSizeY * gridSizeZ;

/**
 * SpatialHandle represents an entry into a single cell of the SpatialMap. Each cell of SpatialMap
 * holds an embedded linked list of SpatialValue's. The front of the list is stored in the
 * spatialCells array.
 */
typedef h32 SpatialHandle;


struct SpatialCell {
	u8 x, y, z;

	bool operator==(const SpatialCell& c) const { return x == c.x && y == c.y && z == c.z; }
	bool operator>=(const SpatialCell& c) const { return x >= c.x && y >= c.y && z >= c.z; }
	bool operator<=(const SpatialCell& c) const { return x <= c.x && y <= c.y && z <= c.z; }
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
	u8				_padding;
	// TODO: should we store a copy of position and radius here for an eventual frustum check, we could iterate
	// the values in a cell adding them to SoA data and then do 4 checks at a time against the frustum using sse
	// but... dynamic objects will have to update all of the copies for each cell they exist in
};


struct SpatialCellProjections {
	u64		xz[gridSizeZ * (gridSizeX / 64)] = {};
	u64		xy[gridSizeY * (gridSizeX / 64)] = {};
	u64		zy[gridSizeY * (gridSizeZ / 64)] = {};
	i16		lowX  = INT16_MAX;
	i16		highX = INT16_MIN;
	i16		lowY  = INT16_MAX;
	i16		highY = INT16_MIN;
	i16		lowZ  = INT16_MAX;
	i16		highZ = INT16_MIN;
};


struct EntityPVS {
	// TODO: the bSphere is stored for a later occlusion culling step, do we even need this?
	Sphere	bSphere[SCENE_MAX_ENTITIES];
	h32		entityOrComponentId[SCENE_MAX_ENTITIES];
	u32		length;
	// TODO: a change in culling function might require SoA data layout
	//r32		x [SCENE_MAX_ENTITIES];
	//r32		y [SCENE_MAX_ENTITIES];
	//r32		z [SCENE_MAX_ENTITIES];
	//r32		radius [SCENE_MAX_ENTITIES];
};


DenseHandleMap16TypedWithBuffer(
	SpatialValue,
	SpatialValueMap,
	SpatialHandle,
	0,
	SCENE_MAX_ENTITIES);


// TODO: rename SpatialWorldChunk or something?
struct SpatialPersistentStorage {
	SpatialHandle	cells[SpatialGridSize];	// each cell stores front of linked list of SpatialValues in the map
	SpatialValueMap	valueMap;				// map stores SpatialValues representing entities in the world 
};


/**
 * SpatialTransientStorage is used per frame for object frustum culling.
 */
struct SpatialTransientStorage {
	SpatialCellProjections	cellProj;					// cell projection data used for frustum visibility check
	SpatialCell				cellPVS[SpatialGridSize];	// resulting dataset from running cell projection algorithm
	u32						cellPVSLength;
	EntityPVS				entityPVS;					// resulting dataset from running bsphere checks after cell projection
};


#define SCENE_MAX_CAMERAS		64
/**
 * TODO: make multiple active cameras supported
 * This is the max number of active cameras for any one frame of a rendered scene. This
 * number includes cameras needed for rendering all viewports and shadow frustums. The
 * frustum culling results are stored in a 4-byte bitset, hence this limitation.
 */
#define SCENE_MAX_ACTIVE_CAMERAS	32
#define SCENE_MAX_LIGHTS			1024

DenseHandleMap16TypedWithBuffer(Entity, EntityMap, EntityId, 0, SCENE_MAX_ENTITIES);

/**
 * 
 */
struct Scene {
	SceneNode 	root;	// root of the scene graph, traversal starts from here
	//CameraList	cameras;
	i32			activeRenderCamera = -1;
	// TODO: contains layer id for RenderEntry???

	EntityMap	entities;

	struct Components
	{
		ComponentStore(SceneNode,      SceneNodeMap,      SceneNodeId,  0, sceneNodes,      SCENE_MAX_ENTITIES);
		ComponentStore(Movement,       MovementMap,       ComponentId,  1, movement,        SCENE_MAX_ENTITIES);
		ComponentStore(CameraInstance, CameraInstanceMap, ComponentId,  2, cameraInstances, SCENE_MAX_CAMERAS);
		ComponentStore(ModelInstance,  ModelInstanceMap,  ComponentId,  3, modelInstances,  SCENE_MAX_ENTITIES);
		ComponentStore(LightInstance,  LightInstanceMap,  ComponentId,  4, lightInstances,  SCENE_MAX_LIGHTS);
	}
	components;

	SpatialPersistentStorage	spatial;
};


// Functions

SpatialCell getSpatialCell(
	r32 x, r32 y, r32 z)
{
	return SpatialCell{
		// TODO: can use simd here
		// note: trunc behavior for negative numbers here is fine since we are clamping to 0 anyway
		(u8)clamp((i32)(x / spatialGridSizeXZ), 0, gridSizeX-1),
		(u8)clamp((i32)(y / spatialGridSizeY),  0, gridSizeY-1),
		(u8)clamp((i32)(z / spatialGridSizeXZ), 0, gridSizeZ-1)
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


u32 getSpatialIndex(
	const SpatialCell& cell)
{
	return (cell.y * gridSizeX * gridSizeZ) + (cell.z * gridSizeX) + cell.x;
}


void addToSpatialCell(
	const SpatialCell& cell,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps);


/**
 * Add an entity to spatial grid cell(s) based on the key passed in. If the entity has already been
 * added, use the updateSpatialKey function to move it and removeFromSpatialMap to remove it.
 */
void addToSpatialMap(
	const SpatialKey& key,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps);


void removeFromSpatialCell(
	const SpatialCell& cell,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps);


/**
 * Remove an entity from all cells that contain it, based on the key passed in.
 */
void removeFromSpatialMap(
	const SpatialKey& key,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps);


/**
 * Updates the cell(s) containing an entity by removing based on the old key and adding based on
 * a new key. Overlapping cells in the old and new set are not changed.
 */
void updateSpatialKey(
	const SpatialKey& prevKey,
	const SpatialKey& newKey,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps);




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


#endif