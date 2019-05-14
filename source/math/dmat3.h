#ifndef _DMAT3_H
#define _DMAT3_H

#include "../utility/common.h"
#include "dvec3.h"


struct dmat3
{
	union {
		dvec3	col[3];
		r64		E[9];
	};

	dmat3() :
		col{{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 }}
	{}

	dmat3(r64 s) :
		col{{ s, 0, 0 },
			{ 0, s, 0 },
			{ 0, 0, s }}
	{}

	dmat3(const dmat3& m) :
		col{ m.col[0], m.col[1], m.col[2] }
	{}

	dmat3(
		r64 x0,  r64 y0,  r64 z0,
		r64 x1,  r64 y1,  r64 z1,
		r64 x2,  r64 y2,  r64 z2) :
		col{{ x0, y0, z0 },
			{ x1, y1, z1 },
			{ x2, y2, z2 }}
	{}

	dmat3(
		const dvec3& col0,
		const dvec3& col1,
		const dvec3& col2) :
		col{ col0, col1, col2 }
	{}

	dvec3& operator[](size_t c) { assert(c < 3); return col[c]; }
	const dvec3& operator[](size_t c) const { assert(c < 3); return col[c]; }
	
	dmat3& operator=(const dmat3& m)
	{
		col[0] = m[0];
		col[1] = m[1];
		col[2] = m[2];
		return *this;
	}
};
static_assert(sizeof(dmat3) == sizeof(r64)*9, "");


dmat3& operator+=(dmat3& m, r64 s)
{
	m[0] += s;
	m[1] += s;
	m[2] += s;
	return m;
}

dmat3& operator+=(dmat3& m1, const dmat3& m2)
{
	m1[0] += m2[0];
	m1[1] += m2[1];
	m1[2] += m2[2];
	return m1;
}

dmat3& operator-=(dmat3& m, r64 s)
{
	m[0] -= s;
	m[1] -= s;
	m[2] -= s;
	return m;
}

dmat3& operator-=(dmat3& m1, const dmat3& m2)
{
	m1[0] -= m2[0];
	m1[1] -= m2[1];
	m1[2] -= m2[2];
	return m1;
}

dmat3& operator*=(dmat3& m, r64 s)
{
	m[0] *= s;
	m[1] *= s;
	m[2] *= s;
	return m;
}

dmat3& operator*=(dmat3& m1, const dmat3& m2)
{
	m1[0] * m2[0][0] + m1[1] * m2[0][1] + m1[2] * m2[0][2];
	m1[0] * m2[1][0] + m1[1] * m2[1][1] + m1[2] * m2[1][2];
	m1[0] * m2[2][0] + m1[1] * m2[2][1] + m1[2] * m2[2][2];
	return m1;
}

dmat3& operator/=(dmat3& m, r64 s)
{
	m[0] /= s;
	m[1] /= s;
	m[2] /= s;
	return m;
}

dmat3 operator-(const dmat3& m)
{
	return dmat3(
		-m[0], 
		-m[1],
		-m[2]);
}

dmat3 operator*(const dmat3& m1, const dmat3& m2)
{
	return dmat3(
		m1[0] * m2[0][0] + m1[1] * m2[0][1] + m1[2] * m2[0][2],
		m1[0] * m2[1][0] + m1[1] * m2[1][1] + m1[2] * m2[1][2],
		m1[0] * m2[2][0] + m1[1] * m2[2][1] + m1[2] * m2[2][2]);
}

/**
 * returns row vector
 */
dvec3 operator*(
	const dvec3& _col,
	const dmat3& m)
{
	return dvec3{
		m[0][0] * _col[0] + m[0][1] * _col[1] + m[0][2] * _col[2],
		m[1][0] * _col[0] + m[1][1] * _col[1] + m[1][2] * _col[2],
		m[2][0] * _col[0] + m[2][1] * _col[1] + m[2][2] * _col[2]};
}

/**
 * returns col vector
 */
dvec3 operator*(
	const dmat3& m,
	const dvec3& _row)
{
	return dvec3{
		m[0][0] * _row[0] + m[1][0] * _row[1] + m[2][0] * _row[2],
		m[0][1] * _row[0] + m[1][1] * _row[1] + m[2][1] * _row[2],
		m[0][2] * _row[0] + m[1][2] * _row[1] + m[2][2] * _row[2]};
}

dmat3 operator/(const dmat3& m, r64 s)
{
	return dmat3(
		m[0] / s,
		m[1] / s,
		m[2] / s);
}

dmat3 operator/(r64 s, const dmat3& m)
{
	return dmat3(
		s / m[0],
		s / m[1],
		s / m[2]);
}

dmat3 inverse(const dmat3& m)
{
	r64 invDeterminant = 1.0 / (
		+ m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
		- m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
		+ m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]));

	dmat3 inv;
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

dmat3& operator/=(dmat3& m1, const dmat3& m2)
{
	m1 *= inverse(m2);
	return m1;
}

/**
 * returns column vector
 */
dvec3 operator/(const dmat3& m, const dvec3& _row)
{
	return inverse(m) * _row;
}

/**
 * returns row vector
 */
dvec3 operator/(const dvec3& _col, const dmat3& m)
{
	return _col * inverse(m);
}

dmat3 operator/(const dmat3& m1, const dmat3& m2)
{
	return m1 * inverse(m2);
}

dmat3 transpose(const dmat3& m)
{
	return dmat3(
		{ m[0][0], m[1][0], m[2][0] },
		{ m[0][1], m[1][1], m[2][1] },
		{ m[0][2], m[1][2], m[2][2] });
}

r64 determinant(const dmat3& m)
{
	return
		+ m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
		- m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
		+ m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
}


#endif