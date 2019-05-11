#ifndef _DVEC3_H
#define _DVEC3_H

#include "dvec2.h"


struct dvec3
{
	union {
		struct { r64 x, y, z; };
		struct { r64 r, g, b; };
		struct { r64 s, t, p; };
		r64 E[3];

		SWIZZLE_dvec3(x, y, z)
		SWIZZLE_dvec3(r, g, b)
		SWIZZLE_dvec3(s, t, p)
	};

	r64& operator[](size_t e) { assert(e < 3); return E[e]; }
	r64  operator[](size_t e) const { assert(e < 3); return E[e]; }

	dvec3& operator=(const dvec2& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *(dvec3*)this;
	}

	dvec3& operator=(const _dvec3& rhs) {
		*this = (dvec3&)rhs;
		return *this;
	}
};

static_assert(sizeof(dvec3) == 24 && sizeof(_dvec3) == 24, "");


// unary operators, pre/post increment and decrement

dvec3 operator-(const dvec3& rhs)
{
	return dvec3{
		-rhs.x,
		-rhs.y,
		-rhs.z
	};
}

dvec3& operator++(dvec3& rhs)
{
	++rhs.x;
	++rhs.y;
	++rhs.z;
	return rhs;
}

dvec3 operator++(dvec3& lhs, int)
{
	dvec3 tmp = lhs;
	++lhs.x;
	++lhs.y;
	++lhs.z;
	return tmp;
}

dvec3& operator--(dvec3& rhs)
{
	--rhs.x;
	--rhs.y;
	--rhs.z;
	return rhs;
}

dvec3 operator--(dvec3& lhs, int)
{
	dvec3 tmp = lhs;
	--lhs.x;
	--lhs.y;
	--lhs.z;
	return tmp;
}

// component-wise assignment operators

dvec3& operator+=(dvec3& lhs, const dvec3& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

dvec3& operator-=(dvec3& lhs, const dvec3& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}

dvec3& operator*=(dvec3& lhs, const dvec3& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	lhs.z *= rhs.z;
	return lhs;
}

dvec3& operator/=(dvec3& lhs, const dvec3& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	lhs.z /= rhs.z;
	return lhs;
}


// scalar assignment operators

dvec3& operator+=(dvec3& lhs, r64 rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	lhs.z += rhs;
	return lhs;
}

dvec3& operator-=(dvec3& lhs, r64 rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	lhs.z -= rhs;
	return lhs;
}

dvec3& operator*=(dvec3& lhs, r64 rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	return lhs;
}

dvec3& operator/=(dvec3& lhs, r64 rhs)
{
	assert(rhs != 0.0f);
	lhs.x /= rhs;
	lhs.y /= rhs;
	lhs.z /= rhs;
	return lhs;
}

// component-wise operators

dvec3 operator+(const dvec3& lhs, const dvec3& rhs)
{
	return dvec3{
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z
	};
}

dvec3 operator-(const dvec3& lhs, const dvec3& rhs)
{
	return dvec3{
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z
	};
}

dvec3 operator*(const dvec3& lhs, const dvec3& rhs)
{
	return dvec3{
		lhs.x * rhs.x,
		lhs.y * rhs.y,
		lhs.z * rhs.z
	};
}

dvec3 operator/(const dvec3& lhs, const dvec3& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
	return dvec3{
		lhs.x / rhs.x,
		lhs.y / rhs.y,
		lhs.z / rhs.z
	};
}

// scalar operators

dvec3 operator+(const dvec3& lhs, r64 rhs)
{
	return dvec3{
		lhs.x + rhs,
		lhs.y + rhs,
		lhs.z + rhs
	};
}

dvec3 operator-(const dvec3& lhs, r64 rhs)
{
	return dvec3{
		lhs.x - rhs,
		lhs.y - rhs,
		lhs.z - rhs
	};
}

dvec3 operator*(const dvec3& lhs, r64 rhs)
{
	return dvec3{
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs
	};
}

dvec3 operator/(const dvec3& lhs, r64 rhs)
{
	assert(rhs != 0.0f);
	return dvec3{
		lhs.x / rhs,
		lhs.y / rhs,
		lhs.z / rhs
	};
}

// Comparison operators

bool operator==(const dvec3& lhs, const dvec3& rhs)
{
	return ((fabs(lhs.x - rhs.x) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs.y) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs.z) <= DVEC_COMPARISON_DELTA));
}

bool operator!=(const dvec3& lhs, const dvec3& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const dvec3& lhs, r64 rhs)
{
	return ((fabs(lhs.x - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= DVEC_COMPARISON_DELTA))
			|| (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>=(const dvec3& lhs, r64 rhs)
{
	return ((fabs(lhs.x - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= DVEC_COMPARISON_DELTA))
			|| (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}

bool operator<(const dvec3& lhs, r64 rhs)
{
	return (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>(const dvec3& lhs, r64 rhs)
{
	return (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}


#endif