#ifndef _INTERSECTION_H
#define _INTERSECTION_H

#include "../utility/types.h"
#include "../utility/intrinsics.h"
#include "../scene/geometry.h"


enum IntersectionResult : u8
{
	Outside      = 0,
	Intersecting = 1,
	Inside       = 3
};


u8 frustum_intersectSphere(
	const Plane planes[6],
	const Sphere& s)
{
	u8 result = Inside;

	for (u32 i = 0; i < 6; ++i)
	{
		const Plane &p = planes[i];
		
		// distance from plane to sphere center point
		r32 dist = dot(p.n, s.center) - p.d;

		// completely on the back side (outside of frustum)
		if (dist < -s.radius) {
			return Outside;
		}
		// completely on the front side of plane (toward inside of frustum)
		else if (dist > s.radius) {
			result &= Inside;
		}
		// intersecting the plane
		else {
			result &= Intersecting;
		}
	}
	return result;
}


u8 frustum_intersectSphere(
	const Frustum& f,
	const Sphere& s)
{
	Plane planes[6] = {};
	frustum_getPlanes(f, planes);
	return frustum_intersectSphere(planes, s);
}


/**
 * REFERENCE IMPLEMENTATION, prefer to use the faster frustum_intersectSpheres_sse function.
 * length * 2(bit) <= size(byte) * 8(bit/byte)
 */
void frustum_intersectSpheres(
	const Frustum& f,
	u32 length,				// number of spheres passed
	const Sphere* spheres,
	u32 resultsSize,		// size in bytes of results, asserted to be large enough to store 2 bits for each sphere (length-1)/4 + 1
	u8* results)
{
	assert(resultsSize * 4 >= length);

	Plane planes[6] = {};
	frustum_getPlanes(f, planes);
	
	for (u32 i = 0; i < length; ++i)
	{
		const Sphere& s = spheres[i];
		const u8 result = frustum_intersectSphere(planes, s);
		// we store 2 bits per sphere for the outside/intersecting/inside result
		const u32 ri = i / 4;
		const u32 shift = (i * 2) & 7;
		results[ri] |= result << shift;
	}
}

void frustum_intersectSpheres_sse(
	const Frustum& f,
	u32 length,				// number of spheres passed
	const Sphere* spheres,
	u32 resultsSize,		// size in bytes of results, asserted to be large enough to store 2 bits for each sphere (length-1)/4 + 1
	u8* results)
{
	assert(resultsSize * 4 >= length);

	// TODO: this algorithm wastes 2 lanes of 8 for every sphere, we could instead try splatting the planes and
	// intersecting 1 plane with 4 spheres at a time

	// we use this formula to determine outside:
	//   dot(p.n, s.center) - p.d < -s.radius
	// and this to determine fully inside:
	//   dot(p.n, s.center) - p.d > s.radius

	const __m128 invert = _mm_set1_ps(-1.0f);
	const __m128 plane_0123_x = _mm_load_ps(f.nx);
	const __m128 plane_0123_y = _mm_load_ps(f.ny);
	const __m128 plane_0123_z = _mm_load_ps(f.nz);
	const __m128 plane_0123_d = _mm_mul_ps(_mm_load_ps(f.d), invert);
	const __m128 plane_2345_x = _mm_load_ps(f.nx+2);
	const __m128 plane_2345_y = _mm_load_ps(f.ny+2);
	const __m128 plane_2345_z = _mm_load_ps(f.nz+2);
	const __m128 plane_2345_d = _mm_mul_ps(_mm_load_ps(f.d+2), invert);

	
	for (u32 i = 0; i < length; ++i)
	{
		// Load sphere into SSE register
		const __m128 s = _mm_load_ps((const float*)(spheres+i));
		const __m128 xxxx = simd_splat_x(s);
		const __m128 yyyy = simd_splat_y(s);
		const __m128 zzzz = simd_splat_z(s);
		const __m128 rrrr = simd_splat_w(s);
		const __m128 rrrr_inv = _mm_mul_ps(rrrr, invert);

		__m128 v, r_outside, r_inside;
		// find signed distance from plane to sphere center
		// dot(p.n, s) - p.d
		v = simd_madd(xxxx, plane_0123_x, plane_0123_d);
		v = simd_madd(yyyy, plane_0123_y, v);
		v = simd_madd(zzzz, plane_0123_z, v);

		// Test for intersection, one of r floats will be set to 0xFFFFFFFF if sphere is outside of the frustum
		r_outside = _mm_cmplt_ps(v, rrrr_inv);
		// and test for fully inside
		r_inside  = _mm_cmpgt_ps(v, rrrr);

		// Same for second set of planes
		v = simd_madd(xxxx, plane_2345_x, plane_2345_d);
		v = simd_madd(yyyy, plane_2345_y, v);
		v = simd_madd(zzzz, plane_2345_z, v);

		// 0123 | 2345
		r_outside = _mm_or_ps(r_outside, _mm_cmplt_ps(v, rrrr_inv));
		r_inside  = _mm_or_ps(r_inside,  _mm_cmpgt_ps(v, rrrr));
		
		// Shuffle and extract the result:
		// 1. movehl(r, r) does this (we're interested in 2 lower floats):
		//     a  b  c  d -> c  d  c  d
		//    02 13 24 35 -> 24 35 24 35
		// 2. then we OR it with the existing value (ignoring 2 upper floats)
		//     a  b | c  d  =  A  B
		//    02 13 | 24 35 = 024 135
		r_outside = _mm_or_ps(r_outside, _mm_movehl_ps(r_outside, r_outside));
		r_inside  = _mm_or_ps(r_inside,  _mm_movehl_ps(r_inside, r_inside));
		// 3. and then we OR it again ignoring all but 1 lowest float:
		//     A  |  B  =  R
		//    024 | 135 = 012345
		// Result is written in the lowest float
		r_outside = _mm_or_ps(r_outside, simd_splat_y(r_outside));
		r_inside  = _mm_or_ps(r_inside,  simd_splat_y(r_inside));

		u32 result_outside, result_inside;
		_mm_store_ss((float*)&result_outside, r_outside);
		_mm_store_ss((float*)&result_inside, r_inside);

		// flip outside bit to represent intersecting bit, combine with the fully inside bit shifted left by 1
		u32 result = (~result_outside) | (result_inside << 1);
		// write the 2-bit result
		const u32 ri = i / 4;
		const u32 shift = (i * 2) & 7;
		results[ri] |= result << shift;
	}
}


#endif