#ifndef _HANDLE_MAP_H
#define _HANDLE_MAP_H

#include <cstdlib>
#include <cstring>
#include "types.h"

/**
 * @struct Id_t
 * @var	free		0 if active, 1 if slot is part of freelist, only applicable to inner ids
 * @var	typeId		relates to itemTypeId parameter of HandleMap
 * @var	generation	incrementing generation of data at the index, for tracking accesses to old data
 * @var	index		When used as a handle (outer id, given to the client):
 *						free==0, index of id in the sparseIds array
 *					When used as an inner id (stored in sparseIds array):
 *						free==0, index of the item in the dense items array
 *						free==1, index of next free slot, forming an embedded linked list
 * @var	value		unioned with the above four vars, used for direct comparison of ids
 */
struct Id_t {
	union {
		/**
		* the order of this bitfield is important for sorting prioritized by free, then typeId,
		* then generation, then index
		*/
		struct {
			u32 index;
			u16 generation;
			u16 typeId : 15;
			u16 free : 1;
		};
		u64 value;
	};
};

#define NullId_t	Id_t{}


// Id_t comparison functions

bool operator==(const Id_t& a, const Id_t& b) { return (a.value == b.value); }
bool operator!=(const Id_t& a, const Id_t& b) { return (a.value != b.value); }
bool operator< (const Id_t& a, const Id_t& b) { return (a.value < b.value); }
bool operator> (const Id_t& a, const Id_t& b) { return (a.value > b.value); }


/**
 * @struct HandleMap
 *	Stores objects using a dense inner array and sparse outer array scheme for good cache coherence
 *	of the inner items. The sparse array contains handles (outer ids) used to identify the item,
 *	and provides an extra indirection allowing the inner array to move items in memory to keep them
 *	tightly packed. The sparse array contains an embedded LIFO freelist, where removed ids push to
 *	the top of the list so the sparse set remains relatively dense.
 */
struct HandleMap {
	// Variables
	void*	items = nullptr;			// array of stored objects, must have one extra element above capacity used in defragment
	Id_t*	sparseIds = nullptr;		// array of Id_ts, these are "inner" ids indexing into items
	u32*	denseToSparse = nullptr;	// array of indices into sparseIds array

	u32		length = 0;					// current number of objects contained in map
	u32		freeListFront = 0;			// front index of the embedded LIFO freelist
	u32		capacity = 0;				// maximum number of objects that can be stored
	u16		elementSizeB = 0;			// size in bytes of individual stored objects
	u8		_fragmented = 0;			// set to 1 if modified by insert or erase since last complete defragment
	u8		_memoryOwned = 0;			// set to 1 if buffer memory is owned by HandleMap

	// Functions
	
	static size_t getTotalBufferSize(u16 elementSizeB, u32 capacity);


	/**
	 * Constructor
	 * @param	elementSizeB	size in bytes of individual objects stored
	 * @param	capacity		maximum number of objects that can be stored
	 * @param	itemTypeId		typeId used by the Id_t::typeId variable for this container
	 * @param	buffer
	 *	Optional pre-allocated buffer for all dynamic storage used in the HandleMap, with ample
	 *	size (obtained by call to getTotalBufferSize). If passed, the memory is not owned by
	 *	HandleMap and thus not freed on delete. Pass nullptr (default) to allocate the storage on
	 *	create and free on delete.
	 */
	explicit HandleMap(u16 _elementSizeB,
					   u32 _capacity,
					   u16 itemTypeId = 0,
					   void* buffer = nullptr)
	{
		init(_elementSizeB, _capacity, itemTypeId, buffer);
	}

	explicit HandleMap() {}

	~HandleMap() {
		deinit();
	}


	/**
	 * Get a direct pointer to a stored item by handle
	 * @param[in]	handle		id of the item
	 * @returns pointer to the item
	 */
	void* at(Id_t handle);
	
	void* operator[](Id_t handle) {
		return at(handle);
	}

	/**
	 * remove the item identified by the provided handle
	 * @param[in]	handle		id of the item
	 * @returns true if item removed, false if not found
	 */
	bool erase(Id_t handle);

	/**
	 * Add one item to the store, return the id, optionally return pointer to the new object for
	 * initialization.
	 * @param[in]	src		optional pointer to an object to copy into inner storage
	 * @param[out]	out		optional return pointer to the new object
	 * @returns the id
	 */
	Id_t insert(void* src = nullptr, void** out = nullptr);

	/**
	 * Removes all items, leaving the sparseIds set intact by adding each entry to the free-
	 * list and incrementing its generation. This operation is slower than @c reset, but safer
	 * for the detection of stale handle lookups later (in debug builds). Prefer to use @c reset
	 * if safety is not a concern.
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
	bool has(Id_t handle);

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
	u32 getInnerIndex(Id_t handle);

	/**
	 * @return the outer id (handle) for a given dense set index
	 */
	Id_t getHandleForInnerIndex(size_t innerIndex);

	inline void* item(u32 innerIndex)
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
			  u32 capacity,
			  u16 itemTypeId = 0,
			  void* buffer = nullptr);

	void deinit();
};
static_assert_aligned_size(HandleMap,8);


size_t HandleMap::getTotalBufferSize(u16 elementSizeB, u32 capacity)
{
	// handle aligned storage, which may increase total size due to padding between buffers
	// add an extra item to the items array for scratch memory used by defragment sort
	size_t size = align(elementSizeB * (capacity+1), 8);
	size = align(size + (sizeof(Id_t) * capacity), 4);
	size += (sizeof(u32) * capacity);
	return size;
}


Id_t HandleMap::insert(void* src, void** out)
{
	assert(length < capacity && "HandleMap is full");
	Id_t handle = NullId_t;
	
	if (length < capacity) {
		u32 sparseIndex = freeListFront;
		Id_t innerId = sparseIds[sparseIndex];

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
			// TODO: memmove?? also consider rewriting this once we have a dynamic array type
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


bool HandleMap::erase(Id_t handle)
{
	if (!has(handle)) {
		return false;
	}

	Id_t innerId = sparseIds[handle.index];
	u32 innerIndex = innerId.index;

	// put this slot at the front of the freelist
	innerId.free = 1;
	innerId.index = freeListFront;
	sparseIds[handle.index] = innerId; // write outer id changes back to the array
	freeListFront = handle.index; // new freelist front is stored

	// remove the object by swapping with the last element, then pop_back
	u32 last = length - 1;
	if (innerIndex != last) {
		itemcpy(item(innerIndex), item(last));
		
		u32 swappedIndex = denseToSparse[last];
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


void HandleMap::clear()
{
	if (length > 0) {
		for (u32 i = 0; i < length; ++i) {
			u32 sparseIndex = denseToSparse[i];
			Id_t innerId = sparseIds[sparseIndex];
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


void HandleMap::reset()
{
	Id_t innerId = { 0, 0, sparseIds[0].typeId, 1 };
	for (u32 i = 0; i < capacity; ++i) {
		++innerId.index;
		sparseIds[i].value = innerId.value;
	}

	freeListFront = 0;
	sparseIds[capacity-1].index = UINT_MAX;
	
	length = 0;
	_fragmented = 0;

	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear item memory to zero (slow build only) to help in debugging
	memset(items, 0, capacity*elementSizeB);
	#endif
}


void* HandleMap::at(Id_t handle)
{
	void* pItem = nullptr;
	if (has(handle)) {
		Id_t innerId = sparseIds[handle.index];
		pItem = item(innerId.index);
	}
	return pItem;
}


bool HandleMap::has(Id_t handle)
{
	assert(handle.index < capacity && "handle index out of range");
	
	Id_t innerId = sparseIds[handle.index];
	
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


u32 HandleMap::getInnerIndex(Id_t handle)
{
	u32 index = UINT_MAX;
	if (has(handle)) {
		index = sparseIds[handle.index].index;
	}
	return index;
}


Id_t HandleMap::getHandleForInnerIndex(size_t innerIndex)
{
	assert(innerIndex < length && innerIndex >= 0 && "inner index out of range");
	
	u32 sparseIndex = denseToSparse[innerIndex];
	Id_t handle = sparseIds[sparseIndex];
	handle.index = sparseIndex;
	
	return handle;
}


size_t HandleMap::defragment(Compare* comp, size_t maxSwaps)
{
	if (_fragmented == 0) {
		return 0;
	}
	size_t swaps = 0;
	
	// get scratch memory from end of the items array above capacity
	void* tmp = (void*)((uintptr_t)items + (capacity * elementSizeB));

	i64 i = 1;
	for (; i < length && (maxSwaps == 0 || swaps < maxSwaps); ++i) {
		itemcpy(tmp, item((u32)i));
		u32 tmpD2S = denseToSparse[i];

		i64 j = i - 1;
		i64 j1 = j + 1;

		while (j >= 0 && comp(item((u32)j), tmp)) {
			sparseIds[denseToSparse[j]].index = (u32)j1;
			--j;
			--j1;
		}
		if (j1 != i) {
			memmove(item((u32)(j1+1)), item((u32)(j1)), elementSizeB * (i - j1));
			memmove(&denseToSparse[j1+1], &denseToSparse[j1], sizeof(u32) * (i - j1));
			++swaps;

			itemcpy(item((u32)j1), tmp);
			denseToSparse[j1] = tmpD2S;
			sparseIds[denseToSparse[j1]].index = (u32)j1;
		}
	}
	if (i == length) {
		_fragmented = 0;
	}

	return swaps;
}


void HandleMap::init(
	u16 _elementSizeB,
	u32 _capacity,
	u16 itemTypeId,
	void* buffer)
{
	elementSizeB = _elementSizeB;
	capacity = _capacity;

	if (!buffer) {
		size_t size = getTotalBufferSize(elementSizeB, capacity);
		buffer = malloc(size);
		memset(buffer, 0, size);
		_memoryOwned = 1;
	}

	items = buffer;
	// round up to aligned storage
	// add an extra item to the items array for scratch memory used by defragment sort
	sparseIds = (Id_t*)align((uintptr_t)items + (elementSizeB * (capacity+1)), 8);
	denseToSparse = (u32*)align((uintptr_t)sparseIds + (sizeof(Id_t) * capacity), 4);

	// check resulting alignment in case element storage plus padding doesn't leave us 8-byte aligned
	assert(is_aligned(sparseIds, 8) && "sparseIds not properly aligned");
	assert(is_aligned(denseToSparse, 4) && "denseToSparse not properly aligned");

	// reset to set up the sparseIds freelist
	sparseIds[0].typeId = itemTypeId & 0x7FFF;
	reset();
}


void HandleMap::deinit()
{
	if (_memoryOwned && items) {
		free(items);
		items = nullptr;
	}
}


// Helper Macros

// Macro for defining a HandleMap storage buffer
#define HandleMapBuffer(Type, name, capacity) \
	u8 name[(sizeof(Type)*(capacity+1)) + (sizeof(Id_t)*capacity) + (sizeof(u32)*capacity)];\
	static_assert(is_aligned(sizeof(Type)*(capacity+1),8),"sizeof items array must be a multiple of 8");


// Macro for defining a type-safe HandleMap wrapper that avoids void* and elementSizeB in the api
#define HandleMapTyped(Type, name, TypeId) \
	struct name {\
		enum { TypeSize = sizeof(Type) };\
		HandleMap _map;\
		static size_t getTotalBufferSize(u32 capacity)\
											{ return HandleMap::getTotalBufferSize(TypeSize, capacity); }\
		explicit name(u32 _capacity, void* buffer = nullptr)\
											{ _map.init(TypeSize, _capacity, TypeId, buffer); }\
		explicit name() {}\
		Type* at(Id_t handle)				{ return (Type*)_map.at(handle); }\
		Type* operator[](Id_t handle)		{ return at(handle); }\
		bool erase(Id_t handle)				{ return _map.erase(handle); }\
		Id_t insert(Type* src = nullptr, Type** out = nullptr)\
											{ return _map.insert((void*)src, (void**)out); }\
		void clear()						{ _map.clear(); }\
		void reset()						{ _map.reset(); }\
		bool has(Id_t handle)				{ return _map.has(handle); }\
		size_t defragment(HandleMap::Compare* comp, size_t maxSwaps = 0)\
											{ return _map.defragment(comp, maxSwaps); }\
		u32 getInnerIndex(Id_t handle)		{ return _map.getInnerIndex(handle); }\
		Id_t getHandleForInnerIndex(size_t innerIndex)\
											{ return _map.getHandleForInnerIndex(innerIndex); }\
		inline Type* item(u32 innerIndex)	{ return (Type*)_map.item(innerIndex); }\
		void init(u32 capacity, void* buffer = nullptr)\
											{ _map.init(TypeSize, capacity, TypeId, buffer); }\
		void deinit()						{ _map.deinit(); }\
	};

#endif