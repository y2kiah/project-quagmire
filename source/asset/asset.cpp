#include "asset.h"
#include "platform/platform_api.h"
#include "utility/hash.h"


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
	collectedAsset->pathString = allocStringCopy(taskMem, filePath, filePathLen);
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
	FILE* file = nullptr;
	const char* pakFilename = "assets/test.pak";
	fopen_s(&file, pakFilename, "w+b");
	
	if (n > 0 && file)
	{
		result = allocType(taskMem, AssetPack);
		AssetPack& pack = *result;

		pack.header.PACK = ((u32)'P' | ((u32)'A' << 8) | ((u32)'C' << 16) | ((u32)'K' << 24));
		pack.header.version = 0;
		
		pack.header.numAssets = n;
		pack.assetInfoOffset = sizeof(AssetPack) + (sizeof(u32) * n);
		pack.assetInfoSize = sizeof(AssetInfo) * n;

		pack.pathStringsOffset = pack.assetInfoOffset + pack.assetInfoSize;

		u32* assetIds = allocArrayOfType(taskMem, u32, n);
		AssetInfo* assetInfo = allocArrayOfType(taskMem, AssetInfo, n);

		CollectedAsset& ca = *sentinel.next;
		for (u16 i = 0; i < n; ++i)
		{
			pack.assetIds[i] = ca.assetId;
			pack.assetInfo[i].size = ca.sizeBytes;
			pack.assetInfo[i].offset = pack.assetDataSize;
			pack.assetInfo[i].pathStringSize = ca.pathStringSize;
			pack.assetInfo[i].pathStringOffset = pack.pathStringsSize;

			pack.pathStringsSize += ca.pathStringSize + 1; // +1 for null terminating character
			pack.assetDataSize += ca.sizeBytes;
			ca = *ca.next;
		}

		char* pathStrings = (char*)allocBuffer(taskMem, pack.pathStringsSize, alignof(char));
		
		pack.assetDataOffset = pack.pathStringsOffset + pack.pathStringsSize;

		// copy path strings to a single buffer
		ca = *sentinel.next;
		char* dst = pathStrings;
		u32 offsetCheck = 0;
		for (u16 i = 0; i < n; ++i)
		{
			assert(pack.assetInfo[i].pathStringOffset == offsetCheck);

			_strncpy_s(
				dst,
				pathStrings+pack.pathStringsSize-dst,
				ca.pathString,
				ca.pathStringSize);

			offsetCheck += ca.pathStringSize + 1;
			dst += ca.pathStringSize + 1;
			ca = *ca.next;
		}

		fwrite(result, sizeof(AssetPack), 1, file);
		fwrite(assetIds, sizeof(u32), n, file);
		fwrite(assetInfo, sizeof(AssetInfo), n, file);
		fwrite(pathStrings, sizeof(char), pack.pathStringsSize, file);

		// append all assets to the file
		u8* tmpBuf = allocBuffer(taskMem, 64, 64);
		ca = *sentinel.next;
		offsetCheck = 0;
		for (
			u16 i = 0;
			i < n && result;
			++i)
		{
			assert(pack.assetInfo[i].offset == offsetCheck);

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
					fwrite(tmpBuf, 1, xferBytes, file);
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

		fclose(file);

		// an error occurred, remove the file
		if (!result) {
			remove(pakFilename);
		}
	}

	return result;
}


u32 loadAssetPackFromFile(
	MemoryArena& transient)
{
	assert(false && "not implemented");

	return true;
}
