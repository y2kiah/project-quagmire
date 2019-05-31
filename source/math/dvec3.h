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

	dvec3& operator=(const dvec2& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		return *(dvec3*)this;
	}

	dvec3& operator=(const _dvec3& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
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
	assert(rhs.x != 0.0 && rhs.y != 0.0 && rhs.z != 0.0);
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
	assert(rhs != 0.0);
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
	assert(rhs.x != 0.0 && rhs.y != 0.0 && rhs.z != 0.0);
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

dvec3 operator*(r64 lhs, const dvec3& rhs)
{
	return dvec3{
		lhs * rhs.x,
		lhs * rhs.y,
		lhs * rhs.z
	};
}

dvec3 operator/(const dvec3& lhs, r64 rhs)
{
	assert(rhs != 0.0);
	return dvec3{
		lhs.x / rhs,
		lhs.y / rhs,
		lhs.z / rhs
	};
}

dvec3 operator/(r64 lhs, const dvec3& rhs)
{
	assert(rhs.x != 0.0 && rhs.y != 0.0 && rhs.z != 0.0);
	return dvec3{
		lhs / rhs.x,
		lhs / rhs.y,
		lhs / rhs.z
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


// vector operations

r64 dot(const dvec3& v1, const dvec3& v2)
{
	return (
		v1.x * v2.x +
		v1.y * v2.y +
		v1.z * v2.z);
}

r64 length2(const dvec3& v)
{
	return dot(v, v);
}

r64 length(const dvec3& v)
{
	return sqrt(dot(v, v));
}

r64 distance(const dvec3& p0, const dvec3& p1)
{
	return length(p1 - p0);
}

dvec3 normalize(const dvec3& v)
{
	assert(dot(v, v) != 0.0);
	return v * (1.0 / sqrt(dot(v, v)));
}

dvec3 cross(const dvec3& v1, const dvec3& v2)
{
	return dvec3{
		v1.y * v2.z - v2.y * v1.z,
		v1.z * v2.x - v2.z * v1.x,
		v1.x * v2.y - v2.x * v1.y};
}

/**
 * Projection of a vector onto another vector
 */
dvec3 projection(
	const dvec3& x,
	const dvec3& normal)
{
	return dot(x, normal) / dot(normal, normal) * normal;
}

/**
 * Perpendicular of a vector from another vector
 */
dvec3 perpendicular(
	const dvec3& x, 
	const dvec3& normal)
{
	return x - projection(x, normal);
}

/**
 * Find the point on vector ab which is the closest to a point. The vector from point to the
 * resulting intersection is perpendicular to the line ab.
 */
dvec3 closestPointOnLine(
	const dvec3& point,
	const dvec3& a,
	const dvec3& b)
{
	r64 lineLength = distance(a, b);
	dvec3 vector = point - a;
	dvec3 lineDirection = (b - a) / lineLength;

	// Project vector to lineDirection to get the distance of point from a
	r64 distance = dot(vector, lineDirection);

	if (distance <= 0.0) {
		return a;
	}
	else if (distance >= lineLength) {
		return b;
	}
	return a + lineDirection * distance;
}

dvec3 faceForward(
	const dvec3& n,
	const dvec3& i,
	const dvec3& nRef)
{
	return (dot(nRef, i) < 0.0 ? n : -n);
}

dvec3 reflect(
	const dvec3& i,
	const dvec3& n)
{
	return i - n * dot(n, i) * 2.0;
}

dvec3 refract(
	const dvec3& i,
	const dvec3& n,
	r64 eta)
{
	r64 dotValue = dot(n, i);
	r64 k = (1.0 - eta * eta * (1.0 - dotValue * dotValue));
	return (eta * i - (eta * dotValue + sqrt(k)) * n) * (r64)(k >= 0.0);
}

dvec3 mix(const dvec3& v1, const dvec3& v2, r64 a)
{
	return v1 * (1.0 - a) + (v2 * a);
}


#endif