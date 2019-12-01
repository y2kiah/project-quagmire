#ifndef _MEMORY_H
#define _MEMORY_H

#include <SDL_mutex.h>
#include <SDL_thread.h>
#include "common.h"

struct MemoryArena;
struct MemoryHeap;

struct MemoryBlock {
	enum MemoryBlockType : u8 { ArenaBlock, HeapBlock };

	void*				base;
	u32					size;
	
	u32					used;		// for MemoryArenas, used bytes in block
									// for MemoryHeaps, non-free allocated bytes plus all HeapAllocation header bytes
	u32					numAllocs;	// for MemoryHeaps only, number of allocations in block

	MemoryBlockType		blockType;
	u8					_padding[3];

	// linked list for blocks in the arena/heap
	MemoryBlock*		next;
	MemoryBlock*		prev;
	
	// TODO: this will result in dangling pointer if arena is destroyed and block remins in the platform list
	// should this be a handle to an arena instead?
	// or should we always free blocks when arena is destroyed and set this null
	union {
		MemoryArena*	arena;
		MemoryHeap*		heap;
	};
};


struct PlatformBlock {
	MemoryBlock			memoryBlock;
	// linked list for all allocated blocks
	PlatformBlock*		next;
	PlatformBlock*		prev;
};
/**
 * PlatformBlock headers must be a multiple of 64 so they do not impact the cache
 * line alignment of a block allocation
 */
static_assert_aligned_size(PlatformBlock, 64);
static_assert(offsetof(PlatformBlock, memoryBlock) == 0, "MemoryBlock must be first member of PlatformBlock");


struct MemoryArena {
	// TODO: do we really need to track first block?
	MemoryBlock*	firstBlock;		// beginning of the block list
	MemoryBlock*	lastBlock;		// end of the block list, new blocks are pushed here
	MemoryBlock*	currentBlock;	// block currently being used for allocations (may not be the last block)

	size_t			totalSize;		// total available capacity in all blocks (not including space for PlatformBlock headers)
	u32				numBlocks;
	u32				_padding;
	u64				threadID;		// threadID is tracked to later assert the threadID matches on allocations
	// TODO: store debug flags like readonly, over/underflow protection, etc.?
};


struct HeapAllocation {
	HeapAllocation*	prev;			// linked list of allocations
	HeapAllocation*	next;
	HeapAllocation*	prevFree;		// linked list of free allocations
	HeapAllocation*	nextFree;
	u32				offset;			// byte offset from MemoryBlock base to HeapAllocation header
	u32				size;			// size of block not including 64-byte header
	u32				requestedSize;	// size requested, which may be smaller than allocation size
	u16				refCount;		// reference count, optional
	u8				free;
	u8				freeTableIdx;	// index in heap's free table (if exists) or 0xFF if not in table
	u8				_padding[12];
	u32				signature;		// known signature for debug checking that an address passed to freeAlloc is actually
									// a valid HeapAllocation, the 4 bytes preceding the memory address must contain this value
};
static_assert(sizeof(HeapAllocation) == 64, "");


struct MemoryHeap {
	MemoryBlock*	firstBlock;		// beginning of the block list
	MemoryBlock*	lastBlock;		// end of the block list, new blocks are pushed here
	
	HeapAllocation*	front;			// allocation block linked list, all allocations including those in free list
	HeapAllocation*	back;
	HeapAllocation*	freeFront;		// free list of allocation blocks
	HeapAllocation*	freeBack;

	size_t			totalSize;		// total capacity in all blocks, not including space for the PlatformBlock
									// headers, but including all space used by HeapAllocation headers
	
	u32				numBlocks : 31;
	u32				freeTable : 1;

	u64				threadID;		// threadID is tracked to later assert the threadID matches on allocations
};


/**
 * PlatformMemory tracks a thread-safe linked list of allocated memory blocks belonging to all
 * MemoryArenas and MemoryHeaps across the system.
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


// MemoryArena Functions

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
	u32 usedStart);

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
	u32 minimumSize = 0);

/**
 * Returns true if the last block is freed, false if the list is empty
 */
bool popBlock(
	MemoryArena& arena);

/**
 * Frees a block from any spot within the arena list.
 */
void removeArenaBlock(
	MemoryBlock* block);


struct TemporaryMemory {
	MemoryBlock*	blockStart;
	u32				usedStart;

	explicit TemporaryMemory(MemoryBlock* bs, u32 us) :
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
	u32				usedStart;

	explicit ScopedTemporaryMemory(MemoryBlock* bs, u32 us) :
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
	u32				alignedSize;
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
	u32 size,
	u32 align);


/**
 * Detect when the currentBlock is nearly full and push a new block without moving the pointer
 * forward. This can be done at advantageous times in the game loop to avoid on-demand allocations
 * within the critical path resulting in frame stutters.
 */
void preemptivelyPushBlock(
	MemoryArena& arena);


void* _allocSize(
	MemoryArena& arena,
	u32 size,
	u32 align);

char* allocStringNCopy(
	MemoryArena& arena,
	const char* src,
	u32 size);

char* allocStringCopy(
	MemoryArena& arena,
	const char* src);

#define allocType(arena, Type) \
	(Type*)_allocSize(arena, sizeof(Type), alignof(Type))

#define allocArrayOfType(arena, Type, n) \
	(Type*)_allocSize(arena, sizeof(Type)*n, alignof(Type))

#define allocBuffer(arena, size, align) \
	(u8*)_allocSize(arena, size, align)


// MemoryHeap Functions

MemoryHeap makeMemoryHeap()
{
	MemoryHeap heap{};
	heap.threadID = SDL_ThreadID();
	return heap;
}


struct HeapFreeIndex {
	HeapAllocation*	heapAlloc;
	u32				size;
	u32				_padding;
};

constexpr const int MHFC = MEMORY_HEAP_FREETABLE_CAPACITY;

struct HeapFreeTable {
	union {
		struct {
			HeapFreeIndex	free1K[MHFC];	// [512, 1K]
			HeapFreeIndex	free4K[MHFC];	// (1K,4K]
			HeapFreeIndex	free16K[MHFC];	// (4K,16K]
			HeapFreeIndex	free64K[MHFC];	// (16K,64K]
			HeapFreeIndex	free256K[MHFC];	// (64K,256K]
			HeapFreeIndex	free1M[MHFC];	// (256K,1M]
			HeapFreeIndex	free4M[MHFC];	// (1M,4M]
			HeapFreeIndex	free16M[MHFC];	// (4M,16M]
		};
		HeapFreeIndex	freeLists[8][MHFC];
	};
	union {
		struct {
			u8	count1K;
			u8	count4K;
			u8	count16K;
			u8	count64K;
			u8	count256K;
			u8	count1M;
			u8	count4M;
			u8	count16M;
		};
		u8		freeCounts[8];
		u64		hasFree;
	};
};


MemoryHeap* makeMemoryHeapWithFreeTable(
	MemoryArena& arena)
{
	u32 totalSize = sizeof(MemoryHeap) + sizeof(HeapFreeTable);
	MemoryHeap* heap = (MemoryHeap*)allocBuffer(arena, totalSize, alignof(MemoryHeap));
	heap->threadID = SDL_ThreadID();
	heap->freeTable = 1;
	return heap;
}

void clearHeap(
	MemoryHeap& heap);

void shrinkHeap(
	MemoryHeap& heap);

/**
 * Adds a new block to the end of the list of at least minimumSize, or the system allocation
 * granularity, whichever is greater. Block sizes are most likely multiples of 64K on Win32.
 * The first 64 bytes of a block contains the MemoryBlock header, so the available size returned
 * in the block will be the total allocation size (multiple of 64K) minus 64.
 */
MemoryBlock* pushBlock(
	MemoryHeap& heap,
	u32 minimumSize = 0);

/**
 * Frees a block from any spot within the heap list.
 */
void removeHeapBlock(
	MemoryBlock* block);

/**
 * Starting at the front of the freelist and moving along the list, looks for the first allocation
 * with room for the requested allocation size, plus an alignment offset if necessary. If the end
 * of the freelist is reached with no result, a new block is pushed.
 */
HeapAllocation* getAllocationToFit(
	MemoryHeap& heap,
	u32 size);


/**
 * Detect when the heap is nearly full and push a new block. This can be done at advantageous
 * times in the game loop to avoid on-demand allocations within the critical path resulting in
 * frame stutters.
 */
void preemptivelyPushBlock(
	MemoryHeap& heap);


void* _heapAllocSize(
	MemoryHeap& heap,
	u32 size,
	bool clearToZero = true);

char* heapAllocStringNCopy(
	MemoryHeap& heap,
	const char* src,
	u32 size);

char* heapAllocStringCopy(
	MemoryHeap& heap,
	const char* src);

#define heapAllocType(arena, Type) \
	(Type*)_heapAllocSize(arena, sizeof(Type), true)

#define heapAllocArrayOfType(arena, Type, n) \
	(Type*)_heapAllocSize(arena, sizeof(Type)*n, true)

#define heapAllocBuffer(arena, size, clearToZero) \
	(u8*)_heapAllocSize(arena, size, clearToZero)


/**
 * Frees a heap allocation. Pass the data address, not the address of the HeapAllocation header.
 * Calling freeAlloc directly when there is a refCount remaining asserts.
 */
void freeAlloc(void* addr);

/**
 * Increments the reference count for a heap allocation. Pass the data address, not the address of
 * the HeapAllocation header. Once the first call to addRef is made, the memory becomes reference
 * counted, and must be freed via symmetric calls to releaseRef.
 * @returns the new reference count
 */
u16 addRef(void* addr);

/**
 * Decrements the reference count for a heap allocation. Pass the data address, not the address of
 * the HeapAllocation header. Calls to releaseRef for a non-reference-counted allocation asserts.
 * If the reference count remaining is 0, the allocation is freed.
 * @returns the new reference count
 */
u16 releaseRef(void* addr);

#endif