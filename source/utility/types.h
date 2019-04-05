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

#define global static


// stringize macros
#define xstr(s) str(s)
#define str(s) #s


// Assert macros
#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0

#ifdef SDL_assert_h_
#define assert(a)	        SDL_assert((a))
#else
#define assert(a)	        if(!(a)) {*(int *)0 = 0;}
#endif

#define ASSERT_GL_ERROR		assert(glGetError() == GL_NO_ERROR)

#else

#define assert(a)
#define ASSERT_GL_ERROR

#endif


// Alignment macros, use only with pow2 alignments
#define is_aligned(addr, bytes)     (((uintptr_t)(const void *)(addr)) % (bytes) == 0)

#define _align_mask(addr, mask)     (((uintptr_t)(addr)+(mask))&~(mask))
#define align(addr, bytes)          _align_mask(addr,(bytes)-1)

// static assert macro for checking struct size to a base alignment when packed in an array
//#define _static_assert_aligned_size_msg(Type,bytes)		#Type " size is not a multiple of " xstr(bytes)
//#define static_assert_aligned_size(Type,bytes)			static_assert(sizeof(Type) % (bytes) == 0, _static_assert_aligned_size_msg(Type,bytes))
#define static_assert_aligned_size(Type,bytes)			static_assert(sizeof(Type) % (bytes) == 0, #Type " size is not a multiple of " xstr(bytes))


// min/max functions, should avoid using macros for this due to possible double-evaluation
inline u8  min(u8 a, u8 b)   { return (a < b ? a : b); }
inline i8  min(i8 a, i8 b)   { return (a < b ? a : b); }
inline u16 min(u16 a, u16 b) { return (a < b ? a : b); }
inline i16 min(i16 a, i16 b) { return (a < b ? a : b); }
inline u32 min(u32 a, u32 b) { return (a < b ? a : b); }
inline i32 min(i32 a, i32 b) { return (a < b ? a : b); }
inline u64 min(u64 a, u64 b) { return (a < b ? a : b); }
inline i64 min(i64 a, i64 b) { return (a < b ? a : b); }

inline u8  max(u8 a, u8 b)   { return (a > b ? a : b); }
inline i8  max(i8 a, i8 b)   { return (a > b ? a : b); }
inline u16 max(u16 a, u16 b) { return (a > b ? a : b); }
inline i16 max(i16 a, i16 b) { return (a > b ? a : b); }
inline u32 max(u32 a, u32 b) { return (a > b ? a : b); }
inline i32 max(i32 a, i32 b) { return (a > b ? a : b); }
inline u64 max(u64 a, u64 b) { return (a > b ? a : b); }
inline i64 max(i64 a, i64 b) { return (a > b ? a : b); }
