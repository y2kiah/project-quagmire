#include <cstdint>
#include <climits>
#include <cstddef>
#include <cfloat>

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

// Assert macros
#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
#define assert(a)	if(!(a)) {*(int *)0 = 0;}
#else
#define assert(a)
#endif

#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
#define ASSERT_GL_ERROR		assert(glGetError() == GL_NO_ERROR)
#else
#define ASSERT_GL_ERROR
#endif

// Alignment macros, use only with pow2 alignments
#define is_aligned(addr, bytes)     (((uintptr_t)(const void *)(addr)) % (bytes) == 0)

#define _align_mask(addr, mask)     (((uintptr_t)(addr)+(mask))&~(mask))
#define align(addr, bytes)          _align_mask(addr,(bytes)-1)