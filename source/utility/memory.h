#ifndef _MEMORY_H
#define _MEMORY_H

#include <SDL_mutex.h>
#include <SDL_thread.h>
#include "utility/common.h"

struct MemoryArena;

struct MemoryBlock {
	void*			base;
	size_t			size;
	uintptr_t		used;
	// linked list for blocks in the arena
	MemoryBlock*	next;
	MemoryBlock*	prev;
	// TODO: this will result in dangling pointer if arena is destroyed and block remins in the platform list
	// should this be a handle to an arena instead?
	// or should we always free blocks when arena is destroyed and set this null
	MemoryArena*	arena;
};


struct PlatformBlock {
	MemoryBlock		arenaBlock;
	// linked list for all allocated blocks
	PlatformBlock*	next;
	PlatformBlock*	prev;
};
/**
 * PlatformBlock headers must be a multiple of 64 so they do not impact the cache
 * line alignment of a block allocation
 */
static_assert_aligned_size(PlatformBlock, 64);
static_assert(offsetof(PlatformBlock, arenaBlock) == 0, "MemoryBlock must be first member of PlatformBlock");


struct MemoryArena {
	// TODO: do we really need to track first block?
	MemoryBlock*	firstBlock;		// beginning of the block list
	MemoryBlock*	lastBlock;		// end of the block list, new blocks are pushed here
	MemoryBlock*	currentBlock;	// block currently being used for allocations (may not be the last block)

	size_t			totalSize;		// total available capacity in all blocks (not including space for headers)
	u32				numBlocks;
	u32				threadID;		// threadID is tracked to later assert the threadID matches on allocations
	// TODO: store allocator type, store flags like readonly, over/underflow protection, etc.
};


/**
 * PlatformMemory tracks a thread-safe linked list of allocated memory blocks belonging to all
 * MemoryArenas across the system.
 */
struct PlatformMemory {
	PlatformBlock	sentinel;		// tracks bounds of PlatformBlock list, allows iteration forward and backward

	SDL_mutex*		lock = nullptr;	// for thread-safe manipulation of the block list, there is not a lot of contention here

	size_t			totalSize = 0;	// total allocated size of all blocks
	u32				numBlocks = 0;


	PlatformMemory() :
		lock{ SDL_CreateMutex() },
		sentinel{ {}, &this->sentinel, &this->sentinel }
	{}

	~PlatformMemory() {
		SDL_DestroyMutex(lock);
	}
};


MemoryArena makeMemoryArena()
{
	MemoryArena arena{};
	arena.threadID = SDL_ThreadID();
	return arena;
}

void clearArena(
	MemoryArena& arena);

void clearForwardOf(
	MemoryBlock* blockStart,
	uintptr_t usedStart);

void shrinkArena(
	MemoryArena& arena);

/**
 * Adds a new block to the end of the list of at least minimumSize, or the system allocation
 * granularity, whichever is greater. Block sizes are most likely multiples of 64K on Win32.
 * The first 64 bytes of a block contains the MemoryBlock header, so the available size returned
 * in the block will be the total allocation size (multiple of 64K) minus 64.
 */
MemoryBlock* pushBlock(
	MemoryArena& arena,
	size_t minimumSize = 0);

/**
 * Returns true if the last block is freed, false if the list is empty
 */
bool popBlock(
	MemoryArena& arena);

/**
 * Frees a block from any spot within the arena list, or not belonging to an arena.
 */
void removeBlock(
	MemoryBlock* block);


struct TemporaryMemory {
	MemoryBlock*	blockStart;
	uintptr_t		usedStart;

	explicit TemporaryMemory(MemoryBlock* bs, uintptr_t us) :
		blockStart{ bs },
		usedStart{ us }
	{}
	TemporaryMemory() : blockStart{0}, usedStart{0} {}
	TemporaryMemory(const TemporaryMemory& t) = delete;
	TemporaryMemory(TemporaryMemory&& t) :
		blockStart{ t.blockStart },
		usedStart{ t.usedStart }
	{
		t.blockStart = nullptr;
		t.usedStart = 0;
	}
	~TemporaryMemory()
	{
		assert(blockStart == nullptr && "TemporaryMemory token destroyed before it was ended");
		end();
	}

	void end()
	{
		if (blockStart)
		{
			// clear all memory forward of the start block/position, this works because temporary
			// memory is always taken from the lastBlock of an arena
			clearForwardOf(blockStart, usedStart);
			
			blockStart = nullptr;
			usedStart = 0;
		}
	}

	void keep()
	{
		assert(blockStart->used >= usedStart && "Temporary memory has already been freed, possibly by overlapping temporary memory");
		blockStart = nullptr;
		usedStart = 0;
	}
};

TemporaryMemory beginTemporaryMemory(
	MemoryArena& arena)
{
	// Note: temporary memory MUST come from the last block in the list, not necessarily the
	// currentBlock, because for unwinding to work, all memory ahead of the start must be unused
	return TemporaryMemory(arena.lastBlock, arena.lastBlock->used);
}

void endTemporaryMemory(
	TemporaryMemory& tm)
{
	tm.end();
}

void keepTemporaryMemory(
	TemporaryMemory& tm)
{
	tm.keep();
}


struct ScopedTemporaryMemory {
	MemoryBlock*	blockStart;
	uintptr_t		usedStart;

	explicit ScopedTemporaryMemory(MemoryBlock* bs, uintptr_t us) :
		blockStart{ bs },
		usedStart{ us }
	{}
	ScopedTemporaryMemory(const TemporaryMemory& t) = delete;
	~ScopedTemporaryMemory()
	{
		if (blockStart)
		{
			// clear all memory forward of the start block/position, this works because temporary
			// memory is always taken from the lastBlock of an arena
			clearForwardOf(blockStart, usedStart);
			
			blockStart = nullptr;
			usedStart = 0;
		}
	}
};

/**
 * ScopedTemporaryMemory is just like temporary memory but it is detroyed only when the memory
 * handle goes out of scope, so there is no need to call end. Normally the temp memory handle
 * should be stored on the stack. You cannot call keep on scoped memory.
 */
ScopedTemporaryMemory scopedTemporaryMemory(
	MemoryArena& arena)
{
	// Note: temporary memory MUST come from the last block in the list, not necessarily the
	// currentBlock, because for unwinding to work, all memory ahead of the start must be unused
	return ScopedTemporaryMemory(arena.lastBlock, arena.lastBlock->used);
}


struct BlockFitResult {
	MemoryBlock*	block;
	void* 			allocAddr;
	size_t			alignedSize;
};


/**
 * Starting at the currentBlock and moving forward in the block list, looks for the first block
 * with room for the requested allocation size, plus an alignment offset if necessary. If the end
 * of the block list is reached with no result, a new block is pushed.
 * 
 * If the new/chosen block ends up with more free space after the allocation is considered (the
 * most likely case), then the arena's currentBlock is moved forward to the new/chosen block. If
 * the opposite is true, the currentBlock is left alone while the chosen block is returned. This
 * behavior guards against the case where a large allocation exceeding the normal 64K size would
 * effectively waste a lot of free space in the currentBlock when the new block is pushed.
 */
BlockFitResult getBlockToFit(
	MemoryArena& arena,
	MemoryBlock* startBlock,
	size_t size,
	u32 align);


#define PREEMPTIVE_ALLOC_THRESHOLD	4096
/**
 * Detect when the currentBlock is nearly full and push a new block without moving the pointer
 * forward. This can be done at advantageous times in the game loop to avoid on-demand allocations
 * within the critical path resulting in frame stutters.
 */
void preemptivelyPushBlock(
	MemoryArena& arena);


void* _allocSize(
	MemoryArena& arena,
	size_t size,
	u32 align);


#define allocType(arena, Type) \
	(Type*)_allocSize(arena, sizeof(Type), alignof(Type))

#define allocArrayOfType(arena, Type, n) \
	(Type*)_allocSize(arena, sizeof(Type)*n, alignof(Type))

#endif