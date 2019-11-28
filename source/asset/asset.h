#ifndef _ASSET_H
#define _ASSET_H

#include "../utility/common.h"
#include "../utility/memory.h"
#include "../utility/dense_queue.h"
#include "../utility/sparse_handle_map_16.h"
#include "../capacity.h"

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


typedef h32 AssetHnd;

enum AssetStatus : u32 {
	Asset_NotLoaded = 0,		// asset is not present or evicted
	Asset_Queued,				// asset is waiting in the async queue
	Asset_Loading,				// reading bytes from disk
	Asset_Loaded,				// all bytes read from disk
	Asset_Building,				// processing on the load thread after bytes are loaded, if needed
	Asset_Built,				// load thread processing done
	Asset_Initializing,			// processing on the game thread, if needed
	Asset_Ready,				// asset can be used
	Asset_Error					// asset load error
};

/**
 * The AssetType is stored in the typeId member of the AssetHnd
 */
enum AssetType : u8 {
	Asset_Unknown = 0,
	Asset_Texture2D,
	Asset_TextureCubeMap,
	Asset_Model,
	Asset_Sound,
	Asset_Music//,
	//Asset_EntityData	TODO: maybe text or other interactive entity data too not appropriate for code?
};


#pragma pack(push, 1)

struct AssetInfo {
	u32			offset;				// offset to asset from base of assetData section
	u32			size;				// assetData size
	AssetHnd	handle;				// 0 on disk, stores handle when asset is created
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

	// These pointers are 0 on disk and intitialized on load

	u32*		assetIds;	// crc32 of the asset path/filename relative to the pack's root,
							// unique id within the pack, array sorted for binary search
	AssetInfo*	assetInfo;	// array of AssetInfo, index corresponds to assetId index
	char*		pathStrings;
};
static_assert(sizeof(AssetPack) == 64, "AssetPack data should be cache-aligned");

#pragma pack(pop)


struct LoadedAssetPack {
	AssetPack*		assetPack;
	const char*		filename;
	FILE*			pakFile;
	u64				pakFileLastWrite;
	//const char*	watchDirectory; // TODO: store some link to an associated directory here (in tools builds)
};


struct AssetCallbacks;


struct Asset {
	AssetStatus		status;
	u32				assetInfoIndex;	// index into AssetInfo array (from bsearch of assetId)
	u32				sizeBytes;
	h32				assetPack;
	
	h32				assetTypeHnd;	// handle to asset type-specific object (e.g. Texture2D_GL)
	u32				flags;			// 4 bytes dedicated for build/init flags used in loading the asset 

	AssetHnd		lruNext;
	AssetHnd		lruPrev;

	void*			assetData;		// buffer for data loaded from AssetPack file

	AssetCallbacks*	callbacks;
};


/**
 * Asset building done on the loading thread, called after bytes are loaded with the
 * asset->assetData buffer loaded with size of asset->sizeBytes. Build/init options may be passed
 * in asset->flags.
 */
typedef void AssetBuildCallbackFunc(
	AssetHnd hnd,
	Asset* asset);

/**
 * Asset initialization done on the game thread. Initialization should not set an asset
 * status directly, but should return a status of either Ready or Error.
 */
typedef AssetStatus AssetInitCallbackFunc(
	AssetHnd hnd,
	Asset* asset);

// Asset removal done on the game thread
typedef void AssetRemoveCallbackFunc(
	AssetHnd hnd,
	Asset* asset);

struct AssetCallbacks {
	AssetBuildCallbackFunc*		buildCallback;
	AssetInitCallbackFunc*		initCallback;
	AssetRemoveCallbackFunc*	removeCallback;
};


SparseHandleMap16TypedWithBuffer(LoadedAssetPack, AssetPackMap, h32, 0, ASSET_PACKS_CAPACITY);
SparseHandleMap16TypedWithBuffer(Asset, AssetMap, AssetHnd, 0, ASSET_MAP_CAPACITY);
//SparseHandleMap16TypedWithBuffer(ModelAsset, ModelMap, ModelId, Asset_Model, ASSET_MODELS_CAPACITY);
ConcurrentQueueTypedWithBuffer(AssetHnd, AssetAsyncQueue, ASSET_LOAD_QUEUE_CAPACITY, 0);


struct AssetCache {
	AssetHnd		lruFront;
	AssetHnd		lruBack;
	size_t			totalSizeBytes;
	size_t			targetMaxSizeBytes;
};


struct AssetStore {
	AssetPackMap		packs;
	
	AssetMap			assets;
	
	AssetCache			assetCache;

	AssetAsyncQueue		loadQueue;
	AssetAsyncQueue		initQueue;

	SDL_Thread*			loadThread;
	
	MemoryHeap			assetHeap;
};


AssetPack* buildAssetPackFromDirectory(
	const char* packDirectory,
	MemoryArena& taskMem);

h32 openAssetPackFile(
	AssetStore& store,
	const char* filename);

/**
 * Starts a thread to process the asset loading queue.
 */
void startAsyncLoadAssets(
	GameMemory* gameMemory);

/**
 * Stops the asset load thread.
 */
void stopAsyncLoadAssets(
	GameMemory* gameMemory);

/**
 * Called from the main game thread, processes loaded assets to get them into final Ready state.
 * Also may queue up more loads for child resources.
 */
void initLoadedAssets(
	AssetStore& store);

void maintainAssetCache(
	AssetStore& store,
	SystemInfo& info);

/**
 * To convert path strings to assetId, use CRC32 macro for static strings and crc32 function for
 * runtime strings.
 */
AssetHnd createAsset(
	AssetStore& store,
	h32 pack,
	u32 assetId,
	AssetType assetType,
	h32 assetTypeHnd,
	u32 flags,
	AssetCallbacks* callbacks);

/**
 * Returns the asset if it exists, otherwise nullptr. If status is NotLoaded, adds asset to the
 * loading queue. If status is Ready, moves asset to back of the LRU list. Calling code must
 * handle nullptr return gracefully. This function is thread-safe and will not result in multiple
 * queued requests for loading.
 */
Asset* getAsset(
	AssetStore& store,
	AssetHnd hnd);

#endif