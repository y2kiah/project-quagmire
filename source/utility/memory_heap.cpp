#include "memory.h"
#include "../platform/platform_api.h"


u8 getFreeTableListIndexForSize(u32 size)
{
	for (u8 li = 8; li >= 0; --li) {
		u32 listUpToSize = 1024 * (1 << (li * 2));
		if (size <= listUpToSize) {
			return li;
		}
	}
	return UCHAR_MAX;
}

void addAllocationToFreeTable(
	MemoryHeap& heap,
	HeapAllocation* halloc)
{
	assert(halloc->free && halloc->freeTableIdx == UCHAR_MAX);
	
	if (heap.freeTable) {
		HeapFreeTable& freeTable = *(HeapFreeTable*)((uintptr_t)&heap + sizeof(MemoryHeap));
		
		u8 li = getFreeTableListIndexForSize(halloc->size);
		if (li != UCHAR_MAX)
		{
			u8 freeCount = freeTable.freeCounts[li];
			if (freeCount < MEMORY_HEAP_FREETABLE_CAPACITY) {
				HeapFreeIndex& hfi = freeTable.freeLists[li][freeCount];
				hfi.heapAlloc = halloc;
				hfi.size = halloc->size;
				++freeTable.freeCounts[li];
				halloc->freeTableIdx = freeCount;
			}
		}
	}
}


void removeAllocationFromFreeTable(
	MemoryHeap& heap,
	HeapAllocation* halloc)
{
	assert(halloc->free);
	if (heap.freeTable && halloc->freeTableIdx != UCHAR_MAX) {
		HeapFreeTable& freeTable = *(HeapFreeTable*)((uintptr_t)&heap + sizeof(MemoryHeap));
		
		u8 li = getFreeTableListIndexForSize(halloc->size);
		
		assert(li != UCHAR_MAX);
		assert(freeTable.freeLists[li][halloc->freeTableIdx].heapAlloc == halloc);

		u8 freeCount = freeTable.freeCounts[li];
		if (freeCount - 1 != halloc->freeTableIdx) {
			freeTable.freeLists[li][halloc->freeTableIdx] =
				freeTable.freeLists[li][freeCount - 1];
		}
		++freeTable.freeCounts[li];	
		halloc->freeTableIdx = UCHAR_MAX;
	}
}


inline bool allocationIsWithinBlock(
	HeapAllocation* ha,
	MemoryBlock& block)
{
	return ((uintptr_t)ha >= (uintptr_t)block.base
			&& (uintptr_t)ha <= (uintptr_t)block.base + block.size);
}


MemoryBlock* pushBlock(
	MemoryHeap& heap,
	u32 minimumSize)
{
	assert(heap.threadID == SDL_ThreadID() && "MemoryHeap thread mismatch");

	// add space for at least one HeapAllocation header to the requested minimum size
	minimumSize += sizeof(HeapAllocation);
	// MemoryBlock safe to cast directly from PlatformBlock, it is asserted to be the base member
	MemoryBlock* newBlock = (MemoryBlock*)platformApi().allocate(minimumSize);
	if (newBlock) {
		newBlock->heap = &heap;
		newBlock->blockType = MemoryBlock::HeapBlock;
		newBlock->numAllocs = 1;
		newBlock->used = sizeof(HeapAllocation);

		HeapAllocation* halloc = (HeapAllocation*)newBlock->base;
		
		// clear memory to zero
		*halloc = HeapAllocation{};
		
		halloc->size = newBlock->size - sizeof(HeapAllocation);
		halloc->offset = (u32)((uintptr_t)halloc - (uintptr_t)newBlock);
		assert(halloc->size % 64 == 0 && halloc->offset == 64);

		// put new block's allocation at the back of the list
		halloc->prev = heap.back;
		heap.back = halloc;
		if (!heap.front) {
			heap.front = halloc;
		}

		// put the allocation at the back of the freelist
		halloc->free = 1;
		halloc->freeTableIdx = UCHAR_MAX;
		halloc->prevFree = heap.freeBack;
		heap.freeBack = halloc;
		if (!heap.freeFront) {
			heap.freeFront = halloc;
		}

		addAllocationToFreeTable(heap, halloc);

		// add to back of heap block linked list
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
	}
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
			removeAllocationFromFreeTable(heap, ha);
		}
		ha = ha->next;
	}
	while (ha && allocationIsWithinBlock(ha, *block));

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
	size = _align(size, 64); // size allocated in 64 byte chunks

	HeapAllocation *ha = nullptr;

	// use freetable to find an allocation
	if (heap.freeTable && heap.freeFront && size <= megabytes(16))
	{
		HeapFreeTable& freeTable = *(HeapFreeTable*)((uintptr_t)&heap + sizeof(MemoryHeap));
		if (freeTable.hasFree) {
			for (u8 li = 0; li < 8; ++li)
			{
				u32 listUpToSize = 1024 * (1 << (li * 2));
				if (size <= listUpToSize && freeTable.freeCounts[li])
				{
					// find smallest allocation in the list to fit
					u32 smallest = UINT32_MAX;
					u16 smallestIdx = UINT16_MAX;
					for (u16 hfi = 0; hfi < freeTable.freeCounts[li]; ++hfi)
					{
						u32 freeAllocSize = freeTable.freeLists[li][hfi].size;
						if (freeAllocSize < smallest) {
							smallest = freeTable.freeLists[li][hfi].size;
							smallestIdx = hfi;
						}
					}
					ha = freeTable.freeLists[li][smallestIdx].heapAlloc;
					break;
				}
			}
		}
	}

	// walk freelist to find an allocation
	if (!ha && heap.freeFront) {
		ha = heap.freeFront;
		// find first allocation to fit
		while (ha && ha->size < size) {
			ha = ha->nextFree;
		}
	}
	
	// or push a new block
	if (!ha) {
		pushBlock(heap, size);
		ha = heap.freeFront;
	}
	
	return ha;
}


/**
 * Calling this function assumes you will be allocating the first part of the split while the
 * remaining size is added to the free list.
 */
HeapAllocation* splitAllocationForSize(
	MemoryHeap& heap,
	HeapAllocation* ha,
	u32 size)
{
	if (!ha) {
		return nullptr;
	}

	size = _align(size, 64); // size allocated in 64 byte chunks

	MemoryBlock& block = *(MemoryBlock*)((uintptr_t)ha - ha->offset);

	// split the allocation if it's too large and the remaining space could fit another allocation
	// of adequate size, including the new HeapAllocation header
	if (ha->size - size - sizeof(HeapAllocation) >= MEMORY_HEAP_MIN_SPLIT_SIZE)
	{
		removeAllocationFromFreeTable(heap, ha);
		
		HeapAllocation* newAlloc = (HeapAllocation*)((uintptr_t)ha + sizeof(HeapAllocation) + size);
		*newAlloc = HeapAllocation{}; // clear memory to zero
		newAlloc->offset = ha->offset + sizeof(HeapAllocation) + size;
		newAlloc->size = ha->size - size - sizeof(HeapAllocation);
		ha->size = size;
		assert((uintptr_t)newAlloc == (uintptr_t)&block + newAlloc->offset);
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
		newAlloc->freeTableIdx = UCHAR_MAX;
		newAlloc->prevFree = ha;
		if (ha->nextFree) {
			ha->nextFree->prevFree = newAlloc;
		}
		else {
			heap.freeBack = newAlloc;
		}
		newAlloc->nextFree = ha->nextFree;
		ha->nextFree = newAlloc;

		addAllocationToFreeTable(heap, newAlloc);
		//addAllocationToFreeTable(heap, ha);
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
	
	if (heap.freeTable) {
		HeapFreeTable& freeTable = *(HeapFreeTable*)((uintptr_t)&heap + sizeof(MemoryHeap));
		freeTable.hasFree = 0;
		heap = makeMemoryHeap();
		heap.freeTable = 1;
	}
	else {
		heap = makeMemoryHeap();
	}
}


void shrinkHeap(
	MemoryHeap& heap)
{
    MemoryBlock* block = heap.firstBlock;
	while (block)
	{
		MemoryBlock* next = block->next;
		if (block->numAllocs == 1 && block->used == sizeof(HeapAllocation))
		{
			removeHeapBlock(block);
		}
		block = next;
	}
}

#define HEAP_ALLOCATION_SIGNATURE	0xDEADC0DE

void* _heapAllocSize(
	MemoryHeap& heap,
	u32 size,
	bool clearToZero)
{
	assert(heap.threadID == SDL_ThreadID() && "MemoryHeap thread mismatch");
	assert(size > 0);
	void* allocAddr = nullptr;

	HeapAllocation* ha =
		splitAllocationForSize(
			heap,
			getAllocationToFit(heap, size),
			size);
			
	if (ha) {
		MemoryBlock& block = *(MemoryBlock*)((uintptr_t)ha - ha->offset);
		block.used += ha->size;
		// remove allocation from freelist
		removeAllocationFromFreeTable(heap, ha);
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

		allocAddr = (void*)((uintptr_t)ha + sizeof(HeapAllocation));
		if (clearToZero) {
			memset(allocAddr, 0, ha->size);
		}
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
	assert(addr);

	HeapAllocation* ha = (HeapAllocation*)((uintptr_t)addr - sizeof(HeapAllocation));
	assert(ha->signature == HEAP_ALLOCATION_SIGNATURE && !ha->free);
	assert(!ha->refCount);
	
	MemoryBlock& block = *(MemoryBlock*)((uintptr_t)ha - ha->offset);
	MemoryHeap& heap = *block.heap;

	// check if we can merge into the previous allocation
	if (ha->prev
		&& ha->prev->free
		&& allocationIsWithinBlock(ha->prev, block))
	{
		HeapAllocation* merge = ha->prev;
		removeAllocationFromFreeTable(heap, merge);

		if (ha->next) {
			ha->next->prev = merge;
		}
		else {
			heap.back = merge;
		}
		merge->next = ha->next;
		
		u32 mergedSize = (u32)sizeof(HeapAllocation) + ha->size;
		merge->size += mergedSize;

		--block.numAllocs;
		block.used -= mergedSize;
		
		ha = merge;
	}
	// if we can't merge with previous allocation, make this allocation free
	else {
		ha->free = 1;
		ha->freeTableIdx = UCHAR_MAX;
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
	if (ha->next
		&& ha->next->free
		&& allocationIsWithinBlock(ha->next, block))
	{
		HeapAllocation* merge = ha->next;
		removeAllocationFromFreeTable(heap, merge);

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

	addAllocationToFreeTable(heap, ha);
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