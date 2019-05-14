#ifndef _DMAT4_H
#define _DMAT4_H

#include "../utility/common.h"
#include "dvec4.h"
#include "dmat3.h"


struct dmat4
{
	union {
		dvec4	col[4];
		r64		E[16];
	};

	dmat4() :
		col{{ 1, 0, 0, 0 },
			{ 0, 1, 0, 0 },
			{ 0, 0, 1, 0 },
			{ 0, 0, 0, 1 }}
	{}

	dmat4(r64 s) :
		col{{ s, 0, 0, 0 },
			{ 0, s, 0, 0 },
			{ 0, 0, s, 0 },
			{ 0, 0, 0, s }}
	{}

	dmat4(const dmat4& m) :
		col{ m.col[0], m.col[1], m.col[2], m.col[3] }
	{}

	dmat4(
		r64 x0,  r64 y0,  r64 z0,  r64 w0,
		r64 x1,  r64 y1,  r64 z1,  r64 w1,
		r64 x2,  r64 y2,  r64 z2,  r64 w2,
		r64 x3,  r64 y3,  r64 z3,  r64 w3) :
		col{{ x0, y0, z0, w0 },
			{ x1, y1, z1, w1 },
			{ x2, y2, z2, w2 },
			{ x3, y3, z3, w3 }}
	{}

	dmat4(
		const dvec4& col0,
		const dvec4& col1,
		const dvec4& col2,
		const dvec4& col3) :
		col{ col0, col1, col2, col3 }
	{}

	dvec4& operator[](size_t c) { assert(c < 4); return col[c]; }
	const dvec4& operator[](size_t c) const { assert(c < 4); return col[c]; }
	
	dmat4& operator=(const dmat4& m)
	{
		col[0] = m[0];
		col[1] = m[1];
		col[2] = m[2];
		col[3] = m[3];
		return *this;
	}
};
static_assert(sizeof(dmat4) == sizeof(r64)*16, "");


dmat4& operator+=(dmat4& m, r64 s)
{
	m[0] += s;
	m[1] += s;
	m[2] += s;
	m[3] += s;
	return m;
}

dmat4& operator+=(dmat4& m1, const dmat4& m2)
{
	m1[0] += m2[0];
	m1[1] += m2[1];
	m1[2] += m2[2];
	m1[3] += m2[3];
	return m1;
}

dmat4& operator-=(dmat4& m, r64 s)
{
	m[0] -= s;
	m[1] -= s;
	m[2] -= s;
	m[3] -= s;
	return m;
}

dmat4& operator-=(dmat4& m1, const dmat4& m2)
{
	m1[0] -= m2[0];
	m1[1] -= m2[1];
	m1[2] -= m2[2];
	m1[3] -= m2[3];
	return m1;
}

dmat4& operator*=(dmat4& m, r64 s)
{
	m[0] *= s;
	m[1] *= s;
	m[2] *= s;
	m[3] *= s;
	return m;
}

dmat4& operator*=(dmat4& m1, const dmat4& m2)
{
	m1[0] * m2[0][0] + m1[1] * m2[0][1] + m1[2] * m2[0][2] + m1[3] * m2[0][3];
	m1[0] * m2[1][0] + m1[1] * m2[1][1] + m1[2] * m2[1][2] + m1[3] * m2[1][3];
	m1[0] * m2[2][0] + m1[1] * m2[2][1] + m1[2] * m2[2][2] + m1[3] * m2[2][3];
	m1[0] * m2[3][0] + m1[1] * m2[3][1] + m1[2] * m2[3][2] + m1[3] * m2[3][3];
	return m1;
}

dmat4& operator/=(dmat4& m, r64 s)
{
	m[0] /= s;
	m[1] /= s;
	m[2] /= s;
	m[3] /= s;
	return m;
}

dmat4 operator*(const dmat4& m1, const dmat4& m2)
{
	return dmat4(
		m1[0] * m2[0][0] + m1[1] * m2[0][1] + m1[2] * m2[0][2] + m1[3] * m2[0][3],
		m1[0] * m2[1][0] + m1[1] * m2[1][1] + m1[2] * m2[1][2] + m1[3] * m2[1][3],
		m1[0] * m2[2][0] + m1[1] * m2[2][1] + m1[2] * m2[2][2] + m1[3] * m2[2][3],
		m1[0] * m2[3][0] + m1[1] * m2[3][1] + m1[2] * m2[3][2] + m1[3] * m2[3][3]);
}

/**
 * returns row vector
 */
dvec4 operator*(
	const dvec4& _col,
	const dmat4& m)
{
	return dvec4{
		m[0][0] * _col[0] + m[0][1] * _col[1] + m[0][2] * _col[2] + m[0][3] * _col[3],
		m[1][0] * _col[0] + m[1][1] * _col[1] + m[1][2] * _col[2] + m[1][3] * _col[3],
		m[2][0] * _col[0] + m[2][1] * _col[1] + m[2][2] * _col[2] + m[2][3] * _col[3],
		m[3][0] * _col[0] + m[3][1] * _col[1] + m[3][2] * _col[2] + m[3][3] * _col[3]};
}

/**
 * returns col vector
 */
dvec4 operator*(
	const dmat4& m,
	const dvec4& _row)
{
	return dvec4{
		m[0][0] * _row[0] + m[1][0] * _row[1] + m[2][0] * _row[2] + m[3][0] * _row[3],
		m[0][1] * _row[0] + m[1][1] * _row[1] + m[2][1] * _row[2] + m[3][1] * _row[3],
		m[0][2] * _row[0] + m[1][2] * _row[1] + m[2][2] * _row[2] + m[3][2] * _row[3],
		m[0][3] * _row[0] + m[1][3] * _row[1] + m[2][3] * _row[2] + m[3][3] * _row[3]};
}

dmat4 operator/(const dmat4& m, r64 s)
{
	return dmat4(
		m[0] / s,
		m[1] / s,
		m[2] / s,
		m[3] / s);
}

dmat4 operator/(r64 s, const dmat4& m)
{
	return dmat4(
		s / m[0],
		s / m[1],
		s / m[2],
		s / m[3]);
}

dmat4 inverse(const dmat4& m)
{
	r64 coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
	r64 coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
	r64 coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

	r64 coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
	r64 coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
	r64 coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

	r64 coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
	r64 coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
	r64 coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

	r64 coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
	r64 coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
	r64 coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

	r64 coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
	r64 coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
	r64 coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

	r64 coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
	r64 coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
	r64 coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

	dvec4 fac0{ coef00, coef00, coef02, coef03 };
	dvec4 fac1{ coef04, coef04, coef06, coef07 };
	dvec4 fac2{ coef08, coef08, coef10, coef11 };
	dvec4 fac3{ coef12, coef12, coef14, coef15 };
	dvec4 fac4{ coef16, coef16, coef18, coef19 };
	dvec4 fac5{ coef20, coef20, coef22, coef23 };

	dvec4 vec0{ m[1][0], m[0][0], m[0][0], m[0][0] };
	dvec4 vec1{ m[1][1], m[0][1], m[0][1], m[0][1] };
	dvec4 vec2{ m[1][2], m[0][2], m[0][2], m[0][2] };
	dvec4 vec3{ m[1][3], m[0][3], m[0][3], m[0][3] };

	dvec4 inv0{ vec1 * fac0 - vec2 * fac1 + vec3 * fac2 };
	dvec4 inv1{ vec0 * fac0 - vec2 * fac3 + vec3 * fac4 };
	dvec4 inv2{ vec0 * fac1 - vec1 * fac3 + vec3 * fac5 };
	dvec4 inv3{ vec0 * fac2 - vec1 * fac4 + vec2 * fac5 };

	dvec4 signA{ +1.0, -1.0, +1.0, -1.0 };
	dvec4 signB{ -1.0, +1.0, -1.0, +1.0 };
	dmat4 inv(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

	dvec4 row0{ inv[0][0], inv[1][0], inv[2][0], inv[3][0] };

	dvec4 dot0(m[0] * row0);
	r64 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

	r64 oneOverDeterminant = 1.0 / dot1;

	return inv * oneOverDeterminant;
}

dmat4& operator/=(dmat4& m1, const dmat4& m2)
{
	m1 *= inverse(m2);
	return m1;
}

/**
 * returns column vector
 */
dvec4 operator/(const dmat4& m, const dvec4& _row)
{
	return inverse(m) * _row;
}

/**
 * returns row vector
 */
dvec4 operator/(const dvec4& _col, const dmat4& m)
{
	return _col * inverse(m);
}

dmat4 operator/(const dmat4& m1, const dmat4& m2)
{
	return m1 * inverse(m2);
}

dmat4 transpose(const dmat4& m)
{
	return dmat4(
		{ m[0][0], m[1][0], m[2][0], m[3][0] },
		{ m[0][1], m[1][1], m[2][1], m[3][1] },
		{ m[0][2], m[1][2], m[2][2], m[3][2] },
		{ m[0][3], m[1][3], m[2][3], m[3][3] });
}

r64 determinant(const dmat4& m)
{
	r64 subFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
	r64 subFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
	r64 subFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
	r64 subFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
	r64 subFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
	r64 subFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];

	dvec4 detCof{
		+ (m[1][1] * subFactor00 - m[1][2] * subFactor01 + m[1][3] * subFactor02),
		- (m[1][0] * subFactor00 - m[1][2] * subFactor03 + m[1][3] * subFactor04),
		+ (m[1][0] * subFactor01 - m[1][1] * subFactor03 + m[1][3] * subFactor05),
		- (m[1][0] * subFactor02 - m[1][1] * subFactor04 + m[1][2] * subFactor05)};

	return
		m[0][0] * detCof[0] + m[0][1] * detCof[1] +
		m[0][2] * detCof[2] + m[0][3] * detCof[3];
}

/*
__m128 determinant_simd(__m128 m[4])
{
	// _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(add)

	//T SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
	//T SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
	//T SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
	//T SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
	//T SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
	//T SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];

	// First 2 columns
 	__m128 Swp2A = _mm_shuffle_ps(m[2], m[2], _MM_SHUFFLE(0, 1, 1, 2));
 	__m128 Swp3A = _mm_shuffle_ps(m[3], m[3], _MM_SHUFFLE(3, 2, 3, 3));
	__m128 MulA = _mm_mul_ps(Swp2A, Swp3A);

	// Second 2 columns
	__m128 Swp2B = _mm_shuffle_ps(m[2], m[2], _MM_SHUFFLE(3, 2, 3, 3));
	__m128 Swp3B = _mm_shuffle_ps(m[3], m[3], _MM_SHUFFLE(0, 1, 1, 2));
	__m128 MulB = _mm_mul_ps(Swp2B, Swp3B);

	// Columns subtraction
	__m128 SubE = _mm_sub_ps(MulA, MulB);

	// Last 2 rows
	__m128 Swp2C = _mm_shuffle_ps(m[2], m[2], _MM_SHUFFLE(0, 0, 1, 2));
	__m128 Swp3C = _mm_shuffle_ps(m[3], m[3], _MM_SHUFFLE(1, 2, 0, 0));
	__m128 MulC = _mm_mul_ps(Swp2C, Swp3C);
	__m128 SubF = _mm_sub_ps(_mm_movehl_ps(MulC, MulC), MulC);

	//tdvec4<T, P> DetCof(
	//	+ (m[1][1] * SubFactor00 - m[1][2] * SubFactor01 + m[1][3] * SubFactor02),
	//	- (m[1][0] * SubFactor00 - m[1][2] * SubFactor03 + m[1][3] * SubFactor04),
	//	+ (m[1][0] * SubFactor01 - m[1][1] * SubFactor03 + m[1][3] * SubFactor05),
	//	- (m[1][0] * SubFactor02 - m[1][1] * SubFactor04 + m[1][2] * SubFactor05));

	__m128 SubFacA = _mm_shuffle_ps(SubE, SubE, _MM_SHUFFLE(2, 1, 0, 0));
	__m128 SwpFacA = _mm_shuffle_ps(m[1], m[1], _MM_SHUFFLE(0, 0, 0, 1));
	__m128 MulFacA = _mm_mul_ps(SwpFacA, SubFacA);

	__m128 SubTmpB = _mm_shuffle_ps(SubE, SubF, _MM_SHUFFLE(0, 0, 3, 1));
	__m128 SubFacB = _mm_shuffle_ps(SubTmpB, SubTmpB, _MM_SHUFFLE(3, 1, 1, 0));//SubF[0], SubE[3], SubE[3], SubE[1];
	__m128 SwpFacB = _mm_shuffle_ps(m[1], m[1], _MM_SHUFFLE(1, 1, 2, 2));
	__m128 MulFacB = _mm_mul_ps(SwpFacB, SubFacB);

	__m128 SubRes = _mm_sub_ps(MulFacA, MulFacB);

	__m128 SubTmpC = _mm_shuffle_ps(SubE, SubF, _MM_SHUFFLE(1, 0, 2, 2));
	__m128 SubFacC = _mm_shuffle_ps(SubTmpC, SubTmpC, _MM_SHUFFLE(3, 3, 2, 0));
	__m128 SwpFacC = _mm_shuffle_ps(m[1], m[1], _MM_SHUFFLE(2, 3, 3, 3));
	__m128 MulFacC = _mm_mul_ps(SwpFacC, SubFacC);

	__m128 AddRes = _mm_add_ps(SubRes, MulFacC);
	__m128 DetCof = _mm_mul_ps(AddRes, _mm_setr_ps( 1.0,-1.0, 1.0,-1.0));

	//return m[0][0] * DetCof[0]
	//	 + m[0][1] * DetCof[1]
	//	 + m[0][2] * DetCof[2]
	//	 + m[0][3] * DetCof[3];

	return dot_simd(m[0], DetCof);
}

r64 determinant_simd(const dmat4& m)
{
	return _mm_cvtss_f32(determinant_simd((__m128 const(*)[4])m.E));
}
*/


#endif