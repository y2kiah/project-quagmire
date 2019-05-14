#ifndef _CONVERSIONS_H
#define _CONVERSIONS_H

#include "vec4.h"
#include "dvec4.h"
#include "mat4.h"
#include "dmat4.h"
#include "quat.h"


vec3 make_vec3(const dvec3& v)
{
	return vec3{ (r32)v.x, (r32)v.y, (r32)v.z };
}

vec3 make_vec3(const vec4& v)
{
	return vec3{ v.x, v.y, v.z };
}

vec3 make_vec3(const dvec4& v)
{
	return vec3{ (r32)v.x, (r32)v.y, (r32)v.z };
}

dvec3 make_dvec3(const vec3& v)
{
	return dvec3{ v.x, v.y, v.z };
}

dvec3 make_dvec3(const dvec4& v)
{
	return dvec3{ v.x, v.y, v.z };
}

vec4 make_vec4(const dvec4& v)
{
	return vec4{ (r32)v.x, (r32)v.y, (r32)v.z, (r32)v.w };
}

vec4 make_vec4(const vec3& v, r32 w)
{
	return vec4{ v.x, v.y, v.z, w };
}

dvec4 make_dvec4(const dvec3& v, r64 w)
{
	return dvec4{ v.x, v.y, v.z, w };
}

mat3 make_mat3(const mat4& m)
{
	return mat3(
		make_vec3(m[0]),
		make_vec3(m[1]),
		make_vec3(m[2]));
}

dmat3 make_dmat3(const dmat4& m)
{
	return dmat3(
		make_dvec3(m[0]),
		make_dvec3(m[1]),
		make_dvec3(m[2]));
}

mat4 make_mat4(const dmat4& m)
{
	return mat4(
		make_vec4(m[0]),
		make_vec4(m[1]),
		make_vec4(m[2]),
		make_vec4(m[3]));
}

mat4 make_mat4(const mat3& m)
{
	return mat4(
		make_vec4(m[0], 0.0f),
		make_vec4(m[1], 0.0f),
		make_vec4(m[2], 0.0f),
		vec4{ 0.0f, 0.0f, 0.0f, 1.0f });
}

mat3 mat3_cast(const quat& q)
{
	r32 qxx(q.x * q.x);
	r32 qyy(q.y * q.y);
	r32 qzz(q.z * q.z);
	r32 qxz(q.x * q.z);
	r32 qxy(q.x * q.y);
	r32 qyz(q.y * q.z);
	r32 qwx(q.w * q.x);
	r32 qwy(q.w * q.y);
	r32 qwz(q.w * q.z);

	return mat3(
		1.0f - 2.0f * (qyy +  qzz),
		2.0f * (qxy + qwz),
		2.0f * (qxz - qwy),

		2.0f * (qxy - qwz),
		1.0f - 2.0f * (qxx +  qzz),
		2.0f * (qyz + qwx),

		2.0f * (qxz + qwy),
		2.0f * (qyz - qwx),
		1.0f - 2.0f * (qxx +  qyy));
}

mat4 mat4_cast(const quat& q)
{
	return make_mat4(mat3_cast(q));
}

quat quat_cast(const mat3& m)
{
    quat result{};
	r32 fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
	r32 fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
	r32 fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
	r32 fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];

	int biggestIndex = 0;
	r32 fourBiggestSquaredMinus1 = fourWSquaredMinus1;
	if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourXSquaredMinus1;
		biggestIndex = 1;
	}
	if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourYSquaredMinus1;
		biggestIndex = 2;
	}
	if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourZSquaredMinus1;
		biggestIndex = 3;
	}

	r32 biggestVal = sqrtf(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
	r32 mult = 0.25f / biggestVal;

	switch (biggestIndex) {
		case 0:
			result.w = biggestVal;
			result.x = (m[1][2] - m[2][1]) * mult;
			result.y = (m[2][0] - m[0][2]) * mult;
			result.z = (m[0][1] - m[1][0]) * mult;
			break;
		case 1:
			result.w = (m[1][2] - m[2][1]) * mult;
			result.x = biggestVal;
			result.y = (m[0][1] + m[1][0]) * mult;
			result.z = (m[2][0] + m[0][2]) * mult;
			break;
		case 2:
			result.w = (m[2][0] - m[0][2]) * mult;
			result.x = (m[0][1] + m[1][0]) * mult;
			result.y = biggestVal;
			result.z = (m[1][2] + m[2][1]) * mult;
			break;
		case 3:
			result.w = (m[0][1] - m[1][0]) * mult;
			result.x = (m[2][0] + m[0][2]) * mult;
			result.y = (m[1][2] + m[2][1]) * mult;
			result.z = biggestVal;
			break;
		default:
			assert(false);
			break;
	}
	return result;
}

quat quat_cast(const mat4& m4)
{
	return quat_cast(make_mat3(m4));
}

#endif