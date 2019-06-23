#include "asset.h"
#include "platform/platform_api.h"
#include "utility/hash.h"
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
	fopen_s(&pakFile, pakFilename, "w+b");
	
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
			fopen_s(&fAsset, ca.pathString, "rb");
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
	const char* filename,
	MemoryArena& transient)
{
	h32 handle = null_h32;
	LoadedAssetPack loadedPack{};

	fopen_s(&loadedPack.pakFile, filename, "rb");
	
	if (loadedPack.pakFile)
	{
		AssetPack tmp{};
		fread(&tmp, sizeof(AssetPack), 1, loadedPack.pakFile);
		assert(tmp.PACK == ASSET_PACK_CODE && tmp.version == ASSET_PACK_VERSION);

		u16 n = tmp.numAssets;
		u32 loadSize = tmp.assetDataOffset;
		
		rewind(loadedPack.pakFile);
		u8* buf = allocBuffer(transient, loadSize, 64);
		
		tmp.assetIds = (u32*)(buf + sizeof(AssetPack));
		tmp.assetInfo = (AssetInfo*)(buf + tmp.assetInfoOffset);
		tmp.pathStrings = (char*)(buf + tmp.pathStringsOffset);
		
		loadedPack.info = (AssetPack*)buf;
		*loadedPack.info = tmp;

		handle = store.packs.insert(&loadedPack);
	}

	return handle;
}


// TODO: replace CRT FILE* ops with platform-specific File I/O
// https://docs.microsoft.com/en-us/windows/desktop/FileIO/i-o-concepts
u8* loadAssetDataFromPack(
	u32 assetId,
	LoadedAssetPack& pack,
	MemoryArena& transient) // TODO: pass an asset heap instead of a MemoryArena
{
	assert(pack.pakFile);
	u8* assetBuffer = nullptr;
	
	// TODO: test search by cache line, compare <=> to last element on a cache line
	// walk backwards if <, jump to end of next cache line if >, repeat
	// could lead to faster overall performace due to better pre-cache prediction
	u32* pId = (u32*)
		bsearch(
			&assetId,
			pack.info->assetIds,
			pack.info->numAssets,
			sizeof(u32),
			assetIdComparator);
	
	if (pId) {
		u16 index = (u16)(pId - pack.info->assetIds); // u32 pointer difference with base yields the index
		AssetInfo& assetInfo = pack.info->assetInfo[index];
		
		fseek(pack.pakFile, pack.info->assetDataOffset + assetInfo.offset, SEEK_SET);
		// TODO: get this from an asset heap, not a linear arena
		// 16 byte align the asset in case we use SSE for processing
		assetBuffer = allocBuffer(transient, assetInfo.size, 16);
		fread(assetBuffer, 1, assetInfo.size, pack.pakFile);
	}

	return assetBuffer;
}


int fileChangeCallback(
	PlatformWatchEventType changeType,
	void* userData,
	MemoryArena& taskMemory)
{
	return 0;
}
