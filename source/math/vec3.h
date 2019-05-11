#ifndef _VEC3_H
#define _VEC3_H

#include "vec2.h"


struct vec3
{
	union {
		struct { r32 x, y, z; };
		struct { r32 r, g, b; };
		struct { r32 s, t, p; };
		r32 E[3];

		SWIZZLE_vec3(x, y, z)
		SWIZZLE_vec3(r, g, b)
		SWIZZLE_vec3(s, t, p)
	};

	r32& operator[](size_t e) { assert(e < 3); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 3); return E[e]; }

	vec3& operator=(const vec2& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *(vec3*)this;
	}

	vec3& operator=(const _vec3& rhs) {
		*this = (vec3&)rhs;
		return *this;
	}
};

static_assert(sizeof(vec3) == 12 && sizeof(_vec3) == 12, "");


// unary operators, pre/post increment and decrement

vec3 operator-(const vec3& rhs)
{
	return vec3{
		-rhs.x,
		-rhs.y,
		-rhs.z
	};
}

vec3& operator++(vec3& rhs)
{
	++rhs.x;
	++rhs.y;
	++rhs.z;
	return rhs;
}

vec3 operator++(vec3& lhs, int)
{
	vec3 tmp = lhs;
	++lhs.x;
	++lhs.y;
	++lhs.z;
	return tmp;
}

vec3& operator--(vec3& rhs)
{
	--rhs.x;
	--rhs.y;
	--rhs.z;
	return rhs;
}

vec3 operator--(vec3& lhs, int)
{
	vec3 tmp = lhs;
	--lhs.x;
	--lhs.y;
	--lhs.z;
	return tmp;
}

// component-wise assignment operators

vec3& operator+=(vec3& lhs, const vec3& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

vec3& operator-=(vec3& lhs, const vec3& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}

vec3& operator*=(vec3& lhs, const vec3& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	lhs.z *= rhs.z;
	return lhs;
}

vec3& operator/=(vec3& lhs, const vec3& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	lhs.z /= rhs.z;
	return lhs;
}


// scalar assignment operators

vec3& operator+=(vec3& lhs, r32 rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	lhs.z += rhs;
	return lhs;
}

vec3& operator-=(vec3& lhs, r32 rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	lhs.z -= rhs;
	return lhs;
}

vec3& operator*=(vec3& lhs, r32 rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	return lhs;
}

vec3& operator/=(vec3& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	lhs.x /= rhs;
	lhs.y /= rhs;
	lhs.z /= rhs;
	return lhs;
}

// component-wise operators

vec3 operator+(const vec3& lhs, const vec3& rhs)
{
	return vec3{
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z
	};
}

vec3 operator-(const vec3& lhs, const vec3& rhs)
{
	return vec3{
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z
	};
}

vec3 operator*(const vec3& lhs, const vec3& rhs)
{
	return vec3{
		lhs.x * rhs.x,
		lhs.y * rhs.y,
		lhs.z * rhs.z
	};
}

vec3 operator/(const vec3& lhs, const vec3& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
	return vec3{
		lhs.x / rhs.x,
		lhs.y / rhs.y,
		lhs.z / rhs.z
	};
}

// scalar operators

vec3 operator+(const vec3& lhs, r32 rhs)
{
	return vec3{
		lhs.x + rhs,
		lhs.y + rhs,
		lhs.z + rhs
	};
}

vec3 operator-(const vec3& lhs, r32 rhs)
{
	return vec3{
		lhs.x - rhs,
		lhs.y - rhs,
		lhs.z - rhs
	};
}

vec3 operator*(const vec3& lhs, r32 rhs)
{
	return vec3{
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs
	};
}

vec3 operator/(const vec3& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	return vec3{
		lhs.x / rhs,
		lhs.y / rhs,
		lhs.z / rhs
	};
}

// Comparison operators

bool operator==(const vec3& lhs, const vec3& rhs)
{
	return ((fabs(lhs.x - rhs.x) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs.y) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs.z) <= VEC_COMPARISON_DELTA));
}

bool operator!=(const vec3& lhs, const vec3& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const vec3& lhs, r32 rhs)
{
	return ((fabs(lhs.x - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= VEC_COMPARISON_DELTA))
			|| (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>=(const vec3& lhs, r32 rhs)
{
	return ((fabs(lhs.x - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= VEC_COMPARISON_DELTA))
			|| (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}

bool operator<(const vec3& lhs, r32 rhs)
{
	return (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>(const vec3& lhs, r32 rhs)
{
	return (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}


#endif