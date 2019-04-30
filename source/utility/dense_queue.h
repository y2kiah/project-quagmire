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

#include <cstdlib>
#include <cstring>
#include "types.h"

/**
 * @struct DenseQueue
 * DenseQueue implements a queue on top of a fixed-size ring buffer, allowing for pushing and
 * popping from both ends of the queue. For queue semantics push, pop_fifo, and pop_lifo helper
 * methods remove the any ambiguity with the use of push_front/push_back and pop_front/pop_back.
 * 
 * The at and operator[] index are relative to queue order, not the in-memory order. Items are
 * contiguous in memory until the end of the buffer is reached, at which point the index wraps to
 * the start of the buffer.
 * Examples:
 * 		capacity=7
 *		| 0 | 1 | 2 | 3 | 4 | 5 | 6 |
 *		|	F-*---*---*---*-B       | frontCursor=1, length=4
 *		|	    F-*---*-B           | frontCursor=2, length=2
 *		|-*---*-B           F-*---*-| frontCursor=5, length=4
 */
struct DenseQueue {
	// Variables
	void*	items = nullptr;
	u32		frontCursor = 0;		// front index of the queue storage
	u32		length = 0;				// current number of objects contained in queue
	u32		capacity = 0;			// maximum number of objects that can be stored
	u16		elementSizeB = 0;		// size in bytes of individual stored objects
	u8		assertOnFull = 1;		// set to 0 if overflow is handled by usage code
	u8		_memoryOwned = 0;		// set to 1 if buffer memory is owned by DenseQueue

	// Functions

	DenseQueue() {}
	
	explicit DenseQueue(
		u16 _elementSizeB,
		u32 _capacity,
		void* buffer = nullptr,
		u8 _assertOnFull = 1)
	{
		init(_elementSizeB, _capacity, buffer, _assertOnFull);
	}

	~DenseQueue() {
		deinit();
	}


	inline bool empty() {
		return (length == 0);
	}

	inline bool full() {
		return (length == capacity);
	}
	
	/**
	 * @returns address of the front item, or nullptr if empty
	 */
	inline void* front() {
		return (length == 0 ? nullptr : item(0));
	}

	/**
	 * @returns address of the back item, or nullptr if empty
	 */
	inline void* back() {
		return (length == 0 ? nullptr : item(length-1));
	}

	/**
	 * @returns address of the next open item at the back (back+1), or nullptr if full
	 */
	inline void* nextBack() {
		return (length < capacity ? item(length) : nullptr);
	}

	/**
	 * Since this is a circular buffer, free slots may wrap to the beginning before the buffer is
	 * full, so multi-n writes into the buffer may have to be split into 2 writes. This function
	 * returns the n count of the first (and possible only) contiguous section.
	 */
	inline u32 maxContiguous() {
		u32 mc = 0;
		u32 backCursor = ((u64)frontCursor + length) % capacity;
		return (backCursor < frontCursor)
			? (frontCursor - backCursor)
			: (capacity - backCursor);
	}

	/**
	 * Pushes item to the back of the queue.
	 * @param[in]	val		optional, pass this to copy the val into the new item
	 *	and optionally copying from the val pointer.
	 * @param[in]	zero	if val is nullptr, pass true to zero the new item memory
	 * @returns pointer to new item, or nullptr if the container is full
	 */
	void* push_back(void* val = nullptr, bool zero = true);

	/**
	 * Pushes n items to the back of the queue. This is done using one or two bulk memory copies,
	 * since items may be split by wrapping. If adding n items would exceed capacity, no items are
	 * copied and nullptr is returned.
	 * @param[in]	n		number of items to push
	 * @param[in]	val		optional, pass this to copy n vals into the new items
	 *	and optionally copying from the val pointer.
	 * @param[in]	zero	if val is nullptr, pass true to zero the new item memory
	 * @returns pointer to first new item, or nullptr if the container is full
	 */
	void* push_back_n(u32 n, void* vals = nullptr, bool zero = true);

	/**
	 * Pushes item to the front of the queue.
	 * @param[in]	val		optional, pass this to copy the val into the new item
	 *	and optionally copying from the val pointer.
	 * @param[in]	zero	if val is nullptr, pass true to zero the new item memory
	 * @returns pointer to new item, or nullptr if the container is full
	 */
	void* push_front(void* val = nullptr, bool zero = true);
	
	/**
	 * Removes item from back of queue for LIFO behavior when push_back is used.
	 * @param[out]	dst		optional, pass this to copy the popped value to the dst pointer
	 * @returns the item's address or nullptr on error
	 */
	void* pop_back(void* dst = nullptr);

	/**
	 * Removes item from front of queue for FIFO behavior when push_back is used.
	 * @param[out]	dst		optional, pass this to copy the popped value to the dst pointer
	 * @returns the item's address or nullptr on error
	 */
	void* pop_front(void* dst = nullptr);

	/**
	 * Removes up to n items from front of queue for FIFO behavior when push_back is used. This is
	 * done using one or two bulk memory copies, since items may be split by wrapping.
	 * @param[in]	n		max number of items to pop, pass 0 to pop all items
	 * @param[out]	dst		optional, pass this to copy the popped value to the dst pointer
	 * @returns the number items popped
	 */
	u32 pop_front_n(u32 n, void* dst);

	/**
	 * Use push, pop_fifo and pop_lifo for worry-free queue semantics.
	 */
	inline void* push(void* val = nullptr, bool zero = true) { return push_back(val,zero); }
	inline void* push_n(u32 n, void* val = nullptr, bool zero = true) { return push_back_n(n,val,zero); }
	inline void* pop_fifo(void* dst = nullptr) { return pop_front(dst); }
	inline void* pop_lifo(void* dst = nullptr) { return pop_back(dst); }
	inline u32   pop_fifo_n(u32 n, void* dst = nullptr) { return pop_front_n(n,dst); }
	
	/**
	 * @param[in]	i	index relative to the queue's front, it should not be interpreted as a
	 *	direct offset into the items buffer.
	 * @returns item at index i in the queue.
	 */
	void* at(u32 i) {
		assert(i < length && "index out of range");
		return (i < length ? item(i) : nullptr);
	}

	void* operator[](u32 i) {
		return at(i);
	}

	void clear();
	
	inline void offsetFront(u32 n = 1)
	{
		frontCursor = ((u64)frontCursor + n) % capacity;
	}

	inline void* item(u32 i)
	{
		assert(i < capacity && "index out of range");
		return (void*)((uintptr_t)items + ((((u64)frontCursor + i) % capacity) * elementSizeB));
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

	void init(
		u16 elementSizeB,
		u32 capacity,
		void* buffer = nullptr,
		u8 assertOnFull = 1);

	void deinit();
};
static_assert_aligned_size(DenseQueue,8);


void* DenseQueue::push_front(void* val, bool zero)
{
	assert(!assertOnFull || length < capacity && "queue is full");
	
	void* addr = nullptr;
	if (length < capacity) {
		addr = item(capacity-1);
		offsetFront(capacity-1);
		++length;

		if (val) {
			itemcpy(addr, val);
		}
		else if (zero) {
			itemzero(addr);
		}
	}
	return addr;
}


void* DenseQueue::push_back(void* val, bool zero)
{
	assert(!assertOnFull || length < capacity && "queue is full");

	void* addr = nullptr;
	if (length < capacity) {
		addr = item(length);
		++length;

		if (val) {
			itemcpy(addr, val);
		}
		else if (zero) {
			itemzero(addr);
		}
	}
	return addr;
}


void* DenseQueue::push_back_n(u32 n, void* vals, bool zero)
{
	assert(!assertOnFull || length + n <= capacity && "queue is full");
	assert(n > 1 && "n should be > 1, for n == 1 use push_back");

	void* addr = nullptr;
	if (length + n <= capacity) {
		u32 backCursor = ((u64)frontCursor + length) % capacity;
		u32 firstn = min(capacity - backCursor, n);
		addr = item(length);
		length += n;

		if (vals) {
			memcpy(addr, vals, elementSizeB * firstn);
			if (firstn < n) {
				memcpy(items, vals, elementSizeB * (n - firstn));
			}
		}
		else if (zero) {
			memset(addr, 0, elementSizeB * firstn);
			if (firstn < n) {
				memset(items, 0, elementSizeB * (n - firstn));
			}
		}
	}
	return addr;
}


void* DenseQueue::pop_front(void* dst)
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
			offsetFront();
		}
	}
	return addr;
}


u32 DenseQueue::pop_front_n(u32 n, void* dst)
{
	void* addr = front();
	if (addr) {
		n = (n == 0 ? length : min(n, length));
		u32 firstn = min(capacity - frontCursor, n);

		if (dst) {
			memcpy(dst, addr, elementSizeB * firstn);
			if (firstn < n) {
				memcpy(dst, items, elementSizeB * (n - firstn));
			}
		}

		length -= n;
		if (length == 0) {
			frontCursor = 0;
		}
		else {
			offsetFront(n);
		}
	}
	else {
		n = 0;
	}
	return n;
}


void* DenseQueue::pop_back(void* dst)
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


void DenseQueue::clear()
{
	frontCursor = 0;
	length = 0;
	
	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear item memory to zero (slow build only) to help in debugging
	memset(items, 0, capacity*elementSizeB);
	#endif
}


void DenseQueue::init(
	u16 _elementSizeB,
	u32 _capacity,
	void* buffer,
	u8 _assertOnFull)
{
	elementSizeB = _elementSizeB;
	capacity = _capacity;
	assertOnFull = _assertOnFull;
	
	if (!buffer) {
		size_t size = elementSizeB * capacity;
		buffer = Q_malloc(size);
		memset(buffer, 0, size);
		_memoryOwned = 1;
	}

	items = buffer;
}


void DenseQueue::deinit()
{
	if (_memoryOwned && items) {
		free(items);
		items = nullptr;
	}
}


// Helper Macros

// Macro for defining a type-safe DenseQueue wrapper that avoids void* and elementSizeB in the api
#define DenseQueueTyped(Type, name) \
	struct name {\
		enum { TypeSize = sizeof(Type) };\
		DenseQueue _q;\
		name() {}\
		explicit name(u32 capacity, void* buffer = nullptr, u8 assertOnFull = 1)\
			: _q(TypeSize, capacity, buffer, assertOnFull) {}\
		inline bool empty()								{ return _q.empty(); }\
		inline bool full()								{ return _q.full(); }\
		inline Type* front()							{ return (Type*)_q.front(); }\
		inline Type* back()								{ return (Type*)_q.back(); }\
		inline Type* nextBack()							{ return (Type*)_q.nextBack(); }\
		inline u32 maxContiguous()						{ return _q.maxContiguous(); }\
		Type* push_back(Type* val = nullptr, bool zero = true)\
														{ return (Type*)_q.push_back((void*)val, zero); }\
		Type* push_back_n(u32 n, Type* vals = nullptr, bool zero = true)\
														{ return (Type*)_q.push_back_n(n, (void*)vals, zero); }\
		Type* push_front(Type* val = nullptr, bool zero = true)\
														{ return (Type*)_q.push_front((void*)val, zero); }\
		Type* pop_back(Type* dst = nullptr)				{ return (Type*)_q.pop_back((void*)dst); }\
		Type* pop_front(Type* dst = nullptr)			{ return (Type*)_q.pop_front((void*)dst); }\
		u32 pop_front_n(u32 n, Type* dst)				{ return _q.pop_front_n(n, (void*)dst); }\
		inline Type* push(Type* val = nullptr, bool zero = true)\
														{ return push_back(val,zero); }\
		inline Type* push_n(u32 n, Type* val = nullptr, bool zero = true)\
														{ return push_back_n(n,val,zero); }\
		inline Type* pop_fifo(Type* dst = nullptr)		{ return pop_front(dst); }\
		inline Type* pop_lifo(Type* dst = nullptr)		{ return pop_back(dst); }\
		inline u32   pop_fifo_n(u32 n, Type* dst = nullptr)\
														{ return pop_front_n(n,dst); }\
		Type* at(u32 i)									{ return (Type*)_q.at(i); }\
		Type* operator[](u32 i)							{ return at(i); }\
		void clear()									{ _q.clear(); }\
		inline Type* item(u32 i)						{ return (Type*)_q.item(i); }\
		void init(u32 capacity, void* buffer = nullptr, u8 assertOnFull = 1)\
														{ _q.init(TypeSize, capacity, buffer, assertOnFull); }\
		void deinit()									{ _q.deinit(); }\
	};

#endif
