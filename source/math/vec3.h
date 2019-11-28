#ifndef _VEC3_H
#define _VEC3_H

#include "vec2.h"


union vec3
{
	struct { r32 x, y, z; };
	struct { r32 r, g, b; };
	struct { r32 s, t, p; };
	r32 E[3];

	SWIZZLE_vec3(x, y, z)
	SWIZZLE_vec3(r, g, b)
	SWIZZLE_vec3(s, t, p)

	r32& operator[](size_t e) { assert(e < 3); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 3); return E[e]; }

	vec3& operator=(const vec2& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *(vec3*)this;
	}

	vec3& operator=(const _vec3& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
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

vec3 operator*(r32 lhs, const vec3& rhs)
{
	return vec3{
		rhs.x * lhs,
		rhs.y * lhs,
		rhs.z * lhs
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

vec3 operator/(r32 lhs, const vec3& rhs)
{
	assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
	return vec3{
		lhs / rhs.x,
		lhs / rhs.y,
		lhs / rhs.z
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

// vector operations

r32 dot(const vec3& v1, const vec3& v2)
{
	return (
		v1.x * v2.x +
		v1.y * v2.y +
		v1.z * v2.z);
}

r32 length2(const vec3& v)
{
	return dot(v, v);
}

r32 length(const vec3& v)
{
	return sqrtf(dot(v, v));
}

r32 distance(const vec3& p0, const vec3& p1)
{
	return length(p1 - p0);
}

vec3 normalize(const vec3& v)
{
	assert(dot(v, v) != 0.0f);
	return v * (1.0f / sqrtf(dot(v, v)));
}

vec3 cross(const vec3& v1, const vec3& v2)
{
	return vec3{
		v1.y * v2.z - v2.y * v1.z,
		v1.z * v2.x - v2.z * v1.x,
		v1.x * v2.y - v2.x * v1.y};
}

/**
 * Projection of a vector onto another vector
 */
vec3 projection(
	const vec3& x,
	const vec3& normal)
{
	return dot(x, normal) / dot(normal, normal) * normal;
}

/**
 * Perpendicular of a vector from another vector
 */
vec3 perpendicular(
	const vec3& x, 
	const vec3& normal)
{
	return x - projection(x, normal);
}

/**
 * Find the point on vector ab which is the closest to a point. The vector from point to the
 * resulting intersection is perpendicular to the line ab.
 */
vec3 closestPointOnLine(
	const vec3& point,
	const vec3& a,
	const vec3& b)
{
	r32 lineLength = distance(a, b);
	vec3 vector = point - a;
	vec3 lineDirection = (b - a) / lineLength;

	// Project vector to lineDirection to get the distance of point from a
	r32 distance = dot(vector, lineDirection);

	if (distance <= 0.0f) {
		return a;
	}
	else if (distance >= lineLength) {
		return b;
	}
	return a + lineDirection * distance;
}

vec3 faceForward(
	const vec3& n,
	const vec3& i,
	const vec3& nRef)
{
	return (dot(nRef, i) < 0.0f ? n : -n);
}

vec3 reflect(
	const vec3& i,
	const vec3& n)
{
	return i - n * dot(n, i) * 2.0f;
}

vec3 refract(
	const vec3& i,
	const vec3& n,
	r32 eta)
{
	r32 dotValue = dot(n, i);
	r32 k = (1.0f - eta * eta * (1.0f - dotValue * dotValue));
	return (eta * i - (eta * dotValue + sqrtf(k)) * n) * (r32)(k >= 0.0f);
}

vec3 mix(const vec3& v1, const vec3& v2, r32 a)
{
	return v1 * (1.0f - a) + (v2 * a);
}


#endif