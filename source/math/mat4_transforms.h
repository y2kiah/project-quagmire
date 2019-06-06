#ifndef _MAT4_TRANSFORM_H
#define _MAT4_TRANSFORM_H

#include "mat4.h"


mat4 translate(
	const mat4& m,
	const vec3& v)
{
	mat4 result(m);
	result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
	return result;
}


mat4 rotate(
	const mat4& m,
	r32 angle,
	const vec3& v)
{
	r32 a = angle;
	r32 c = cos(a);
	r32 s = sin(a);

	vec3 axis(normalize(v));
	vec3 temp((1.0f - c) * axis);

	mat4 rot(
		{
			c + temp[0] * axis[0],
			temp[0] * axis[1] + s * axis[2],
			temp[0] * axis[2] - s * axis[1],
			0.0f
		},
		{
			temp[1] * axis[0] - s * axis[2],
			c + temp[1] * axis[1],
			temp[1] * axis[2] + s * axis[0],
			0.0f
		},
		{
			temp[2] * axis[0] + s * axis[1],
			temp[2] * axis[1] - s * axis[0],
			c + temp[2] * axis[2],
			0.0f
		},
		{});

	return mat4(
		m[0] * rot[0][0] + m[1] * rot[0][1] + m[2] * rot[0][2],
		m[0] * rot[1][0] + m[1] * rot[1][1] + m[2] * rot[1][2],
		m[0] * rot[2][0] + m[1] * rot[2][1] + m[2] * rot[2][2],
		m[3]);
}
	

mat4 rotate_slow(
	const mat4& m,
	r32 angle,
	const vec3& v)
{
	r32 a = angle;
	r32 c = cos(a);
	r32 s = sin(a);
	mat4 result;

	vec3 axis = normalize(v);

	result[0][0] = c + (1.0f - c)      * axis.x     * axis.x;
	result[0][1] = (1.0f - c) * axis.x * axis.y + s * axis.z;
	result[0][2] = (1.0f - c) * axis.x * axis.z - s * axis.y;
	result[0][3] = 0.0f;

	result[1][0] = (1.0f - c) * axis.y * axis.x - s * axis.z;
	result[1][1] = c + (1.0f - c) * axis.y * axis.y;
	result[1][2] = (1.0f - c) * axis.y * axis.z + s * axis.x;
	result[1][3] = 0.0f;

	result[2][0] = (1.0f - c) * axis.z * axis.x + s * axis.y;
	result[2][1] = (1.0f - c) * axis.z * axis.y - s * axis.x;
	result[2][2] = c + (1.0f - c) * axis.z * axis.z;
	result[2][3] = 0.0f;

	result[3] = vec4{ 0, 0, 0, 1 };
	return m * result;
}


mat4 scale(
	const mat4& m,
	const vec3& v)
{
	return mat4(
		m[0] * v[0],
		m[1] * v[1],
		m[2] * v[2],
		m[3]);
}


mat4 scale_slow(
	const mat4& m,
	const vec3& v)
{
	mat4 result;
	result[0][0] = v.x;
	result[1][1] = v.y;
	result[2][2] = v.z;
	return m * result;
}


mat4 ortho(
	r32 left, r32 right,
	r32 bottom, r32 top)
{
	mat4 result;
	result[0][0] = 2.0f / (right - left);
	result[1][1] = 2.0f / (top - bottom);
	result[2][2] = -1.0f;
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);
	return result;
}


mat4 orthoLH(
	r32 left, r32 right,
	r32 bottom, r32 top,
	r32 zNear, r32 zFar)
{
	mat4 result;
	result[0][0] = 2.0f / (right - left);
	result[1][1] = 2.0f / (top - bottom);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = 1.0f / (zFar - zNear);
	result[3][2] = -zNear / (zFar - zNear);
#	else
	result[2][2] = 2.0f / (zFar - zNear);
	result[3][2] = -(zFar + zNear) / (zFar - zNear);
#	endif

	return result;
}


mat4 orthoRH(
	r32 left, r32 right,
	r32 bottom, r32 top,
	r32 zNear, r32 zFar)
{
	mat4 result;
	result[0][0] = 2.0f / (right - left);
	result[1][1] = 2.0f / (top - bottom);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = -1.0f / (zFar - zNear);
	result[3][2] = -zNear / (zFar - zNear);
#	else
	result[2][2] = -2.0f / (zFar - zNear);
	result[3][2] = -(zFar + zNear) / (zFar - zNear);
#	endif

	return result;
}


mat4 frustumLH(
	r32 left, r32 right,
	r32 bottom, r32 top,
	r32 nearVal, r32 farVal)
{
	mat4 result(0.0f);
	result[0][0] = (2.0f * nearVal) / (right - left);
	result[1][1] = (2.0f * nearVal) / (top - bottom);
	result[2][0] = (right + left) / (right - left);
	result[2][1] = (top + bottom) / (top - bottom);
	result[2][3] = 1.0f;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = farVal / (farVal - nearVal);
	result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#	else
	result[2][2] = (farVal + nearVal) / (farVal - nearVal);
	result[3][2] = -(2.0f * farVal * nearVal) / (farVal - nearVal);
#	endif

	return result;
}


mat4 frustumRH(
	r32 left, r32 right,
	r32 bottom, r32 top,
	r32 nearVal, r32 farVal)
{
	mat4 result(0.0f);
	result[0][0] = (2.0f * nearVal) / (right - left);
	result[1][1] = (2.0f * nearVal) / (top - bottom);
	result[2][0] = (right + left) / (right - left);
	result[2][1] = (top + bottom) / (top - bottom);
	result[2][3] = -1.0f;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = farVal / (nearVal - farVal);
	result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#	else
	result[2][2] = -(farVal + nearVal) / (farVal - nearVal);
	result[3][2] = -(2.0f * farVal * nearVal) / (farVal - nearVal);
#	endif

	return result;
}


mat4 perspectiveRH(
	r32 fovy,
	r32 aspect,
	r32 zNear,
	r32 zFar)
{
	assert(abs(aspect - FLT_EPSILON) > 0.0f);

	r32 h = 1.0f / tanf(fovy * 0.5f);

	mat4 result(0.0f);
	result[0][0] = h * (1.0f / aspect);
	result[1][1] = h;
	result[2][3] = -1.0f;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zNear - zFar);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	r32 invZDist = 1.0f / (zFar - zNear);
	result[2][2] = -(zFar + zNear) * invZDist;
	result[3][2] = -(2.0f * zFar * zNear) * invZDist;
#	endif

	return result;
}


mat4 perspectiveLH(
	r32 fovy,
	r32 aspect,
	r32 zNear,
	r32 zFar)
{
	assert(abs(aspect - FLT_EPSILON) > 0.0f);

	r32 tanHalfFovy = tanf(fovy * 0.5f);
	
	mat4 result(0.0f);
	result[0][0] = 1.0f / (aspect * tanHalfFovy);
	result[1][1] = 1.0f / tanHalfFovy;
	result[2][3] = 1.0f;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zFar - zNear);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	r32 invZDist = 1.0f / (zFar - zNear);
	result[2][2] = (zFar + zNear) * invZDist;
	result[3][2] = -(2.0f * zFar * zNear) * invZDist;
#	endif

	return result;
}


mat4 perspectiveFovRH(
	r32 fovy,
	r32 width, r32 height,
	r32 zNear, r32 zFar)
{
	assert(width > 0.0f);
	assert(height > 0.0f);
	assert(fovy > 0.0f);

	r32 h = 1.0f / tanf(fovy * 0.5f);

	mat4 result(0.0f);
	result[0][0] = h * (height / width);
	result[1][1] = h;
	result[2][3] = -1.0f;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zNear - zFar);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	r32 invZDist = 1.0f / (zFar - zNear);
	result[2][2] = -(zFar + zNear) * invZDist;
	result[3][2] = -(2.0f * zFar * zNear) * invZDist;
#	endif

	return result;
}


mat4 perspectiveFovLH(
	r32 fovy,
	r32 width, r32 height,
	r32 zNear, r32 zFar)
{
	assert(width > 0.0f);
	assert(height > 0.0f);
	assert(fovy > 0.0f);

	r32 h = cosf(0.5f * fovy) / sinf(0.5f * fovy);
	r32 w = h * height / width;

	mat4 result(0.0f);
	result[0][0] = w;
	result[1][1] = h;
	result[2][3] = 1.0f;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zFar - zNear);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	r32 invZDist = 1.0f / (zFar - zNear);
	result[2][2] = (zFar + zNear) * invZDist;
	result[3][2] = -(2.0f * zFar * zNear) * invZDist;
#	endif

	return result;
}


mat4 infinitePerspectiveRH(
	r32 fovy,
	r32 aspect,
	r32 zNear)
{
	r32 range = tanf(fovy * 0.5f) * zNear;
	r32 left = -range * aspect;
	r32 right = range * aspect;
	r32 bottom = -range;
	r32 top = range;

	mat4 result(0.0f);
	result[0][0] = (2.0f * zNear) / (right - left);
	result[1][1] = (2.0f * zNear) / (top - bottom);
	result[2][2] = -1.0f;
	result[2][3] = -1.0f;
	result[3][2] = -2.0f * zNear;
	return result;
}


mat4 infinitePerspectiveLH(
	r32 fovy,
	r32 aspect,
	r32 zNear)
{
	r32 range = tanf(fovy * 0.5f) * zNear;
	r32 left = -range * aspect;
	r32 right = range * aspect;
	r32 bottom = -range;
	r32 top = range;

	mat4 result(0.0f);
	result[0][0] = (2.0f * zNear) / (right - left);
	result[1][1] = (2.0f * zNear) / (top - bottom);
	result[2][2] = 1.0f;
	result[2][3] = 1.0f;
	result[3][2] = -2.0f * zNear;
	return result;
}

// Infinite projection matrix: http://www.terathon.com/gdc07_lengyel.pdf

mat4 tweakedInfinitePerspective(
	r32 fovy,
	r32 aspect,
	r32 zNear,
	r32 ep)
{
	r32 range = tanf(fovy * 0.5f) * zNear;	
	r32 left = -range * aspect;
	r32 right = range * aspect;
	r32 bottom = -range;
	r32 top = range;

	mat4 result(0.0f);
	result[0][0] = (2.0f * zNear) / (right - left);
	result[1][1] = (2.0f * zNear) / (top - bottom);
	result[2][2] = ep - 1.0f;
	result[2][3] = -1.0f;
	result[3][2] = (ep - 2.0f) * zNear;
	return result;
}


mat4 tweakedInfinitePerspective(
	r32 fovy,
	r32 aspect,
	r32 zNear)
{
	return tweakedInfinitePerspective(fovy, aspect, zNear, FLT_EPSILON);
}


vec3 project(
	const vec3& obj,
	const mat4& model,
	const mat4& proj,
	const vec4& viewport)
{
	vec4 tmp = vec4{ obj.x, obj.y, obj.z, 1.0f };
	tmp = model * tmp;
	tmp = proj * tmp;

	tmp /= tmp.w;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	tmp.x = tmp.x * 0.5f + 0.5f;
	tmp.y = tmp.y * 0.5f + 0.5f;
#	else
	tmp = tmp * 0.5f + 0.5f;
#	endif

	tmp[0] = tmp[0] * viewport[2] + viewport[0];
	tmp[1] = tmp[1] * viewport[3] + viewport[1];

	return tmp.xyz;
}


vec3 unProject(
	const vec3& win,
	const mat4& model,
	const mat4& proj,
	const vec4& viewport)
{
	mat4 inv = inverse(proj * model);

	vec4 tmp = vec4{ win.x, win.y, win.z, 1.0f };
	tmp.x = (tmp.x - viewport[0]) / viewport[2];
	tmp.y = (tmp.y - viewport[1]) / viewport[3];

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	tmp.x = tmp.x * 2.0f - 1.0f;
	tmp.y = tmp.y * 2.0f - 1.0f;
#	else
	tmp = tmp * 2.0f - 1.0f;
#	endif

	vec4 obj = inv * tmp;
	obj /= obj.w;

	return obj.xyz;
}


mat4 pickMatrix(
	const vec2& center,
	const vec2& delta,
	const vec4& viewport)
{
	assert(delta.x > 0.0f && delta.y > 0.0f);
	mat4 result;

	if (!(delta.x > 0.0f && delta.y > 0.0f)) {
		return result; // Error
	}

	vec3 temp{
		(viewport[2] - 2.0f * (center.x - viewport[0])) / delta.x,
		(viewport[3] - 2.0f * (center.y - viewport[1])) / delta.y,
		0.0f};

	// Translate and scale the picked region to the entire window
	result = translate(result, temp);
	return scale(result, vec3{ viewport[2] / delta.x, viewport[3] / delta.y, 1.0f });
}


mat4 lookAtRH(
	const vec3& eye,
	const vec3& target,
	const vec3& up)
{
	static const vec3 xAxis = { 1.0f, 0.0f,  0.0f };
	static const vec3 yAxis = { 0.0f, 1.0f,  0.0f };
	static const vec3 zAxis = { 0.0f, 0.0f, -1.0f };

	vec3 B(normalize(eye - target));
	vec3 S(normalize(cross(up, B)));
	vec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0f && B.y == 0.0f && B.z == 0.0f)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0f && U.y == 0.0f && U.z == 0.0f)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0f) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return mat4(
		S.x, U.x, B.x, 0.0f,
		S.y, U.y, B.y, 0.0f,
		S.z, U.z, B.z, 0.0f,
		-dot(S, eye), -dot(U, eye), -dot(B, eye), 1.0f);
}


/**
 * This can be used to create the ModelView matrix for a camera.
 */
mat4 lookAlongRH(
	const vec3& eye,
	const vec3& viewDir,
	const vec3& up)
{
	static const vec3 xAxis = { 1.0f, 0.0f,  0.0f };
	static const vec3 yAxis = { 0.0f, 1.0f,  0.0f };
	static const vec3 zAxis = { 0.0f, 0.0f, -1.0f };

	assert(length2(viewDir) != 0.0f && "viewDir must be normalized");
	vec3 B(-viewDir);
	vec3 S(normalize(cross(up, B)));
	vec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0f && B.y == 0.0f && B.z == 0.0f)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0f && U.y == 0.0f && U.z == 0.0f)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0f) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return mat4(
		S.x, U.x, B.x, 0.0f,
		S.y, U.y, B.y, 0.0f,
		S.z, U.z, B.z, 0.0f,
		-dot(S, eye), -dot(U, eye), -dot(B, eye), 1.0f);
}


mat4 lookAtLH(
	const vec3& eye,
	const vec3& target,
	const vec3& up)
{
	static const vec3 xAxis = { 1.0f, 0.0f, 0.0f };
	static const vec3 yAxis = { 0.0f, 1.0f, 0.0f };
	static const vec3 zAxis = { 0.0f, 0.0f, 1.0f };

	vec3 F(normalize(target - eye));
	vec3 S(normalize(cross(up, F)));
	vec3 U(cross(F, S));

	if (F.x == 0.0f && F.y == 0.0f && F.z == 0.0f)
	{
		F = zAxis;
	}

	if (U.x == 0.0f && U.y == 0.0f && U.z == 0.0f)
	{
		U = yAxis;
	}

	if (dot(F, U) == 0.0f) {
		U = (F != xAxis) ? cross(F, xAxis) : cross(F, zAxis);
	}

	return mat4(
		S.x, U.x, F.x, 0.0f,
		S.y, U.y, F.y, 0.0f,
		S.z, U.z, F.z, 0.0f,
		-dot(S, eye), -dot(U, eye), -dot(F, eye), 1.0f);
}


/**
 * Get a transform matrix to align to a vector. This is the inverse/transpose of lookAtRH.
 * lookAt returns a viewing matrix (transforms coordinates into viewspace) whereas this function
 * returns a transform into world space (from a local model space into world space). The transform
 * will both translate and rotate the model space to face a target from the eye point.
 */
mat4 alignToRH(
	const vec3& eye,
	const vec3& target,
	const vec3& up)
{
	static const vec3 xAxis = { 1.0f, 0.0f,  0.0f };
	static const vec3 yAxis = { 0.0f, 1.0f,  0.0f };
	static const vec3 zAxis = { 0.0f, 0.0f, -1.0f };
	
	vec3 B(normalize(eye - target));
	vec3 S(normalize(cross(up, B)));
	vec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0f && B.y == 0.0f && B.z == 0.0f)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0f && U.y == 0.0f && U.z == 0.0f)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0f) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return mat4(
		S.x, S.y, S.z, -dot(S, eye),
		U.x, U.y, U.z, -dot(U, eye),
		B.x, B.y, B.z, -dot(B, eye),
		0.0f, 0.0f, 0.0f, 1.0f);
}


mat4 alignToLH(
	const vec3& eye,
	const vec3& target,
	const vec3& up)
{
	static const vec3 xAxis = { 1.0f, 0.0f, 0.0f };
	static const vec3 yAxis = { 0.0f, 1.0f, 0.0f };
	static const vec3 zAxis = { 0.0f, 0.0f, 1.0f };

	vec3 F(normalize(target - eye));
	vec3 S(normalize(cross(up, F)));
	vec3 U(cross(F, S));

	if (F.x == 0.0f && F.y == 0.0f && F.z == 0.0f)
	{
		F = zAxis;
	}

	if (U.x == 0.0f && U.y == 0.0f && U.z == 0.0f)
	{
		U = yAxis;
	}

	if (dot(F, U) == 0.0f) {
		U = (F != xAxis) ? cross(F, xAxis) : cross(F, zAxis);
	}

	return mat4(
		S.x, S.y, S.z, -dot(S, eye),
		U.x, U.y, U.z, -dot(U, eye),
		F.x, F.y, F.z, -dot(F, eye),
		0.0f, 0.0f, 0.0f, 1.0f);
}


/**
 * Get a rotation matrix to align to a vector. This is the inverse/transpose of the upper 3x3
 * portion of the lookAtRH matrix. lookAt returns a viewing matrix (transforms coordinates into
 * viewspace) whereas this function returns a rotation into world space (from a local model space
 * into world space). Unlike alignToRH, this matrix is rotation only and will not set any
 * translation to an eye point, just a direction.
 */
mat4 alignAlongRH(
	const vec3& viewDir,
	const vec3& up)
{
	static const vec3 xAxis = { 1.0f, 0.0f,  0.0f };
	static const vec3 yAxis = { 0.0f, 1.0f,  0.0f };
	static const vec3 zAxis = { 0.0f, 0.0f, -1.0f };

	assert(length2(viewDir) != 0.0f && "viewDir must be normalized");
	vec3 B(-viewDir);
	vec3 S(normalize(cross(up, B)));
	vec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0f && B.y == 0.0f && B.z == 0.0f)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0f && U.y == 0.0f && U.z == 0.0f)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0f) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return mat4(
		S.x,  S.y,  S.z,  0.0f,
		U.x,  U.y,  U.z,  0.0f,
		B.x,  B.y,  B.z,  0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}


mat4 alignAlongLH(
	const vec3& eye,
	const vec3& target,
	const vec3& up)
{
	static const vec3 xAxis = { 1.0f, 0.0f, 0.0f };
	static const vec3 yAxis = { 0.0f, 1.0f, 0.0f };
	static const vec3 zAxis = { 0.0f, 0.0f, 1.0f };
	
	vec3 F(normalize(target - eye));
	vec3 S(normalize(cross(up, F)));
	vec3 U(cross(F, S));

	if (F.x == 0.0f && F.y == 0.0f && F.z == 0.0f)
	{
		F = zAxis;
	}

	if (U.x == 0.0f && U.y == 0.0f && U.z == 0.0f)
	{
		U = yAxis;
	}

	if (dot(F, U) == 0.0f) {
		U = (F != xAxis) ? cross(F, xAxis) : cross(F, zAxis);
	}

	return mat4(
		S.x,  S.y,  S.z,  0.0f,
		U.x,  U.y,  U.z,  0.0f,
		F.x,  F.y,  F.z,  0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}


mat4 affineInverse(const mat4& m)
{
	mat3 inv(inverse(make_mat3(m)));
	
	return mat4(
		make_vec4(inv[0], 0.0f),
		make_vec4(inv[1], 0.0f),
		make_vec4(inv[2], 0.0f),
		make_vec4(-inv * make_vec3(m[3]), 1.0f));
}


#endif