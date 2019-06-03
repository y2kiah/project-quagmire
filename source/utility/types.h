#ifndef _TYPES_H
#define _TYPES_H

#include <cstdint>
#include <climits>
#include <cstddef>
#include <cfloat>
#include <cstring>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    r32;
typedef double   r64;
typedef float    f32;
typedef double   f64;

#define global static

// Handles

/**
 * @struct h32/h64
 * @var	free		0 if active, 1 if slot is part of freelist, only applicable to inner ids
 * @var	generation	incrementing generation of data at the index, for tracking accesses to old data
 * @var	typeId		relates to itemTypeId parameter of DenseHandleMap16
 * @var	index		When used as a handle (outer id, given to the client):
 *						free==0, index of id in the sparseIds array
 *					When used as an inner id (stored in sparseIds array):
 *						free==0, index of the item in the dense items array
 *						free==1, index of next free slot, forming an embedded linked list
 * @var	value		unioned with the above four vars, used for direct comparison of ids
 */
struct h32 {
	union {
		struct {
			u16 index;
			u8  typeId;
			u8  generation : 7;
			u8  free       : 1;
		};
		u32 value;
	};
};

struct h64 {
	union {
		struct {
			u32 index;
			u16 typeId;
			u16 generation : 15;
			u16 free       : 1;
		};
		u64 value;
	};
};

#define null_h32	h32{}
#define null_h64	h64{}

// handle comparison functions
bool operator==(const h32& a, const h32& b) { return (a.value == b.value); }
bool operator!=(const h32& a, const h32& b) { return (a.value != b.value); }
bool operator< (const h32& a, const h32& b) { return (a.value < b.value); }
bool operator> (const h32& a, const h32& b) { return (a.value > b.value); }

bool operator==(const h64& a, const h64& b) { return (a.value == b.value); }
bool operator!=(const h64& a, const h64& b) { return (a.value != b.value); }
bool operator< (const h64& a, const h64& b) { return (a.value < b.value); }
bool operator> (const h64& a, const h64& b) { return (a.value > b.value); }


#endif