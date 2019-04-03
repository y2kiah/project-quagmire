#ifndef _DENSE_QUEUE_H
#define _DENSE_QUEUE_H

/**
 * @struct dense_queue
 * dense_queue implements a queue on top of std::vector, unlike std::queue which uses deque or
 * list. This container continues to grow while items are pushed and the queue is not empty.
 * When the queue empties, the offset is set back to zero. This container should be used when
 * contiguous memory is important, and when the queue is often filled and emptied in cycles.
 */
struct dense_queue {
	// Variables
	void*	items = nullptr;
	u32		offset = 0;					// front index of the queue storage
	u32		length = 0;					// current number of objects contained in queue
	u32		capacity = 0;				// maximum number of objects that can be stored
	u32		elementSizeB = 0;			// size in bytes of individual stored objects
	u8		_memoryOwned = 0;
	u8     	_padding[7] = {};

	// Functions

	bool empty() {
		return (length == 0);
	}
	
	void* front() {
		return (length == 0 ? nullptr : item(0));
	}
	void* back() {
		return (length == 0 ? nullptr : item(length-1));
	}

	/**
	 * Pushes item to the back of the queue, returning the new item's address or nullptr on error,
	 * and optionally copying from the val pointer.
	 */
	void* push_back(void* val = nullptr);

	/**
	 * Removes item from front of queue for FIFO behavior, returning the item's address or nullptr
	 * on error, and optionally copying the value to the dst pointer.
	 */
	void* pop_front(void** dst = nullptr);
	
	/**
	 * Removes item from back of queue for LIFO behavior, returning the item's address or nullptr
	 * on error, and optionally copying the value to the dst pointer.
	 */
	void* pop_back(void** dst = nullptr);

	void reset_offset();
	
	/**
	 * Returns item at index i in the queue. Index i is relative to the queue's start offset, it
	 * should not be interpreted as a direct index into the items buffer. The index of an item does
	 * not change due a call to reset_offset.
	 */
	void* at(u32 i) {
		assert(i < length && "index out of range");
		return (length > i ? nullptr : item(i));
	}

	void* operator[](u32 i) {
		return at(i);
	}
	
	void clear();

	inline void* item(u32 i)
	{
		assert(offset + i < capacity && "index exceeds capacity");
		return (void*)((uintptr_t)items + ((offset+i) * elementSizeB));
	}

	inline void itemcpy(void* dst, void* src)
	{
		// TODO: test to make sure this "optimization" for small types to avoid memcpy is actually worthwhile
		switch (elementSizeB) {
			case 1:  *(u8*)dst = *(u8*)src; break;
			case 2:  *(u16*)dst = *(u16*)src; break;
			case 4:  *(u32*)dst = *(u32*)src; break;
			case 8:  *(u64*)dst = *(u64*)src; break;
			default: memcpy(dst, src, elementSizeB);
		}
	}

	inline void itemzero(void* dst) {
		// TODO: test to make sure this "optimization" for small types to avoid memcpy is actually worthwhile
		switch (elementSizeB) {
			case 1:  *(u8*)dst = 0; break;
			case 2:  *(u16*)dst = 0; break;
			case 4:  *(u32*)dst = 0; break;
			case 8:  *(u64*)dst = 0UL; break;
			default: memset(dst, 0, elementSizeB);
		}
	}

	explicit dense_queue(u16 elementSizeB,
						 u32 capacity,
						 void* buffer = nullptr) :
		elementSizeB{ elementSizeB },
		capacity{ capacity }
	{
		if (!buffer) {
			size_t size = elementSizeB * capacity;
			buffer = malloc(size);
			memset(buffer, 0, size);
			_memoryOwned = 1;
		}

		items = buffer;
	}

	~dense_queue() {
		if (_memoryOwned && items) {
			free(items);
		}
	}
};


void* dense_queue::push_back(void* val)
{
	void* addr = nullptr;
	if (length < capacity) {
		if (offset + length + 1 == capacity) {
			reset_offset();
		}

		addr = item(length);
		++length;

		if (val) {
			itemcpy(addr, val);
		}
		else {
			itemzero(addr);
		}
	}
	return addr;
}


void* dense_queue::pop_front(void** dst)
{
	void* addr = front();
	if (addr) {
		if (dst) {
			itemcpy(dst, addr);
		}

		if (--length == 0) {
			offset = 0;
		}
		else {
			++offset;
		}
	}
	return addr;
}


void* dense_queue::pop_back(void** dst)
{
	void* addr = back();
	if (addr) {
		if (dst) {
			itemcpy(dst, addr);
		}
		
		if (--length == 0) {
			offset = 0;
		}
	}
	return addr;
}


/**
 * Force a reset of the offset to 0, moving queued values from the end of the buffer to the front.
 */
void dense_queue::reset_offset()
{
	if (offset > 0 && length > 0) {
		memmove(items, front(), length*elementSizeB);
		offset = 0;
	}
}


void dense_queue::clear()
{
	offset = 0;
	length = 0;
	
	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear item memory to zero (slow build only) to help in debugging
	memset(items, 0, capacity*elementSizeB);
	#endif
}

#endif