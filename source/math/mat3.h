#ifndef _MAT3_H
#define _MAT3_H

#include "../utility/common.h"
#include "vec3.h"


struct mat3
{
	union {
		vec3	col[3];
		r32		E[9];
	};

	mat3() :
		col{{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }}
	{}

	mat3(r32 s) :
		col{{ s, 0, 0 },
			{ 0, s, 0 },
			{ 0, 0, s }}
	{}

	mat3(const mat3& m) :
		col{ m.col[0], m.col[1], m.col[2] }
	{}

	mat3(
		r32 x0,  r32 y0,  r32 z0,
		r32 x1,  r32 y1,  r32 z1,
		r32 x2,  r32 y2,  r32 z2) :
		col{{ x0, y0, z0 },
			{ x1, y1, z1 },
			{ x2, y2, z2 }}
	{}

	mat3(
		const vec3& col0,
		const vec3& col1,
		const vec3& col2) :
		col{ col0, col1, col2 }
	{}

	vec3& operator[](size_t c) { assert(c < 3); return col[c]; }
	const vec3& operator[](size_t c) const { assert(c < 3); return col[c]; }
	
	mat3& operator=(const mat3& m)
	{
		col[0] = m[0];
		col[1] = m[1];
		col[2] = m[2];
		return *this;
	}
};
static_assert(sizeof(mat3) == sizeof(r32)*9, "");


mat3& operator+=(mat3& m, r32 s)
{
	m[0] += s;
	m[1] += s;
	m[2] += s;
	return m;
}

mat3& operator+=(mat3& m1, const mat3& m2)
{
	m1[0] += m2[0];
	m1[1] += m2[1];
	m1[2] += m2[2];
	return m1;
}

mat3& operator-=(mat3& m, r32 s)
{
	m[0] -= s;
	m[1] -= s;
	m[2] -= s;
	return m;
}

mat3& operator-=(mat3& m1, const mat3& m2)
{
	m1[0] -= m2[0];
	m1[1] -= m2[1];
	m1[2] -= m2[2];
	return m1;
}

mat3& operator*=(mat3& m, r32 s)
{
	m[0] *= s;
	m[1] *= s;
	m[2] *= s;
	return m;
}

mat3& operator*=(mat3& m1, const mat3& m2)
{
	m1[0] * m2[0][0] + m1[1] * m2[0][1] + m1[2] * m2[0][2];
	m1[0] * m2[1][0] + m1[1] * m2[1][1] + m1[2] * m2[1][2];
	m1[0] * m2[2][0] + m1[1] * m2[2][1] + m1[2] * m2[2][2];
	return m1;
}

mat3& operator/=(mat3& m, r32 s)
{
	m[0] /= s;
	m[1] /= s;
	m[2] /= s;
	return m;
}

mat3 operator*(const mat3& m1, const mat3& m2)
{
	return mat3(
		m1[0] * m2[0][0] + m1[1] * m2[0][1] + m1[2] * m2[0][2],
		m1[0] * m2[1][0] + m1[1] * m2[1][1] + m1[2] * m2[1][2],
		m1[0] * m2[2][0] + m1[1] * m2[2][1] + m1[2] * m2[2][2]);
}

/**
 * returns row vector
 */
vec3 operator*(
	const vec3& _col,
	const mat3& m)
{
	return vec3{
		m[0][0] * _col[0] + m[0][1] * _col[1] + m[0][2] * _col[2],
		m[1][0] * _col[0] + m[1][1] * _col[1] + m[1][2] * _col[2],
		m[2][0] * _col[0] + m[2][1] * _col[1] + m[2][2] * _col[2]};
}

/**
 * returns col vector
 */
vec3 operator*(
	const mat3& m,
	const vec3& _row)
{
	return vec3{
		m[0][0] * _row[0] + m[1][0] * _row[1] + m[2][0] * _row[2],
		m[0][1] * _row[0] + m[1][1] * _row[1] + m[2][1] * _row[2],
		m[0][2] * _row[0] + m[1][2] * _row[1] + m[2][2] * _row[2]};
}

mat3 operator/(const mat3& m, r32 s)
{
	return mat3(
		m[0] / s,
		m[1] / s,
		m[2] / s);
}

mat3 operator/(r32 s, const mat3& m)
{
	return mat3(
		s / m[0],
		s / m[1],
		s / m[2]);
}

mat3 inverse(const mat3& m)
{
	r32 invDeterminant = 1.0f / (
		+ m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
		- m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
		+ m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]));

	mat3 inv;
	inv[0][0] = + (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invDeterminant;
	inv[1][0] = - (m[1][0] * m[2][2] - m[2][0] * m[1][2]) * invDeterminant;
	inv[2][0] = + (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invDeterminant;
	inv[0][1] = - (m[0][1] * m[2][2] - m[2][1] * m[0][2]) * invDeterminant;
	inv[1][1] = + (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * invDeterminant;
	inv[2][1] = - (m[0][0] * m[2][1] - m[2][0] * m[0][1]) * invDeterminant;
	inv[0][2] = + (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * invDeterminant;
	inv[1][2] = - (m[0][0] * m[1][2] - m[1][0] * m[0][2]) * invDeterminant;
	inv[2][2] = + (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invDeterminant;

	return inv;
}

mat3& operator/=(mat3& m1, const mat3& m2)
{
	m1 *= inverse(m2);
	return m1;
}

/**
 * returns column vector
 */
vec3 operator/(const mat3& m, const vec3& _row)
{
	return inverse(m) * _row;
}

/**
 * returns row vector
 */
vec3 operator/(const vec3& _col, const mat3& m)
{
	return _col * inverse(m);
}

mat3 operator/(const mat3& m1, const mat3& m2)
{
	return m1 * inverse(m2);
}

mat3 transpose(const mat3& m)
{
	return mat3(
		{ m[0][0], m[1][0], m[2][0] },
		{ m[0][1], m[1][1], m[2][1] },
		{ m[0][2], m[1][2], m[2][2] });
}

r32 determinant(const mat3& m)
{
	return
		+ m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
		- m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
		+ m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
}


#endif