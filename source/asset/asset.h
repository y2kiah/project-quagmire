#ifndef _ASSET_H
#define _ASSET_H

#include "utility/common.h"
#include "utility/memory.h"
#include "utility/dense_queue.h"
#include "utility/sparse_handle_map_16.h"
#include "capacity.h"

/**
 * Assets are streamed in from AssetPacks, which are single .pak files in a shipping build, and may
 * be paired with a directory on the file system of the same name for development. Assets bundled
 * in a pack are not necessarily loaded in batch, they are streamed in as-needed with an LRU
 * controlling eviction. Assets that are referenced from other assets (like textures for a model)
 * are always colocated within the same pack, or are references to global resources.
 * 
 * AssetPacks include a 64-byte header, then an assetId table containing crc32 hashes of the
 * original path string sorted ascending for binary search, then AssetInfo data with an index
 * corresponding to the assetId, then a buffer of original path names indexed by an offset within
 * the AssetInfo struct, and finally the large concatenation of asset data indexed by the offset
 * within the AssetInfo struct. When the pak file is loaded into memory, all except for the asset
 * data is resident in memory.
 */


typedef h32 AssetId;


#pragma pack(push, 1)

struct AssetInfo {
	u32			offset;				// offset to asset from base of assetData section
	u32			size;				// assetData size
	u32			pathStringOffset;	// offset to path string
	u32			pathStringSize;		// size of path string not including null terminator
};


struct alignas(64) AssetPack
{
	u32			PACK;				// {'P','A','C','K'}
	u16			version;
	u16			numAssets;
	u32			assetInfoOffset;
	u32			assetInfoSize;
	u32			pathStringsOffset;
	u32			pathStringsSize;	// total size of path string data including all null terminating characters
	u32			assetDataOffset;
	u32			assetDataSize;
	
	u32			_padding[2];

	// These pointers are zero'd on disk and intitialized on load

	u32*		assetIds;	// crc32 of the asset path/filename relative to the pack's root,
							// unique id within the pack, array sorted for binary search
	AssetInfo*	assetInfo;	// array of AssetInfo, index corresponds to assetId index
	char*		pathStrings;
};
static_assert(sizeof(AssetPack) == 64, "AssetPack data should be cache-aligned");

#pragma pack(pop)


struct LoadedAssetPack {
	AssetPack*	info;
	FILE*		pakFile;
	u64			pakFileLastWrite;
	//const char*	watchDirectory; // TODO: store some link to an associated directory in tools builds here
};


enum AssetStatus {
	Asset_NotLoaded = 0,		// asset is not present
	Asset_BytesLoaded,
	Asset_Processing,
	Asset_Processed,
	Asset_Framework_Loaded,
	Asset_Framework_Unloaded
};

enum AssetCategory : u8 {
	Asset_None = 0,
	Asset_Texture,
	Asset_Model,
	Asset_Sound,
	Asset_Music//,
	//Asset_EntityData	TODO: maybe text or other interactive entity data too not appropriate for code?
};

struct Asset {
	
	u8		statusFlags;
};

DenseQueueTyped(u16, AssetCacheLRU);
SparseHandleMap16TypedWithBuffer(LoadedAssetPack, AssetPackMap, h32, 0, ASSET_PACKS_CAPACITY);


struct AssetStore {
	AssetPackMap	packs;

	AssetCacheLRU	textureCache;
	AssetCacheLRU	modelCache;
	AssetCacheLRU	soundCache;
};


AssetPack* buildAssetPackFromDirectory(
	const char* packDirectory,
	MemoryArena& taskMem);

h32 openAssetPackFile(
	AssetStore& store,
	const char* filename,
	MemoryArena& transient);

#endif