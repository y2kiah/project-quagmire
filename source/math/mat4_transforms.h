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
	result[2][2] = - 1.0f;
	result[3][0] = - (right + left) / (right - left);
	result[3][1] = - (top + bottom) / (top - bottom);
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
	result[3][0] = - (right + left) / (right - left);
	result[3][1] = - (top + bottom) / (top - bottom);

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = 1.0f / (zFar - zNear);
	result[3][2] = - zNear / (zFar - zNear);
#	else
	result[2][2] = 2.0f / (zFar - zNear);
	result[3][2] = - (zFar + zNear) / (zFar - zNear);
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
	result[3][0] = - (right + left) / (right - left);
	result[3][1] = - (top + bottom) / (top - bottom);

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = - 1.0f / (zFar - zNear);
	result[3][2] = - zNear / (zFar - zNear);
#	else
	result[2][2] = - 2.0f / (zFar - zNear);
	result[3][2] = - (zFar + zNear) / (zFar - zNear);
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

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = farVal / (farVal - nearVal);
	result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#	else
	result[2][2] = (farVal + nearVal) / (farVal - nearVal);
	result[3][2] = - (2.0f * farVal * nearVal) / (farVal - nearVal);
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

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = farVal / (nearVal - farVal);
	result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#	else
	result[2][2] = - (farVal + nearVal) / (farVal - nearVal);
	result[3][2] = - (2.0f * farVal * nearVal) / (farVal - nearVal);
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

	r32 tanHalfFovy = tan(fovy / 2.0f);

	mat4 result(0.0f);
	result[0][0] = 1.0f / (aspect * tanHalfFovy);
	result[1][1] = 1.0f / (tanHalfFovy);
	result[2][3] = - 1.0f;

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zNear - zFar);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = - (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0f * zFar * zNear) / (zFar - zNear);
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

	r32 tanHalfFovy = tan(fovy / 2.0f);
	
	mat4 result(0.0f);
	result[0][0] = 1.0f / (aspect * tanHalfFovy);
	result[1][1] = 1.0f / (tanHalfFovy);
	result[2][3] = 1.0f;

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zFar - zNear);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0f * zFar * zNear) / (zFar - zNear);
#	endif

	return result;
}


mat4 perspectiveFovRH(
	r32 fov,
	r32 width, r32 height,
	r32 zNear, r32 zFar)
{
	assert(width > 0.0f);
	assert(height > 0.0f);
	assert(fov > 0.0f);

	r32 rad = fov;
	r32 h = cosf(0.5f * rad) / sinf(0.5f * rad);
	r32 w = h * height / width; // TODO: max(width , Height) / min(width , Height)?

	mat4 result(0.0f);
	result[0][0] = w;
	result[1][1] = h;
	result[2][3] = - 1.0f;

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zNear - zFar);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = - (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0f * zFar * zNear) / (zFar - zNear);
#	endif

	return result;
}


mat4 perspectiveFovLH(
	r32 fov,
	r32 width, r32 height,
	r32 zNear, r32 zFar)
{
	assert(width > 0.0f);
	assert(height > 0.0f);
	assert(fov > 0.0f);

	r32 rad = fov;
	r32 h = cosf(0.5f * rad) / sinf(0.5f * rad);
	r32 w = h * height / width; // TODO: max(width , Height) / min(width , Height)?

	mat4 result(0.0f);
	result[0][0] = w;
	result[1][1] = h;
	result[2][3] = 1.0f;

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zFar - zNear);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0f * zFar * zNear) / (zFar - zNear);
#	endif

	return result;
}


mat4 infinitePerspectiveRH(
	r32 fovy,
	r32 aspect,
	r32 zNear)
{
	r32 range = tan(fovy / 2.0f) * zNear;
	r32 left = -range * aspect;
	r32 right = range * aspect;
	r32 bottom = -range;
	r32 top = range;

	mat4 result(0.0f);
	result[0][0] = (2.0f * zNear) / (right - left);
	result[1][1] = (2.0f * zNear) / (top - bottom);
	result[2][2] = - 1.0f;
	result[2][3] = - 1.0f;
	result[3][2] = - 2.0f * zNear;
	return result;
}


mat4 infinitePerspectiveLH(
	r32 fovy,
	r32 aspect,
	r32 zNear)
{
	r32 range = tan(fovy / 2.0f) * zNear;
	r32 left = -range * aspect;
	r32 right = range * aspect;
	r32 bottom = -range;
	r32 top = range;

	mat4 result(0.0f);
	result[0][0] = (2.0f * zNear) / (right - left);
	result[1][1] = (2.0f * zNear) / (top - bottom);
	result[2][2] = 1.0f;
	result[2][3] = 1.0f;
	result[3][2] = - 2.0f * zNear;
	return result;
}

// Infinite projection matrix: http://www.terathon.com/gdc07_lengyel.pdf

mat4 tweakedInfinitePerspective(
	r32 fovy,
	r32 aspect,
	r32 zNear,
	r32 ep)
{
	r32 range = tan(fovy / 2.0f) * zNear;	
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

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
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

#	if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
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
	const vec3& center,
	const vec3& up)
{
	vec3 f(normalize(center - eye));
	vec3 s(normalize(cross(f, up)));
	vec3 u(cross(s, f));

	mat4 result;
	result[0][0] = s.x;
	result[1][0] = s.y;
	result[2][0] = s.z;
	result[0][1] = u.x;
	result[1][1] = u.y;
	result[2][1] = u.z;
	result[0][2] = -f.x;
	result[1][2] = -f.y;
	result[2][2] = -f.z;
	result[3][0] = -dot(s, eye);
	result[3][1] = -dot(u, eye);
	result[3][2] = dot(f, eye);
	return result;
}


mat4 lookAtLH(
	const vec3& eye,
	const vec3& center,
	const vec3& up)
{
	vec3 f(normalize(center - eye));
	vec3 s(normalize(cross(up, f)));
	vec3 u(cross(f, s));

	mat4 result;
	result[0][0] = s.x;
	result[1][0] = s.y;
	result[2][0] = s.z;
	result[0][1] = u.x;
	result[1][1] = u.y;
	result[2][1] = u.z;
	result[0][2] = f.x;
	result[1][2] = f.y;
	result[2][2] = f.z;
	result[3][0] = -dot(s, eye);
	result[3][1] = -dot(u, eye);
	result[3][2] = -dot(f, eye);
	return result;
}


#endif