#ifndef _MEMORY_H
#define _MEMORY_H

#include <SDL_mutex.h>
#include "utility/common.h"

struct MemoryArena;

struct MemoryBlock {
	void*			base;
	size_t			size;
	uintptr_t		used;

	MemoryBlock*	next;
	MemoryBlock*	prev;

	MemoryArena*	arena;

	// TODO: should we store a thread id so getBlockToFit chooses a matching block?
	// this would give us safe single-threaded blocks, within an overall shared arena.
	// Would require multiple currentBlock pointers in the arena, one for each thread id.

	u64				_padding[2];
};
/**
 * MemoryBlock headers must be a multiple of 64 so they do not impact the cache
 * line alignment of a block allocation
 */
static_assert_aligned_size(MemoryBlock, 64);

struct MemoryArena {
	// TODO: do we really need to track first block?
	MemoryBlock*	firstBlock = nullptr;	// beginning of the block list
	MemoryBlock*	lastBlock = nullptr;	// end of the block list, new blocks are pushed here
	MemoryBlock*	currentBlock = nullptr;	// block currently being used for allocations (may not be the last block)
	
	SDL_mutex*		lock = nullptr;			// for thread-safe manipulation of the block list, there is not a lot of contention here

	size_t			totalSize = 0;			// total allocated size of all blocks
	u32				numBlocks = 0;
	
	// TODO: store allocator type, store flags like readonly, over/underflow protection etc.
	u32				_padding = 0;

	MemoryArena() {
		lock = SDL_CreateMutex();
	}

	~MemoryArena() {
		SDL_DestroyMutex(lock);
	}
};


/**
 * Adds a new block to the end of the list of at least minimumSize, or the system allocation
 * granularity, whichever is greater. Block sizes are most likely multiples of 64K on Win32.
 * The first 64 bytes of a block contains the MemoryBlock header, so the available size returned
 * in the block will be the total allocation size (multiple of 64K) minus 64.
 */
MemoryBlock* pushBlock(
	MemoryArena* arena,
	size_t minimumSize = 0);


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
 * 
 * This function does not take a MemoryArena as a parameter, so you can call it on a MemoryBlock
 * list that is not owned by an arena. If the startBlock belongs to an arena, arena processing
 * detailed above will not be skipped.
 */
BlockFitResult getBlockToFit(
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
	MemoryArena* arena);


void* _allocSize(
	MemoryArena* arena,
	size_t size,
	u32 align);


#define allocType(arena, Type) \
	(Type*)_allocSize(arena, sizeof(Type), alignof(Type))


#endif