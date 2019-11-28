#include "asset.h"
#include "../platform/platform_api.h"
#include "../utility/hash.h"
#include <cstdio>


#define ASSET_PACK_CODE	(((u32)'P' << 0) | ((u32)'A' << 8) | ((u32)'C' << 16) | ((u32)'K' << 24))
#define ASSET_PACK_VERSION	1


struct CollectedAsset {
	CollectedAsset* next;
	u32		assetId;
	u32		sizeBytes;
	char*	pathString;
	u32		pathStringSize;
};

struct CollectAssetParams {
	//AssetPack&		assetPack;
	MemoryArena&	taskMem;
	CollectedAsset*	collectedAssets;
};

struct AssetIndexSort {
	u32			originalIndex;
	u32			assetId;
	AssetInfo	assetInfo;
};


void collectAsset(
	const char* filePath,
	u32 sizeBytes,
	bool isDirectory,
	void* collectAssetParams)
{
	assert(collectAssetParams);
	CollectAssetParams& params = *(CollectAssetParams*)collectAssetParams;
	MemoryArena& taskMem = params.taskMem;

	CollectedAsset* collectedAsset = allocType(taskMem, CollectedAsset);
	params.collectedAssets->next = collectedAsset;
	params.collectedAssets = collectedAsset;
	
	CollectedAsset& ca = *collectedAsset;
	
	u16 filePathLen = (u16)strlen(filePath);
	ca.assetId = crc32(filePath, filePathLen);
	ca.sizeBytes = sizeBytes;
	ca.pathStringSize = filePathLen;
	collectedAsset->pathString = allocStringNCopy(taskMem, filePath, filePathLen);
}


int assetIdComparator(const void* ia, const void* ib) {
	u32 a = ((AssetIndexSort*)ia)->assetId;
	u32 b = ((AssetIndexSort*)ib)->assetId;
	return (a < b ? -1 : (a > b ? 1 : 0));
}


AssetPack* buildAssetPackFromDirectory(
	const char* packDirectory,
	MemoryArena& taskMem)
{
	ScopedTemporaryMemory temp = scopedTemporaryMemory(taskMem);

	AssetPack* result = nullptr;

	CollectedAsset sentinel;
	sentinel.next = &sentinel;
	CollectAssetParams params{ taskMem, &sentinel };

	PlatformFindAllFilesResult ffr =
		platformApi().findAllFiles(
			"assets/test/*", // TODO: packDirectory
			true,
			MAX_FILE_RECURSION_DEPTH,
			&collectAsset,
			(void*)&params);
	
	assert(ffr.numFiles < USHRT_MAX);

	u16 n = (u16)ffr.numFiles;
	FILE* pakFile = nullptr;
	const char* pakFilename = "assets/test.pak";
	_fopen_s(&pakFile, pakFilename, "w+b");
	
	if (n > 0 && pakFile)
	{
		result = allocType(taskMem, AssetPack);
		AssetPack& pack = *result;

		pack.PACK = ASSET_PACK_CODE;
		pack.version = ASSET_PACK_VERSION;
		
		pack.numAssets = n;
		pack.assetInfoOffset = sizeof(AssetPack) + (sizeof(pack.assetIds[0]) * n);
		pack.assetInfoSize = sizeof(AssetInfo) * n;

		pack.pathStringsOffset = pack.assetInfoOffset + pack.assetInfoSize;

		// create an indexing array to be sorted
		AssetIndexSort* index = allocArrayOfType(taskMem, AssetIndexSort, n);
		
		CollectedAsset& ca = *sentinel.next;
		for (u16 i = 0; i < n; ++i)
		{
			index[i].originalIndex = i;
			index[i].assetId = ca.assetId;
			index[i].assetInfo.size = ca.sizeBytes;
			index[i].assetInfo.offset = pack.assetDataSize;
			index[i].assetInfo.pathStringSize = ca.pathStringSize;
			index[i].assetInfo.pathStringOffset = pack.pathStringsSize;

			pack.pathStringsSize += ca.pathStringSize + 1; // +1 for null terminating character
			pack.assetDataSize += ca.sizeBytes;
			ca = *ca.next;
		}

		// sort the index array based on assetIds
		qsort(index, n, sizeof(index[0]), assetIdComparator);

		u32* assetIds = allocArrayOfType(taskMem, u32, n);
		AssetInfo* assetInfo = allocArrayOfType(taskMem, AssetInfo, n);
		char* pathStrings = (char*)allocBuffer(taskMem, pack.pathStringsSize, alignof(char));

		// build the final assetIds and corresponding assetInfo arrays using sorted index
		// copy path strings to a single buffer in the original order
		ca = *sentinel.next;
		char* dst = pathStrings;
		u32 offsetCheck = 0;
		for (u16 i = 0; i < n; ++i)
		{
			pack.assetIds[i] = index[i].assetId;
			pack.assetInfo[i] = index[i].assetInfo;

			assert(pack.assetInfo[index[i].originalIndex].pathStringOffset == offsetCheck);

			_strncpy_s(
				dst,
				pathStrings+pack.pathStringsSize-dst,
				ca.pathString,
				ca.pathStringSize);

			offsetCheck += ca.pathStringSize + 1;
			dst += ca.pathStringSize + 1;
			ca = *ca.next;
		}

		fwrite(result, sizeof(AssetPack), 1, pakFile);
		fwrite(assetIds, sizeof(assetIds[0]), n, pakFile);
		fwrite(assetInfo, sizeof(AssetInfo), n, pakFile);
		fwrite(pathStrings, sizeof(char), pack.pathStringsSize, pakFile);

		// append all assets to the pakFile
		pack.assetDataOffset = pack.pathStringsOffset + pack.pathStringsSize;

		u8* tmpBuf = allocBuffer(taskMem, 64, 64);
		ca = *sentinel.next;
		offsetCheck = 0;
		for (u16 i = 0;
			i < n && result;
			++i)
		{
			assert(pack.assetInfo[index[i].originalIndex].offset == offsetCheck);

			FILE* fAsset = nullptr;
			_fopen_s(&fAsset, ca.pathString, "rb");
			if (fAsset) {
				u32 b = ca.sizeBytes;
				while (b > 0) {
					u32 xferBytes = min(64U, b);
					if (fread(tmpBuf, 1, xferBytes, fAsset) != xferBytes) {
						result = nullptr;
						break;
					}
					fwrite(tmpBuf, 1, xferBytes, pakFile);
					b -= xferBytes;
				}
				assert(feof(fAsset));
				fclose(fAsset);
				
				offsetCheck += ca.sizeBytes;
			}
			else {
				result = nullptr;
			}
			ca = *ca.next;
		}

		pack.assetIds = assetIds;
		pack.assetInfo = assetInfo;
		pack.pathStrings = pathStrings;

		fclose(pakFile);

		// an error occurred, remove the pakFile
		if (!result) {
			remove(pakFilename);
		}
	}

	return result;
}


h32 openAssetPackFile(
	AssetStore& store,
	const char* filename)
{
	h32 handle = null_h32;
	LoadedAssetPack loadedPack{};

	loadedPack.filename = filename;

	_fopen_s(&loadedPack.pakFile, filename, "rb");
	
	if (loadedPack.pakFile)
	{
		AssetPack tmp{};
		fread(&tmp, sizeof(AssetPack), 1, loadedPack.pakFile);
		assert(tmp.PACK == ASSET_PACK_CODE && tmp.version == ASSET_PACK_VERSION);

		u16 n = tmp.numAssets;
		u32 loadSize = tmp.assetDataOffset;
		
		rewind(loadedPack.pakFile);
		u8* buf = heapAllocBuffer(store.assetHeap, loadSize, false);
		
		tmp.assetIds = (u32*)(buf + sizeof(AssetPack));
		tmp.assetInfo = (AssetInfo*)(buf + tmp.assetInfoOffset);
		tmp.pathStrings = (char*)(buf + tmp.pathStringsOffset);
		
		loadedPack.assetPack = (AssetPack*)buf;
		*loadedPack.assetPack = tmp;

		handle = store.packs.insert(&loadedPack);
	}

	return handle;
}


u32 getAssetInfoIndex(
	u32 assetId,
	LoadedAssetPack& pack)
{
	assert(pack.pakFile);
	u32 index = UINT_MAX;

	// TODO: test search by cache line, compare <=> to last element on a cache line
	// walk backwards if <, jump to end of next cache line if >, repeat
	// could lead to faster overall performace due to better pre-cache prediction
	u32* pId = (u32*)
		bsearch(
			&assetId,
			pack.assetPack->assetIds,
			pack.assetPack->numAssets,
			sizeof(u32),
			assetIdComparator);
	
	if (pId) {
		index = (u32)(pId - pack.assetPack->assetIds); // u32 pointer difference with base gives the index
		assert(index <= USHRT_MAX);
	}
	return index;
}


// TODO: replace FILE* ops with platform-specific File I/O
// https://docs.microsoft.com/en-us/windows/desktop/FileIO/i-o-concepts
AssetStatus loadAssetDataFromPack(
	AssetStore& store,
	AssetHnd hnd,
	Asset* asset)
{
	assert(asset && asset->assetData && asset->status == Asset_NotLoaded);

	LoadedAssetPack& pack = *store.packs[asset->assetPack];
	assert(pack.pakFile);
	
	AssetInfo& assetInfo = pack.assetPack->assetInfo[asset->assetInfoIndex];
	assert(assetInfo.handle == hnd);

	int ok = fseek(pack.pakFile, pack.assetPack->assetDataOffset + assetInfo.offset, SEEK_SET);
	
	if (ok == 0) {
		asset->status = Asset_Loading;

		size_t bytesRead = fread(asset->assetData, 1, assetInfo.size, pack.pakFile);
		
		if (bytesRead == assetInfo.size) {
			asset->status = Asset_Loaded;
		}
		else {
			asset->status = Asset_Error;
		}
	}
	else {
		asset->status = Asset_Error;
		logger::critical(logger::Category_Error, "seek error in pack file %s.", pack.filename);
	}

	if (ferror(pack.pakFile)) {
		logger::critical(logger::Category_Error, "read error in pack file %s.", pack.filename);
		clearerr(pack.pakFile);
	}

	return asset->status;
}


int fileChangeCallback(
	PlatformWatchEventType changeType,
	void* userData,
	MemoryArena& taskMemory)
{
	return 0;
}


void addToLRU(
	AssetStore& store,
	AssetHnd hnd,
	Asset* asset)
{
	asset->lruPrev = store.assetCache.lruBack;
	store.assetCache.lruBack = hnd;
	if (store.assetCache.lruFront == null_h32) {
		store.assetCache.lruFront = hnd;
	}
	store.assetCache.totalSizeBytes += asset->sizeBytes;
}

void removeFromLRU(
	AssetStore& store,
	AssetHnd hnd,
	Asset* asset)
{
	if (asset->lruNext != null_h32) {
		store.assets[asset->lruNext]->lruPrev = asset->lruPrev;
	}
	else {
		store.assetCache.lruBack = asset->lruPrev;
	}
	
	if (asset->lruPrev != null_h32) {
		store.assets[asset->lruPrev]->lruNext = asset->lruNext;
	}
	else {
		store.assetCache.lruFront = asset->lruNext;
	}

	store.assetCache.totalSizeBytes -= asset->sizeBytes;
}

void setLRUMostRecent(
	AssetStore& store,
	AssetHnd hnd,
	Asset* asset)
{
	// add to lru list
	if (asset->lruNext == null_h32
		&& asset->lruPrev == null_h32)
	{
		addToLRU(store, hnd, asset);
	}
	// move to lru list back if not already
	else if (store.assetCache.lruBack != hnd)
	{
		store.assets[asset->lruNext]->lruPrev = asset->lruPrev;
		if (asset->lruPrev != null_h32) {
			store.assets[asset->lruPrev]->lruNext = asset->lruNext;
		}
		asset->lruPrev = store.assetCache.lruBack;
		asset->lruNext = null_h32;
		store.assetCache.lruBack = hnd;
	}
}

void freeOneAssetFromLRU(
	AssetStore& store)
{
	AssetHnd hnd = store.assetCache.lruFront;
	while (hnd != null_h32)
	{
		Asset* asset = store.assets[hnd];
		
		if (asset->status == Asset_Ready
			|| asset->status == Asset_Error)
		{
			if (asset->status == Asset_Ready
				&& asset->callbacks && asset->callbacks->removeCallback)
			{
				asset->callbacks->removeCallback(hnd, asset);
			}
			if (asset->assetData) {
				freeAlloc(asset->assetData);
				asset->assetData = nullptr;
			}
			removeFromLRU(store, hnd, asset);

			asset->status = Asset_NotLoaded;

			break;
		}
		
		hnd = asset->lruNext;
	}
}

int loadAssetsProcess(
	void* ctx)
{
	logger::debug("asset loading thread started");

	GameMemory* gameMemory = (GameMemory*)ctx;
	AssetStore& store = gameMemory->game->assetStore;

	for (;;)
	{
		AssetHnd hnd = null_h32;
		store.loadQueue.wait_pop(&hnd);

		// exit thread when a null handle is pushed onto the queue
		if (hnd == null_h32) {
			break;
		}
		
		Asset* asset = store.assets[hnd];
		assert(asset);

		AssetStatus status = loadAssetDataFromPack(store, hnd, asset);

		if (status == Asset_Loaded
			&& asset->callbacks && asset->callbacks->buildCallback)
		{
			asset->status = Asset_Building;
			asset->callbacks->buildCallback(hnd, asset);
			asset->status = Asset_Built;
		}
		
		store.initQueue.push(hnd);
	}
	store.loadThread = nullptr;
	logger::debug("asset loading thread stopped");
	return 0;
}


void startAsyncLoadAssets(
	GameMemory* gameMemory)
{
	AssetStore& store = gameMemory->game->assetStore;

	if (store.loadThread == nullptr)
	{
		store.loadThread =
			SDL_CreateThread(
				loadAssetsProcess,
				"AssetLoadingThread",
				(void*)gameMemory);
	}
}


void stopAsyncLoadAssets(
	GameMemory* gameMemory)
{
	AssetStore& store = gameMemory->game->assetStore;

	if (store.loadThread != nullptr)
	{
		store.loadQueue.clear();
		store.loadQueue.push(null_h32);
		SDL_WaitThread(store.loadThread, nullptr);
		assert(!store.loadThread);
	}
}


DenseQueueTyped(AssetHnd, AssetHndQueue);

void initLoadedAssets(
	AssetStore& store)
{
	AssetHnd handles[ASSET_LOAD_QUEUE_CAPACITY] = {};
	u32 count = store.initQueue.try_pop_all(handles, countof(handles));
	for (u32 h = 0; h < count; ++h) {
		AssetHnd hnd = handles[h];
		Asset* asset = store.assets[hnd];
		if (asset->status != Asset_Error
			&& asset->callbacks && asset->callbacks->initCallback)
		{
			asset->status = Asset_Initializing;
			AssetStatus newStatus = asset->callbacks->initCallback(hnd, asset);
			assert(newStatus > Asset_Initializing);
			asset->status = newStatus;
		}
		else {
			asset->status = Asset_Ready;
		}
	}
}


void maintainAssetCache(
	AssetStore& store,
	SystemInfo& info)
{
	// try to leave a 10% margin of total RAM available to the OS and other processes
	size_t margin = (size_t)(megabytes(info.systemRAM) * 0.1f);
	
	// available bytes (not considering the asset heap)
	size_t availBytes = min(info.availPhysBytes, info.availVirtBytes)
						+ store.assetHeap.totalSize;
	
	size_t usableBytes = (availBytes > margin ? availBytes - margin : 0ULL);
	// don't let usableBytes go below the minimum initial heap size
	usableBytes = max(usableBytes, (size_t)INIT_MIN_ASSETHEAP_BLOCK_MEGABYTES);

	store.assetCache.targetMaxSizeBytes = usableBytes;

	// if usable bytes is smaller than the heap, but larger than actual usage, try to shrink it
	if (usableBytes < store.assetHeap.totalSize
		&& usableBytes >= store.assetCache.totalSizeBytes)
	{
		shrinkHeap(store.assetHeap);
	}
}


bool allocateBufferForAsset(
	AssetStore& store,
	AssetHnd hnd,
	Asset* asset)
{
	bool result = false;
	if (!asset->assetData) {
		// make room for the new asset
		while (store.assetCache.totalSizeBytes + asset->sizeBytes > store.assetCache.targetMaxSizeBytes) {
			freeOneAssetFromLRU(store);
		}

		asset->assetData = heapAllocBuffer(store.assetHeap, asset->sizeBytes, false);
		if (asset->assetData) {
			setLRUMostRecent(store, hnd, asset);
			result = true;
		}
	}
	return result;
}


AssetHnd createAsset(
	AssetStore& store,
	h32 pack,
	u32 assetId,
	AssetType assetType,
	h32 assetTypeHnd,
	u32 flags,
	AssetCallbacks* callbacks)
{
	LoadedAssetPack& lp = *store.packs[pack];
	u32 idx = getAssetInfoIndex(assetId, lp);
	AssetInfo& ai = lp.assetPack->assetInfo[idx];
	assert(ai.handle == null_h32);

	Asset* asset = nullptr;
	AssetHnd hnd = store.assets.insert(nullptr, &asset, assetType);
	ai.handle = hnd;
	asset->status = Asset_NotLoaded;
	asset->assetInfoIndex = idx;
	asset->sizeBytes = ai.size;
	asset->assetPack = pack;
	asset->assetTypeHnd = assetTypeHnd;
	asset->flags = flags;
	asset->callbacks = callbacks;
	
	return hnd;
}


void loadAsset(
	AssetStore& store,
	AssetHnd hnd,
	Asset* asset)
{
	// reset error state when we try a reload
	if (asset->status == Asset_Error) {
		asset->status = Asset_NotLoaded;
	}

	// set Queued if currently NotLoaded
	if (SDL_AtomicCAS(
			(SDL_atomic_t*)&asset->status,
			Asset_NotLoaded,
			Asset_Queued))
	{
		if (asset->assetData
			|| allocateBufferForAsset(store, hnd, asset))
		{
			AssetHnd* newHnd = store.loadQueue.push(hnd);
			if (!newHnd) {
				// queue is full, try again on next request
				asset->status = Asset_NotLoaded;
			}
		}
		else {
			asset->status = Asset_Error;
		}
	}
}


Asset* getAsset(
	AssetStore& store,
	AssetHnd hnd)
{
	Asset* asset = store.assets[hnd];
	if (asset) {
		u32 status = SDL_AtomicGet((SDL_atomic_t*)&asset->status);
		if (status == Asset_NotLoaded) {
			loadAsset(store, hnd, asset);
		}
		else if (status == Asset_Ready) {
			setLRUMostRecent(store, hnd, asset);
		}
	}
	return asset;
}
