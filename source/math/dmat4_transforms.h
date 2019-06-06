#ifndef _DMAT4_TRANSFORM_H
#define _DMAT4_TRANSFORM_H

#include "dmat4.h"


dmat4 translate(
	const dmat4& m,
	const dvec3& v)
{
	dmat4 result(m);
	result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
	return result;
}


dmat4 rotate(
	const dmat4& m,
	r64 angle,
	const dvec3& v)
{
	r64 a = angle;
	r64 c = cos(a);
	r64 s = sin(a);

	dvec3 axis(normalize(v));
	dvec3 temp((1.0 - c) * axis);

	dmat4 rot(
		{
			c + temp[0] * axis[0],
			temp[0] * axis[1] + s * axis[2],
			temp[0] * axis[2] - s * axis[1],
			0.0
		},
		{
			temp[1] * axis[0] - s * axis[2],
			c + temp[1] * axis[1],
			temp[1] * axis[2] + s * axis[0],
			0.0
		},
		{
			temp[2] * axis[0] + s * axis[1],
			temp[2] * axis[1] - s * axis[0],
			c + temp[2] * axis[2],
			0.0
		},
		{});

	return dmat4(
		m[0] * rot[0][0] + m[1] * rot[0][1] + m[2] * rot[0][2],
		m[0] * rot[1][0] + m[1] * rot[1][1] + m[2] * rot[1][2],
		m[0] * rot[2][0] + m[1] * rot[2][1] + m[2] * rot[2][2],
		m[3]);
}
	

dmat4 rotate_slow(
	const dmat4& m,
	r64 angle,
	const dvec3& v)
{
	r64 a = angle;
	r64 c = cos(a);
	r64 s = sin(a);
	dmat4 result;

	dvec3 axis = normalize(v);

	result[0][0] = c + (1.0 - c)      * axis.x     * axis.x;
	result[0][1] = (1.0 - c) * axis.x * axis.y + s * axis.z;
	result[0][2] = (1.0 - c) * axis.x * axis.z - s * axis.y;
	result[0][3] = 0.0;

	result[1][0] = (1.0 - c) * axis.y * axis.x - s * axis.z;
	result[1][1] = c + (1.0 - c) * axis.y * axis.y;
	result[1][2] = (1.0 - c) * axis.y * axis.z + s * axis.x;
	result[1][3] = 0.0;

	result[2][0] = (1.0 - c) * axis.z * axis.x + s * axis.y;
	result[2][1] = (1.0 - c) * axis.z * axis.y - s * axis.x;
	result[2][2] = c + (1.0 - c) * axis.z * axis.z;
	result[2][3] = 0.0;

	result[3] = dvec4{ 0, 0, 0, 1 };
	return m * result;
}


dmat4 scale(
	const dmat4& m,
	const dvec3& v)
{
	return dmat4(
		m[0] * v[0],
		m[1] * v[1],
		m[2] * v[2],
		m[3]);
}


dmat4 scale_slow(
	const dmat4& m,
	const dvec3& v)
{
	dmat4 result;
	result[0][0] = v.x;
	result[1][1] = v.y;
	result[2][2] = v.z;
	return m * result;
}


dmat4 ortho(
	r64 left, r64 right,
	r64 bottom, r64 top)
{
	dmat4 result;
	result[0][0] = 2.0 / (right - left);
	result[1][1] = 2.0 / (top - bottom);
	result[2][2] = - 1.0;
	result[3][0] = - (right + left) / (right - left);
	result[3][1] = - (top + bottom) / (top - bottom);
	return result;
}


dmat4 orthoLH(
	r64 left, r64 right,
	r64 bottom, r64 top,
	r64 zNear, r64 zFar)
{
	dmat4 result;
	result[0][0] = 2.0 / (right - left);
	result[1][1] = 2.0 / (top - bottom);
	result[3][0] = - (right + left) / (right - left);
	result[3][1] = - (top + bottom) / (top - bottom);

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = 1.0 / (zFar - zNear);
	result[3][2] = - zNear / (zFar - zNear);
#	else
	result[2][2] = 2.0 / (zFar - zNear);
	result[3][2] = - (zFar + zNear) / (zFar - zNear);
#	endif

	return result;
}


dmat4 orthoRH(
	r64 left, r64 right,
	r64 bottom, r64 top,
	r64 zNear, r64 zFar)
{
	dmat4 result;
	result[0][0] = 2.0 / (right - left);
	result[1][1] = 2.0 / (top - bottom);
	result[3][0] = - (right + left) / (right - left);
	result[3][1] = - (top + bottom) / (top - bottom);

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = - 1.0 / (zFar - zNear);
	result[3][2] = - zNear / (zFar - zNear);
#	else
	result[2][2] = - 2.0 / (zFar - zNear);
	result[3][2] = - (zFar + zNear) / (zFar - zNear);
#	endif

	return result;
}


dmat4 frustumLH(
	r64 left, r64 right,
	r64 bottom, r64 top,
	r64 nearVal, r64 farVal)
{
	dmat4 result(0.0);
	result[0][0] = (2.0 * nearVal) / (right - left);
	result[1][1] = (2.0 * nearVal) / (top - bottom);
	result[2][0] = (right + left) / (right - left);
	result[2][1] = (top + bottom) / (top - bottom);
	result[2][3] = 1.0;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = farVal / (farVal - nearVal);
	result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#	else
	result[2][2] = (farVal + nearVal) / (farVal - nearVal);
	result[3][2] = - (2.0 * farVal * nearVal) / (farVal - nearVal);
#	endif

	return result;
}


dmat4 frustumRH(
	r64 left, r64 right,
	r64 bottom, r64 top,
	r64 nearVal, r64 farVal)
{
	dmat4 result(0.0);
	result[0][0] = (2.0 * nearVal) / (right - left);
	result[1][1] = (2.0 * nearVal) / (top - bottom);
	result[2][0] = (right + left) / (right - left);
	result[2][1] = (top + bottom) / (top - bottom);
	result[2][3] = -1.0;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = farVal / (nearVal - farVal);
	result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#	else
	result[2][2] = - (farVal + nearVal) / (farVal - nearVal);
	result[3][2] = - (2.0 * farVal * nearVal) / (farVal - nearVal);
#	endif

	return result;
}


dmat4 perspectiveRH(
	r64 fovy,
	r64 aspect,
	r64 zNear,
	r64 zFar)
{
	assert(abs(aspect - DBL_EPSILON) > 0.0);

	r64 tanHalfFovy = tan(fovy / 2.0);

	dmat4 result(0.0);
	result[0][0] = 1.0 / (aspect * tanHalfFovy);
	result[1][1] = 1.0 / (tanHalfFovy);
	result[2][3] = - 1.0;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zNear - zFar);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = - (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0 * zFar * zNear) / (zFar - zNear);
#	endif

	return result;
}


dmat4 perspectiveLH(
	r64 fovy,
	r64 aspect,
	r64 zNear,
	r64 zFar)
{
	assert(abs(aspect - DBL_EPSILON) > 0.0);

	r64 tanHalfFovy = tan(fovy / 2.0);
	
	dmat4 result(0.0);
	result[0][0] = 1.0 / (aspect * tanHalfFovy);
	result[1][1] = 1.0 / (tanHalfFovy);
	result[2][3] = 1.0;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zFar - zNear);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0 * zFar * zNear) / (zFar - zNear);
#	endif

	return result;
}


dmat4 perspectiveFovRH(
	r64 fov,
	r64 width, r64 height,
	r64 zNear, r64 zFar)
{
	assert(width > 0.0);
	assert(height > 0.0);
	assert(fov > 0.0);

	r64 rad = fov;
	r64 h = cos(0.5 * rad) / sin(0.5 * rad);
	r64 w = h * height / width; //TODO: max(width , Height) / min(width , Height)?

	dmat4 result(0.0);
	result[0][0] = w;
	result[1][1] = h;
	result[2][3] = - 1.0;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zNear - zFar);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = - (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0 * zFar * zNear) / (zFar - zNear);
#	endif

	return result;
}


dmat4 perspectiveFovLH(
	r64 fov,
	r64 width, r64 height,
	r64 zNear, r64 zFar)
{
	assert(width > 0.0);
	assert(height > 0.0);
	assert(fov > 0.0);

	r64 rad = fov;
	r64 h = cos(0.5 * rad) / sin(0.5 * rad);
	r64 w = h * height / width; // TODO: max(width , Height) / min(width , Height)?

	dmat4 result(0.0);
	result[0][0] = w;
	result[1][1] = h;
	result[2][3] = 1.0;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	result[2][2] = zFar / (zFar - zNear);
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
#	else
	result[2][2] = (zFar + zNear) / (zFar - zNear);
	result[3][2] = - (2.0 * zFar * zNear) / (zFar - zNear);
#	endif

	return result;
}


dmat4 infinitePerspectiveRH(
	r64 fovy,
	r64 aspect,
	r64 zNear)
{
	r64 range = tan(fovy / 2.0) * zNear;
	r64 left = -range * aspect;
	r64 right = range * aspect;
	r64 bottom = -range;
	r64 top = range;

	dmat4 result(0.0);
	result[0][0] = (2.0 * zNear) / (right - left);
	result[1][1] = (2.0 * zNear) / (top - bottom);
	result[2][2] = - 1.0;
	result[2][3] = - 1.0;
	result[3][2] = - 2.0 * zNear;
	return result;
}


dmat4 infinitePerspectiveLH(
	r64 fovy,
	r64 aspect,
	r64 zNear)
{
	r64 range = tan(fovy / 2.0) * zNear;
	r64 left = -range * aspect;
	r64 right = range * aspect;
	r64 bottom = -range;
	r64 top = range;

	dmat4 result(0.0);
	result[0][0] = (2.0 * zNear) / (right - left);
	result[1][1] = (2.0 * zNear) / (top - bottom);
	result[2][2] = 1.0;
	result[2][3] = 1.0;
	result[3][2] = - 2.0 * zNear;
	return result;
}

// Infinite projection matrix: http://www.terathon.com/gdc07_lengyel.pdf

dmat4 tweakedInfinitePerspective(
	r64 fovy,
	r64 aspect,
	r64 zNear,
	r64 ep)
{
	r64 range = tan(fovy / 2.0) * zNear;	
	r64 left = -range * aspect;
	r64 right = range * aspect;
	r64 bottom = -range;
	r64 top = range;

	dmat4 result(0.0);
	result[0][0] = (2.0 * zNear) / (right - left);
	result[1][1] = (2.0 * zNear) / (top - bottom);
	result[2][2] = ep - 1.0;
	result[2][3] = -1.0;
	result[3][2] = (ep - 2.0) * zNear;
	return result;
}


dmat4 tweakedInfinitePerspective(
	r64 fovy,
	r64 aspect,
	r64 zNear)
{
	return tweakedInfinitePerspective(fovy, aspect, zNear, DBL_EPSILON);
}


dvec3 project(
	const dvec3& obj,
	const dmat4& model,
	const dmat4& proj,
	const dvec4& viewport)
{
	dvec4 tmp = dvec4{ obj.x, obj.y, obj.z, 1.0 };
	tmp = model * tmp;
	tmp = proj * tmp;

	tmp /= tmp.w;

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	tmp.x = tmp.x * 0.5 + 0.5;
	tmp.y = tmp.y * 0.5 + 0.5;
#	else
	tmp = tmp * 0.5 + 0.5;
#	endif

	tmp[0] = tmp[0] * viewport[2] + viewport[0];
	tmp[1] = tmp[1] * viewport[3] + viewport[1];

	return tmp.xyz;
}


dvec3 unProject(
	const dvec3& win,
	const dmat4& model,
	const dmat4& proj,
	const dvec4& viewport)
{
	dmat4 inv = inverse(proj * model);

	dvec4 tmp = dvec4{ win.x, win.y, win.z, 1.0 };
	tmp.x = (tmp.x - viewport[0]) / viewport[2];
	tmp.y = (tmp.y - viewport[1]) / viewport[3];

#	if DEPTH_CLIP_SPACE == DEPTH_ZERO_TO_ONE
	tmp.x = tmp.x * 2.0 - 1.0;
	tmp.y = tmp.y * 2.0 - 1.0;
#	else
	tmp = tmp * 2.0 - 1.0;
#	endif

	dvec4 obj = inv * tmp;
	obj /= obj.w;

	return obj.xyz;
}


dmat4 pickMatrix(
	const dvec2& center,
	const dvec2& delta,
	const dvec4& viewport)
{
	assert(delta.x > 0.0 && delta.y > 0.0);
	dmat4 result;

	if (!(delta.x > 0.0 && delta.y > 0.0)) {
		return result; // Error
	}

	dvec3 temp{
		(viewport[2] - 2.0 * (center.x - viewport[0])) / delta.x,
		(viewport[3] - 2.0 * (center.y - viewport[1])) / delta.y,
		0.0};

	// Translate and scale the picked region to the entire window
	result = translate(result, temp);
	return scale(result, dvec3{ viewport[2] / delta.x, viewport[3] / delta.y, 1.0 });
}


dmat4 lookAtRH(
	const dvec3& eye,
	const dvec3& target,
	const dvec3& up)
{
	static const dvec3 xAxis = { 1.0, 0.0,  0.0 };
	static const dvec3 yAxis = { 0.0, 1.0,  0.0 };
	static const dvec3 zAxis = { 0.0, 0.0, -1.0 };

	dvec3 B(normalize(eye - target));
	dvec3 S(normalize(cross(up, B)));
	dvec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0 && B.y == 0.0 && B.z == 0.0)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0 && U.y == 0.0 && U.z == 0.0)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return dmat4(
		S.x, U.x, B.x, 0.0,
		S.y, U.y, B.y, 0.0,
		S.z, U.z, B.z, 0.0,
		-dot(S, eye), -dot(U, eye), -dot(B, eye), 1.0);
}


/**
 * This can be used to create the ModelView matrix for a camera.
 */
dmat4 lookAlongRH(
	const dvec3& eye,
	const dvec3& viewDir,
	const dvec3& up)
{
	static const dvec3 xAxis = { 1.0, 0.0,  0.0 };
	static const dvec3 yAxis = { 0.0, 1.0,  0.0 };
	static const dvec3 zAxis = { 0.0, 0.0, -1.0 };

	assert(length2(viewDir) != 0.0 && "viewDir must be normalized");
	dvec3 B(-viewDir);
	dvec3 S(normalize(cross(up, B)));
	dvec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0 && B.y == 0.0 && B.z == 0.0)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0 && U.y == 0.0 && U.z == 0.0)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return dmat4(
		S.x, U.x, B.x, 0.0,
		S.y, U.y, B.y, 0.0,
		S.z, U.z, B.z, 0.0,
		-dot(S, eye), -dot(U, eye), -dot(B, eye), 1.0);
}


dmat4 lookAtLH(
	const dvec3& eye,
	const dvec3& target,
	const dvec3& up)
{
	static const dvec3 xAxis = { 1.0, 0.0, 0.0 };
	static const dvec3 yAxis = { 0.0, 1.0, 0.0 };
	static const dvec3 zAxis = { 0.0, 0.0, 1.0 };

	dvec3 F(normalize(target - eye));
	dvec3 S(normalize(cross(up, F)));
	dvec3 U(cross(F, S));

	if (F.x == 0.0 && F.y == 0.0 && F.z == 0.0)
	{
		F = zAxis;
	}

	if (U.x == 0.0 && U.y == 0.0 && U.z == 0.0)
	{
		U = yAxis;
	}

	if (dot(F, U) == 0.0) {
		U = (F != xAxis) ? cross(F, xAxis) : cross(F, zAxis);
	}

	return dmat4(
		S.x, U.x, F.x, 0.0,
		S.y, U.y, F.y, 0.0,
		S.z, U.z, F.z, 0.0,
		-dot(S, eye), -dot(U, eye), -dot(F, eye), 1.0);
}


/**
 * Get a transform matrix to align to an vector. This is the inverse/transpose of lookAtRH.
 * lookAt returns a viewing matrix (transforms coordinates into viewspace) whereas this function
 * returns a transform into world space (from a local model space into world space). The transform
 * will both translate and rotate the model space to face a target from the eye point.
 */
dmat4 alignToRH(
	const dvec3& eye,
	const dvec3& target,
	const dvec3& up)
{
	static const dvec3 xAxis = { 1.0, 0.0,  0.0 };
	static const dvec3 yAxis = { 0.0, 1.0,  0.0 };
	static const dvec3 zAxis = { 0.0, 0.0, -1.0 };
	
	dvec3 B(normalize(eye - target));
	dvec3 S(normalize(cross(up, B)));
	dvec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0 && B.y == 0.0 && B.z == 0.0)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0 && U.y == 0.0 && U.z == 0.0)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return dmat4(
		S.x, S.y, S.z, -dot(S, eye),
		U.x, U.y, U.z, -dot(U, eye),
		B.x, B.y, B.z, -dot(B, eye),
		0.0, 0.0, 0.0, 1.0);
}


dmat4 alignToLH(
	const dvec3& eye,
	const dvec3& target,
	const dvec3& up)
{
	static const dvec3 xAxis = { 1.0, 0.0, 0.0 };
	static const dvec3 yAxis = { 0.0, 1.0, 0.0 };
	static const dvec3 zAxis = { 0.0, 0.0, 1.0 };

	dvec3 F(normalize(target - eye));
	dvec3 S(normalize(cross(up, F)));
	dvec3 U(cross(F, S));

	if (F.x == 0.0 && F.y == 0.0 && F.z == 0.0)
	{
		F = zAxis;
	}

	if (U.x == 0.0 && U.y == 0.0 && U.z == 0.0)
	{
		U = yAxis;
	}

	if (dot(F, U) == 0.0) {
		U = (F != xAxis) ? cross(F, xAxis) : cross(F, zAxis);
	}

	return dmat4(
		S.x, S.y, S.z, -dot(S, eye),
		U.x, U.y, U.z, -dot(U, eye),
		F.x, F.y, F.z, -dot(F, eye),
		0.0, 0.0, 0.0, 1.0);
}


/**
 * Get a rotation matrix to align to an vector. This is the inverse/transpose of the upper 3x3
 * portion of the lookAtRH matrix. lookAt returns a viewing matrix (transforms coordinates into
 * viewspace) whereas this function returns a rotation into world space (from a local model space
 * into world space). Unlike alignToRH, this matrix is rotation only and will not set any
 * translation to an eye point, just a direction.
 */
dmat4 alignAlongRH(
	const dvec3& viewDir,
	const dvec3& up)
{
	static const dvec3 xAxis = { 1.0, 0.0,  0.0 };
	static const dvec3 yAxis = { 0.0, 1.0,  0.0 };
	static const dvec3 zAxis = { 0.0, 0.0, -1.0 };

	assert(length2(viewDir) != 0.0 && "viewDir must be normalized");
	dvec3 B(-viewDir);
	dvec3 S(normalize(cross(up, B)));
	dvec3 U(cross(B, S));

	// ensure that the target direction is non-zero.
	if (B.x == 0.0 && B.y == 0.0 && B.z == 0.0)
	{
		B = zAxis;
	}

	// Ensure that the up direction is non-zero.
	if (U.x == 0.0 && U.y == 0.0 && U.z == 0.0)
	{
		U = yAxis;
	}

	// if view dir and up are parallel or opposite, then compute a new,
	// arbitrary up that is not parallel or opposite to view dir
	if (dot(B, U) == 0.0) {
		U = (B != xAxis) ? cross(B, xAxis) : cross(B, zAxis);
	}

	return dmat4(
		S.x,  S.y,  S.z,  0.0,
		U.x,  U.y,  U.z,  0.0,
		B.x,  B.y,  B.z,  0.0,
		0.0, 0.0, 0.0, 1.0);
}


dmat4 alignAlongLH(
	const dvec3& eye,
	const dvec3& target,
	const dvec3& up)
{
	static const dvec3 xAxis = { 1.0, 0.0, 0.0 };
	static const dvec3 yAxis = { 0.0, 1.0, 0.0 };
	static const dvec3 zAxis = { 0.0, 0.0, 1.0 };
	
	dvec3 F(normalize(target - eye));
	dvec3 S(normalize(cross(up, F)));
	dvec3 U(cross(F, S));

	if (F.x == 0.0 && F.y == 0.0 && F.z == 0.0)
	{
		F = zAxis;
	}

	if (U.x == 0.0 && U.y == 0.0 && U.z == 0.0)
	{
		U = yAxis;
	}

	if (dot(F, U) == 0.0) {
		U = (F != xAxis) ? cross(F, xAxis) : cross(F, zAxis);
	}

	return dmat4(
		S.x,  S.y,  S.z,  0.0,
		U.x,  U.y,  U.z,  0.0,
		F.x,  F.y,  F.z,  0.0,
		0.0, 0.0, 0.0, 1.0);
}


dmat4 affineInverse(const dmat4& m)
{
	dmat3 inv(inverse(make_dmat3(m)));
	
	return dmat4(
		make_dvec4(inv[0], 0.0),
		make_dvec4(inv[1], 0.0),
		make_dvec4(inv[2], 0.0),
		make_dvec4(-inv * make_dvec3(m[3]), 1.0));
}


void getForwardUpRight(
	const dmat4& view,
	dvec3& f,
	dvec3& u,
	dvec3& r)
{ 
	r = {  view[0][0],  view[1][0],  view[2][0] };
	u = {  view[0][1],  view[1][1],  view[2][1] };
	f = { -view[0][2], -view[1][2], -view[2][2] };
}


#endif