#include "camera.h"

mat4 alignZAxisWithTarget(vec3 targetDir, vec3 upDir);

const vec3 c_xAxis{ 1.0f, 0.0f, 0.0f };
const vec3 c_yAxis{ 0.0f, 1.0f, 0.0f };
const vec3 c_zAxis{ 0.0f, 0.0f, 1.0f };


///////////////////////////////////////////////////////////////////////////////////////////
// CameraPersp2

vec3 CameraPersp2::getEulerAngles() const
{
    return eulerAngles(viewRotationQuat);
}

void CameraPersp2::setTranslationYawPitchRoll(const dvec3& position, double yaw, double pitch, double roll)
{
    setEyePoint(position);
    dmat4 rotation = eulerAngleZ(roll);
    rotation *= eulerAngleXY(pitch, yaw);
    viewRotationQuat = quat_cast(rotation);
    
    // could be delayed?
    viewMatrix = translate(rotation, -eyePoint);
    viewProjectionMatrix = projectionMatrix * viewMatrix;
}

void CameraPersp2::lookAt(const dvec3 &position, const dvec3 &target, const dvec3 &worldUp)
{
    setEyePoint(position);
    viewMatrix = lookAt(eyePoint, target, worldUp);
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
    orientation = normalize(quat(alignZAxisWithTarget(-viewDirection, worldUp)));
    modelViewCached = false;
}

void Camera::lookAt(const dvec3 &target)
{
    viewDirection = normalize(target - eyePoint);
    orientation = normalize(quat(alignZAxisWithTarget(-viewDirection, worldUp)));
    modelViewCached = false;
}

void Camera::lookAt(const dvec3 &aEyePoint, const dvec3 &target)
{
    eyePoint = aEyePoint;
    viewDirection = normalize(target - eyePoint);
    orientation = normalize(quat(alignZAxisWithTarget(-viewDirection, worldUp)));
    modelViewCached = false;
}

void Camera::lookAt(const dvec3 &aEyePoint, const dvec3 &target, const dvec3 &aWorldUp)
{
    eyePoint = aEyePoint;
    worldUp = normalize(aWorldUp);
    viewDirection = normalize(target - eyePoint);
    orientation = normalize(quat(alignZAxisWithTarget(-viewDirection, worldUp)));
    modelViewCached = false;
}

void Camera::getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
    calcMatrices();

    vec3 viewDirection(normalize(viewDirection));

    *topLeft = eyePoint + dvec3((nearClip * viewDirection) + (frustumTop * mV) + (frustumLeft * mU));
    *topRight = eyePoint + dvec3((nearClip * viewDirection) + (frustumTop * mV) + (frustumRight * mU));
    *bottomLeft = eyePoint + dvec3((nearClip * viewDirection) + (frustumBottom * mV) + (frustumLeft * mU));
    *bottomRight = eyePoint + dvec3((nearClip * viewDirection) + (frustumBottom * mV) + (frustumRight * mU));
}

void Camera::getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
    calcMatrices();

    vec3 viewDirection(normalize(viewDirection));

    r32 ratio = farClip / nearClip;

    *topLeft = eyePoint + dvec3((farClip * viewDirection) + (ratio * frustumTop * mV) + (ratio * frustumLeft * mU));
    *topRight = eyePoint + dvec3((farClip * viewDirection) + (ratio * frustumTop * mV) + (ratio * frustumRight * mU));
    *bottomLeft = eyePoint + dvec3((farClip * viewDirection) + (ratio * frustumBottom * mV) + (ratio * frustumLeft * mU));
    *bottomRight = eyePoint + dvec3((farClip * viewDirection) + (ratio * frustumBottom * mV) + (ratio * frustumRight * mU));
}

void Camera::getFrustum(r32 *left, r32 *top, r32 *right, r32 *bottom, r32 *near, r32 *far)
{
    calcMatrices();

    *left = frustumLeft;
    *top = frustumTop;
    *right = frustumRight;
    *bottom = frustumBottom;
    *near = nearClip;
    *far = farClip;
}

//Ray Camera::generateRay(r32 uPos, r32 vPos, r32 imagePlaneApectRatio) const
//{
//	calcMatrices();

//	r32 s = (uPos - 0.5f) * imagePlaneApectRatio;
//	r32 t = (vPos - 0.5f);
//	r32 viewDistance = imagePlaneApectRatio / math<r32>::abs(frustumRight - frustumLeft) * nearClip;
//	return Ray(eyePoint, (mU * s + mV * t - (mW * viewDistance)).normalized());
//}

void Camera::getBillboardVectors(dvec3 *right, dvec3 *up) const
{
    auto& m = modelViewMatrix;
    right->x = m[0][0];
    right->y = m[1][0];
    right->z = m[2][0];
    up->x    = m[0][1];
    up->y    = m[1][1];
    up->z    = m[2][1];
}

vec2 Camera::worldToScreen(const dvec3 &worldCoord, r32 screenWidth, r32 screenHeight) const
{
    auto eye = _dvec4(worldCoord, 1.0) * modelViewMatrix;
    auto ndc = vec3(eye * projectionMatrix);

    return vec2{
        (ndc.x + 1.0f) / 2.0f * screenWidth,
        (1.0f - (ndc.y + 1.0f) / 2.0f) * screenHeight};
}

vec3 Camera::worldToEye(const dvec3 &worldCoord)
{
    return _vec3(_dvec4(worldCoord, 1.0) * modelViewMatrix);
}

r32 Camera::worldToEyeDepth(const dvec3 &worldCoord) const
{
    auto &m = modelViewMatrix;
    double d = m[0][2] * worldCoord.x + m[1][2] * worldCoord.y + m[2][2] * worldCoord.z + m[3][2];
    return static_cast<r32>(d);
}

vec3 Camera::worldToNdc(const dvec3 &worldCoord)
{
    auto eye = dvec4(worldCoord, 1.0) * modelViewMatrix;
    return vec3(eye * projectionMatrix);
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
    mW = normalize(-viewDirection);
    mU = orientation * c_xAxis;
    mV = orientation * c_yAxis;

    dvec3 d(dot(-eyePoint, dvec3(mU)), dot(-eyePoint, dvec3(mV)), dot(-eyePoint, dvec3(mW)));
    auto& m = mModelViewMatrix;
    m[0][0] = mU.x; m[1][0] = mU.y; m[2][0] = mU.z; m[3][0] = d.x;
    m[0][1] = mV.x; m[1][1] = mV.y; m[2][1] = mV.z; m[3][1] = d.y;
    m[0][2] = mW.x; m[1][2] = mW.y; m[2][2] = mW.z; m[3][2] = d.z;
    m[0][3] = 0.0;  m[1][3] = 0.0;  m[2][3] = 0.0;  m[3][3] = 1.0;

    modelViewCached = true;
    mInverseModelViewCached = false;
}

void Camera::calcInverseModelView()
{
    if (!modelViewCached) {
        calcModelView();
    }

    mInverseModelViewMatrix = affineInverse(mModelViewMatrix);
    mInverseModelViewCached = true;
}

////////////////////////////////////////////////////////////////////////////////////////
// CameraPersp

CameraPersp::CameraPersp(int pixelWidth, int pixelHeight, r32 fovDegrees) :
    Camera(),
    lensShift(0.0f)
{
    r32 eyeX = pixelWidth / 2.0f;
    r32 eyeY = pixelHeight / 2.0f;
    r32 halfFov = 3.14159f * fovDegrees / 360.0f;
    r32 theTan = tanf(halfFov);
    r32 dist = eyeY / theTan;
    r32 nearDist = dist / 10.0f;	// near / far clip plane
    r32 farDist = dist * 10.0f;
    r32 aspect = pixelWidth / (r32)pixelHeight;

    setPerspective(fovDegrees, aspect, nearDist, farDist);
    lookAt(vec3(eyeX, eyeY, dist), vec3(eyeX, eyeY, 0.0f));
}

CameraPersp::CameraPersp(int pixelWidth, int pixelHeight, r32 fovDegrees, r32 nearPlane, r32 farPlane) :
    Camera(),
    lensShift(0.0f)
{
    r32 halfFov, theTan, aspect;

    r32 eyeX = pixelWidth / 2.0f;
    r32 eyeY = pixelHeight / 2.0f;
    halfFov = PIf * fovDegrees / 360.0f;
    theTan = tanf(halfFov);
    r32 dist = eyeY / theTan;
    aspect = pixelWidth / (r32)pixelHeight;

    setPerspective(fovDegrees, aspect, nearPlane, farPlane);
    lookAt(vec3(eyeX, eyeY, dist), vec3(eyeX, eyeY, 0.0f));
}

// Creates a default camera resembling Maya Persp
CameraPersp::CameraPersp() :
    Camera(),
    lensShift(0.0f)
{
    lookAt(vec3(28.0f, 21.0f, 28.0f), vec3(0.0f), c_yAxis);
    setCenterOfInterest(44.822f);
    setPerspective(35.0f, 1.0f, 0.1f, 1000.0f);
}

void CameraPersp::setPerspective(r32 verticalFovDegrees, r32 aspectRatio, r32 nearPlane, r32 farPlane)
{
    fov = verticalFovDegrees;
    aspectRatio = aspectRatio;
    nearClip = nearPlane;
    farClip = farPlane;

    projectionCached = false;
}

void CameraPersp::calcProjection()
{
    frustumTop = nearClip * tanf(PIf / 180.0f * fov * 0.5f);
    frustumBottom = -frustumTop;
    frustumRight = frustumTop * aspectRatio;
    frustumLeft = -frustumRight;

    // perform lens shift
    if (lensShift.y != 0.0f) {
        frustumTop = glm::mix(0.0f, 2.0f * frustumTop, 0.5f + 0.5f * lensShift.y);
        frustumBottom = glm::mix(2.0f * frustumBottom, 0.0f, 0.5f + 0.5f * lensShift.y);
    }

    if (lensShift.x != 0.0f) {
        frustumRight = glm::mix(2.0f * frustumRight, 0.0f, 0.5f - 0.5f * lensShift.x);
        frustumLeft = glm::mix(0.0f, 2.0f * frustumLeft, 0.5f - 0.5f * lensShift.x);
    }

    projectionMatrix = perspective(radians(fov), aspectRatio, nearClip, farClip);
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
CameraOrtho::CameraOrtho() :
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
    mInverseModelViewCached = true;
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

////////////////////////////////////////////////////////////////////////////////////////
// CameraStereo

dvec3 CameraStereo::getEyePointShifted() const
{
    if (!mIsStereo) {
        return eyePoint;
    }
    if (mIsLeft) {
        return eyePoint - (dquat(orientation) * dvec3(c_xAxis) * (0.5 * mEyeSeparation));
    }
    return eyePoint + (dquat(orientation) * dvec3(c_xAxis) * (0.5 * mEyeSeparation));
}

void CameraStereo::getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
    calcMatrices();

    auto viewDirection = normalize(viewDirection);
    auto eye = getEyePointShifted();

    r32 shift = 0.5f * mEyeSeparation * (nearClip / mConvergence);
    shift *= (mIsStereo ? (mIsLeft ? 1.0f : -1.0f) : 0.0f);

    r32 left = frustumLeft + shift;
    r32 right = frustumRight + shift;

    *topLeft = eye + dvec3((nearClip * viewDirection) + (frustumTop * mV) + (left * mU));
    *topRight = eye + dvec3((nearClip * viewDirection) + (frustumTop * mV) + (right * mU));
    *bottomLeft = eye + dvec3((nearClip * viewDirection) + (frustumBottom * mV) + (left * mU));
    *bottomRight = eye + dvec3((nearClip * viewDirection) + (frustumBottom * mV) + (right * mU));
}

void CameraStereo::getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight)
{
    calcMatrices();

    vec3 viewDirection(normalize(viewDirection));

    r32 ratio = farClip / nearClip;

    vec3 eye(getEyePointShifted());

    r32 shift = 0.5f * mEyeSeparation * (nearClip / mConvergence);
    shift *= (mIsStereo ? (mIsLeft ? 1.0f : -1.0f) : 0.0f);

    r32 left = frustumLeft + shift;
    r32 right = frustumRight + shift;

    *topLeft = eye + (farClip * viewDirection) + (ratio * frustumTop * mV) + (ratio * left * mU);
    *topRight = eye + (farClip * viewDirection) + (ratio * frustumTop * mV) + (ratio * right * mU);
    *bottomLeft = eye + (farClip * viewDirection) + (ratio * frustumBottom * mV) + (ratio * left * mU);
    *bottomRight = eye + (farClip * viewDirection) + (ratio * frustumBottom * mV) + (ratio * right * mU);
}

const mat4& CameraStereo::projectionMatrix const
{
    assert(projectionCached);

    if (!mIsStereo) {
        return projectionMatrix;
    }
    else if (mIsLeft) {
        return projectionMatrixLeft;
    }
    return projectionMatrixRight;
}

const dmat4& CameraStereo::modelViewMatrix const
{
    assert(modelViewCached);

    if (!mIsStereo) {
        return mModelViewMatrix;
    }
    else if (mIsLeft) {
        return mModelViewMatrixLeft;
    }
    return mModelViewMatrixRight;
}

const dmat4& CameraStereo::getInverseModelViewMatrix() const
{
    assert(mInverseModelViewCached);

    if (!mIsStereo) {
        return mInverseModelViewMatrix;
    }
    else if (mIsLeft) {
        return mInverseModelViewMatrixLeft;
    }
    return mInverseModelViewMatrixRight;
}

void CameraStereo::calcModelView()
{
    // calculate default matrix first
    CameraPersp::calcModelView();

    mModelViewMatrixLeft = mModelViewMatrix;
    mModelViewMatrixRight = mModelViewMatrix;

    // calculate left matrix
    dvec3 eye = eyePoint - dvec3(orientation * c_xAxis * (0.5f * mEyeSeparation));
    dvec3 d = dvec3(dot(-eye, dvec3(mU)), dot(-eye, dvec3(mV)), dot(-eye, dvec3(mW)));

    auto& m = mModelViewMatrixLeft;
    m[3][0] = d.x; m[3][1] = d.y; m[3][2] = d.z;

    // calculate right matrix
    eye = eyePoint + dvec3(orientation * c_xAxis * (0.5f * mEyeSeparation));
    d = dvec3(dot(-eye, dvec3(mU)), dot(-eye, dvec3(mV)), dot(-eye, dvec3(mW)));

    m = mModelViewMatrixRight;
    m[3][0] = d.x; m[3][1] = d.y; m[3][2] = d.z;

    modelViewCached = true;
    mInverseModelViewCached = false;
}

void CameraStereo::calcInverseModelView()
{
    if (!modelViewCached) {
        calcModelView();
    }

    mInverseModelViewMatrix = affineInverse(mModelViewMatrix);
    mInverseModelViewMatrixLeft = affineInverse(mModelViewMatrixLeft);
    mInverseModelViewMatrixRight = affineInverse(mModelViewMatrixRight);
    mInverseModelViewCached = true;
}

void CameraStereo::calcProjection()
{
    // calculate default matrices first
    CameraPersp::calcProjection();

    projectionMatrixLeft = projectionMatrix;
    mInverseProjectionMatrixLeft = mInverseProjectionMatrix;

    projectionMatrixRight = projectionMatrix;
    mInverseProjectionMatrixRight = mInverseProjectionMatrix;

    // calculate left matrices
    auto& m = projectionMatrixLeft;
    m[2][0] = (frustumRight + frustumLeft + mEyeSeparation * (nearClip / mConvergence)) / (frustumRight - frustumLeft);

    m = mInverseProjectionMatrixLeft;
    m[3][0] = (frustumRight + frustumLeft + mEyeSeparation * (nearClip / mConvergence)) / (2.0f * nearClip);

    // calculate right matrices
    m = projectionMatrixRight;
    m[2][0] = (frustumRight + frustumLeft - mEyeSeparation * (nearClip / mConvergence)) / (frustumRight - frustumLeft);

    m = mInverseProjectionMatrixRight;
    m[3][0] = (frustumRight + frustumLeft - mEyeSeparation * (nearClip / mConvergence)) / (2.0f * nearClip);
    
    projectionCached = true;
}


mat4 alignZAxisWithTarget(vec3 targetDir, vec3 upDir)
{
    // Ensure that the target direction is non-zero.
    if (length2(targetDir) == 0) {
        targetDir = c_zAxis;
    }

    // Ensure that the up direction is non-zero.
    if (length2(upDir) == 0) {
        upDir = c_yAxis;
    }

    // Check for degeneracies.  If the upDir and targetDir are parallel 
    // or opposite, then compute a new, arbitrary up direction that is
    // not parallel or opposite to the targetDir.
    if (length2(cross(upDir, targetDir)) == 0) {
        upDir = cross(targetDir, c_xAxis);
        if (length2(upDir) == 0) {
            upDir = cross(targetDir, c_zAxis);
        }
    }

    // Compute the x-, y-, and z-axis vectors of the new coordinate system.
    vec3 targetPerpDir = cross(upDir, targetDir);
    vec3 targetUpDir = cross(targetDir, targetPerpDir);

    // Rotate the x-axis into targetPerpDir (row 0),
    // rotate the y-axis into targetUpDir   (row 1),
    // rotate the z-axis into targetDir     (row 2).
    vec3 row[3];
    row[0] = normalize(targetPerpDir);
    row[1] = normalize(targetUpDir);
    row[2] = normalize(targetDir);

    mat4 mat(row[0].x, row[0].y, row[0].z, 0,
                row[1].x, row[1].y, row[1].z, 0,
                row[2].x, row[2].y, row[2].z, 0,
                0, 0, 0, 1);

    return mat;
}