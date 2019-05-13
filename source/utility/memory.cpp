#include "memory.h"
#include "../platform/platform_api.h"


MemoryBlock* pushBlock(
	MemoryArena& arena,
	size_t minimumSize)
{
	assert(arena.threadID == SDL_ThreadID() && "MemoryArena thread mismatch");

	// MemoryBlock safe to cast directly from PlatformBlock, it is asserted to be the base member
	MemoryBlock* newBlock = (MemoryBlock*)platform->allocate(minimumSize);
	
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

		platform->deallocate((PlatformBlock*)last);
	}
	return (last != nullptr);
}


void removeBlock(
	MemoryBlock* block)
{
	assert(block);
	if (block->arena) {
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

		platform->deallocate((PlatformBlock*)block);
	}
}


BlockFitResult getBlockToFit(
	MemoryArena& arena,
	MemoryBlock* startBlock,
	size_t size,
	u32 align)
{
	assert(startBlock != nullptr && "don't call getBlockToFit without a starting block, call pushBlock instead");

	MemoryBlock* block = startBlock;
	void* allocAddr = nullptr;
	size_t requiredSize = size;

	do {
		uintptr_t currentAddr = (uintptr_t)block->base + block->used;
		uintptr_t alignmentOffset = _align(currentAddr, (uintptr_t)align) - currentAddr;
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
		size_t blockRemaining = block->size - block->used - requiredSize;
		size_t startRemaining = startBlock->size - startBlock->used;
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
		size_t remaining = arena.lastBlock->size - arena.lastBlock->used;
		if (remaining <= PREEMPTIVE_ALLOC_THRESHOLD) {
			pushBlock(arena);	
		}
	}
}


void* _allocSize(
	MemoryArena& arena,
	size_t size,
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


void clearArena(
	MemoryArena& arena)
{
	MemoryBlock* block = arena.firstBlock;
	while (block) {
		MemoryBlock* next = block->next;
		platform->deallocate((PlatformBlock*)block);
		block = next;
	}
	arena = makeMemoryArena();
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
		platform->deallocate((PlatformBlock*)arena->lastBlock);
		
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