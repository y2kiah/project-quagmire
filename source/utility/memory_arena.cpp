#include "memory.h"
#include "platform/platform_api.h"


MemoryBlock* pushBlock(
	MemoryArena& arena,
	u32 minimumSize)
{
	assert(arena.threadID == SDL_ThreadID() && "MemoryArena thread mismatch");

	// MemoryBlock safe to cast directly from PlatformBlock, it is asserted to be the base member
	MemoryBlock* newBlock = (MemoryBlock*)platformApi().allocate(minimumSize);
	newBlock->arena = &arena;
	newBlock->blockType = MemoryBlock::ArenaBlock;
	newBlock->prev = arena.lastBlock;
	if (arena.lastBlock) {
		arena.lastBlock->next = newBlock;
	}
	else {
		// pushing first block in the arena
		arena.firstBlock = arena.lastBlock = arena.currentBlock = newBlock;
	}
	arena.totalSize += newBlock->size;
	++arena.numBlocks;

	return newBlock;
}


bool popBlock(
	MemoryArena& arena)
{
	assert(arena.threadID == SDL_ThreadID() && "MemoryArena thread mismatch");

	MemoryBlock *last = arena.lastBlock;
	if (last) {
		if (arena.currentBlock == last) {
			arena.currentBlock = last->prev;
		}
		if (arena.firstBlock == last) {
			arena.firstBlock = nullptr;
		}
		arena.lastBlock = last->prev;
		
		--arena.numBlocks;
		arena.totalSize -= last->size;

		platformApi().deallocate((PlatformBlock*)last);
	}
	return (last != nullptr);
}


void removeArenaBlock(
	MemoryBlock* block)
{
	assert(block && block->blockType == MemoryBlock::ArenaBlock);
	assert(block->arena);

	MemoryArena& arena = *block->arena;
	
	assert(arena.threadID == SDL_ThreadID() && "MemoryArena thread mismatch");

	if (block->next) {
		block->next->prev = block->prev;
	}
	else {
		arena.lastBlock = block->prev;
	}

	if (block->prev) {
		block->prev->next = block->next;
	}
	else {
		arena.firstBlock = block->next;
	}

	if (arena.currentBlock == block) {
		arena.currentBlock = (block->next ? block->next : block->prev);
	}

	--arena.numBlocks;
	arena.totalSize -= block->size;

	platformApi().deallocate((PlatformBlock*)block);
}


BlockFitResult getBlockToFit(
	MemoryArena& arena,
	MemoryBlock* startBlock,
	u32 size,
	u32 align)
{
	assert(startBlock != nullptr && "don't call getBlockToFit without a starting block, call pushBlock instead");
	assert(startBlock->arena == &arena);

	MemoryBlock* block = startBlock;
	void* allocAddr = nullptr;
	u32 requiredSize = size;

	do {
		uintptr_t currentAddr = (uintptr_t)block->base + block->used;
		u32 alignmentOffset = (u32)(_align(currentAddr, (uintptr_t)align) - currentAddr);
		requiredSize = size + alignmentOffset;

		if (block->used + requiredSize <= block->size) {
			allocAddr = (void*)(currentAddr + alignmentOffset);
			break;
		}
		
		block = block->next;
	}
	while (block != nullptr);

	if (!block) {
		block = pushBlock(arena, size);
		requiredSize = size;
		allocAddr = block->base;
	}
	
	if (block != startBlock)
	{
		u32 blockRemaining = block->size - block->used - requiredSize;
		u32 startRemaining = startBlock->size - startBlock->used;
		if (blockRemaining > startRemaining) {
			arena.currentBlock = block;
		}
	}
	
	return { block, allocAddr, requiredSize };
}


void preemptivelyPushBlock(
	MemoryArena& arena)
{
	assert(arena.threadID == SDL_ThreadID() && "MemoryArena thread mismatch");

	if (arena.lastBlock) {
		u32 remaining = arena.lastBlock->size - arena.lastBlock->used;
		if (remaining <= MEMORY_ARENA_PREEMPTIVE_ALLOC_THRESHOLD) {
			pushBlock(arena);	
		}
	}
}


void clearArena(
	MemoryArena& arena)
{
	MemoryBlock* block = arena.firstBlock;
	while (block) {
		MemoryBlock* next = block->next;
		platformApi().deallocate((PlatformBlock*)block);
		block = next;
	}
	arena = makeMemoryArena();
}


void clearForwardOf(
	MemoryBlock* blockStart,
	u32 usedStart)
{
	assert(blockStart && usedStart <= blockStart->used);
	assert(blockStart->blockType == MemoryBlock::ArenaBlock);

	// clear all memory forward of the start block/position
	blockStart->used = usedStart;
	memset(
		(void*)((uintptr_t)blockStart->base + usedStart),
		0,
		blockStart->size - blockStart->used);
	
	// clear block(s) forward of a start index until the end or an unused block is reached
	for (;;)
	{
		blockStart = blockStart->next;
		if (!blockStart || blockStart->used == 0) {
			break;
		}
		memset(blockStart->base, 0, blockStart->used);
		blockStart->used = 0;
	}
}


void shrinkArena(
	MemoryArena* arena)
{
	while (arena->lastBlock
		   && arena->lastBlock->used == 0)
	{
		MemoryBlock* prev = arena->lastBlock->prev;
		arena->totalSize -= arena->lastBlock->size;
		--arena->numBlocks;
		platformApi().deallocate((PlatformBlock*)arena->lastBlock);
		
		if (arena->currentBlock == arena->lastBlock) {
			arena->currentBlock = prev;
		}
		arena->lastBlock = prev;
	}
	if (!arena->lastBlock) {
		arena->firstBlock = nullptr;
		assert(arena->numBlocks == 0 && arena->totalSize == 0);
	}
}


void* _allocSize(
	MemoryArena& arena,
	u32 size,
	u32 align)
{
	assert(arena.threadID == SDL_ThreadID() && "MemoryArena thread mismatch");

	void* allocAddr = nullptr;

	if (!arena.currentBlock) {
		MemoryBlock* block = pushBlock(arena, size);
		allocAddr = block->base;
		block->used += size;
	}
	else {
		BlockFitResult fit = getBlockToFit(arena, arena.currentBlock, size, align);
		MemoryBlock* block = fit.block;
		allocAddr = fit.allocAddr;
		block->used += fit.alignedSize;
	}

	return allocAddr;
}


char* allocStringNCopy(
	MemoryArena& arena,
	const char* src,
	u32 size)
{
	assert(src);
	char* dest = (char*)_allocSize(arena, size+1, alignof(char)); // +1 for null terminating character
	_strncpy_s(dest, size+1, src, size);
	return dest;
}


char* allocStringCopy(
	MemoryArena& arena,
	const char* src)
{
	return allocStringNCopy(arena, src, (u32)strlen(src));
}
