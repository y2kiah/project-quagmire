#include "camera.h"


static const vec3 c_xAxis{ 1.0f, 0.0f, 0.0f };
static const vec3 c_yAxis{ 0.0f, 1.0f, 0.0f };
static const vec3 c_zAxis{ 0.0f, 0.0f, 1.0f };


///////////////////////////////////////////////////////////////////////////////////////////
// CameraPersp2
/*
vec3 CameraPersp::getEulerAngles() const
{
	return eulerAngles(viewRotationQuat);
}

void CameraPersp::setTranslationYawPitchRoll(const dvec3& position, double yaw, double pitch, double roll)
{
	setEyePoint(position);
	dmat4 rotation = eulerAngleZ(roll);
	rotation *= eulerAngleXY(pitch, yaw);
	viewRotationQuat = quat_cast(rotation);
	
	// could be delayed?
	viewMatrix = make_mat4(translate(rotation, -eyePoint));
	viewProjectionMatrix = projectionMatrix * viewMatrix;
}
*/
void CameraPersp::lookAt(const dvec3 &position, const dvec3 &target)
{
	setEyePoint(position);
	viewMatrix = make_mat4(lookAtRH(eyePoint, target, make_dvec3(worldUp)));
	viewRotationQuat = quat_cast(viewMatrix);
	
	// could be delayed?
	viewProjectionMatrix = projectionMatrix * viewMatrix;
}


///////////////////////////////////////////////////////////////////////////////////////////
// Camera

void Camera::setEyePoint(const dvec3 &aEyePoint)
{
	eyePoint = aEyePoint;
	modelViewCached = false;
}

void Camera::setCenterOfInterestPoint(const dvec3 &centerOfInterestPoint)
{
	centerOfInterest = distance(eyePoint, centerOfInterestPoint);
	lookAt(centerOfInterestPoint);
}

void Camera::setViewDirection(const vec3 &aViewDirection)
{
	viewDirection = normalize(aViewDirection);
	orientation = quatFromNormalizedVectors(vec3{ 0.0f, 0.0f, -1.0f }, viewDirection);
	modelViewCached = false;
}

void Camera::setOrientation(const quat &aOrientation)
{
	orientation = normalize(aOrientation);
	viewDirection = orientation * vec3{ 0.0f, 0.0f, -1.0f };
	modelViewCached = false;
}

void Camera::setWorldUp(const vec3 &aWorldUp)
{
	worldUp = normalize(aWorldUp);
	orientation = quat_alignAlongRH(viewDirection, worldUp);
	modelViewCached = false;
}

void Camera::lookAt(const dvec3 &target)
{
	viewDirection = make_vec3(normalize(target - eyePoint));
	orientation = quat_alignAlongRH(viewDirection, worldUp);
	modelViewCached = false;
}

void Camera::lookAt(const dvec3 &aEyePoint, const dvec3 &target)
{
	eyePoint = aEyePoint;
	viewDirection = make_vec3(normalize(target - eyePoint));
	orientation = quat_alignAlongRH(viewDirection, worldUp);
	modelViewCached = false;
}

void Camera::lookAt(const dvec3 &aEyePoint, const dvec3 &target, const dvec3 &aWorldUp)
{
	eyePoint = aEyePoint;
	worldUp = make_vec3(normalize(aWorldUp));
	viewDirection = make_vec3(normalize(target - eyePoint));
	orientation = quat_alignAlongRH(viewDirection, worldUp);
	modelViewCached = false;
}

void Camera::getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
	calcMatrices();

	vec3 F(normalize(viewDirection));

	*topLeft = eyePoint + make_dvec3((nearClip * F) + (frustumTop * up) + (frustumLeft * right));
	*topRight = eyePoint + make_dvec3((nearClip * F) + (frustumTop * up) + (frustumRight * right));
	*bottomLeft = eyePoint + make_dvec3((nearClip * F) + (frustumBottom * up) + (frustumLeft * right));
	*bottomRight = eyePoint + make_dvec3((nearClip * F) + (frustumBottom * up) + (frustumRight * right));
}

void Camera::getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
	calcMatrices();

	vec3 F(normalize(viewDirection));

	r32 ratio = farClip / nearClip;

	*topLeft = eyePoint + make_dvec3((farClip * F) + (ratio * frustumTop * up) + (ratio * frustumLeft * right));
	*topRight = eyePoint + make_dvec3((farClip * F) + (ratio * frustumTop * up) + (ratio * frustumRight * right));
	*bottomLeft = eyePoint + make_dvec3((farClip * F) + (ratio * frustumBottom * up) + (ratio * frustumLeft * right));
	*bottomRight = eyePoint + make_dvec3((farClip * F) + (ratio * frustumBottom * up) + (ratio * frustumRight * right));
}

void Camera::getFrustum(
	r32 *leftPlane,
	r32 *topPlane,
	r32 *rightPlane,
	r32 *bottomPlane,
	r32 *nearPlane,
	r32 *farPlane)
{
	calcMatrices();

	*leftPlane = frustumLeft;
	*topPlane = frustumTop;
	*rightPlane = frustumRight;
	*bottomPlane = frustumBottom;
	*nearPlane = nearClip;
	*farPlane = farClip;
}

//Ray Camera::generateRay(r32 uPos, r32 vPos, r32 imagePlaneApectRatio) const
//{
//	calcMatrices();

//	r32 s = (uPos - 0.5f) * imagePlaneApectRatio;
//	r32 t = (vPos - 0.5f);
//	r32 viewDistance = imagePlaneApectRatio / math<r32>::abs(frustumRight - frustumLeft) * nearClip;
//	return Ray(eyePoint, (right * s + up * t - (back * viewDistance)).normalized());
//}

void Camera::getBillboardVectors(dvec3 *billboardRight, dvec3 *billboardUp) const
{
	auto& m = modelViewMatrix;
	billboardRight->x = m[0][0];
	billboardRight->y = m[1][0];
	billboardRight->z = m[2][0];
	billboardUp->x    = m[0][1];
	billboardUp->y    = m[1][1];
	billboardUp->z    = m[2][1];
}

vec2 Camera::worldToScreen(const dvec3 &worldCoord, r32 screenWidth, r32 screenHeight) const
{
	vec4 eye = make_vec4(make_dvec4(worldCoord, 1.0) * modelViewMatrix);
	vec3 ndc = make_vec3(eye * projectionMatrix);

	return vec2{
		(ndc.x + 1.0f) / 2.0f * screenWidth,
		(1.0f - (ndc.y + 1.0f) / 2.0f) * screenHeight};
}

vec3 Camera::worldToEye(const dvec3 &worldCoord)
{
	return make_vec3(make_dvec4(worldCoord, 1.0) * modelViewMatrix);
}

r32 Camera::worldToEyeDepth(const dvec3 &worldCoord) const
{
	const dmat4& m = modelViewMatrix;
	r64 d = m[0][2] * worldCoord.x + m[1][2] * worldCoord.y + m[2][2] * worldCoord.z + m[3][2];
	return (r32)d;
}

vec3 Camera::worldToNdc(const dvec3 &worldCoord)
{
	vec4 eye = make_vec4(make_dvec4(worldCoord, 1.0) * modelViewMatrix);
	return make_vec3(eye * projectionMatrix);
}

//* This only mostly works
/*r32 Camera::getScreenRadius(const Sphere &sphere, r32 screenWidth, r32 screenHeight) const
{
	vec2 screenCenter(worldToScreen(sphere.getCenter(), screenWidth, screenHeight));
	vec3 orthog = viewDirection.getOrthogonal().normalized();
	vec2 screenPerimeter = worldToScreen(sphere.getCenter() + sphere.getRadius() * orthog, screenWidth, screenHeight);
	return distance(screenPerimeter, screenCenter);
}*/

void Camera::calcModelView()
{
	back = normalize(-viewDirection);
	right = orientation * c_xAxis;
	up = orientation * c_yAxis;

	dvec3 d{
		dot(-eyePoint, make_dvec3(right)),
		dot(-eyePoint, make_dvec3(up)),
		dot(-eyePoint, make_dvec3(back))};

	auto& m = modelViewMatrix;
	m[0][0] = right.x; m[1][0] = right.y; m[2][0] = right.z; m[3][0] = d.x;
	m[0][1] = up.x;    m[1][1] = up.y;    m[2][1] = up.z;    m[3][1] = d.y;
	m[0][2] = back.x;  m[1][2] = back.y;  m[2][2] = back.z;  m[3][2] = d.z;
	m[0][3] = 0.0;     m[1][3] = 0.0;     m[2][3] = 0.0;     m[3][3] = 1.0;

	modelViewCached = true;
	inverseModelViewCached = false;
}

void Camera::calcInverseModelView()
{
	if (!modelViewCached) {
		calcModelView();
	}

	inverseModelViewMatrix = affineInverse(modelViewMatrix);
	inverseModelViewCached = true;
}

////////////////////////////////////////////////////////////////////////////////////////
// CameraPersp

void makePerspectiveCamera(r32 aspect, r32 fovDegrees)
{
//	lensShift(0.0f)
//	r32 eyeX = pixelWidth / 2.0f;
//	r32 eyeY = pixelHeight / 2.0f;
//	r32 halfFov = 3.14159f * fovDegrees / 360.0f;
//	r32 theTan = tanf(halfFov);
//	r32 dist = eyeY / theTan;
//	r32 nearDist = dist / 10.0f;	// near / far clip plane
//	r32 farDist = dist * 10.0f;
//	r32 aspect = pixelWidth / (r32)pixelHeight;
//
//	setPerspective(fovDegrees, aspect, nearPlane, farPlane);
//	lookAt(vec3(eyeX, eyeY, dist), vec3(eyeX, eyeY, 0.0f));
}

void CameraPersp::setPerspective(
	r32 verticalFovDegrees,
	r32 aspect,
	r32 nearPlane,
	r32 farPlane)
{
	fov = verticalFovDegrees;
	aspectRatio = aspect;
	nearClip = nearPlane;
	farClip = farPlane;

	projectionCached = false;
}

void Camera::calcPerspProjection()
{
	frustumTop = nearClip * tanf(PIf / 180.0f * fov * 0.5f);
	frustumBottom = -frustumTop;
	frustumRight = frustumTop * aspectRatio;
	frustumLeft = -frustumRight;

	// perform lens shift
	if (lensShift.y != 0.0f) {
		frustumTop = mix(0.0f, 2.0f * frustumTop, 0.5f + 0.5f * lensShift.y);
		frustumBottom = mix(2.0f * frustumBottom, 0.0f, 0.5f + 0.5f * lensShift.y);
	}

	if (lensShift.x != 0.0f) {
		frustumRight = mix(2.0f * frustumRight, 0.0f, 0.5f - 0.5f * lensShift.x);
		frustumLeft = mix(0.0f, 2.0f * frustumLeft, 0.5f - 0.5f * lensShift.x);
	}

	projectionMatrix = perspectiveRH(fov * DEG_TO_RADf, aspectRatio, nearClip, farClip);
	projectionCached = true;
}

void CameraPersp::setLensShift(r32 horizontal, r32 vertical)
{
	lensShift.x = horizontal;
	lensShift.y = vertical;

	projectionCached = false;
}

/*CameraPersp	CameraPersp::getFrameSphere(const Sphere &worldSpaceSphere, int maxIterations) const
{
	CameraPersp result = *this;
	result.setEyePoint(worldSpaceSphere.getCenter() - result.viewDirection * getCenterOfInterest());

	r32 minDistance = 0.01f, maxDistance = 100000.0f;
	r32 curDistance = getCenterOfInterest();
	for (int i = 0; i < maxIterations; ++i) {
		r32 curRadius = result.getScreenRadius(worldSpaceSphere, 2.0f, 2.0f);
		if (curRadius < 1.0f) { // we should get closer
			maxDistance = curDistance;
			curDistance = (curDistance + minDistance) * 0.5f;
		}
		else { // we should get farther
			minDistance = curDistance;
			curDistance = (curDistance + maxDistance) * 0.5f;
		}
		result.setEyePoint(worldSpaceSphere.getCenter() - result.viewDirection * curDistance);
	}

	result.setCenterOfInterest(result.getEyePoint().distance(worldSpaceSphere.getCenter()));
	return result;
}*/

////////////////////////////////////////////////////////////////////////////////////////
// CameraOrtho
/*CameraOrtho::CameraOrtho() :
	Camera()
{
	lookAt(vec3(0.0f, 0.0f, 0.1f), vec3(0.0f), c_yAxis);
	setCenterOfInterest(0.1f);
	setFov(35.0f);
}

CameraOrtho::CameraOrtho(r32 left, r32 right, r32 bottom, r32 top, r32 nearPlane, r32 farPlane) :
	Camera()
{
	frustumLeft = left;
	frustumRight = right;
	frustumTop = top;
	frustumBottom = bottom;
	nearClip = nearPlane;
	farClip = farPlane;

	projectionCached = false;
	modelViewCached = true;
	inverseModelViewCached = true;
}

void CameraOrtho::setOrtho(r32 left, r32 right, r32 bottom, r32 top, r32 nearPlane, r32 farPlane)
{
	frustumLeft = left;
	frustumRight = right;
	frustumTop = top;
	frustumBottom = bottom;
	nearClip = nearPlane;
	farClip = farPlane;

	projectionCached = false;
}

void CameraOrtho::calcProjection()
{
	projectionMatrix = glm::ortho(frustumLeft, frustumRight, frustumBottom, frustumTop, nearClip, farClip);
	projectionCached = true;
}
*/
////////////////////////////////////////////////////////////////////////////////////////
// CameraStereo

/*dvec3 CameraStereo::getEyePointShifted() const
{
	if (!isStereo) {
		return eyePoint;
	}
	if (isLeft) {
		return eyePoint - (dquat(orientation) * dvec3(c_xAxis) * (0.5 * eyeSeparation));
	}
	return eyePoint + (dquat(orientation) * dvec3(c_xAxis) * (0.5 * eyeSeparation));
}

void CameraStereo::getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
	calcMatrices();

	auto viewDirection = normalize(viewDirection);
	auto eye = getEyePointShifted();

	r32 shift = 0.5f * eyeSeparation * (nearClip / convergence);
	shift *= (isStereo ? (isLeft ? 1.0f : -1.0f) : 0.0f);

	r32 left = frustumLeft + shift;
	r32 right = frustumRight + shift;

	*topLeft = eye + dvec3((nearClip * viewDirection) + (frustumTop * up) + (left * right));
	*topRight = eye + dvec3((nearClip * viewDirection) + (frustumTop * up) + (right * right));
	*bottomLeft = eye + dvec3((nearClip * viewDirection) + (frustumBottom * up) + (left * right));
	*bottomRight = eye + dvec3((nearClip * viewDirection) + (frustumBottom * up) + (right * right));
}

void CameraStereo::getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
	calcMatrices();

	vec3 viewDirection(normalize(viewDirection));

	r32 ratio = farClip / nearClip;

	vec3 eye(getEyePointShifted());

	r32 shift = 0.5f * eyeSeparation * (nearClip / convergence);
	shift *= (isStereo ? (isLeft ? 1.0f : -1.0f) : 0.0f);

	r32 left = frustumLeft + shift;
	r32 right = frustumRight + shift;

	*topLeft = eye + (farClip * viewDirection) + (ratio * frustumTop * up) + (ratio * left * right);
	*topRight = eye + (farClip * viewDirection) + (ratio * frustumTop * up) + (ratio * right * right);
	*bottomLeft = eye + (farClip * viewDirection) + (ratio * frustumBottom * up) + (ratio * left * right);
	*bottomRight = eye + (farClip * viewDirection) + (ratio * frustumBottom * up) + (ratio * right * right);
}

const mat4& CameraStereo::projectionMatrix const
{
	assert(projectionCached);

	if (!isStereo) {
		return projectionMatrix;
	}
	else if (isLeft) {
		return projectionMatrixLeft;
	}
	return projectionMatrixRight;
}

const dmat4& CameraStereo::modelViewMatrix const
{
	assert(modelViewCached);

	if (!isStereo) {
		return modelViewMatrix;
	}
	else if (isLeft) {
		return modelViewMatrixLeft;
	}
	return modelViewMatrixRight;
}

const dmat4& CameraStereo::getInverseModelViewMatrix() const
{
	assert(inverseModelViewCached);

	if (!isStereo) {
		iturn inverseModelViewMatrix;
	}
	else if (isLeft) {
		iturn inverseModelViewMatrixLeft;
	}
	iturn inverseModelViewMatrixRight;
}

void CameraStereo::calcModelView()
{
	// calculate default matrix first
	CameraPersp::calcModelView();

	modelViewMatrixLeft = modelViewMatrix;
	modelViewMatrixRight = modelViewMatrix;

	// calculate left matrix
	dvec3 eye = eyePoint - dvec3(orientation * c_xAxis * (0.5f * eyeSeparation));
	dvec3 d = dvec3(dot(-eye, dvec3(right)), dot(-eye, dvec3(up)), dot(-eye, dvec3(back)));

	auto& m = modelViewMatrixLeft;
	m[3][0] = d.x; m[3][1] = d.y; m[3][2] = d.z;

	// calculate right matrix
	eye = eyePoint + dvec3(orientation * c_xAxis * (0.5f * eyeSeparation));
	d = dvec3(dot(-eye, dvec3(right)), dot(-eye, dvec3(up)), dot(-eye, dvec3(back)));

	m = modelViewMatrixRight;
	m[3][0] = d.x; m[3][1] = d.y; m[3][2] = d.z;

	modelViewCached = true;
	inverseModelViewCached = false;
}

void CameraStereo::calcInverseModelView()
{
	if (!modelViewCached) {
		calcModelView();
	}

	inverseModelViewMatrix = affineInverse(modelViewMatrix);
	inverseModelViewMatrixLeft = affineInverse(modelViewMatrixLeft);
	inverseModelViewMatrixRight = affineInverse(modelViewMatrixRight);
	inverseModelViewCached = true;
}

void CameraStereo::calcProjection()
{
	// calculate default matrices first
	CameraPersp::calcProjection();

	projectionMatrixLeft = projectionMatrix;
	inverseProjectionMatrixLeft = inverseProjectionMatrix;

	projectionMatrixRight = projectionMatrix;
	inverseProjectionMatrixRight = inverseProjectionMatrix;

	// calculate left matrices
	auto& m = projectionMatrixLeft;
	m[2][0] = (frustumRight + frustumLeft + eyeSeparation * (nearClip / convergence)) / (frustumRight - frustumLeft);

	m = inverseProjectionMatrixLeft;
	m[3][0] = (frustumRight + frustumLeft + eyeSeparation * (nearClip / convergence)) / (2.0f * nearClip);

	// calculate right matrices
	m = projectionMatrixRight;
	m[2][0] = (frustumRight + frustumLeft - eyeSeparation * (nearClip / convergence)) / (frustumRight - frustumLeft);

	m = inverseProjectionMatrixRight;
	m[3][0] = (frustumRight + frustumLeft - eyeSeparation * (nearClip / convergence)) / (2.0f * nearClip);
	
	projectionCached = true;
}
*/

