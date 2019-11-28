#ifndef _DVEC2_H
#define _DVEC2_H

#include "math_core.h"
#include "swizzle.h"


union dvec2
{
	struct { r64 x, y; };
	struct { r64 r, g; };
	struct { r64 s, t; };
	r64 E[2];

	SWIZZLE_dvec2(x, y)
	SWIZZLE_dvec2(r, g)
	SWIZZLE_dvec2(s, t)

	r64& operator[](size_t e) { assert(e < 2); return E[e]; }
	r64  operator[](size_t e) const { assert(e < 2); return E[e]; }

	dvec2& operator=(const _dvec2& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *this;
	}
};

static_assert(sizeof(dvec2) == 16 && sizeof(_dvec2) == 16, "");


// unary operators, pre/post increment and decrement

dvec2 operator-(const dvec2& rhs)
{
	return dvec2{
		-rhs.x,
		-rhs.y
	};
}

dvec2& operator++(dvec2& rhs)
{
	++rhs.x;
	++rhs.y;
	return rhs;
}

dvec2 operator++(dvec2& lhs, int)
{
	dvec2 tmp = lhs;
	++lhs.x;
	++lhs.y;
	return tmp;
}

dvec2& operator--(dvec2& rhs)
{
	--rhs.x;
	--rhs.y;
	return rhs;
}

dvec2 operator--(dvec2& lhs, int)
{
	dvec2 tmp = lhs;
	--lhs.x;
	--lhs.y;
	return tmp;
}

// component-wise assignment operators

dvec2& operator+=(dvec2& lhs, const dvec2& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}

dvec2& operator-=(dvec2& lhs, const dvec2& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

dvec2& operator*=(dvec2& lhs, const dvec2& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	return lhs;
}

dvec2& operator/=(dvec2& lhs, const dvec2& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f);
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	return lhs;
}


// scalar assignment operators

dvec2& operator+=(dvec2& lhs, r64 rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	return lhs;
}

dvec2& operator-=(dvec2& lhs, r64 rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	return lhs;
}

dvec2& operator*=(dvec2& lhs, r64 rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	return lhs;
}

dvec2& operator/=(dvec2& lhs, r64 rhs)
{
	assert(rhs != 0.0f);
	lhs.x /= rhs;
	lhs.y /= rhs;
	return lhs;
}

// component-wise operators

dvec2 operator+(const dvec2& lhs, const dvec2& rhs)
{
	return dvec2{
		lhs.x + rhs.x,
		lhs.y + rhs.y
	};
}

dvec2 operator-(const dvec2& lhs, const dvec2& rhs)
{
	return dvec2{
		lhs.x - rhs.x,
		lhs.y - rhs.y
	};
}

dvec2 operator*(const dvec2& lhs, const dvec2& rhs)
{
	return dvec2{
		lhs.x * rhs.x,
		lhs.y * rhs.y
	};
}

dvec2 operator/(const dvec2& lhs, const dvec2& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f);
	return dvec2{
		lhs.x / rhs.x,
		lhs.y / rhs.y
	};
}

// scalar operators

dvec2 operator+(const dvec2& lhs, r64 rhs)
{
	return dvec2{
		lhs.x + rhs,
		lhs.y + rhs
	};
}

dvec2 operator-(const dvec2& lhs, r64 rhs)
{
	return dvec2{
		lhs.x - rhs,
		lhs.y - rhs
	};
}

dvec2 operator*(const dvec2& lhs, r64 rhs)
{
	return dvec2{
		lhs.x * rhs,
		lhs.y * rhs
	};
}

dvec2 operator/(const dvec2& lhs, r64 rhs)
{
	assert(rhs != 0.0f);
	return dvec2{
		lhs.x / rhs,
		lhs.y / rhs
	};
}

// Comparison operators

bool operator==(const dvec2& lhs, const dvec2& rhs)
{
	return ((fabs(lhs.x - rhs.x) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs.y) <= DVEC_COMPARISON_DELTA));
}

bool operator!=(const dvec2& lhs, const dvec2& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const dvec2& lhs, r64 rhs)
{
	return ((fabs(lhs.x - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= DVEC_COMPARISON_DELTA))
			|| (lhs.x < rhs && lhs.y < rhs);
}

bool operator>=(const dvec2& lhs, r64 rhs)
{
	return ((fabs(lhs.x - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= DVEC_COMPARISON_DELTA))
			|| (lhs.x > rhs && lhs.y > rhs);
}

bool operator<(const dvec2& lhs, r64 rhs)
{
	return (lhs.x < rhs && lhs.y < rhs);
}

bool operator>(const dvec2& lhs, r64 rhs)
{
	return (lhs.x > rhs && lhs.y > rhs);
}


#endif