#include "scene.h"
#include "../utility/intrinsics.h"
#include "geometry.h"
#include "intersection.h"


void addToSpatialCell(
	const SpatialCell& cell,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps)
{
	SpatialValue val;
	val.entityOrComponentId = entityOrComponentId;
	val.cell = cell;
	u32 idx = getSpatialIndex(cell);
	SpatialHandle front = sps.cells[idx];
	
	val.next = front;
	SpatialHandle newFront = sps.valueMap.insert(&val);
	sps.cells[idx] = newFront;
}


void addToSpatialMap(
	const SpatialKey& key,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps)
{
	if (key.cs == key.ce) {
		addToSpatialCell(key.cs, entityOrComponentId, sps);
	}
	else {
		for (u8 y = key.cs.y; y <= key.ce.y; ++y) {
			for (u8 z = key.cs.z; z <= key.ce.z; ++z) {
				for (u8 x = key.cs.x; x <= key.ce.x; ++x) {
					addToSpatialCell(
						SpatialCell{ x, y, z },
						entityOrComponentId,
						sps);
				}
			}
		}
	}
}


void removeFromSpatialCell(
	const SpatialCell& cell,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps)
{
	u32 idx = getSpatialIndex(cell);
	SpatialHandle curr = sps.cells[idx];

	SpatialHandle prev = null_h32;
	SpatialValue* prevVal = nullptr;

	while (curr != null_h32) {
		SpatialValue* val = sps.valueMap[curr];
		
		if (val->entityOrComponentId == entityOrComponentId)
		{
			if (prevVal) {
				prevVal->next = val->next;
			}
			else {
				sps.cells[idx] = val->next;
			}

			sps.valueMap.erase(curr);
			break;
		}
		
		prev = curr;
		prevVal = val;
		curr = val->next;
	}
}


void removeFromSpatialMap(
	const SpatialKey& key,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps)
{
	if (key.cs == key.ce) {
		removeFromSpatialCell(key.cs, entityOrComponentId, sps);
	}
	else {
		for (u8 y = key.cs.y; y <= key.ce.y; ++y) {
			for (u8 z = key.cs.z; z <= key.ce.z; ++z) {
				for (u8 x = key.cs.x; x <= key.ce.x; ++x) {
					removeFromSpatialCell(
						SpatialCell{ x, y, z },
						entityOrComponentId,
						sps);
				}
			}
		}
	}
}


void updateSpatialKey(
	const SpatialKey& prevKey,
	const SpatialKey& newKey,
	h32 entityOrComponentId,
	SpatialPersistentStorage& sps)
{
	if (prevKey.cs == prevKey.ce &&
		!(prevKey.cs >= newKey.cs && prevKey.cs <= newKey.ce))
	{
		removeFromSpatialCell(prevKey.cs, entityOrComponentId, sps);
	}
	else {
		for (u8 y = prevKey.cs.y; y <= prevKey.ce.y; ++y) {
			for (u8 z = prevKey.cs.z; z <= prevKey.ce.z; ++z) {
				for (u8 x = prevKey.cs.x; x <= prevKey.ce.x; ++x) {
					SpatialCell cell{ x, y, z };
					// remove from cell if the cell isn't also in the new key
					if (!(cell >= newKey.cs && cell <= newKey.ce)) {
						removeFromSpatialCell(cell, entityOrComponentId, sps);
					}
				}
			}
		}
	}

	if (newKey.cs == newKey.ce &&
		!(newKey.cs >= prevKey.cs && newKey.cs <= prevKey.ce))
	{
		addToSpatialCell(newKey.cs, entityOrComponentId, sps);
	}
	else {
		for (u8 y = newKey.cs.y; y <= newKey.ce.y; ++y) {
			for (u8 z = newKey.cs.z; z <= newKey.ce.z; ++z) {
				for (u8 x = newKey.cs.x; x <= newKey.ce.x; ++x) {
					SpatialCell cell{ x, y, z };
					// add to cell if the cell isn't also in the prev key
					if (!(cell >= prevKey.cs && cell <= prevKey.ce)) {
						addToSpatialCell(cell, entityOrComponentId, sps);
					}
				}
			}
		}
	}
}


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
				u16 lowBit = max((i16)(lowX - xBits), (i16)0);
				u16 highBit = min((i16)(highX - xBits), (i16)63);
				u64 bitset = (2UL << highBit) - (1UL << lowBit);
				
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
	const Frustum& f,
	SpatialCellProjections& cp)
{
	cp = SpatialCellProjections(); // reset to default (not just zeroing memory)

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


/**
 * Once the three axis aligned buffers contain the rasterized frustum, the bits must be tested for
 * intersection to be determined visible. A cell x,y,z must have its bit set in all three of the
 * orthogonal projections. To do this, we loop over pixels in yz, and if set, bitwise AND the rows
 * of xz and xy together to test 64 cells at a time. We only loop over the box of cells ranging
 * from low to high coords determined during rasterization, to touch as little memory as needed.
 * Cells within the projected region that contain entities are added to the cellPVS.
 */
void getCellPVSFromProjections(
	SpatialTransientStorage& sts,
	SpatialPersistentStorage& sps)
{
	SpatialCellProjections& cp = sts.cellProj;
	sts.cellPVSLength = 0;

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
					
					u8 set = BitScanFwd64(&lowX, bitset)
						  && BitScanRev64(&highX, bitset);
					
					if (set) {
						for (u32 x = lowX + (xBits*64);
							x <= highX + (xBits*64);
							++x)
						{
							SpatialCell cell{ (u8)x, (u8)y, (u8)z };
							u32 idx = getSpatialIndex(cell);
							
							SpatialHandle hnd = sps.cells[idx];
							// if the cell contains objects, add it to the cell PVS
							if (hnd != null_h32)
							{
								sts.cellPVS[sts.cellPVSLength++] = cell;
							}
						}
					}
				}
			}
		}
	}
}


/**
 * Each cell in the cellPVS (determined by projection/rasterization algorithm) is then bsphere
 * tested against the frustum to see if it intersects the boundary, or is fully contained. If fully
 * contained, the cell's entities are added to the entityPVS. If the cell's bsphere intersects,
 * entities are individually bsphere tested and added to the entityPVS. If the cell's bsphere does
 * not intersect, assert since that indicates a bug in the projection code.
 */
void getEntityPVSFromCellPVS(
	const Frustum& f,
	SpatialTransientStorage& sts,
	SpatialPersistentStorage& sps)
{
	EntityPVS& ePVS = sts.entityPVS;
	ePVS.length = 0;

	// transform the frustum planes into a "homogeneous grid space" which is basically world space
	// with a squashed Y axis so the grid cell is a cube
	Frustum f_hgs = f;
	for (int p = 0; p < 6; ++p) {
		f_hgs.ny[p] *= spatialGridSize_XZ_Y_ratio;
	}
	plane_normalize_4_sse(f_hgs.nx, f_hgs.ny, f_hgs.nz, f_hgs.d);
	plane_normalize_4_sse(f_hgs.nx+2, f_hgs.ny+2, f_hgs.nz+2, f_hgs.d+2);

	for (u32 c = 0; c < sts.cellPVSLength; ++c)
	{
		SpatialCell& cell = sts.cellPVS[c];
		u32 idx = getSpatialIndex(cell);
		SpatialHandle hnd = sps.cells[idx];
		assert(hnd != null_h32);

		// Test the spatial cell's world space bounding sphere, if it is
		// fully contained we can skip testing individual objects in the
		// cell. The y scaling factor is applied to transform the world
		// space cell into a homogeneous xyz grid space. The volume to test
		// against is now a sphere instead of an AABB (much faster test).
							
		vec3 cellCenter{ (r32)cell.x, (r32)cell.y, (r32)cell.z }; // TODO: worldspace needs to be r64?
		cellCenter *= spatialGridSizeXZ;
		cellCenter += (spatialGridSizeXZ * 0.5f);
		Sphere cellBSphere{ cellCenter, spatialGridCellRadius };

		u8 result = 0;
		frustum_intersectSpheres_sse(f_hgs, 1, &cellBSphere, 1, &result);
		assert(result != Outside);

		if (result == Inside)
		{
			// add all objects in cell to the PVS
			do {
				SpatialValue* sv = sps.valueMap[hnd];
				assert(sv);

				// TODO: prevent adding duplicates
				
				sv->entityOrComponentId; // look up entity/component using the handle
				ePVS.entityOrComponentId[ePVS.length] = sv->entityOrComponentId;
				ePVS.bSphere[ePVS.length] = {}; // get bounding sphere
				++ePVS.length;

				hnd = sv->next;
			}
			while (hnd != null_h32);
		}	
		else { // Intersecting
			// for intersecting cells, test bspheres of all objects and add to the PVS if not outside
			// for these tests we use the real world-space frustum, not the y-scaled one used for the cell
			do {
				SpatialValue* sv = sps.valueMap[hnd];
				assert(sv);

				// TODO: prevent adding duplicates
				
				sv->entityOrComponentId; // look up entity/component using the handle
				Sphere objSphere = {}; // get bounding sphere
				u8 objResult = 0;
				frustum_intersectSpheres_sse(f, 1, &objSphere, 1, &objResult);
				
				if (objResult != Outside) {
					ePVS.entityOrComponentId[ePVS.length] = sv->entityOrComponentId;
					ePVS.bSphere[ePVS.length] = {};
					++ePVS.length;
				}

				hnd = sv->next;
			}
			while (hnd != null_h32);
		}
	}
}


void interpolateSceneNodes(
	Scene& scene,
    r32 interpolation)
{
	for (u32 m = 0;
		m < scene.components.movement.length();
		++m)
	{
		Movement& move = scene.components.movement.item(m).component;
		if (move.rotationDirty == 0 && move.prevRotationDirty == 0 &&
			move.translationDirty == 0 && move.prevTranslationDirty == 0)
		{
			continue;
		}

		SceneNode& node = scene.components.sceneNodes[move.sceneNodeId]->component;

		// nlerp the rotation
		if (move.rotationDirty == 1) {
			node.rotationLocal = 
				normalize(
					lerp(move.prevRotation,
						 move.nextRotation,
						 (r64)interpolation));
			node.orientationDirty = 1;
		}
		// This is needed when rotation stops to ensure the orientation isn't left where the last
		// interpolation step put it, which is most likely approaching but not quite reaching the
		// target "next" orientation. We continue interpolating for one frame beyond movement
		// stopping so the orientation can be set to the exact simulated value.
		else if (move.prevRotationDirty == 1) {
			node.rotationLocal = move.nextRotation;
			node.orientationDirty = 1;
			move.prevRotationDirty = 0;
		}

		if (move.translationDirty == 1) {
			node.translationLocal =
				mix(move.prevTranslation,
					move.nextTranslation,
					(r64)interpolation);
			node.positionDirty = 1;
		}
		else if (move.prevTranslationDirty == 1) {
			node.translationLocal = move.nextTranslation;
			node.positionDirty = 1;
			move.prevTranslationDirty = 0;
		}
	}
}


/**
 * Traverse the scene graph starting at root node and calculate new world positions in
 * breadth-first order. Progress down a branch only when a dirty flag is set.
 */
void updateNodeTransforms(
	Scene& scene,
	MemoryArena& transient)
{
	/**
	 * BFSQueueItem is used for traversal of the scene graph by breadth-first search without recursion.
	 */
	struct BFSQueueItem {
		SceneNode*	sceneNode;
		u8			ancestorPositionDirty;
		u8			ancestorOrientationDirty;
		dvec3		translationToWorld;
		dquat		rotationToWorld;
	};
	DenseQueueTyped(BFSQueueItem, BFSQueue);

	// get some temporary storage for the traversal queue
	TemporaryMemory temp = beginTemporaryMemory(transient);

	const size_t bfsQueueSize = scene.components.sceneNodes.length();
	BFSQueueItem* bfsQueueBuffer = allocArrayOfType(transient, BFSQueueItem, bfsQueueSize);
	BFSQueue bfsQueue(bfsQueueSize, bfsQueueBuffer);

	// start traversal at the root node
	bfsQueue.push({
		&scene.root,
		0, 0,
		{ 0.0, 0.0, 0.0 },
		{ 0.0, 0.0, 0.0, 1.0 }
	});

	// traverse the scene graph and calculate new world positions if needed
	while (!bfsQueue.empty()) {
		const BFSQueueItem& bfs = *bfsQueue.front();

		SceneNode& node = *bfs.sceneNode;

		// recalc world position if this, or any ancestors have moved since last frame
		bool positionDirty = (node.positionDirty == 1) || (bfs.ancestorPositionDirty == 1);
		if (positionDirty) {
			node.positionWorld = bfs.translationToWorld + node.translationLocal;
			node.positionDirty = 0;
		}

		// recalc world orientation if this, or any ancestors have rotated since last frame
		bool orientationDirty = (node.orientationDirty == 1) || (bfs.ancestorOrientationDirty == 1);
		if (orientationDirty) {
			node.orientationWorld = normalize(bfs.rotationToWorld * node.rotationLocal);
			node.orientationDirty = 0;
		}

		if (node.numChildren > 0) {
			// add all children to the traversal queue
			SceneNodeId childId = node.firstChild;
			for (uint32_t c = 0; c < node.numChildren; ++c) {
				assert(childId != null_h32 && "expected scene node is null, broken linked list or numChildren out of sync");
				SceneNode& child = scene.components.sceneNodes[childId]->component;

				bfsQueue.push({
					&child,
					positionDirty, orientationDirty,
					node.positionWorld,
					node.orientationWorld
				});

				childId = child.nextSibling;
			}
		}

		bfsQueue.pop_fifo();
	}

	endTemporaryMemory(temp);
}


/**
* 1.	After scene nodes are interpolated, update render entry worldspace AABB/quadtree indices.
* 2.	Filter objects "per camera/frustum" in wordspace using integer AABB/quadtree index
*		containing frustum against the integer AABB/quadtree index containing object.
*		Set frustum bit to 1 for each renderable object in each frustum space.
* 3.	Update filtered entity list viewspace positions and bounding spheres
* 4.	Frustum cull against view space bounding spheres, set frustum bits back to 0 if culled
* 5.	Final list of render entries is submitted to renderer. Double-precision world coordinates
*		are transformed to single-precision in viewspace.
*/
void frustumCullScene(
	Scene& scene)
{
	auto& rciStore = components.renderCullInfo;

	//for (int activeFrustum = 0; activeFrustum < numActiveFrustums; frustumMask <<= 1; ++activeFrustum) {
		// To do the frustum index, we need an active cameras list, take index into that list for the camera.
		// Can have > 32 cameras in scene, but only up to 32 active cameras.
		// Do not simply take the camera index itself, we want the "active camera" index.
	u32 activeFrustum = 0;
	u32 frustumMask = 1;
	
		Plane frustumPlanes[6] = {};
		// TODO: code to get frustum

		for (auto& rci : rciStore.getComponents().getItems()) {
			auto inside = intersect(frustumPlanes, *reinterpret_cast<Sphere*>(&rci.component.viewspaceBSphere));
			rci.component.visibleFrustumBits |= frustumMask & (inside != Outside);
		}
	//}

	//for (i = 0; i < gridCell->blockCounts[blockIter]; ++i) {
		// filter list here (if masks[i] is zero it should be skipped)
		// ...
	//}
}


void renderScene(
	Scene& scene,
    r32 interpolation)
{
	auto& render = *g_renderPtr.lock();
	auto& loader = *g_resourceLoader.lock();

	i8 activeViewport = 0; // TEMP, hard coded to one viewport

	// update position/orientation of active camera from the scene graph
	//	only supports one active camera now, but the active cameras could be extended into a list to support several views
	for (auto& camInstance : entityMgr.getComponentStore<CameraInstance>().getComponents().getItems()) {
		if (camInstance.component.cameraId == s.activeRenderCamera) {
			auto& node = entityMgr.getComponent<SceneNode>(camInstance.component.sceneNodeId);
			auto& cam = *s.cameras[s.activeRenderCamera];

			cam.setEyePoint(node.positionWorld);
			cam.setOrientation(node.orientationWorld);
			cam.calcModelView();

			// set the renderer viewport to the active camera
			mat4 viewProjMat(cam.getProjectionMatrix() * mat4(cam.getModelViewMatrix()));
			r32 frustumDistance = cam.getFarClip() - cam.getNearClip();

			// set viewport 0 which is the main view viewport
			render.setViewParameters(activeViewport, render::ViewParameters{
				cam.getModelViewMatrix(),
				cam.getProjectionMatrix(),
				viewProjMat,
				cam.getNearClip(),
				cam.getFarClip(),
				frustumDistance,
				1.0f / frustumDistance
			});

			break;
		}
	}

	// TODO: hard-coded to one frustum, need to support > 1 to get shadow mapping
	u32 activeFrustum = 0;
	u32 frustumMask = 1;

	// run the frustum culling system to determine visible objects
	frustumCullScene(scene);

	// render all visible mesh instances
	auto& rciStore = entityMgr.getComponentStore<RenderCullInfo>();
	
	// TODO: should we really loop through all components AGAIN? Frustum culling could build a list of entityids instead
	
	for (auto& rci : rciStore.getComponents().getItems()) {
		// TODO: uncomment this once frustum culling is working
		//if (rci.component.visibleFrustumBits & frustumMask != 0) {
		
		ComponentMask mask = entityMgr.getEntityComponentMask(rci.entityId);
		
		// if it's a Model_GL
		if (mask[ModelInstance::componentType]) {
			auto modelCmp = *entityMgr.getEntityComponent<ModelInstance>(rci.entityId);
			auto modelPtr = loader.getResource(modelCmp.modelId, resource::Cache_Models);
			auto& model = modelPtr->getResource<render::Model_GL>();

			model.render(rci.entityId, s, activeViewport, engine);
		}
			//auto& node = entityMgr.getComponent<SceneNode>(rci.component.sceneNodeId);
			
			// call "render" function which should only add render entries to the viewport's queue
			// the renderer will sort the queue and call the object's "draw" function with a callback function pointer
			/*
			RenderQueueKey key;
			key.value = rci.component.renderQueueKey;

			RenderEntry re{};
			re.entityId = rci.entityId;
			re.positionWorld = node.positionWorld;
			re.orientationWorld = node.orientationWorld;
			*/

			//render.addRenderEntry(activeViewport, key, std::move(re));
		//}
	}
}
