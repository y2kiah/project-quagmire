#include "memory.h"
#include "platform/platform_api.h"


MemoryBlock* pushBlock(
	MemoryHeap& heap,
	u32 minimumSize)
{
	assert(heap.threadID == SDL_ThreadID() && "MemoryHeap thread mismatch");

	// add space for at least one HeapAllocation header to the requested minimum size
	minimumSize += sizeof(HeapAllocation);
	// MemoryBlock safe to cast directly from PlatformBlock, it is asserted to be the base member
	MemoryBlock* newBlock = (MemoryBlock*)platformApi().allocate(minimumSize);
	newBlock->blockType = MemoryBlock::HeapBlock;
	newBlock->numAllocs = 1;
	newBlock->used = sizeof(HeapAllocation);

	HeapAllocation* halloc = (HeapAllocation*)newBlock->base;
	// clear memory to zero
	*halloc = HeapAllocation{};
	halloc->size = newBlock->size - sizeof(HeapAllocation);
	// put new block's allocation at the back of the list
	halloc->prev = heap.back;
	heap.back = halloc;
	if (!heap.front) {
		heap.front = halloc;
	}

	// put the allocation at the back of the freelist
	halloc->free = 1;
	halloc->prevFree = heap.freeBack;
	heap.freeBack = halloc;
	if (!heap.freeFront) {
		heap.freeFront = halloc;
	}

	newBlock->prev = heap.lastBlock;
	if (heap.lastBlock) {
		heap.lastBlock->next = newBlock;
	}
	else {
		// pushing first block in the heap
		heap.firstBlock = heap.lastBlock = newBlock;
	}
	heap.totalSize += newBlock->size;
	++heap.numBlocks;

	return newBlock;
}


void removeHeapBlock(
	MemoryBlock* block)
{
	assert(block && block->blockType == MemoryBlock::HeapBlock);
	assert(block->heap);

	MemoryHeap& heap = *block->heap;
	
	assert(heap.threadID == SDL_ThreadID() && "MemoryHeap thread mismatch");

	if (block->next) {
		block->next->prev = block->prev;
	}
	else {
		heap.lastBlock = block->prev;
	}

	if (block->prev) {
		block->prev->next = block->next;
	}
	else {
		heap.firstBlock = block->next;
	}

	--heap.numBlocks;
	heap.totalSize -= block->size;

	// remove blocks's allocations from list and free list
	HeapAllocation* ha = (HeapAllocation*)block->base;
	HeapAllocation* lastBeforeBlock = ha->prev;
	HeapAllocation* lastFreeBeforeBlock = nullptr;
	HeapAllocation* firstFreeAfterBlock = nullptr;
	do {
		if (ha->free) {
			if (!lastFreeBeforeBlock) {
				lastFreeBeforeBlock = ha->prevFree;
			}
			firstFreeAfterBlock = ha->nextFree;
		}
		ha = ha->next;
	}
	while (ha && (uintptr_t)ha <= (uintptr_t)block->base + block->size);
	HeapAllocation* firstAfterBlock = ha;

	if (lastBeforeBlock) {
		lastBeforeBlock->next = firstAfterBlock;
	}
	else {
		heap.front = firstAfterBlock;
	}
	if (firstAfterBlock) {
		firstAfterBlock->prev = lastBeforeBlock;
	}
	else {
		heap.back = lastBeforeBlock;
	}

	if (lastFreeBeforeBlock) {
		lastFreeBeforeBlock->next = firstFreeAfterBlock;
	}
	else {
		heap.freeFront = firstFreeAfterBlock;
	}
	if (firstFreeAfterBlock) {
		firstFreeAfterBlock->prev = lastFreeBeforeBlock;
	}
	else {
		heap.freeBack = lastFreeBeforeBlock;
	}

	platformApi().deallocate((PlatformBlock*)block);
}


HeapAllocation* getAllocationToFit(
	MemoryHeap& heap,
	u32 size)
{
	u32 requestedSize = size;
	size = _align(size, 64); // size allocated in 64 byte chunks

	HeapAllocation *ha = heap.freeFront;
	// find first allocation to fit
	while (ha && ha->size < size) {
		ha = ha->nextFree;
	}
	// or push a new block
	if (!ha) {
		pushBlock(heap, size);
		ha = heap.freeFront;
	}

	MemoryBlock& block = *(MemoryBlock*)((uintptr_t)ha - ha->offset);
	
	// split the allocation if it's too large and the remaining space could fit another allocation
	// of adequate size, including the new HeapAllocation header
	if (ha->size - size - sizeof(HeapAllocation) >= MEMORY_HEAP_MIN_SPLIT_SIZE)
	{
		HeapAllocation* newAlloc = (HeapAllocation*)((uintptr_t)ha + sizeof(HeapAllocation) + size);
		*newAlloc = HeapAllocation{}; // clear memory to zero
		newAlloc->offset = ha->offset + sizeof(HeapAllocation) + size;
		newAlloc->size = ha->size - size - sizeof(HeapAllocation);
		ha->size = size;
		assert((uintptr_t)newAlloc == (uintptr_t)ha + newAlloc->offset);
		++block.numAllocs;
		block.used += sizeof(HeapAllocation);
		
		newAlloc->prev = ha;
		if (ha->next) {
			ha->next->prev = newAlloc;
		}
		else {
			heap.back = newAlloc;
		}
		newAlloc->next = ha->next;
		ha->next = newAlloc;

		newAlloc->free = 1;
		newAlloc->prevFree = ha;
		if (ha->nextFree) {
			ha->nextFree->prevFree = newAlloc;
		}
		else {
			heap.freeBack = newAlloc;
		}
		newAlloc->nextFree = ha->nextFree;
		ha->nextFree = newAlloc;
	}
	
	return ha;
}


void preemptivelyPushBlock(
	MemoryHeap& heap)
{
	assert(heap.threadID == SDL_ThreadID() && "MemoryHeap thread mismatch");

	if (heap.front && !heap.freeFront) {
		pushBlock(heap);
	}
}


void clearHeap(
	MemoryHeap& heap)
{
	MemoryBlock* block = heap.firstBlock;
	while (block) {
		MemoryBlock* next = block->next;
		platformApi().deallocate((PlatformBlock*)block);
		block = next;
	}
	heap = makeMemoryHeap();
}


void shrinkHeap(
	MemoryHeap* heap)
{
    MemoryBlock* block = heap->firstBlock;
	while (block)
	{
		if (block->numAllocs == 1 && block->used == sizeof(HeapAllocation))
		{
			removeHeapBlock(block);
		}
	}
}

#define HEAP_ALLOCATION_SIGNATURE	0xDEADC0DE

void* _heapAllocSize(
	MemoryHeap& heap,
	u32 size,
	bool clearToZero)
{
	assert(heap.threadID == SDL_ThreadID() && "MemoryHeap thread mismatch");

	HeapAllocation* ha = getAllocationToFit(heap, size);
	MemoryBlock& block = *(MemoryBlock*)((uintptr_t)ha - ha->offset);
	block.used += ha->size;
	// remove allocation from freelist
	ha->free = 0;
	ha->requestedSize = size;
	ha->signature = HEAP_ALLOCATION_SIGNATURE;
	if (ha->prevFree) {
		ha->prevFree->nextFree = ha->nextFree;
	}
	else {
		heap.freeFront = ha->nextFree;
	}
	if (ha->nextFree) {
		ha->nextFree->prevFree = ha->prevFree;
	}
	else {
		heap.freeBack = ha->prevFree;
	}

	void* allocAddr = (void*)((uintptr_t)ha + sizeof(HeapAllocation));
	if (clearToZero) {
		memset(allocAddr, 0, ha->size);
	}

	return allocAddr;
}


char* heapAllocStringNCopy(
	MemoryHeap& heap,
	const char* src,
	u32 size)
{
	assert(src);
	char* dest = (char*)_heapAllocSize(heap, size+1); // +1 for null terminating character
	_strncpy_s(dest, size+1, src, size);
	return dest;
}


char* heapAllocStringCopy(
	MemoryHeap& heap,
	const char* src)
{
	return heapAllocStringNCopy(heap, src, (u32)strlen(src));
}


void freeAlloc(void* addr)
{
	HeapAllocation* ha = (HeapAllocation*)((uintptr_t)addr - sizeof(HeapAllocation));
	assert(ha->signature == HEAP_ALLOCATION_SIGNATURE && !ha->free);
	assert(!ha->refCount);
	MemoryBlock& block = *(MemoryBlock*)((uintptr_t)ha - ha->offset);
	MemoryHeap& heap = *block.heap;

	// check if we can merge into the previous allocation
	if (ha->prev && ha->prev->free)
	{
		if (ha->next) {
			ha->next->prev = ha->prev;
		}
		else {
			heap.back = ha->prev;
		}
		ha->prev->next = ha->next;
		
		u32 mergedSize = (u32)sizeof(HeapAllocation) + ha->size;
		ha->prev->size += mergedSize;

		--block.numAllocs;
		block.used -= mergedSize;
		
		ha = ha->prev;
	}
	// if we can't merge with previous allocation, make this allocation free
	else {
		ha->free = 1;
		ha->signature = 0;
		ha->requestedSize = 0;

		ha->prevFree = heap.freeBack;
		heap.freeBack = ha;
		if (!heap.freeFront) {
			heap.freeFront = ha;
		}

		block.used -= ha->size;
	}

	// check if we can merge with the next allocation
	if (ha->next && ha->next->free)
	{
		HeapAllocation* merge = ha->next;
		if (merge->next) {
			merge->next->prev = ha;
		}
		else {
			heap.back = ha;
		}
		ha->next = merge->next;

		if (merge->prevFree) {
			merge->prevFree->nextFree = merge->nextFree;
		}
		else {
			heap.freeFront = merge->nextFree;
		}
		if (merge->nextFree) {
			merge->nextFree->prevFree = merge->prevFree;
		}
		else {
			heap.freeBack = merge->prevFree;
		}

		u32 mergedSize = sizeof(HeapAllocation) + ha->next->size;
		ha->size += mergedSize;

		--block.numAllocs;
		block.used -= sizeof(HeapAllocation);
	}
}

u16 addRef(void* addr)
{
	HeapAllocation* ha = (HeapAllocation*)((uintptr_t)addr - sizeof(HeapAllocation));
	assert(ha->signature == HEAP_ALLOCATION_SIGNATURE && !ha->free);
	return ++ha->refCount;
}

u16 releaseRef(void* addr)
{
	HeapAllocation* ha = (HeapAllocation*)((uintptr_t)addr - sizeof(HeapAllocation));
	assert(ha->signature == HEAP_ALLOCATION_SIGNATURE && !ha->free);
	assert(ha->refCount);
	u16 newCount = --ha->refCount;
	if (newCount == 0) {
		freeAlloc(addr);
	}
	return newCount;
}