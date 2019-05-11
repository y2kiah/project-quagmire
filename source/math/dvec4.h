#ifndef _DVEC4_H
#define _DVEC4_H

#include "dvec3.h"


struct dvec4
{
	union {
		struct { r64 x, y, z, w; };
		struct { r64 r, g, b, a; };
		struct { r64 s, t, p, q; };
		r64 E[4];

		SWIZZLE_dvec4(x, y, z, w)
		SWIZZLE_dvec4(r, g, b, a)
		SWIZZLE_dvec4(s, t, p, q)
	};

	r64& operator[](size_t e) { assert(e < 4); return E[e]; }
	r64  operator[](size_t e) const { assert(e < 4); return E[e]; }
	
	dvec4& operator=(const dvec2& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *(dvec4*)this;
	}

	dvec4& operator=(const dvec3& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *(dvec4*)this;
	}
	
	dvec4& operator=(const dvec4& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *(dvec4*)this;
	}

	dvec4& operator=(const _dvec4& rhs) {
		*this = (dvec4&)rhs;
		return *this;
	}
};

static_assert(sizeof(dvec4) == 32 && sizeof(_dvec4) == 32, "");


// unary operators, pre/post increment and decrement

dvec4 operator-(const dvec4& rhs)
{
	return dvec4{
		-rhs.x,
		-rhs.y,
		-rhs.z,
		-rhs.w
	};
}

dvec4& operator++(dvec4& rhs)
{
	++rhs.x;
	++rhs.y;
	++rhs.z;
	++rhs.w;
	return rhs;
}

dvec4 operator++(dvec4& lhs, int)
{
	dvec4 tmp = lhs;
	++lhs.x;
	++lhs.y;
	++lhs.z;
	++lhs.w;
	return tmp;
}

dvec4& operator--(dvec4& rhs)
{
	--rhs.x;
	--rhs.y;
	--rhs.z;
	--rhs.w;
	return rhs;
}

dvec4 operator--(dvec4& lhs, int)
{
	dvec4 tmp = lhs;
	--lhs.x;
	--lhs.y;
	--lhs.z;
	--lhs.w;
	return tmp;
}

// component-wise assignment operators

dvec4& operator+=(dvec4& lhs, const dvec4& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	lhs.w += rhs.w;
	return lhs;
}

dvec4& operator-=(dvec4& lhs, const dvec4& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	lhs.w -= rhs.w;
	return lhs;
}

dvec4& operator*=(dvec4& lhs, const dvec4& rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	lhs.z *= rhs.z;
	lhs.w *= rhs.w;
	return lhs;
}

dvec4& operator/=(dvec4& lhs, const dvec4& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
	lhs.x /= rhs.x;
	lhs.y /= rhs.y;
	lhs.z /= rhs.z;
	lhs.w /= rhs.w;
	return lhs;
}


// scalar assignment operators

dvec4& operator+=(dvec4& lhs, r64 rhs)
{
	lhs.x += rhs;
	lhs.y += rhs;
	lhs.z += rhs;
	lhs.w += rhs;
	return lhs;
}

dvec4& operator-=(dvec4& lhs, r64 rhs)
{
	lhs.x -= rhs;
	lhs.y -= rhs;
	lhs.z -= rhs;
	lhs.w -= rhs;
	return lhs;
}

dvec4& operator*=(dvec4& lhs, r64 rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	lhs.w *= rhs;
	return lhs;
}

dvec4& operator/=(dvec4& lhs, r64 rhs)
{
	assert(rhs != 0.0f);
	lhs.x /= rhs;
	lhs.y /= rhs;
	lhs.z /= rhs;
	lhs.w /= rhs;
	return lhs;
}

// component-wise operators

dvec4 operator+(const dvec4& lhs, const dvec4& rhs)
{
	return dvec4{
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z,
		lhs.w + rhs.w
	};
}

dvec4 operator-(const dvec4& lhs, const dvec4& rhs)
{
	return dvec4{
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z,
		lhs.w - rhs.w
	};
}

dvec4 operator*(const dvec4& lhs, const dvec4& rhs)
{
	return dvec4{
		lhs.x * rhs.x,
		lhs.y * rhs.y,
		lhs.z * rhs.z,
		lhs.w * rhs.w
	};
}

dvec4 operator/(const dvec4& lhs, const dvec4& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
	return dvec4{
		lhs.x / rhs.x,
		lhs.y / rhs.y,
		lhs.z / rhs.z,
		lhs.w / rhs.w
	};
}

// scalar operators

dvec4 operator+(const dvec4& lhs, r64 rhs)
{
	return dvec4{
		lhs.x + rhs,
		lhs.y + rhs,
		lhs.z + rhs,
		lhs.w + rhs
	};
}

dvec4 operator-(const dvec4& lhs, r64 rhs)
{
	return dvec4{
		lhs.x - rhs,
		lhs.y - rhs,
		lhs.z - rhs,
		lhs.w - rhs
	};
}

dvec4 operator*(const dvec4& lhs, r64 rhs)
{
	return dvec4{
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs,
		lhs.w * rhs
	};
}

dvec4 operator/(const dvec4& lhs, r64 rhs)
{
	assert(rhs != 0.0f);
	return dvec4{
		lhs.x / rhs,
		lhs.y / rhs,
		lhs.z / rhs,
		lhs.w / rhs
	};
}

// Comparison operators

bool operator==(const dvec4& lhs, const dvec4& rhs)
{
	return ((fabs(lhs.x - rhs.x) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs.y) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs.z) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.w - rhs.w) <= DVEC_COMPARISON_DELTA));
}

bool operator!=(const dvec4& lhs, const dvec4& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const dvec4& lhs, r64 rhs)
{
	return ((fabs(lhs.x - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.w - rhs) <= DVEC_COMPARISON_DELTA))
			|| (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs && lhs.w < rhs);
}

bool operator>=(const dvec4& lhs, r64 rhs)
{
	return ((fabs(lhs.x - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= DVEC_COMPARISON_DELTA)
			&& (fabs(lhs.w - rhs) <= DVEC_COMPARISON_DELTA))
			|| (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs && lhs.w > rhs);
}

bool operator<(const dvec4& lhs, r64 rhs)
{
	return (lhs.x < rhs && lhs.y < rhs && lhs.z < rhs && lhs.w > rhs);
}

bool operator>(const dvec4& lhs, r64 rhs)
{
	return (lhs.x > rhs && lhs.y > rhs && lhs.z > rhs && lhs.w > rhs);
}


#endif