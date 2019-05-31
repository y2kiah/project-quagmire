#ifndef _DQUAT_H
#define _DQUAT_H

#include "dvec4.h"
#include "dmat3.h"
#include "dmat4.h"


struct dquat
{
	union {
		struct { r64 w, x, y, z; };
		r64 E[4];
	};

	r64& operator[](size_t e) { assert(e < 4); return E[e]; }
	r64  operator[](size_t e) const { assert(e < 4); return E[e]; }
	
	dquat& operator=(const dquat& rhs) {
		w = rhs.w;
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}

	dquat& operator=(const dvec4& rhs) {
		w = rhs.w;
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}
};

const static dquat dquat_default = { 1.0, 0.0, 0.0, 0.0 };

static_assert(sizeof(dquat) == 32, "");


dquat inverse(const dquat& q);
dvec3 cross(const dquat& q, const dvec3& v);


// unary operators, pre/post increment and decrement

dquat operator-(const dquat& rhs)
{
	return dquat{
		-rhs.w
		-rhs.x,
		-rhs.y,
		-rhs.z,
	};
}

dquat& operator++(dquat& rhs)
{
	++rhs.w;
	++rhs.x;
	++rhs.y;
	++rhs.z;
	return rhs;
}

dquat operator++(dquat& lhs, int)
{
	dquat tmp = lhs;
	++lhs.w;
	++lhs.x;
	++lhs.y;
	++lhs.z;
	return tmp;
}

dquat& operator--(dquat& rhs)
{
	--rhs.w;
	--rhs.x;
	--rhs.y;
	--rhs.z;
	return rhs;
}

dquat operator--(dquat& lhs, int)
{
	dquat tmp = lhs;
	--lhs.w;
	--lhs.x;
	--lhs.y;
	--lhs.z;
	return tmp;
}

// assignment operators

dquat& operator+=(dquat& lhs, const dquat& rhs)
{
	lhs.w += rhs.w;
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

dquat& operator-=(dquat& lhs, const dquat& rhs)
{
	lhs.w -= rhs.w;
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}

dquat& operator*=(dquat& lhs, const dquat& rhs)
{
	dquat l(lhs);
	lhs.w = l.w * rhs.w - l.x * rhs.x - l.y * rhs.y - l.z * rhs.z;
	lhs.x = l.w * rhs.x + l.x * rhs.w + l.y * rhs.z - l.z * rhs.y;
	lhs.y = l.w * rhs.y + l.y * rhs.w + l.z * rhs.x - l.x * rhs.z;
	lhs.z = l.w * rhs.z + l.z * rhs.w + l.x * rhs.y - l.y * rhs.x;
	return lhs;
}

// scalar assignment operators

dquat& operator+=(dquat& lhs, r64 rhs)
{
	lhs.w += rhs;
	lhs.x += rhs;
	lhs.y += rhs;
	lhs.z += rhs;
	return lhs;
}

dquat& operator-=(dquat& lhs, r64 rhs)
{
	lhs.w -= rhs;
	lhs.x -= rhs;
	lhs.y -= rhs;
	lhs.z -= rhs;
	return lhs;
}

dquat& operator*=(dquat& lhs, r64 rhs)
{
	lhs.w *= rhs;
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	return lhs;
}

dquat& operator/=(dquat& lhs, r64 rhs)
{
	assert(rhs != 0.0);
	lhs.w /= rhs;
	lhs.x /= rhs;
	lhs.y /= rhs;
	lhs.z /= rhs;
	return lhs;
}

// dquaternion operators

dquat operator+(const dquat& lhs, const dquat& rhs)
{
	return dquat{
		lhs.w + rhs.w,
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z
	};
}

dquat operator-(const dquat& lhs, const dquat& rhs)
{
	return dquat{
		lhs.w - rhs.w,
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z
	};
}

dquat operator*(const dquat& q, const dquat& p)
{
	dquat result(q);
	result *= p;
	return result;
}

dvec3 operator*(const dquat& q, const dvec3& v)
{
	dvec3 dquatVector{ q.x, q.y, q.z };
	dvec3 uv(cross(dquatVector, v));
	dvec3 uuv(cross(dquatVector, uv));

	return v + ((uv * q.w) + uuv) * 2.0;
}

dvec3 operator*(const dvec3& v, const dquat& q)
{
	return inverse(q) * v;
}

dvec4 operator*(const dquat& q, const dvec4& v)
{
	dvec3 qMul = q * (dvec3&)v.xyz;
	return dvec4{ qMul.x, qMul.y, qMul.z , v.w };
}

dvec4 operator*(const dvec4& v, const dquat& q)
{
	return inverse(q) * v;
}

// scalar operators

dquat operator+(const dquat& lhs, r64 rhs)
{
	return dquat{
		lhs.w + rhs,
		lhs.x + rhs,
		lhs.y + rhs,
		lhs.z + rhs
	};
}

dquat operator-(const dquat& lhs, r64 rhs)
{
	return dquat{
		lhs.w - rhs,
		lhs.x - rhs,
		lhs.y - rhs,
		lhs.z - rhs
	};
}

dquat operator*(const dquat& lhs, r64 rhs)
{
	return dquat{
		lhs.w * rhs,
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs
	};
}

dquat operator*(r64 lhs, const dquat& rhs)
{
	return dquat{
		rhs.w * lhs,
		rhs.x * lhs,
		rhs.y * lhs,
		rhs.z * lhs
	};
}

dquat operator/(const dquat& lhs, r64 rhs)
{
	assert(rhs != 0.0);
	return dquat{
		lhs.w / rhs,
		lhs.x / rhs,
		lhs.y / rhs,
		lhs.z / rhs
	};
}

// Comparison operators

bool operator==(const dquat& lhs, const dquat& rhs)
{
	return ((abs(lhs.w - rhs.w) <= VEC_COMPARISON_DELTA)
			&& (abs(lhs.x - rhs.x) <= VEC_COMPARISON_DELTA)
			&& (abs(lhs.y - rhs.y) <= VEC_COMPARISON_DELTA)
			&& (abs(lhs.z - rhs.z) <= VEC_COMPARISON_DELTA));
}

bool operator!=(const dquat& lhs, const dquat& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const dquat& lhs, r64 rhs)
{
	return ((abs(lhs.w - rhs) <= QUAT_COMPARISON_DELTA)
			&& (abs(lhs.x - rhs) <= QUAT_COMPARISON_DELTA)
			&& (abs(lhs.y - rhs) <= QUAT_COMPARISON_DELTA)
			&& (abs(lhs.z - rhs) <= QUAT_COMPARISON_DELTA))
			|| (lhs.w < rhs && lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>=(const dquat& lhs, r64 rhs)
{
	return ((abs(lhs.w - rhs) <= QUAT_COMPARISON_DELTA)
			&& (abs(lhs.x - rhs) <= QUAT_COMPARISON_DELTA)
			&& (abs(lhs.y - rhs) <= QUAT_COMPARISON_DELTA)
			&& (abs(lhs.z - rhs) <= QUAT_COMPARISON_DELTA))
			|| (lhs.w > rhs && lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}

bool operator<(const dquat& lhs, r64 rhs)
{
	return (lhs.w > rhs && lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>(const dquat& lhs, r64 rhs)
{
	return (lhs.w > rhs && lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}

// dquaternion operations

dquat conjugate(const dquat& q)
{
	return dquat{ q.w, -q.x, -q.y, -q.z };
}

r64 dot(const dquat& q1, const dquat& q2)
{
	// TODO: test speed of these two alternative ways of writing dot
	// the compiler might generate faster code or be more likely to use SIMD with the first method
	alignas(16) dquat mul(q1);
	mul *= q2;
	return (q2.w + q2.x + q2.y + q2.z);
	/*return (
		q1.x * q2.x +
		q1.y * q2.y +
		q1.z * q2.z +
		q1.w * q2.w);*/
}

dquat inverse(const dquat& q)
{
	return conjugate(q) / dot(q, q);
}

r64 length2(const dquat& q)
{
	return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

r64 length(const dquat& q)
{
	return sqrt(length2(q));
}

dquat normalize(const dquat& q)
{
	r64 len = length(q);
	if (len <= 0.0) {
		return dquat_default;
	}
	r64 invLen = 1.0 / len;
	return dquat{ q.w*invLen, q.x*invLen, q.y*invLen, q.z*invLen };
}

dquat mix(const dquat& q1, const dquat& q2, r64 a)
{
	return q1 * (1.0 - a) + (q2 * a);
}

dquat lerp(const dquat& q1, const dquat& q2, r64 a)
{
	return q1 * (1.0 - a) + (q2 * a);
}

dquat nlerp(const dquat& q1, const dquat& q2, r64 a)
{
	return normalize(q1 * (1.0 - a) + (q2 * a));
}

dquat slerp(const dquat& q1, const dquat& q2, r64 a)
{
	r64 cosTheta = dot(q1, q2);
	dquat _q2(q2);

	// If cosTheta < 0, the interpolation will take the long way around the sphere. 
	// To fix this, one dquat must be negated.
	if (cosTheta < 0.0) {
		_q2 = -q2;
		cosTheta = -cosTheta;
	}

	// Perform a nlerp when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
	if (cosTheta > 1.0 - DBL_EPSILON) {
		return nlerp(q1, _q2, a);
	}
	else {
		// Essential Mathematics, page 467
		r64 angle = acos(cosTheta);
		return (sin((1.0 - a) * angle) * q1 + sin(a * angle) * _q2) / sin(angle);
	}
}

dquat cross(const dquat& q1, const dquat& q2)
{
	return dquat{
		q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
		q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
		q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z,
		q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x};
}

dvec3 cross(const dvec3& v, const dquat& q)
{
	return inverse(q) * v;
}

dvec3 cross(const dquat& q, const dvec3& v)
{
	return q * v;
}

dquat squad(
	const dquat& q1,
	const dquat& q2,
	const dquat& s1,
	const dquat& s2,
	r64 h)
{
	return mix(mix(q1, q2, h), mix(s1, s2, h), 2.0 * (1.0 - h) * h);
}

dquat exp(const dquat& q)
{
	dvec3 u{ q.x, q.y, q.z };
	r64 angle = length(u);
	if (angle < DBL_EPSILON) {
		return dquat_default;
	}

	dvec3 v(u / angle);
	v *= sin(angle);

	return dquat{ cos(angle), v.x, v.y, v.z };
}

dvec3 rotate(const dquat& q, const dvec3& v)
{
	return q * v;
}

dvec4 rotate(const dquat& q, const dvec4& v)
{
	return q * v;
}

dquat rotate(const dquat& q, r64 angle, const dvec3& v)
{
	dvec3 tmp(v);

	// Axis of rotation must be normalised
	r64 len = length(tmp);
	if (abs(len - 1.0) > 0.001) {
		r64 oneOverLen = 1.0 / len;
		tmp.x *= oneOverLen;
		tmp.y *= oneOverLen;
		tmp.z *= oneOverLen;
	}

	r64 angleRad(angle);
	r64 dsin = sin(angleRad * 0.5);

	return q * dquat{ cos(angleRad * 0.5), tmp.x * dsin, tmp.y * dsin, tmp.z * dsin };
}

r64 roll(const dquat& q)
{
	return atan2(
		2.0 * (q.x * q.y + q.w * q.z),
		q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
}

r64 pitch(const dquat& q)
{
	return atan2(
		2.0 * (q.y * q.z + q.w * q.x),
		q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);
}

r64 yaw(const dquat& q)
{
	return asin(clamp(-2.0 * (q.x * q.z - q.w * q.y), -1.0, 1.0));
}

dvec3 eulerAngles(const dquat& x)
{
	return dvec3{ pitch(x), yaw(x), roll(x) };
}

r64 angle(const dquat& x)
{
	return acos(x.w) * 2.0;
}

dvec3 axis(const dquat& x)
{
	r64 tmp1 = 1.0 - x.w * x.w;
	if (tmp1 <= 0.0) {
		return dvec3{ 0.0, 0.0, 1.0 };
	}
	r64 tmp2 = 1.0 / sqrt(tmp1);
	return dvec3{ x.x * tmp2, x.y * tmp2, x.z * tmp2 };
}

dquat angleAxis(r64 angle, const dvec3& v)
{
	dquat result;

	r64 a(angle);
	r64 s = sin(a * 0.5);

	result.w = cos(a * 0.5);
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	return result;
}

/*
template <typename T, precision P>
GLM_FUNC_QUALIFIER dquat rotation(const dvec3& orig, const dvec3& dest)
{
	T cosTheta = dot(orig, dest);
	dvec3 rotationAxis;

	if(cosTheta >= 1.0 - DBL_EPSILON)
		return dquat();

	if(cosTheta < static_cast<T>(-1) + DBL_EPSILON)
	{
		// special case when vectors in opposite directions :
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		// This implementation favors a rotation around the Up axis (Y),
		// since it's often what you want to do.
		rotationAxis = cross(dvec3(0, 0, 1), orig);
		if(length2(rotationAxis) < DBL_EPSILON) // bad luck, they were parallel, try again!
			rotationAxis = cross(dvec3(1, 0, 0), orig);

		rotationAxis = normalize(rotationAxis);
		return angleAxis(pi<T>(), rotationAxis);
	}

	// Implementation from Stan Melax's Game Programming Gems 1 article
	rotationAxis = cross(orig, dest);

	T s = sqrt((1.0 + cosTheta) * static_cast<T>(2));
	T invs = 1.0 / s;

	return dquat(
		s * static_cast<T>(0.5), 
		rotationAxis.x * invs,
		rotationAxis.y * invs,
		rotationAxis.z * invs);
}

r64 extractRealComponent(const dquat& q)
{
	r64 w = 1.0 - q.x * q.x - q.y * q.y - q.z * q.z;
	return (w < 0.0 ? 0.0 : -sqrt(w));
}
*/

/**
 * Create a dquaternion representing the rotation between two normalized vectors
 * http://lolengine.net/blog/2013/09/18/beautiful-maths-dquaternion-from-vectors
 */
dquat dquatFromNormalizedVectors(
	const dvec3& nu,
	const dvec3& nv)
{
	dvec3 w(cross(nu, nv));
	dquat q{ 1.0 + dot(nu, nv), w.x, w.y, w.z };
	return normalize(q);
}

/**
 * Create a dquaternion representing the rotation between two vectors that are not unit length
 * http://lolengine.net/blog/2013/09/18/beautiful-maths-dquaternion-from-vectors
 */
dquat dquatFromVectors(
	const dvec3& u,
	const dvec3& v)
{
	r64 m = sqrt(2.0 + 2.0 * dot(u, v));
	dvec3 w = (1.0 / m) * cross(u, v);
	return dquat{ 0.5 * m, w.x, w.y, w.z };
}

dquat dquat_alignAlongLH(
	const dvec3& viewDir,
	const dvec3& worldUp)
{
	assert(abs(length2(viewDir) - 1.0) <= DBL_EPSILON && "viewDir must be normalized");

	const dvec3& F = viewDir;
	dvec3 S(normalize(cross(F, worldUp)));	// side axis
	dvec3 U(cross(S, F));					// rotation up axis

	r64 trace = S.x + U.y + F.z;
	if (trace > 0.0)
	{
		r64 s = 0.5 / sqrt(trace + 1.0);
		return dquat{
			0.25 / s,
			(U.z - F.y) * s,
			(F.x - S.z) * s,
			(S.y - U.x) * s};
	}
	else {
		if (S.x > U.y && S.x > F.z)
		{
			r64 s = 2.0 * sqrt(1.0 + S.x - U.y - F.z);
			r64 invS = 1.0 / s;
			return dquat{
				(U.z - F.y) * invS,
				0.25 * s,
				(U.x + S.y) * invS,
				(F.x + S.z) * invS};
		}
		else if (U.y > F.z)
		{
			r64 s = 2.0 * sqrt(1.0 + U.y - S.x - F.z);
			r64 invS = 1.0 / s;
			return dquat{
				(F.x - S.z) * invS,
				(U.x + S.y) * invS,
				0.25 * s,
				(F.y + U.z) * invS};
		}
		else {
			r64 s = 2.0 * sqrt(1.0 + F.z - S.x - U.y);
			r64 invS = 1.0 / s;
			return dquat{
				(S.y - U.x) * invS,
				(F.x + S.z) * invS,
				(F.y + U.z) * invS,
				0.25 * s};
		}
	}
}

dquat dquat_alignToLH(
	const dvec3& eye,
	const dvec3& target,
	const dvec3& worldUp)
{
	dvec3 F(normalize(target - eye)); // parallel view direction
	return dquat_alignAlongLH(F, worldUp);
}

dquat dquat_alignAlongRH(
	const dvec3& viewDir,
	const dvec3& worldUp)
{
	assert(abs(length2(viewDir) - 1.0) <= DBL_EPSILON && "viewDir must be normalized");

	dvec3 B(-viewDir);						// back axis (reverse viewDir)
	dvec3 S(normalize(cross(worldUp, B)));	// side axis
	dvec3 U(cross(B, S));					// rotation up axis

	r64 trace = S.x + U.y + B.z;
	if (trace > 0.0)
	{
		r64 s = 0.5 / sqrt(trace + 1.0);
		return dquat{
			0.25 / s,
			(U.z - B.y) * s,
			(B.x - S.z) * s,
			(S.y - U.x) * s};
	}
	else {
		if (S.x > U.y && S.x > B.z)
		{
			r64 s = 2.0 * sqrt(1.0 + S.x - U.y - B.z);
			r64 invS = 1.0 / s;
			return dquat{
				(U.z - B.y) * invS,
				0.25 * s,
				(U.x + S.y) * invS,
				(B.x + S.z) * invS};
		}
		else if (U.y > B.z)
		{
			r64 s = 2.0 * sqrt(1.0 + U.y - S.x - B.z);
			r64 invS = 1.0 / s;
			return dquat{
				(B.x - S.z) * invS,
				(U.x + S.y) * invS,
				0.25 * s,
				(B.y + U.z) * invS};
		}
		else {
			r64 s = 2.0 * sqrt(1.0 + B.z - S.x - U.y);
			r64 invS = 1.0 / s;
			return dquat{
				(S.y - U.x) * invS,
				(B.x + S.z) * invS,
				(B.y + U.z) * invS,
				0.25 * s};
		}
	}
}

dquat dquat_alignToRH(
	const dvec3& eye,
	const dvec3& target,
	const dvec3& worldUp)
{
	dvec3 F(normalize(target - eye)); // parallel view direction
	return dquat_alignAlongRH(F, worldUp);
}


#endif