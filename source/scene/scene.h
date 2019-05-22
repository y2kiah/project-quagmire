#ifndef _SCENE_H_
#define _SCENE_H_

#include "../utility/common.h"
#include "../math/math_core.h"
#include "../math/vec3.h"
#include "../utility/dense_handle_map_16.h"
#include <intrin.h>


/**
 * SpatialHandle represents an entry into a single cell of the SpatialMap. Each cell of SpatialMap
 * holds an embedded linked list of SpatialValue's. The front of the list is stored in the
 * spatialCells array.
 */
typedef h32 SpatialHandle;


struct SpatialCell {
	u8 x, y, z;

	bool operator==(SpatialCell& c) { return x == c.x && y == c.y && z == c.z; }
	bool operator>=(SpatialCell& c) { return x >= c.x && y >= c.y && z >= c.z; }
	bool operator<=(SpatialCell& c) { return x <= c.x && y <= c.y && z <= c.z; }
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

const i16 gridSizeX = 256;
const i16 gridSizeY = 16;
const i16 gridSizeZ = 256;

f32 spatialGridSizeX = 1000.0f;
f32 spatialGridSizeY = 10000.0f;
f32 spatialGridSizeZ = 1000.0f;

vec3 invSpatialGridSizeXYZ = vec3{
	1.0f / spatialGridSizeX,
	1.0f / spatialGridSizeY,
	1.0f / spatialGridSizeZ
};

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


void updateSpatialKey(
	SpatialKey& prevKey,
	SpatialKey& newKey,
	h32 entityOrComponentId)
{
	if (prevKey.cs == prevKey.ce &&
		!(prevKey.cs >= newKey.cs && prevKey.cs <= newKey.ce))
	{
		removeFromSpatialCell(prevKey.cs, entityOrComponentId);
	}
	else {
		for (u8 y = prevKey.cs.y; y <= prevKey.ce.y; ++y) {
			for (u8 z = prevKey.cs.z; z <= prevKey.ce.z; ++z) {
				for (u8 x = prevKey.cs.x; x <= prevKey.ce.x; ++x) {
					SpatialCell cell{ x, y, z };
					// remove from cell if the cell isn't also in the new key
					if (!(cell >= newKey.cs && cell <= newKey.ce)) {
						removeFromSpatialCell(cell, entityOrComponentId);
					}
				}
			}
		}
	}

	if (newKey.cs == newKey.ce &&
		!(newKey.cs >= prevKey.cs && newKey.cs <= prevKey.ce))
	{
		addToSpatialCell(newKey.cs, entityOrComponentId);
	}
	else {
		for (u8 y = newKey.cs.y; y <= newKey.ce.y; ++y) {
			for (u8 z = newKey.cs.z; z <= newKey.ce.z; ++z) {
				for (u8 x = newKey.cs.x; x <= newKey.ce.x; ++x) {
					SpatialCell cell{ x, y, z };
					// add to cell if the cell isn't also in the prev key
					if (!(cell >= prevKey.cs && cell <= prevKey.ce)) {
						addToSpatialCell(cell, entityOrComponentId);
					}
				}
			}
		}
	}
}


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


/**
 * We know one side will be straight with the longest y diff, find that side, could be right or
 * left (or both if one side is horizontal). The opposite side will have two shorter y spans.
 *       L_s == S1_s
 *       ^
 *      / \ <=S1
 * L=> /   \
 *    /   ..' S == S1_e == S2_s
 *   /..''  <=S2
 *  L_e == S2_e
 * 
 * sets lowest and highest bit set for both x and y over the whole triangle into outLowX, outLowY
 */
void scanConvertTriangle(
	vec2& L_s, // long side start
	vec2& L_e, // long side end
	vec2& S,   // short side intersection
	u64* plane,
	i16 planeSizeX, i16 planeSizeY,
	i16& outLowX, i16& outHighX,
	i16& outLowY, i16& outHighY)
{
	i16 Ly_s = (i16)L_s.y;
	i16 Ly_e = (i16)L_e.y;
	i16 S1y_s = Ly_s;
	i16 S1y_e = (i16)S.y;
	i16 S2y_s = S1y_e;
	i16 S2y_e = Ly_e;
	r32 Lx_step  = (L_e.x - L_s.x) / (Ly_e - Ly_s);
	r32 S1x_step = (S.x - L_s.x) / (S1y_e - S1y_s);
	r32 S2x_step = (L_e.x - S.x) / (S2y_e - S2y_s);

	// for each y row, find low and high x, fill bits between them inclusive
	r32 Lx  = L_s.x + (Ly_s < 0 ? -Ly_s * Lx_step : 0.0f);
	r32 S1x = L_s.x + (S1y_s < 0 ? -S1y_s * S1x_step : 0.0f);
	r32 S2x = S.x + (S2y_s < 0 ? -S2y_s * S2x_step : 0.0f);
	
	for (i16 y = max(Ly_s, (i16)0);
		y <= min(Ly_e, (i16)(planeSizeY-1));
		++y)
	{
		i16 x1 = (i16)Lx;
		Lx += Lx_step;
		
		i16 x2 = x1;
		if (y < S1y_e) {
			x2 = (i16)S1x;
			S1x += S1x_step;
		}
		else {
			x2 = (i16)S2x;
			S2x += S2x_step;
		}
		
		i16 lowX = (x1 < x2 ? x1 : x2);
		i16 highX = (x1 < x2 ? x2 : x1);

		i16 planeIndex = y * planeSizeX / 64;

		for (i16 xBits = 0;
			xBits < planeSizeX;
			xBits += 64)
		{
			if (highX >= xBits
				&& lowX < xBits + 64)
			{
				// set all bits between low and high bit index
				// example using u8 bytes:
				//              13-8    20-16
				// |--------|-----***|*****---|--------|
				//                5 7|0   4
				u16 lowBit = max(lowX - xBits, 0);
				u16 highBit = min(highX - xBits, 63);
				u64 bitset = (u64)((2 << highBit) - (1 << lowBit));
				
				plane[planeIndex] |= bitset;
				++planeIndex; 
			}
		}

		outLowX  = min(outLowX, lowX);
		outHighX = max(outHighX, highX);
	}
	outLowY  = min(outLowY, Ly_s);
	outHighY = max(outHighY, Ly_e);
}


void rasterizeTriangle(
	vec2 A,
	vec2 B,
	vec2 C,
	u64* plane,
	i16 planeSizeX, i16 planeSizeY,
	i16& outLowX, i16& outHighX,
	i16& outLowY, i16& outHighY)
{
	assert(plane);

	r32 ABy = fabs(B.y - A.y);
	r32 BCy = fabs(C.y - B.y);
	r32 CAy = fabs(A.y - C.y);

	if (ABy >= BCy && ABy >= CAy) {
		// AB is the long side
		if (A.y < B.y) {
			scanConvertTriangle(
				A, B, C,
				plane, planeSizeX, planeSizeY,
				outLowX, outHighX, outLowY, outHighY);
		}
		else {
			scanConvertTriangle(
				B, A, C,
				plane, planeSizeX, planeSizeY,
				outLowX, outHighX, outLowY, outHighY);
		}
	}
	else if (BCy >= ABy && BCy >= CAy) {
		// BC is the long side
		if (B.y < C.y) {
			scanConvertTriangle(
				B, C, A,
				plane, planeSizeX, planeSizeY,
				outLowX, outHighX, outLowY, outHighY);
		}
		else {
			scanConvertTriangle(
				C, B, A,
				plane, planeSizeX, planeSizeY,
				outLowX, outHighX, outLowY, outHighY);
		}
	}
	else {
		// CA is the long side
		if (C.y < A.y) {
			scanConvertTriangle(
				C, A, B,
				plane, planeSizeX, planeSizeY,
				outLowX, outHighX, outLowY, outHighY);
		}
		else {
			scanConvertTriangle(
				A, C, B,
				plane, planeSizeX, planeSizeY,
				outLowX, outHighX, outLowY, outHighY);
		}
	}
}


/**
 * Takes frustum points in world space and projects the top, right and left triangles onto the
 * three axis-aligned planes xz, xy and zy. The triangle vertices are divided into grid space
 * but kept in floating point. Then, each triangle is rasterized in grid space integer coords 
 * and clipped to grid boundaries. The resulting bits are stored in the xz, xy and zy planes of a
 * SpatialCellProjections object.
 */
void scanConvertFrustum(
	SpatialCellProjections& cp)
{
	cp = {}; // reset to default

	// get frustum into grid space
	// TODO: if grid origin ever moves from world origin, need a translation here as well
	vec3 fo, tr, tl, br, bl;
	fo *= invSpatialGridSizeXYZ;
	tr *= invSpatialGridSizeXYZ;
	tl *= invSpatialGridSizeXYZ;
	br *= invSpatialGridSizeXYZ;
	bl *= invSpatialGridSizeXYZ;
	
	// xz plane
	rasterizeTriangle( // frustum top
		fo.xz, tr.xz, tl.xz,
		cp.xz, gridSizeX, gridSizeZ,
		cp.lowX, cp.highX, cp.lowZ, cp.highZ);

	rasterizeTriangle( // frustum left
		fo.xz, tl.xz, bl.xz,
		cp.xz, gridSizeX, gridSizeZ,
		cp.lowX, cp.highX, cp.lowZ, cp.highZ);

	rasterizeTriangle( // frustum right
		fo.xz, br.xz, tr.xz,
		cp.xz, gridSizeX, gridSizeZ,
		cp.lowX, cp.highX, cp.lowZ, cp.highZ);

	// xy plane
	rasterizeTriangle(
		fo.xy, tr.xy, tl.xy,
		cp.xy, gridSizeX, gridSizeY,
		cp.lowX, cp.highX, cp.lowY, cp.highY);

	rasterizeTriangle(
		fo.xy, tl.xy, bl.xy,
		cp.xy, gridSizeX, gridSizeY,
		cp.lowX, cp.highX, cp.lowY, cp.highY);

	rasterizeTriangle(
		fo.xy, br.xy, tr.xy,
		cp.xy, gridSizeX, gridSizeY,
		cp.lowX, cp.highX, cp.lowY, cp.highY);

	// zy plane
	rasterizeTriangle(
		fo.zy, tr.zy, tl.zy,
		cp.zy, gridSizeZ, gridSizeY,
		cp.lowZ, cp.highZ, cp.lowY, cp.highY);

	rasterizeTriangle(
		fo.zy, tl.zy, bl.zy,
		cp.zy, gridSizeZ, gridSizeY,
		cp.lowZ, cp.highZ, cp.lowY, cp.highY);

	rasterizeTriangle(
		fo.zy, br.zy, tr.zy,
		cp.zy, gridSizeZ, gridSizeY,
		cp.lowZ, cp.highZ, cp.lowY, cp.highY);
}


void getPVSFromProjections(
	SpatialCellProjections& cp)
{
	// run loops from lowY to highY, lowZ to highZ, lowX/64 to highX/64

	for (i16 y = max(cp.lowY, (i16)0);
		y <= min(cp.highY, (i16)(gridSizeY-1));
		++y)
	{
		for (i16 z = max(cp.lowZ, (i16)0);
			z <= min(cp.highZ, (i16)(gridSizeZ-1));
			++z)
		{
			// if bit is set in the zy plane
			i16 zyBit = ((cp.zy[y*4+z/64] >> (z & 63)) & 0x01);
			if (zyBit) {
				for (i16 xBits = max(cp.lowX, (i16)0) / 64;
					xBits <= min(cp.highX, (i16)(gridSizeX-1)) / 64;
					++xBits)
				{
					// test xz,xy intersection with bitwise AND
					u64 bitset = cp.xz[z*4+xBits] & cp.xy[y*4+xBits];
					
					u32 lowX = 0;
					u32 highX = 0;
					u8 set = 0;
					
					#ifdef _MSC_VER
					set = _BitScanForward64((unsigned long *)&lowX, bitset);
					set = _BitScanReverse64((unsigned long *)&highX, bitset);
					#else
					static_assert(false, "need to implement");
					#endif
					
					if (set) {
						for (i16 x = lowX + (xBits*64);
							x <= highX + (xBits*64);
							++x)
						{
							u32 idx = getSpatialIndex(SpatialCell{ x, y, z });
							// add all objects in cell to the
							SpatialHandle hnd = spatialCells[idx];
							while (hnd != null_h32) {
								SpatialValue* val = spatialMap[hnd];
								if (!val) {
									break;
								}
								// TODO: add to AABB data
								//val->entityOrComponentId;
								hnd = val->next;
							}
						}
					}
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