#include "memory.h"
#include "../platform/platform.h"


MemoryBlock* pushBlock(
	MemoryArena* arena,
	size_t minimumSize)
{
	void* memory = platform->allocate(minimumSize);
	MemoryBlock* newBlock = (MemoryBlock*)memory;
	
	if (arena) {
		SDL_LockMutex(arena->lock);
		
		newBlock->prev = arena->lastBlock;
		if (arena->lastBlock) {
			arena->lastBlock->next = newBlock;
		}
		else {
			// pushing first block in the arena
			arena->firstBlock = arena->lastBlock = arena->currentBlock = newBlock;
		}
		arena->totalSize += newBlock->size;
		++arena->numBlocks;

		SDL_UnlockMutex(arena->lock);
	}

	return newBlock;
}


BlockFitResult getBlockToFit(
	MemoryBlock* startBlock,
	size_t size,
	u32 align)
{
	assert(startBlock != nullptr && "don't call getBlockToFit without a starting block, call pushBlock instead");

	MemoryArena* arena = startBlock->arena;
	MemoryBlock* block = startBlock;
	void* allocAddr = nullptr;
	size_t requiredSize = size;

	do {
		// TODO: if block is not a thread match to the original block, skip
		uintptr_t currentAddr = (uintptr_t)block->base + block->used;
		uintptr_t alignmentOffset = align(currentAddr, (uintptr_t)align) - currentAddr;
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
	
	if (arena && block != startBlock)
	{
		size_t blockRemaining = block->size - block->used - requiredSize;
		size_t startRemaining = startBlock->size - startBlock->used;
		if (blockRemaining > startRemaining) {
			// TODO: for now locking this, but once MemoryBlocks are thread specific, and each
			// thread has its own currentBlock pointer, this will no longer require locking
			SDL_LockMutex(arena->lock);
			arena->currentBlock = block;
			SDL_UnlockMutex(arena->lock);
		}
	}
	
	return { block, allocAddr, requiredSize };
}


void preemptivelyPushBlock(
	MemoryArena* arena)
{
	if (arena->currentBlock) {
		size_t remaining = arena->currentBlock->size - arena->currentBlock->used;
		if (remaining <= PREEMPTIVE_ALLOC_THRESHOLD) {
			pushBlock(arena);	
		}
	}
}


void* _allocSize(
	MemoryArena* arena,
	size_t size,
	u32 align)
{
	void* allocAddr = nullptr;

	if (!arena->currentBlock) {
		MemoryBlock* block = pushBlock(arena, size);
		allocAddr = block->base;
		block->used += size;
	}
	else {
		BlockFitResult fit = getBlockToFit(arena->currentBlock, size, align);
		MemoryBlock* block = fit.block;
		allocAddr = fit.allocAddr;
		block->used += fit.alignedSize;
	}

	return allocAddr;
}
