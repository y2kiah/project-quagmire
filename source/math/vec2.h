#ifndef _VEC2_H
#define _VEC2_H

#include "math_core.h"
#include "swizzle.h"


struct vec2
{
	union {
		struct { r32 x, y; };
		struct { r32 r, g; };
		struct { r32 s, t; };
		r32 E[2];

		SWIZZLE_vec2(x, y)
		SWIZZLE_vec2(r, g)
		SWIZZLE_vec2(s, t)
	};

	r32& operator[](size_t e) { assert(e < 2); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 2); return E[e]; }

	vec2& operator=(const _vec2& rhs) {
		*this = (vec2&)rhs;
		return *this;
	}
};

static_assert(sizeof(vec2) == 8 && sizeof(_vec2) == 8, "");


// unary operators, pre/post increment and decrement

vec2 operator-(const vec2& rhs)
{
	return vec2{
		-rhs.x,
		-rhs.y
	};
}

vec2& operator++(vec2& rhs)
{
	++rhs.x;
	++rhs.y;
	return rhs;
}

vec2 operator++(vec2& lhs, int)
{
	vec2 tmp = lhs;
	++lhs.x;
	++lhs.y;
	return tmp;
}

vec2& operator--(vec2& rhs)
{
	--rhs.x;
	--rhs.y;
	return rhs;
}

vec2 operator--(vec2& lhs, int)
{
	vec2 tmp = lhs;
	--lhs.x;
	--lhs.y;
	return tmp;
}

// component-wise assignment operators

vec2& operator+=(vec2& lhs, const vec2& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}

vec2& operator-=(vec2& lhs, const vec2& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

vec2& operator*=(vec2& lhs, const vec2& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	return lhs;
}

vec2& operator/=(vec2& lhs, const vec2& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f);
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	return lhs;
}


// scalar assignment operators

vec2& operator+=(vec2& lhs, r32 rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	return lhs;
}

vec2& operator-=(vec2& lhs, r32 rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	return lhs;
}

vec2& operator*=(vec2& lhs, r32 rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	return lhs;
}

vec2& operator/=(vec2& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	lhs.x /= rhs;
	lhs.y /= rhs;
	return lhs;
}

// component-wise operators

vec2 operator+(const vec2& lhs, const vec2& rhs)
{
	return vec2{
		lhs.x + rhs.x,
		lhs.y + rhs.y
	};
}

vec2 operator-(const vec2& lhs, const vec2& rhs)
{
	return vec2{
		lhs.x - rhs.x,
		lhs.y - rhs.y
	};
}

vec2 operator*(const vec2& lhs, const vec2& rhs)
{
	return vec2{
		lhs.x * rhs.x,
		lhs.y * rhs.y
	};
}

vec2 operator/(const vec2& lhs, const vec2& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f);
	return vec2{
		lhs.x / rhs.x,
		lhs.y / rhs.y
	};
}

// scalar operators

vec2 operator+(const vec2& lhs, r32 rhs)
{
	return vec2{
		lhs.x + rhs,
		lhs.y + rhs
	};
}

vec2 operator-(const vec2& lhs, r32 rhs)
{
	return vec2{
		lhs.x - rhs,
		lhs.y - rhs
	};
}

vec2 operator*(const vec2& lhs, r32 rhs)
{
	return vec2{
		lhs.x * rhs,
		lhs.y * rhs
	};
}

vec2 operator/(const vec2& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	return vec2{
		lhs.x / rhs,
		lhs.y / rhs
	};
}

// Comparison operators

bool operator==(const vec2& lhs, const vec2& rhs)
{
	return ((fabs(lhs.x - rhs.x) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs.y) <= VEC_COMPARISON_DELTA));
}

bool operator!=(const vec2& lhs, const vec2& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const vec2& lhs, r32 rhs)
{
	return ((fabs(lhs.x - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= VEC_COMPARISON_DELTA))
			|| (lhs.x < rhs && lhs.y < rhs);
}

bool operator>=(const vec2& lhs, r32 rhs)
{
	return ((fabs(lhs.x - rhs) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= VEC_COMPARISON_DELTA))
			|| (lhs.x > rhs && lhs.y > rhs);
}

bool operator<(const vec2& lhs, r32 rhs)
{
	return (lhs.x < rhs && lhs.y < rhs);
}

bool operator>(const vec2& lhs, r32 rhs)
{
	return (lhs.x > rhs && lhs.y > rhs);
}


#endif