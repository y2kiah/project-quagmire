#ifndef _QUAT_H
#define _QUAT_H

#include "vec4.h"
#include "mat3.h"
#include "mat4.h"


struct quat
{
	union {
		struct { r32 w, x, y, z; };
		r32 E[4];
	};

	r32& operator[](size_t e) { assert(e < 4); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 4); return E[e]; }
	
	quat& operator=(const quat& rhs) {
		w = rhs.w;
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}

	quat& operator=(const vec4& rhs) {
		w = rhs.w;
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}
};

const static quat quat_default = { 1.0f, 0.0f, 0.0f, 0.0f };

static_assert(sizeof(quat) == 16, "");


quat inverse(const quat& q);
vec3 cross(const quat& q, const vec3& v);


// unary operators, pre/post increment and decrement

quat operator-(const quat& rhs)
{
	return quat{
		-rhs.w
		-rhs.x,
		-rhs.y,
		-rhs.z,
	};
}

quat& operator++(quat& rhs)
{
	++rhs.w;
	++rhs.x;
	++rhs.y;
	++rhs.z;
	return rhs;
}

quat operator++(quat& lhs, int)
{
	quat tmp = lhs;
	++lhs.w;
	++lhs.x;
	++lhs.y;
	++lhs.z;
	return tmp;
}

quat& operator--(quat& rhs)
{
	--rhs.w;
	--rhs.x;
	--rhs.y;
	--rhs.z;
	return rhs;
}

quat operator--(quat& lhs, int)
{
	quat tmp = lhs;
	--lhs.w;
	--lhs.x;
	--lhs.y;
	--lhs.z;
	return tmp;
}

// assignment operators

quat& operator+=(quat& lhs, const quat& rhs)
{
	lhs.w += rhs.w;
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

quat& operator-=(quat& lhs, const quat& rhs)
{
	lhs.w -= rhs.w;
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}

quat& operator*=(quat& lhs, const quat& rhs)
{
	quat l(lhs);
	lhs.w = l.w * rhs.w - l.x * rhs.x - l.y * rhs.y - l.z * rhs.z;
	lhs.x = l.w * rhs.x + l.x * rhs.w + l.y * rhs.z - l.z * rhs.y;
	lhs.y = l.w * rhs.y + l.y * rhs.w + l.z * rhs.x - l.x * rhs.z;
	lhs.z = l.w * rhs.z + l.z * rhs.w + l.x * rhs.y - l.y * rhs.x;
	return lhs;
}

// scalar assignment operators

quat& operator+=(quat& lhs, r32 rhs)
{
	lhs.w += rhs;
	lhs.x += rhs;
	lhs.y += rhs;
	lhs.z += rhs;
	return lhs;
}

quat& operator-=(quat& lhs, r32 rhs)
{
	lhs.w -= rhs;
	lhs.x -= rhs;
	lhs.y -= rhs;
	lhs.z -= rhs;
	return lhs;
}

quat& operator*=(quat& lhs, r32 rhs)
{
	lhs.w *= rhs;
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	return lhs;
}

quat& operator/=(quat& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	lhs.w /= rhs;
	lhs.x /= rhs;
	lhs.y /= rhs;
	lhs.z /= rhs;
	return lhs;
}

// quaternion operators

quat operator+(const quat& lhs, const quat& rhs)
{
	return quat{
		lhs.w + rhs.w,
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z
	};
}

quat operator-(const quat& lhs, const quat& rhs)
{
	return quat{
		lhs.w - rhs.w,
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z
	};
}

quat operator*(const quat& q, const quat& p)
{
	quat result(q);
	result *= p;
	return result;
}

vec3 operator*(const quat& q, const vec3& v)
{
	vec3 quatVector{ q.x, q.y, q.z };
	vec3 uv(cross(quatVector, v));
	vec3 uuv(cross(quatVector, uv));

	return v + ((uv * q.w) + uuv) * 2.0f;
}

vec3 operator*(const vec3& v, const quat& q)
{
	return inverse(q) * v;
}

vec4 operator*(const quat& q, const vec4& v)
{
	vec3 qMul = q * (vec3&)v.xyz;
	return vec4{ qMul.x, qMul.y, qMul.z , v.w };
}

vec4 operator*(const vec4& v, const quat& q)
{
	return inverse(q) * v;
}

// scalar operators

quat operator+(const quat& lhs, r32 rhs)
{
	return quat{
		lhs.w + rhs,
		lhs.x + rhs,
		lhs.y + rhs,
		lhs.z + rhs
	};
}

quat operator-(const quat& lhs, r32 rhs)
{
	return quat{
		lhs.w - rhs,
		lhs.x - rhs,
		lhs.y - rhs,
		lhs.z - rhs
	};
}

quat operator*(const quat& lhs, r32 rhs)
{
	return quat{
		lhs.w * rhs,
		lhs.x * rhs,
		lhs.y * rhs,
		lhs.z * rhs
	};
}

quat operator*(r32 lhs, const quat& rhs)
{
	return quat{
		rhs.w * lhs,
		rhs.x * lhs,
		rhs.y * lhs,
		rhs.z * lhs
	};
}

quat operator/(const quat& lhs, r32 rhs)
{
	assert(rhs != 0.0f);
	return quat{
		lhs.w / rhs,
		lhs.x / rhs,
		lhs.y / rhs,
		lhs.z / rhs
	};
}

// Comparison operators

bool operator==(const quat& lhs, const quat& rhs)
{
	return ((fabs(lhs.w - rhs.w) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.x - rhs.x) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs.y) <= VEC_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs.z) <= VEC_COMPARISON_DELTA));
}

bool operator!=(const quat& lhs, const quat& rhs)
{
	return !(lhs == rhs);
}

bool operator<=(const quat& lhs, r32 rhs)
{
	return ((fabs(lhs.w - rhs) <= QUAT_COMPARISON_DELTA)
			&& (fabs(lhs.x - rhs) <= QUAT_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= QUAT_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= QUAT_COMPARISON_DELTA))
			|| (lhs.w < rhs && lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>=(const quat& lhs, r32 rhs)
{
	return ((fabs(lhs.w - rhs) <= QUAT_COMPARISON_DELTA)
			&& (fabs(lhs.x - rhs) <= QUAT_COMPARISON_DELTA)
			&& (fabs(lhs.y - rhs) <= QUAT_COMPARISON_DELTA)
			&& (fabs(lhs.z - rhs) <= QUAT_COMPARISON_DELTA))
			|| (lhs.w > rhs && lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}

bool operator<(const quat& lhs, r32 rhs)
{
	return (lhs.w > rhs && lhs.x < rhs && lhs.y < rhs && lhs.z < rhs);
}

bool operator>(const quat& lhs, r32 rhs)
{
	return (lhs.w > rhs && lhs.x > rhs && lhs.y > rhs && lhs.z > rhs);
}

// quaternion operations

quat conjugate(const quat& q)
{
	return quat{ q.w, -q.x, -q.y, -q.z };
}

r32 dot(const quat& q1, const quat& q2)
{
	// TODO: test speed of these two alternative ways of writing dot
	// the compiler might generate faster code or be more likely to use SIMD with the first method
	alignas(16) quat mul(q1);
	mul *= q2;
	return (q2.w + q2.x + q2.y + q2.z);
	/*return (
		q1.x * q2.x +
		q1.y * q2.y +
		q1.z * q2.z +
		q1.w * q2.w);*/
}

quat inverse(const quat& q)
{
	return conjugate(q) / dot(q, q);
}

r32 length2(const quat& q)
{
	return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

r32 length(const quat& q)
{
	return sqrtf(length2(q));
}

quat normalize(const quat& q)
{
	r32 len = length(q);
	if (len <= 0.0f) {
		return quat_default;
	}
	r32 invLen = 1.0f / len;
	return quat{ q.w*invLen, q.x*invLen, q.y*invLen, q.z*invLen };
}

quat mix(const quat& q1, const quat& q2, r32 a)
{
	return q1 * (1.0f - a) + (q2 * a);
}

quat lerp(const quat& q1, const quat& q2, r32 a)
{
	return q1 * (1.0f - a) + (q2 * a);
}

quat nlerp(const quat& q1, const quat& q2, r32 a)
{
	return normalize(q1 * (1.0f - a) + (q2 * a));
}

quat slerp(const quat& q1, const quat& q2, r32 a)
{
	r32 cosTheta = dot(q1, q2);
	quat _q2(q2);

	// If cosTheta < 0, the interpolation will take the long way around the sphere. 
	// To fix this, one quat must be negated.
	if (cosTheta < 0.0f) {
		_q2 = -q2;
		cosTheta = -cosTheta;
	}

	// Perform a nlerp when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
	if (cosTheta > 1.0f - FLT_EPSILON) {
		return nlerp(q1, _q2, a);
	}
	else {
		// Essential Mathematics, page 467
		r32 angle = acosf(cosTheta);
		return (sinf((1.0f - a) * angle) * q1 + sinf(a * angle) * _q2) / sinf(angle);
	}
}

quat cross(const quat& q1, const quat& q2)
{
	return quat{
		q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
		q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
		q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z,
		q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x};
}

vec3 cross(const vec3& v, const quat& q)
{
	return inverse(q) * v;
}

vec3 cross(const quat& q, const vec3& v)
{
	return q * v;
}

quat squad(
	const quat& q1,
	const quat& q2,
	const quat& s1,
	const quat& s2,
	r32 h)
{
	return mix(mix(q1, q2, h), mix(s1, s2, h), 2.0f * (1.0f - h) * h);
}

quat exp(const quat& q)
{
	vec3 u{ q.x, q.y, q.z };
	r32 angle = length(u);
	if (angle < FLT_EPSILON) {
		return quat_default;
	}

	vec3 v(u / angle);
	v *= sinf(angle);

	return quat{ cosf(angle), v.x, v.y, v.z };
}

vec3 rotate(const quat& q, const vec3& v)
{
	return q * v;
}

vec4 rotate(const quat& q, const vec4& v)
{
	return q * v;
}

quat rotate(const quat& q, r32 angle, const vec3& v)
{
	vec3 tmp(v);

	// Axis of rotation must be normalised
	r32 len = length(tmp);
	if (fabs(len - 1.0f) > 0.001f) {
		r32 oneOverLen = 1.0f / len;
		tmp.x *= oneOverLen;
		tmp.y *= oneOverLen;
		tmp.z *= oneOverLen;
	}

	r32 angleRad(angle);
	r32 sin = sinf(angleRad * 0.5f);

	return q * quat{ cosf(angleRad * 0.5f), tmp.x * sin, tmp.y * sin, tmp.z * sin };
}

r32 roll(const quat& q)
{
	return atan2f(
		2.0f * (q.x * q.y + q.w * q.z),
		q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
}

r32 pitch(const quat& q)
{
	return atan2f(
		2.0f * (q.y * q.z + q.w * q.x),
		q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);
}

r32 yaw(const quat& q)
{
	return asinf(clamp(-2.0f * (q.x * q.z - q.w * q.y), -1.0f, 1.0f));
}

vec3 eulerAngles(const quat& x)
{
	return vec3{ pitch(x), yaw(x), roll(x) };
}

r32 angle(const quat& x)
{
	return acosf(x.w) * 2.0f;
}

vec3 axis(const quat& x)
{
	r32 tmp1 = 1.0f - x.w * x.w;
	if (tmp1 <= 0.0f) {
		return vec3{ 0.0f, 0.0f, 1.0f };
	}
	r32 tmp2 = 1.0f / sqrtf(tmp1);
	return vec3{ x.x * tmp2, x.y * tmp2, x.z * tmp2 };
}

quat angleAxis(r32 angle, const vec3& v)
{
	quat result;

	r32 a(angle);
	r32 s = sinf(a * 0.5f);

	result.w = cosf(a * 0.5f);
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	return result;
}

/*
template <typename T, precision P>
GLM_FUNC_QUALIFIER quat rotation(const vec3& orig, const vec3& dest)
{
	T cosTheta = dot(orig, dest);
	vec3 rotationAxis;

	if(cosTheta >= 1.0f - FLT_EPSILON)
		return quat();

	if(cosTheta < static_cast<T>(-1) + FLT_EPSILON)
	{
		// special case when vectors in opposite directions :
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		// This implementation favors a rotation around the Up axis (Y),
		// since it's often what you want to do.
		rotationAxis = cross(vec3(0, 0, 1), orig);
		if(length2(rotationAxis) < FLT_EPSILON) // bad luck, they were parallel, try again!
			rotationAxis = cross(vec3(1, 0, 0), orig);

		rotationAxis = normalize(rotationAxis);
		return angleAxis(pi<T>(), rotationAxis);
	}

	// Implementation from Stan Melax's Game Programming Gems 1 article
	rotationAxis = cross(orig, dest);

	T s = sqrt((1.0f + cosTheta) * static_cast<T>(2));
	T invs = 1.0f / s;

	return quat(
		s * static_cast<T>(0.5f), 
		rotationAxis.x * invs,
		rotationAxis.y * invs,
		rotationAxis.z * invs);
}

r32 extractRealComponent(const quat& q)
{
	r32 w = 1.0f - q.x * q.x - q.y * q.y - q.z * q.z;
	return (w < 0.0f ? 0.0f : -sqrt(w));
}
*/

/**
 * Create a quaternion representing the rotation between two normalized vectors
 * http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
 */
quat quatFromNormalizedVectors(
	const vec3& nu,
	const vec3& nv)
{
	vec3 w(cross(nu, nv));
	quat q{ 1.0f + dot(nu, nv), w.x, w.y, w.z };
	return normalize(q);
}

/**
 * Create a quaternion representing the rotation between two vectors that are not unit length
 * http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
 */
quat quatFromVectors(
	const vec3& u,
	const vec3& v)
{
	r32 m = sqrt(2.0f + 2.0f * dot(u, v));
	vec3 w = (1.0f / m) * cross(u, v);
	return quat{ 0.5f * m, w.x, w.y, w.z };
}

quat quat_alignAlongLH(
	const vec3& viewDir,
	const vec3& worldUp)
{
	assert(fabs(length2(viewDir) - 1.0f) <= FLT_EPSILON && "viewDir must be normalized");

	const vec3& F = viewDir;
	vec3 S(normalize(cross(F, worldUp)));	// side axis
	vec3 U(cross(S, F));					// rotation up axis

	r32 trace = S.x + U.y + F.z;
	if (trace > 0.0f)
	{
		r32 s = 0.5f / sqrtf(trace + 1.0f);
		return quat{
			0.25f / s,
			(U.z - F.y) * s,
			(F.x - S.z) * s,
			(S.y - U.x) * s};
	}
	else {
		if (S.x > U.y && S.x > F.z)
		{
			r32 s = 2.0f * sqrtf(1.0f + S.x - U.y - F.z);
			r32 invS = 1.0f / s;
			return quat{
				(U.z - F.y) * invS,
				0.25f * s,
				(U.x + S.y) * invS,
				(F.x + S.z) * invS};
		}
		else if (U.y > F.z)
		{
			r32 s = 2.0f * sqrtf(1.0f + U.y - S.x - F.z);
			r32 invS = 1.0f / s;
			return quat{
				(F.x - S.z) * invS,
				(U.x + S.y) * invS,
				0.25f * s,
				(F.y + U.z) * invS};
		}
		else {
			r32 s = 2.0f * sqrtf(1.0f + F.z - S.x - U.y);
			r32 invS = 1.0f / s;
			return quat{
				(S.y - U.x) * invS,
				(F.x + S.z) * invS,
				(F.y + U.z) * invS,
				0.25f * s};
		}
	}
}

quat quat_alignToLH(
	const vec3& eye,
	const vec3& target,
	const vec3& worldUp)
{
	vec3 F(normalize(target - eye)); // parallel view direction
	return quat_alignAlongLH(F, worldUp);
}

quat quat_alignAlongRH(
	const vec3& viewDir,
	const vec3& worldUp)
{
	assert(fabs(length2(viewDir) - 1.0f) <= FLT_EPSILON && "viewDir must be normalized");

	vec3 B(-viewDir);						// back axis (reverse viewDir)
	vec3 S(normalize(cross(worldUp, B)));	// side axis
	vec3 U(cross(B, S));					// rotation up axis

	r32 trace = S.x + U.y + B.z;
	if (trace > 0.0f)
	{
		r32 s = 0.5f / sqrtf(trace + 1.0f);
		return quat{
			0.25f / s,
			(U.z - B.y) * s,
			(B.x - S.z) * s,
			(S.y - U.x) * s};
	}
	else {
		if (S.x > U.y && S.x > B.z)
		{
			r32 s = 2.0f * sqrtf(1.0f + S.x - U.y - B.z);
			r32 invS = 1.0f / s;
			return quat{
				(U.z - B.y) * invS,
				0.25f * s,
				(U.x + S.y) * invS,
				(B.x + S.z) * invS};
		}
		else if (U.y > B.z)
		{
			r32 s = 2.0f * sqrtf(1.0f + U.y - S.x - B.z);
			r32 invS = 1.0f / s;
			return quat{
				(B.x - S.z) * invS,
				(U.x + S.y) * invS,
				0.25f * s,
				(B.y + U.z) * invS};
		}
		else {
			r32 s = 2.0f * sqrtf(1.0f + B.z - S.x - U.y);
			r32 invS = 1.0f / s;
			return quat{
				(S.y - U.x) * invS,
				(B.x + S.z) * invS,
				(B.y + U.z) * invS,
				0.25f * s};
		}
	}
}

quat quat_alignToRH(
	const vec3& eye,
	const vec3& target,
	const vec3& worldUp)
{
	vec3 F(normalize(target - eye)); // parallel view direction
	return quat_alignAlongRH(F, worldUp);
}


#endif