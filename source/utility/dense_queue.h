#ifndef _DENSE_QUEUE_H
#define _DENSE_QUEUE_H
/**
 * The containers in this library are meant to store POD (plain-old-data) types in cache-friendly
 * dense buffers, where items are kept contiguous and tightly packed whenever possible.
 * 
 * C++ classes with non-trivial constructors/destructors like those that manage resources through
 * the RAII pattern, smart pointers, etc., should only be stored if new/delete are invoked
 * externally and direct memcpy of the objects is deemed safe, however this is not recommended.
 * 
 * The container implementations make use of both C and C++ patterns. C style memory management is
 * used for performance and flexibility, alongside C++ member functions and constructors to provide
 * a cohesive and terse API.
 * 
 * For the purpose of storage in a "generic" container, type erasure is achieved through the use of
 * void*. This avoids the compilation overhead and type system implications of C++ templates. This
 * also enables storage of disparate types (of the same size) for free, without the need for C++
 * solutions like abstract base classes and virtual functions.
 * 
 * It is advisable to wrap calls to the containers in corresponding type safe functions in order to
 * make usage code safer and more readable.
 * 
 * You retain ultimate flexibility when it comes to memory management. You can pass a pointer to
 * pre-allocated memory to the containers, granting control over the memory but not over its
 * lifetime. Alternatively the containers will allocate and free their own buffers in RAII fashion.
 * New items can be created in-place with their memory zero'd, or they can be copied in from
 * existing objects. You generally always get the option to copy or ignore removed objects as well,
 * since item memory is zero'd on insertion but not on removal. Items are treated as raw bytes at
 * the container level, so constructors and destructors are never invoked automatically.
 * 
 * All containers provide O(1) random access to items in memory due to the use of fixed size
 * buffers as backing storage. The semantics of each particular container type are implemented on
 * top of basic array semantics. Item indices/keys are stable across long-term accesses unless
 * otherwise stated, but pointers returned should always be used immediately and discarded. Most
 * containers will move items around in memory to keep them tightly packed.
 * 
 * The containers generally assert when they reach capacity rather than growing via reallocate or
 * new blocks. Usage code must be written to always work within the pre-defined capacity. If you
 * assert during testing, it means you need to 1) increase the capacity, or 2) not allow usage code
 * to exceed capacity under any circumstance.
 * 
 * The training wheels are off! It's your responsibility to understand and use the containers in a
 * correct and safe way. Nothing is hidden, there are no private members, everything is open. So,
 * if you want to get funky with memory there is nothing stopping you. It's assumed you will
 * understand how the memory is managed under the hood so you don't invalide things.
 */

/**
 * @struct DenseQueue
 * DenseQueue implements a queue on top of a fixed-size ring buffer, allowing for pushing and
 * popping from both ends of the queue. For queue semantics push, pop_fifo, and pop_lifo helper
 * methods remove the any ambiguity with the use of push_front/push_back and pop_front/pop_back.
 * 
 * The at and operator[] index are relative to queue order, not the in-memory order. Items are
 * contiguous in memory until the end of the buffer is reached, at which point the index wraps to
 * the start of the buffer.
 */
struct DenseQueue {
	// Variables
	void*	items = nullptr;
	u32		frontCursor = 0;		// front index of the queue storage
	u32		backCursor = 0;			// back index of the queue storage
	u32		length = 0;				// current number of objects contained in queue
	u32		capacity = 0;			// maximum number of objects that can be stored
	u32		elementSizeB = 0;		// size in bytes of individual stored objects
	u8		_memoryOwned = 0;
	u8     	_padding[7] = {};

	// Functions

	explicit DenseQueue(u16 elementSizeB,
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

	~DenseQueue() {
		if (_memoryOwned && items) {
			free(items);
		}
	}


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
	 * Pushes item to the back of the queue.
	 * @param	dst		optional, pass this to copy the val into the new item
	 * and optionally copying from the val pointer.
	 * @returns pointer to new item, or nullptr if the container is full
	 */
	void* push_back(void* val = nullptr);

	/**
	 * Pushes item to the front of the queue.
	 * @param	dst		optional, pass this to copy the val into the new item
	 * and optionally copying from the val pointer.
	 * @returns pointer to new item, or nullptr if the container is full
	 */
	void* push_front(void* val = nullptr);

	/**
	 * Removes item from front of queue for FIFO behavior when push_back is used.
	 * @param	dst		optional, pass this to copy the popped value to the dst pointer
	 * @returns the item's address or nullptr on error
	 */
	void* pop_front(void** dst = nullptr);
	
	/**
	 * Removes item from front of queue for LIFO behavior when push_back is used.
	 * on error, and optionally copying the value to the dst pointer.
	 * @param	dst		optional, pass this to copy the popped value to the dst pointer
	 * @returns the item's address or nullptr on error
	 */
	void* pop_back(void** dst = nullptr);

	/**
	 * Use push, pop_fifo and pop_lifo for worry-free queue semantics.
	 */
	inline void* push(void* val = nullptr) { return push_back(val); }
	inline void* pop_fifo(void** dst = nullptr) { return pop_front(dst); }
	inline void* pop_lifo(void** dst = nullptr) { return pop_back(dst); }

	/**
	 * Pointers obtained from at and operator[] should not be stored or used across calls to
	 * realign, this should only be called when guaranteed not to create bugs. The index used to
	 * look up items is not invalidated by this call. Use this sparingly or not at all, since
	 * emptying the queue will also realign for new items without the cost of swapping memory.
	 */
	void realign();
	
	/**
	 * @param	i	index relative to the queue's front, it should not be interpreted as a direct
	 *	offset into the items buffer. The index of an item is stable across calls to realign,
	 *	whereas the pointer returned may be erroneous.
	 * @returns item at index i in the queue.
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
		assert(frontCursor + i < capacity && "index exceeds capacity");
		return (void*)((uintptr_t)items + ((frontCursor+i) * elementSizeB));
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
};
static_assert_aligned_size(DenseQueue,8);


void* DenseQueue::push_back(void* val)
{
	void* addr = nullptr;
	if (length < capacity) {
		if (frontCursor + length + 1 == capacity) {
			//realign();
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


void* DenseQueue::pop_front(void** dst)
{
	void* addr = front();
	if (addr) {
		if (dst) {
			itemcpy(dst, addr);
		}

		if (--length == 0) {
			frontCursor = 0;
		}
		else {
			++frontCursor;
		}
	}
	return addr;
}


void* DenseQueue::pop_back(void** dst)
{
	void* addr = back();
	if (addr) {
		if (dst) {
			itemcpy(dst, addr);
		}
		
		if (--length == 0) {
			frontCursor = 0;
		}
	}
	return addr;
}


/**
 * Force a reset of the frontCursor to 0, rotating queued values so they are contiguous in memory.
 */
void DenseQueue::realign()
{
	// TODO: implement this
	assert(!"need algorithm to rotate");
	//if (frontCursor > 0 && length > 0) {
	//	memmove(items, front(), length*elementSizeB);
	//	frontCursor = 0;
	//}
}


void DenseQueue::clear()
{
	frontCursor = 0;
	length = 0;
	
	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear item memory to zero (slow build only) to help in debugging
	memset(items, 0, capacity*elementSizeB);
	#endif
}

#endif