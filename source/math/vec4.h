#ifndef _VEC4_H
#define _VEC4_H

#include "vec3.h"


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
		return *(vec4*)this;
	}

	vec4& operator=(const _vec4& rhs) {
		*this = (vec4&)rhs;
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


#endif