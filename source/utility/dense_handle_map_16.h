#ifndef _DENSE_HANDLE_MAP_16_H
#define _DENSE_HANDLE_MAP_16_H

#include <cstdlib>
#include <cstring>
#include "common.h"


/**
 * @struct DenseHandleMap16
 *	Stores objects using a dense inner array and sparse outer array scheme for good cache coherence
 *	of the inner items. The sparse array contains handles (outer ids) used to identify the item,
 *	and provides an extra indirection allowing the inner array to move items in memory to keep them
 *	tightly packed. The sparse array contains an embedded LIFO freelist, where removed ids push to
 *	the top of the list so the sparse set remains relatively dense.
 *
 *	Uses 32-bit handles allowing up to 2^16 stored items, 256 unique type ids, and 128 generations
 *	before wrapping. 14 bits store the element size, allowing storage of items up to 16K bytes.
 */
struct DenseHandleMap16 {
	// Variables
	void*	items = nullptr;			// array of stored objects, must have one extra element above capacity used in defragment
	h32*	sparseIds = nullptr;		// array of h32, these are "inner" ids indexing into items
	u16*	denseToSparse = nullptr;	// array of indices into sparseIds array

	u16		length = 0;					// current number of objects contained in map
	u16		freeListFront = 0;			// front index of the embedded LIFO freelist
	u16		capacity = 0;				// maximum number of objects that can be stored
	u16		elementSizeB : 14;			// size in bytes of individual stored objects
	u16		_fragmented  : 1;			// set to 1 if modified by insert or erase since last complete defragment
	u16		_memoryOwned : 1;			// set to 1 if buffer memory is owned by DenseHandleMap16
	
	// Functions
	
	static size_t getTotalBufferSize(u16 elementSizeB, u16 capacity);


	/**
	 * Constructor
	 * @param	elementSizeB	size in bytes of individual objects stored
	 * @param	capacity		maximum number of objects that can be stored
	 * @param	itemTypeId		typeId used by the h32::typeId variable for this container
	 * @param	buffer
	 *	Optional pre-allocated buffer for all dynamic storage used in the DenseHandleMap16, with ample
	 *	size (obtained by call to getTotalBufferSize). If passed, the memory is not owned by
	 *	DenseHandleMap16 and thus not freed on delete. Pass nullptr (default) to allocate the storage on
	 *	create and free on delete.
	 */
	explicit DenseHandleMap16(
		u16 _elementSizeB,
		u16 _capacity,
		u8 itemTypeId = 0,
		void* buffer = nullptr)
	{
		init(_elementSizeB, _capacity, itemTypeId, buffer);
	}

	explicit DenseHandleMap16() {}

	~DenseHandleMap16() {
		deinit();
	}


	/**
	 * Get a direct pointer to a stored item by handle
	 * @param[in]	handle		id of the item
	 * @returns pointer to the item
	 */
	void* at(h32 handle);
	
	/**
	 * Same as "at", does not insert new values
	 */
	void* operator[](h32 handle) {
		return at(handle);
	}

	/**
	 * remove the item identified by the provided handle
	 * @param[in]	handle		id of the item
	 * @returns true if item removed, false if not found
	 */
	bool erase(h32 handle);

	/**
	 * Add one item to the store, return the id, optionally return pointer to the new object for
	 * initialization.
	 * @param[in]	src		optional pointer to an object to copy into inner storage
	 * @param[out]	out		optional return pointer to the new object
	 * @returns the id
	 */
	h32 insert(void* src = nullptr, void** out = nullptr);

	/**
	 * Removes all items, leaving the sparseIds set intact by adding each entry to the free-
	 * list and keeping its generation. This operation is slower than @c reset, but safer for the
	 * detection of stale handle lookups later (in debug builds). Prefer to use @c reset if safety
	 * is not a concern.
	 * Complexity is linear.
	 */
	void clear();

	/**
	 * Removes all items, destroying the sparseIds set. Leaves the container's capacity, but
	 * otherwise equivalent to a default-constructed container. This is faster than @c clear,
	 * but cannot safely detect lookups by stale handles obtained before the reset. Use @c clear
	 * if safety is a concern, at least until it's proven not to be a problem.
	 * Complexity is constant.
	 */
	void reset();

	/**
	* @returns true if handle handle refers to a valid item
	*/
	bool has(h32 handle);

	/**
	 * defragment uses the comparison function @c comp to establish an ideal order for the dense
	 *	set in order to maximum cache locality for traversals. The dense set can become
	 *	fragmented over time due to removal operations. This can be an expensive operation, so
	 *	the sort operation is reentrant. Use the @c maxSwaps parameter to limit the number of
	 *	swaps that will occur before the function returns.
	 * @param[in]	comp		pointer to comparison function returning ?true if the first
	 *	argument is greater than (i.e. is ordered after) the second. The function should not
	 *	write to the pointers passed in.
	 * @param[in]	maxSwaps	maximum number of items to reorder in the insertion sort
	 *	before the function returns. Pass 0 (default) to run until completion.
	 * @returns the number of swaps that occurred, keeping in mind that this value could
	 *	overflow on very large data sets
	 */
	typedef bool Compare(void*, void*);
	size_t defragment(Compare* comp, size_t maxSwaps = 0);

	/**
	 * @returns index into the inner DenseSet for a given outer id
	 */
	u16 getInnerIndex(h32 handle);

	/**
	 * @return the outer id (handle) for a given dense set index
	 */
	h32 getHandleForInnerIndex(size_t innerIndex);

	inline void* item(u16 innerIndex)
	{
		assert(innerIndex < capacity && "inner index out of range");
		return (void*)((uintptr_t)items + (innerIndex * elementSizeB));
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

	void init(u16 elementSizeB,
			  u16 capacity,
			  u8 itemTypeId = 0,
			  void* buffer = nullptr);

	void deinit();
};
static_assert_aligned_size(DenseHandleMap16,8);


size_t DenseHandleMap16::getTotalBufferSize(u16 elementSizeB, u16 capacity)
{
	// handle aligned storage, which may increase total size due to padding between buffers
	// add an extra item to the items array for scratch memory used by defragment sort
	size_t size = _align((size_t)elementSizeB * (capacity+1), 4);
	size = _align(size + (sizeof(h32) * capacity), 2);
	size += (sizeof(u16) * capacity);
	return size;
}


h32 DenseHandleMap16::insert(void* src, void** out)
{
	assert(length < capacity && "DenseHandleMap16 is full");
	h32 handle = null_h32;
	
	if (length < capacity) {
		u16 sparseIndex = freeListFront;
		h32 innerId = sparseIds[sparseIndex];

		freeListFront = innerId.index; // the index of a free slot refers to the next free slot

		// convert the index from freelist to inner index
		innerId.free = 0;
		++innerId.generation; // increment generation so remaining outer ids go stale
		innerId.index = length;
		sparseIds[sparseIndex] = innerId;

		handle = innerId;
		handle.index = sparseIndex;

		denseToSparse[length] = handle.index;
		
		void* pItem = item(length);
		if (src) {
			itemcpy(pItem, src);
		}
		else {
			itemzero(pItem);
		}
		if (out) {
			*out = pItem;
		}

		++length;
		_fragmented = 1;
	}

	return handle;
}


bool DenseHandleMap16::erase(h32 handle)
{
	if (!has(handle)) {
		return false;
	}

	h32 innerId = sparseIds[handle.index];
	u16 innerIndex = innerId.index;

	// put this slot at the front of the freelist
	innerId.free = 1;
	innerId.index = freeListFront;
	sparseIds[handle.index] = innerId; // write outer id changes back to the array
	freeListFront = handle.index; // new freelist front is stored

	// remove the object by swapping with the last element, then pop_back
	u16 last = length - 1;
	if (innerIndex != last) {
		itemcpy(item(innerIndex), item(last));
		
		u16 swappedIndex = denseToSparse[last];
		denseToSparse[innerIndex] = swappedIndex;

		// fix the index of the swapped inner id
		sparseIds[swappedIndex].index = innerIndex;
	}

	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear removed item memory to zero (slow build only) to help in debugging
	itemzero(item(last));
	#endif

	--length;
	_fragmented = 1;

	return true;
}


void DenseHandleMap16::clear()
{
	if (length > 0) {
		for (u16 i = 0; i < length; ++i) {
			u16 sparseIndex = denseToSparse[i];
			h32 innerId = sparseIds[sparseIndex];
			innerId.free = 1;
			innerId.index = freeListFront;
			sparseIds[sparseIndex] = innerId;
			freeListFront = sparseIndex;
		}

		#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
		// clear item memory to zero (slow build only) to help in debugging
		memset(items, 0, length*elementSizeB);
		#endif

		length = 0;
		_fragmented = 0;
	}
}


void DenseHandleMap16::reset()
{
	h32 innerId = { 0, sparseIds[0].typeId, 0, 1 };
	for (u16 i = 0; i < capacity; ++i) {
		++innerId.index;
		sparseIds[i].value = innerId.value;
	}

	freeListFront = 0;
	sparseIds[capacity-1].index = USHRT_MAX;
	
	length = 0;
	_fragmented = 0;

	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear item memory to zero (slow build only) to help in debugging
	memset(items, 0, capacity*elementSizeB);
	#endif
}


void* DenseHandleMap16::at(h32 handle)
{
	void* pItem = nullptr;
	if (has(handle)) {
		h32 innerId = sparseIds[handle.index];
		pItem = item(innerId.index);
	}
	return pItem;
}


bool DenseHandleMap16::has(h32 handle)
{
	assert(handle.index < capacity && "handle index out of range");
	
	h32 innerId = sparseIds[handle.index];
	
	assert(innerId.free == 0 && "handle to a removed object");
	assert(innerId.index < length && "inner index out of range");
	assert(innerId.typeId == handle.typeId && "handle typeId mismatch");
	assert(innerId.generation == handle.generation && "handle with old generation");
	
	return (handle.index < capacity
		&&	innerId.free == 0
		&&	innerId.index < length
		&&	innerId.typeId == handle.typeId
		&&	innerId.generation == handle.generation);
}


u16 DenseHandleMap16::getInnerIndex(h32 handle)
{
	u16 index = USHRT_MAX;
	if (has(handle)) {
		index = sparseIds[handle.index].index;
	}
	return index;
}


h32 DenseHandleMap16::getHandleForInnerIndex(size_t innerIndex)
{
	assert(innerIndex < length && innerIndex >= 0 && "inner index out of range");
	
	u16 sparseIndex = denseToSparse[innerIndex];
	h32 handle = sparseIds[sparseIndex];
	handle.index = sparseIndex;
	
	return handle;
}


size_t DenseHandleMap16::defragment(Compare* comp, size_t maxSwaps)
{
	if (_fragmented == 0) {
		return 0;
	}
	size_t swaps = 0;
	
	// get scratch memory from end of the items array above capacity
	void* tmp = (void*)((uintptr_t)items + (capacity * elementSizeB));

	i64 i = 1;
	for (; i < length && (maxSwaps == 0 || swaps < maxSwaps); ++i) {
		itemcpy(tmp, item((u16)i));
		u16 tmpD2S = denseToSparse[i];

		i64 j = i - 1;
		i64 j1 = j + 1;

		while (j >= 0 && comp(item((u16)j), tmp)) {
			sparseIds[denseToSparse[j]].index = (u16)j1;
			--j;
			--j1;
		}
		if (j1 != i) {
			memmove(item((u16)(j1+1)), item((u16)(j1)), elementSizeB * (i - j1));
			memmove(&denseToSparse[j1+1], &denseToSparse[j1], sizeof(u16) * (i - j1));
			++swaps;

			itemcpy(item((u16)j1), tmp);
			denseToSparse[j1] = tmpD2S;
			sparseIds[denseToSparse[j1]].index = (u16)j1;
		}
	}
	if (i == length) {
		_fragmented = 0;
	}

	return swaps;
}


void DenseHandleMap16::init(
	u16 _elementSizeB,
	u16 _capacity,
	u8 itemTypeId,
	void* buffer)
{
	elementSizeB = _elementSizeB & 0x3FFF;
	assert(elementSizeB == _elementSizeB && "element size too large");
	capacity = _capacity;

	if (!buffer) {
		size_t size = getTotalBufferSize(elementSizeB, capacity);
		buffer = Q_malloc(size);
		memset(buffer, 0, size);
		_memoryOwned = 1;
	}

	items = buffer;
	// round up to aligned storage
	// add an extra item to the items array for scratch memory used by defragment sort
	sparseIds = (h32*)_align((uintptr_t)items + (elementSizeB * (capacity+1)), 4);
	denseToSparse = (u16*)_align((uintptr_t)sparseIds + (sizeof(h32) * capacity), 2);

	// check resulting alignment in case element storage plus padding doesn't leave us 4-byte aligned
	assert(is_aligned(sparseIds, 4) && "sparseIds not properly aligned");
	assert(is_aligned(denseToSparse, 2) && "denseToSparse not properly aligned");

	// reset to set up the sparseIds freelist
	sparseIds[0].typeId = itemTypeId;
	reset();
}


void DenseHandleMap16::deinit()
{
	if (_memoryOwned && items) {
		free(items);
		items = nullptr;
	}
}


// Helper Macros

// Macro for defining a DenseHandleMap16 storage buffer
#define DenseHandleMap16Buffer(Type, Name, capacity) \
	u8 Name[(sizeof(Type)*(capacity+1)) + (sizeof(h32)*capacity) + (sizeof(u16)*capacity)];\
	static_assert(is_aligned(sizeof(Type)*(capacity+1),4), "sizeof items array must be a multiple of 4");\
	static_assert(capacity <= USHRT_MAX-1, "capacity must be <= USHRT_MAX-1");


// Macro for defining a type-safe DenseHandleMap16 wrapper that avoids void* and elementSizeB in the api
#define DenseHandleMap16Typed(Type, Name, HndType, TypeId) \
	struct Name {\
		enum { TypeSize = sizeof(Type) };\
		DenseHandleMap16 _map;\
		static size_t getTotalBufferSize(u16 capacity) {\
			return DenseHandleMap16::getTotalBufferSize(TypeSize, capacity);\
		}\
		explicit Name(u16 _capacity, Type* buffer = nullptr) {\
			_map.init(TypeSize, _capacity, TypeId, (void*)buffer);\
		}\
		explicit Name() {}\
		Type* at(HndType handle)			{ return (Type*)_map.at(handle); }\
		Type* operator[](HndType handle)	{ return at(handle); }\
		bool erase(HndType handle)			{ return _map.erase(handle); }\
		HndType insert(Type* src = nullptr, Type** out = nullptr) {\
			return _map.insert((void*)src, (void**)out);\
		}\
		void clear()						{ _map.clear(); }\
		void reset()						{ _map.reset(); }\
		bool has(HndType handle)			{ return _map.has(handle); }\
		size_t defragment(DenseHandleMap16::Compare* comp, size_t maxSwaps = 0) {\
			return _map.defragment(comp, maxSwaps);\
		}\
		u16 getInnerIndex(HndType handle)	{ return _map.getInnerIndex(handle); }\
		HndType getHandleForInnerIndex(size_t innerIndex) {\
			return _map.getHandleForInnerIndex(innerIndex);\
		}\
		inline Type& item(u16 innerIndex)	{ return *(Type*)_map.item(innerIndex); }\
		inline Type* items() 				{ return (Type*)_map.items; }\
		inline u16 length()					{ return _map.length; }\
		void init(u16 capacity, void* buffer = nullptr) {\
			_map.init(TypeSize, capacity, TypeId, buffer);\
		}\
		void deinit()						{ _map.deinit(); }\
	};\
	static_assert(std::is_same<h32,HndType>::value, #HndType " must be typedef h32");


// Macro like DenseHandleMap16Typed but also internally includes the storage buffer, so there is no
// need to call init or create the buffer externally
#define DenseHandleMap16TypedWithBuffer(Type, Name, HndType, TypeId, _capacity) \
	struct Name {\
		enum { TypeSize = sizeof(Type) };\
		DenseHandleMap16 _map;\
		DenseHandleMap16Buffer(Type, _buffer, _capacity)\
		static size_t getTotalBufferSize(u16 capacity) {\
			return sizeof(_buffer);\
		}\
		explicit Name()	: _buffer{}			{ _map.init(TypeSize, _capacity, TypeId, &_buffer); }\
		Type* at(HndType handle)			{ return (Type*)_map.at(handle); }\
		Type* operator[](HndType handle)	{ return at(handle); }\
		bool erase(HndType handle)			{ return _map.erase(handle); }\
		HndType insert(Type* src = nullptr, Type** out = nullptr) {\
			return _map.insert((void*)src, (void**)out);\
		}\
		void clear()						{ _map.clear(); }\
		void reset()						{ _map.reset(); }\
		bool has(HndType handle)			{ return _map.has(handle); }\
		size_t defragment(DenseHandleMap16::Compare* comp, size_t maxSwaps = 0) {\
			return _map.defragment(comp, maxSwaps);\
		}\
		u16 getInnerIndex(HndType handle)	{ return _map.getInnerIndex(handle); }\
		HndType getHandleForInnerIndex(size_t innerIndex) {\
			return _map.getHandleForInnerIndex(innerIndex);\
		}\
		inline Type& item(u16 innerIndex)	{ return *(Type*)_map.item(innerIndex); }\
		inline Type* items() 				{ return (Type*)_map.items; }\
		inline u16 length()					{ return _map.length; }\
	};\
	static_assert(std::is_same<h32,HndType>::value, #HndType " must be typedef h32");


#endif