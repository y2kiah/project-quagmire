#ifndef _INTRINSICS_H
#define _INTRINSICS_H

#include "types.h"
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif



inline __m128 simd_set(float x, float y, float z, float w)
{
	return _mm_set_ps(w, z, y, x);
}

inline __m128 simd_splat_x(__m128 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)); }
inline __m128 simd_splat_y(__m128 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)); }
inline __m128 simd_splat_z(__m128 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)); }
inline __m128 simd_splat_w(__m128 v) { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)); }

inline __m128 simd_madd(__m128 a, __m128 b, __m128 c)
{
	return _mm_add_ps(_mm_mul_ps(a, b), c);
}


/**
 * BitScan* implementations provided under MIT license:
 * https://github.com/dotnet/coreclr/blob/master/src/gc/env/gcenv.base.h
 */

#ifdef _MSC_VER
	#pragma intrinsic(_BitScanForward)
	#pragma intrinsic(_BitScanReverse)
	#if _WIN64
		#pragma intrinsic(_BitScanForward64)
		#pragma intrinsic(_BitScanReverse64)
	#endif
#endif

/**
 * Cross-platform wrapper for the _BitScanForward compiler intrinsic.
 * A value is unconditionally stored through the bitIndex argument,
 * but callers should only rely on it when the function returns true;
 * otherwise, the stored value is undefined and varies by implementation
 * and hardware platform.
 */
inline u8 BitScanFwd(
	u32 *bitIndex,
	u32 mask)
{
#ifdef _MSC_VER
	return _BitScanForward((unsigned long *)bitIndex, mask);
#else
	int iIndex = __builtin_ffs(mask);
	*bitIndex = (u32)(iIndex - 1);
	// Both GCC and Clang generate better, smaller code if we check whether the
	// mask was/is zero rather than the equivalent check that iIndex is zero.
	return mask != 0 ? true : false;
#endif // _MSC_VER
}

/**
 * Cross-platform wrapper for the _BitScanForward64 compiler intrinsic.
 * A value is unconditionally stored through the bitIndex argument,
 * but callers should only rely on it when the function returns true;
 * otherwise, the stored value is undefined and varies by implementation
 * and hardware platform.
 */
inline u8 BitScanFwd64(
	u32 *bitIndex,
	u64 mask)
{
#ifdef _MSC_VER
	#if _WIN64
		return _BitScanForward64((unsigned long *)bitIndex, mask);
	#else
		// MSVC targeting a 32-bit target does not support this intrinsic.
		// We can fake it using two successive invocations of _BitScanForward.
		u32 hi = (mask >> 32) & 0xFFFFFFFF;
		u32 lo = mask & 0xFFFFFFFF;
		u32 fakeBitIndex = 0;

		u8 result = BitScanForward(bitIndex, lo);
		if (result == 0)
		{
			result = BitScanForward(&fakeBitIndex, hi);
			if (result != 0)
			{
				*bitIndex = fakeBitIndex + 32;
			}
		}

		return result;
	#endif // _WIN64
#else
	int iIndex = __builtin_ffsll(mask);
	// Set the Index after deducting unity
	*bitIndex = (u32)(iIndex - 1);
	// Both GCC and Clang generate better, smaller code if we check whether the
	// mask was/is zero rather than the equivalent check that iIndex is zero.
	return mask != 0 ? true : false;
#endif // _MSC_VER
}

/**
 * Cross-platform wrapper for the _BitScanReverse compiler intrinsic.
 */
inline u8 BitScanRev(
	u32* bitIndex,
	u32 mask)
{
#ifdef _MSC_VER
	return _BitScanReverse((unsigned long *)bitIndex, mask);
#else
	// The result of __builtin_clzl is undefined when mask is zero,
	// but it's still OK to call the intrinsic in that case (just don't use the output).
	// Unconditionally calling the intrinsic in this way allows the compiler to
	// emit branchless code for this function when possible (depending on how the
	// intrinsic is implemented for the target platform).
	int lzcount = __builtin_clzl(mask);
	*bitIndex = (u32)(31 - lzcount);
	return mask != 0 ? true : false;
#endif // _MSC_VER
}

/**
 * Cross-platform wrapper for the _BitScanReverse64 compiler intrinsic.
 */
inline u8 BitScanRev64(
	u32* bitIndex,
	u64 mask)
{
#ifdef _MSC_VER
	#if _WIN64
		return _BitScanReverse64((unsigned long *)bitIndex, mask);
	#else
		// MSVC targeting a 32-bit target does not support this intrinsic.
		// We can fake it checking whether the upper 32 bits are zeros (or not)
		// then calling _BitScanReverse() on either the upper or lower 32 bits.
		u32 upper = (u32)(mask >> 32);

		if (upper != 0)
		{
			u8 result = _BitScanReverse((unsigned long *)bitIndex, upper);
			*bitIndex += 32;
			return result;
		}

		return _BitScanReverse((unsigned long *)bitIndex, (u32)(mask));
	#endif // _WIN64
#else
	// The result of __builtin_clzll is undefined when mask is zero,
	// but it's still OK to call the intrinsic in that case (just don't use the output).
	// Unconditionally calling the intrinsic in this way allows the compiler to
	// emit branchless code for this function when possible (depending on how the
	// intrinsic is implemented for the target platform).
	int lzcount = __builtin_clzll(mask);
	*bitIndex = (u32)(63 - lzcount);
	return mask != 0 ? true : false;
#endif // _MSC_VER
}

#endif