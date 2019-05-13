#ifndef _VEC4_H
#define _VEC4_H

#include "vec3.h"

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


struct vec4
{
	union {
		struct { r32 x, y, z, w; };
		struct { r32 r, g, b, a; };
		struct { r32 s, t, p, q; };
		r32 E[4];

		SWIZZLE_vec4(x, y, z, w)
		SWIZZLE_vec4(r, g, b, a)
		SWIZZLE_vec4(s, t, p, q)
	};

	r32& operator[](size_t e) { assert(e < 4); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 4); return E[e]; }
	
	vec4& operator=(const vec2& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *(vec4*)this;
	}

	vec4& operator=(const vec3& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *(vec4*)this;
	}
	
	vec4& operator=(const vec4& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
		return *(vec4*)this;
	}

	vec4& operator=(const _vec4& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
		return *this;
	}
};

static_assert(sizeof(vec4) == 16 && sizeof(_vec4) == 16, "");


// unary operators, pre/post increment and decrement

vec4 operator-(const vec4& rhs)
{
	return vec4{
		-rhs.x,
		-rhs.y,
		-rhs.z,
		-rhs.w
	};
}

vec4& operator++(vec4& rhs)
{
	++rhs.x;
	++rhs.y;
	++rhs.z;
	++rhs.w;
	return rhs;
}

vec4 operator++(vec4& lhs, int)
{
	vec4 tmp = lhs;
	++lhs.x;
	++lhs.y;
	++lhs.z;
	++lhs.w;
	return tmp;
}

vec4& operator--(vec4& rhs)
{
	--rhs.x;
	--rhs.y;
	--rhs.z;
	--rhs.w;
	return rhs;
}

vec4 operator--(vec4& lhs, int)
{
	vec4 tmp = lhs;
	--lhs.x;
	--lhs.y;
	--lhs.z;
	--lhs.w;
	return tmp;
}

// component-wise assignment operators

vec4& operator+=(vec4& lhs, const vec4& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	lhs.w += rhs.w;
	return lhs;
}

vec4& operator-=(vec4& lhs, const vec4& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	lhs.w -= rhs.w;
	return lhs;
}

vec4& operator*=(vec4& lhs, const vec4& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	lhs.z *= rhs.z;
	lhs.w *= rhs.w;
	return lhs;
}

vec4& operator/=(vec4& lhs, const vec4& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	lhs.z /= rhs.z;
	lhs.w /= rhs.w;
	return lhs;
}


// scalar assignment operators

vec4& operator+=(vec4& lhs, r32 rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	lhs.z += rhs;
	lhs.w += rhs;
	return lhs;
}

vec4& operator-=(vec4& lhs, r32 rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	lhs.z -= rhs;
	lhs.w -= rhs;
	return lhs;
}

vec4& operator*=(vec4& lhs, r32 rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	lhs.w *= rhs;
	return lhs;
}

vec4& operator/=(vec4& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	lhs.x /= rhs;
	lhs.y /= rhs;
	lhs.z /= rhs;
	lhs.w /= rhs;
	return lhs;
}

// component-wise operators

vec4 operator+(const vec4& lhs, const vec4& rhs)
{
	return vec4{
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z,
		lhs.w + rhs.w
	};
}

vec4 operator-(const vec4& lhs, const vec4& rhs)
{
	return vec4{
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z,
		lhs.w - rhs.w
	};
}

vec4 operator*(const vec4& lhs, const vec4& rhs)
{
	return vec4{
		lhs.x * rhs.x,
		lhs.y * rhs.y,
		lhs.z * rhs.z,
		lhs.w * rhs.w
	};
}

vec4 operator/(const vec4& lhs, const vec4& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
	return vec4{
		lhs.x / rhs.x,
		lhs.y / rhs.y,
		lhs.z / rhs.z,
		lhs.w / rhs.w
	};
}

// scalar operators

vec4 operator+(const vec4& lhs, r32 rhs)
{
	return vec4{
		lhs.x + rhs,
		lhs.y + rhs,
		lhs.z + rhs,
		lhs.w + rhs
	};
}

vec4 operator-(const vec4& lhs, r32 rhs)
{
	return vec4{
		lhs.x - rhs,
		lhs.y - rhs,
		lhs.z - rhs,
		lhs.w - rhs
	};
}

vec4 operator*(const vec4& lhs, r32 rhs)
{
	return vec4{
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs,
		lhs.w * rhs
	};
}

vec4 operator*(r32 lhs, const vec4& rhs)
{
	return vec4{
		rhs.x * lhs,
		rhs.y * lhs,
		rhs.z * lhs,
		rhs.w * lhs
	};
}

vec4 operator/(const vec4& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	return vec4{
		lhs.x / rhs,
		lhs.y / rhs,
		lhs.z / rhs,
		lhs.w / rhs
	};
}

vec4 operator/(r32 lhs, const vec4& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
	return vec4{
		lhs / rhs.x,
		lhs / rhs.y,
		lhs / rhs.z,
		lhs / rhs.w
	};
}

// Comparison operators

bool operator==(const vec4& lhs, const vec4& rhs)
{
	return ((fabs(lhs.x - rhs.x) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs.y) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs.z) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.w - rhs.w) <= VEC_COMPARISON_DELTA));
}

bool operator!=(const vec4& lhs, const vec4& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const vec4& lhs, r32 rhs)
{
	return ((fabs(lhs.x - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.w - rhs) <= VEC_COMPARISON_DELTA))
			|| (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs && lhs.w < rhs);
}

bool operator>=(const vec4& lhs, r32 rhs)
{
	return ((fabs(lhs.x - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.w - rhs) <= VEC_COMPARISON_DELTA))
			|| (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs && lhs.w > rhs);
}

bool operator<(const vec4& lhs, r32 rhs)
{
	return (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs && lhs.w > rhs);
}

bool operator>(const vec4& lhs, r32 rhs)
{
	return (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs && lhs.w > rhs);
}

// vector operations

r32 dot(const vec4& v1, const vec4& v2)
{
	// TODO: test speed of these two alternative ways of writing dot
	// the compiler might generate faster code or be more likely to use SIMD with the first method
	alignas(16) vec4 mul{ v1.x, v1.y, v1.z, v1.w };
	mul *= v2;
	return (v2.x + v2.y + v2.z + v2.w);
	/*return (
		v1.x * v2.x +
		v1.y * v2.y +
		v1.z * v2.z +
		v1.w * v2.w);*/
}

vec4 dot4_simd(
	vec4 v1[4],
	vec4 v2[4])
{
	// TODO: temp load code, this is converting 4 vectors from AoS to SoA, the data should have
	// already been stored in this format coming into the function, perhaps the function would
	// take a soa_vec4 struct type instead, to help with understanding the data layout 
	alignas(16) r32 v1x[4] = { v1[0].x, v1[1].x, v1[2].x, v1[3].x };
	alignas(16) r32 v1y[4] = { v1[0].y, v1[1].y, v1[2].y, v1[3].y };
	alignas(16) r32 v1z[4] = { v1[0].z, v1[1].z, v1[2].z, v1[3].z };
	alignas(16) r32 v1w[4] = { v1[0].w, v1[1].w, v1[2].w, v1[3].w };

	alignas(16) r32 v2x[4] = { v2[0].x, v2[1].x, v2[2].x, v2[3].x };
	alignas(16) r32 v2y[4] = { v2[0].y, v2[1].y, v2[2].y, v2[3].y };
	alignas(16) r32 v2z[4] = { v2[0].z, v2[1].z, v2[2].z, v2[3].z };
	alignas(16) r32 v2w[4] = { v2[0].w, v2[1].w, v2[2].w, v2[3].w };

	__m128 ax = _mm_load_ps(v1x);
	__m128 bx = _mm_load_ps(v2x);
	__m128 ay = _mm_load_ps(v1y);
	__m128 by = _mm_load_ps(v2y);
	__m128 az = _mm_load_ps(v1z);
	__m128 bz = _mm_load_ps(v2z);
	__m128 aw = _mm_load_ps(v1w);
	__m128 bw = _mm_load_ps(v2w);
	// End temp load code

	// this is the dot code itself, see https://www.gdcvault.com/play/1022249/SIMD-at-Insomniac-Games-How
	__m128 dx = _mm_mul_ps(ax, bx); // dx = ax * bx
	__m128 dy = _mm_mul_ps(ay, by); // dy = ay * by
	__m128 dz = _mm_mul_ps(az, bz); // dz = az * bz
	__m128 dw = _mm_mul_ps(aw, bw); // dw = aw * bw
	__m128 a0 = _mm_add_ps(dx, dy); // a0 = dx + dy
	__m128 a1 = _mm_add_ps(dz, dw); // a1 = dz + dw
	__m128 dots = _mm_add_ps(a0, a1); // dots = a0 + a1
	
	// TODO: also this store code could be avoided, if the soa_vec4 class uses __m128 as its type
	// (not unioned with float[4]) it can be packed / unpacked in batch
	vec4 vec;
	_mm_store_ps(vec.E, dots);
	return vec;
}

r32 length(const vec4& v)
{
	return sqrtf(dot(v, v));
}

vec4 normalize(const vec4& v)
{
	return v * (1.0f / sqrtf(dot(v, v)));
}

/* // TODO: need to write SoA version like dot above
vec4 normalize_simd(const vec4& v)
{
	__m128 tmp = _mm_load_ps(v.E);
	__m128 dp = _mm_dp_ps(tmp, tmp, 0x7F);
	dp = _mm_rsqrt_ps(dp);
	tmp = _mm_mul_ps(tmp, dp);
	
	vec4 vec;
	_mm_store_ps(vec.E, tmp);
	vec.w = 1.0f;
	return vec;
}
*/

#endif