#ifndef _SPARSE_HANDLE_MAP_32_H
#define _SPARSE_HANDLE_MAP_32_H

#include <cstdlib>
#include <cstring>
#include "common.h"


/**
 * @struct SparseHandleMap32
 *	Stores objects using a sparse array for items that don't shift around in memory, making the
 *	storage of large objects more tenable than the alternative DenseHandleMap. The sparse array
 *	contains headers before each item with a slot generation and an embedded LIFO freelist.
 *
 *	Uses 64-bit handles allowing up to 2^32 stored items, 65535 unique type ids, and 32768
 *	generations before wrapping. 16 bits store the element size, allowing storage of items up to 64K
 *	bytes.
 */
struct SparseHandleMap32 {
	/**
	 * Each data item has an 8-byte header containing generation and freelist data. We can't reuse
	 * item storage because the generation would get wiped, and the header should be at least 8
	 * bytes for alignment of the data item, so we have enough bytes to just use the header.
	 */
	struct Header {
		u32 next;
		u16 typeId;
		u16 generation : 15;
		u16 free       : 1;
	};

	/**
	 * Item is used to return data from the container, it is not the stored type itself.
	 */
	struct Item {
		Header* header;
		void*   data;
	};
	
	// Variables
	void*	items = nullptr;			// array of stored objects, must have one extra element above capacity used in defragment

	u32		length = 0;					// current number of objects contained in map
	u32		freeListFront = 0;			// front index of the embedded LIFO freelist
	u32		capacity = 0;				// maximum number of objects that can be stored
	u16		elementSizeB = 0;			// size in bytes of individual stored objects
	u8		_memoryOwned = 0;			// set to 1 if buffer memory is owned by SparseHandleMap32
	u8		_padding = 0;

	// Functions
	
	static size_t getTotalBufferSize(u16 elementSizeB, u32 capacity);


	/**
	 * Constructor
	 * @param	elementSizeB	size in bytes of individual objects stored
	 * @param	capacity		maximum number of objects that can be stored
	 * @param	buffer
	 *	Optional pre-allocated buffer for all dynamic storage used in the SparseHandleMap32, with ample
	 *	size (obtained by call to getTotalBufferSize). If passed, the memory is not owned by
	 *	SparseHandleMap32 and thus not freed on delete. Pass nullptr (default) to allocate the storage on
	 *	create and free on delete.
	 */
	explicit SparseHandleMap32(
		u16 _elementSizeB,
		u32 _capacity,
		void* buffer = nullptr)
	{
		init(_elementSizeB, _capacity, buffer);
	}

	explicit SparseHandleMap32() {}

	~SparseHandleMap32() {
		deinit();
	}


	/**
	 * Get a direct pointer to a stored item by handle
	 * @param[in]	handle		id of the item
	 * @returns pointer to the item
	 */
	void* at(h64 handle);
	
	void* operator[](h64 handle) {
		return at(handle);
	}

	/**
	 * remove the item identified by the provided handle
	 * @param[in]	handle		id of the item
	 * @returns true if item removed, false if not found
	 */
	bool erase(h64 handle);

	/**
	 * Add one item to the store, return the id, optionally return pointer to the new object for
	 * initialization.
	 * @param[in]	src		optional pointer to an object to copy into inner storage
	 * @param[out]	out		optional return pointer to the new object
	 * @param		typeId	typeId used by the h64::typeId variable for this container
	 * @returns the id
	 */
	h64 insert(
		void* src = nullptr,
		void** out = nullptr,
		u16 typeId = 0);

	/**
	 * Removes all items by adding each entry to the free-list and leaving its generation intact.
	 * This operation is slower than @c reset, but safer
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
	* @returns address of item cast to a uintptr_t type if it exists, 0 if handle is not valid.
	*/
	uintptr_t has(h64 handle);

	inline Item item(u32 index)
	{
		assert(index < capacity && "index out of range");
		uintptr_t item = (uintptr_t)items + (index * (elementSizeB + sizeof(Header)));
		return {
			(Header*)item,
			(void*)(item+sizeof(Header))
		};
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
			  void* buffer = nullptr);

	void deinit();
};
static_assert_aligned_size(SparseHandleMap32,8);


size_t SparseHandleMap32::getTotalBufferSize(u16 elementSizeB, u32 capacity)
{
	return _align(((size_t)elementSizeB + sizeof(Header)) * capacity, 8);
}


h64 SparseHandleMap32::insert(
	void* src,
	void** out,
	u16 typeId)
{
	assert(length < capacity && "SparseHandleMap32 is full");
	h64 handle = null_h64;
	
	if (length < capacity) {
		u32 index = freeListFront;
		Item i = item(index);

		freeListFront = i.header->next;

		i.header->next = index;
		++i.header->generation;
		i.header->free = 0;
		i.header->typeId = typeId;
		
		handle = *(h64*)i.header;
		
		if (src) {
			itemcpy(i.data, src);
		}
		else {
			itemzero(i.data);
		}
		if (out) {
			*out = i.data;
		}

		++length;
	}

	return handle;
}


bool SparseHandleMap32::erase(h64 handle)
{
	Item i = item(handle.index);
	if (i.header->free) {
		return false;
	}

	// put this slot at the front of the freelist
	i.header->free = 1;
	i.header->next = freeListFront;
	freeListFront = handle.index;

	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear removed item memory to zero (slow build only) to help in debugging
	itemzero(i.data);
	#endif

	--length;

	return true;
}


void SparseHandleMap32::clear()
{
	uintptr_t item = (uintptr_t)items;
	while (length > 0) {
		Header* header = (Header*)item;
		if (!header->free) {
			u32 index = header->next; // next contains items own index when not part of freelist
			header->free = 1;
			header->next = freeListFront;
			freeListFront = index;

			#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
			// clear removed item memory to zero (slow build only) to help in debugging
			itemzero((void*)(item+sizeof(Header)));
			#endif
			
			--length;
		}

		item += sizeof(Header) + elementSizeB;
	}
}


void SparseHandleMap32::reset()
{
	#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
	// clear item memory to zero (slow build only) to help in debugging
	memset(items, 0, capacity * (elementSizeB + sizeof(Header)));
	#endif

	Header h = { 0, 0, 0, 1 };
	uintptr_t item = (uintptr_t)items;
	for (u32 i = 0; i < capacity; ++i) {
		++h.next;
		*(Header*)item = h;
		item += sizeof(Header) + elementSizeB;
	}

	freeListFront = 0;	
	length = 0;
}


void* SparseHandleMap32::at(h64 handle)
{
	return (void*)has(handle);
}


uintptr_t SparseHandleMap32::has(h64 handle)
{
	assert(handle.index < capacity && "handle index out of range");
	
	Item i = item(handle.index);
	
	assert(i.header->free == 0 && "handle to a removed object");
	assert(i.header->typeId == handle.typeId && "handle typeId mismatch");
	assert(i.header->generation == handle.generation && "handle with old generation");
	
	return ((handle.index < capacity
			 && i.header->free == 0
			 && i.header->typeId == handle.typeId
			 && i.header->generation == handle.generation)
			? (uintptr_t)i.data
			: 0);
}


void SparseHandleMap32::init(
	u16 _elementSizeB,
	u32 _capacity,
	void* buffer)
{
	elementSizeB = _elementSizeB;
	assert(elementSizeB == _elementSizeB && "element size too large");
	capacity = _capacity;

	if (!buffer) {
		size_t size = getTotalBufferSize(elementSizeB, capacity);
		buffer = Q_malloc(size);
		memset(buffer, 0, size);
		_memoryOwned = 1;
	}

	items = buffer;

	// reset to set up the sparseIds freelist
	reset();
}


void SparseHandleMap32::deinit()
{
	if (_memoryOwned && items) {
		free(items);
		items = nullptr;
	}
}


// Helper Macros

// Macro for defining a SparseHandleMap32 storage buffer
#define SparseHandleMap32Buffer(Type, Name, capacity) \
	u8 Name[(sizeof(Type) + sizeof(SparseHandleMap32::Header)) * capacity];\
	static_assert(is_aligned(sizeof(Name),8),"sizeof items array must be a multiple of 8");


// Macro for defining a type-safe SparseHandleMap32 wrapper that avoids void* and elementSizeB in the api
#define SparseHandleMap32Typed(Type, Name, TypeId) \
	struct Name {\
		enum { TypeSize = sizeof(Type) };\
		struct Item { SparseHandleMap32::Header* header; Type* data; };\
		SparseHandleMap32 _map;\
		static size_t getTotalBufferSize(u32 capacity)\
											{ return SparseHandleMap32::getTotalBufferSize(TypeSize, capacity); }\
		explicit Name(u32 _capacity, void* buffer = nullptr)\
											{ _map.init(TypeSize, _capacity, buffer); }\
		explicit Name() {}\
		Type* at(h64 handle)				{ return (Type*)_map.at(handle); }\
		Type* operator[](h64 handle)		{ return at(handle); }\
		bool erase(h64 handle)				{ return _map.erase(handle); }\
		h64 insert(Type* src = nullptr, Type** out = nullptr, u16 typeId = TypeId)\
											{ return _map.insert((void*)src, (void**)out, typeId); }\
		void clear()						{ _map.clear(); }\
		void reset()						{ _map.reset(); }\
		uintptr_t has(h64 handle)			{ return _map.has(handle); }\
		inline Item item(u32 index) {\
			SparseHandleMap32::Item i = _map.item(index);\
			return Item{ i.header, (Type*)i.data };\
		}\
		void init(u32 capacity, void* buffer = nullptr)\
											{ _map.init(TypeSize, capacity, buffer); }\
		void deinit()						{ _map.deinit(); }\
	};

// Macro like SparseHandleMap32Typed but also internally includes the storage buffer, so there is no
// need to call init or create the buffer externally
#define SparseHandleMap32TypedWithBuffer(Type, Name, HndType, TypeId, _capacity) \
	struct Name {\
		enum { TypeSize = sizeof(Type) };\
		struct Item { SparseHandleMap32::Header* header; Type* data; };\
		SparseHandleMap32 _map;\
		u8 _buffer[(sizeof(Type) + sizeof(SparseHandleMap32::Header)) * _capacity];\
		static_assert(is_aligned(sizeof(Name::_buffer),8),"sizeof items array must be a multiple of 8");\
		static size_t getTotalBufferSize(u32 capacity) {\
			return sizeof(_buffer);\
		}\
		explicit Name()	: _buffer{} {\
			_map.init(TypeSize, _capacity, &_buffer);\
		}\
		Type* at(HndType handle)			{ return (Type*)_map.at(handle); }\
		Type* operator[](HndType handle)	{ return at(handle); }\
		bool erase(HndType handle)			{ return _map.erase(handle); }\
		HndType insert(Type* src = nullptr, Type** out = nullptr, u8 typeId = TypeId) {\
			return _map.insert((void*)src, (void**)out, typeId);\
		}\
		void clear()						{ _map.clear(); }\
		void reset()						{ _map.reset(); }\
		uintptr_t has(HndType handle)		{ return _map.has(handle); }\
		inline Item item(u32 index) {\
			SparseHandleMap32::Item i = _map.item(index);\
			return Item{ i.header, (Type*)i.data };\
		}\
		void init(u32 capacity, void* buffer = nullptr)\
											{ _map.init(TypeSize, capacity, buffer); }\
		void deinit()						{ _map.deinit(); }\
	};\
	static_assert(std::is_same<h64,HndType>::value, #HndType " must be typedef h64");

#endif