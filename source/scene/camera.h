#ifndef _CAMERA_H
#define _CAMERA_H

#include "../math/qmath.h"


struct CameraFrame
{
	mat4	projectionMatrix;
	mat4	inverseProjectionMatrix;
	bool	projectionCached;
	dmat4	modelViewMatrix;
	bool	modelViewCached;
	dmat4	inverseModelViewMatrix;
	bool	inverseModelViewCached;

	r32		frustumLeft;
	r32		frustumRight;
	r32		frustumTop;
	r32		frustumBottom;
};

struct Camera
{
	dvec3	eyePoint;
	vec3	viewDirection;
	quat	orientation;
	r64		centerOfInterest;
	vec3	worldUp;	// set to { 0.0f, 1.0f, 0.0f }

	r32		fov;
	r32		aspectRatio;
	r32		nearClip;
	r32		farClip;

	vec3	right;		// Right vector
	vec3	up;			// Up vector
	vec3	back;		// Negative view direction

	vec2	lensShift;

	// TODO: move these to CameraFrame?
	mat4	projectionMatrix;
	mat4	inverseProjectionMatrix;
	bool	projectionCached;
	dmat4	modelViewMatrix;
	bool	modelViewCached;
	dmat4	inverseModelViewMatrix;
	bool	inverseModelViewCached;

	r32		frustumLeft;
	r32		frustumRight;
	r32		frustumTop;
	r32		frustumBottom;

	quat	viewRotationQuat;
	mat4	viewMatrix;
	mat4	viewProjectionMatrix;


	void setEyePoint(const dvec3 &aEyePoint);

	void setCenterOfInterest(r64 aCenterOfInterest) { centerOfInterest = aCenterOfInterest; }

	dvec3 getCenterOfInterestPoint() const
	{
		return eyePoint
			+ (dvec3{ viewDirection.x, viewDirection.y, viewDirection.z }
				* centerOfInterest);
	}
	void setCenterOfInterestPoint(const dvec3 &centerOfInterestPoint);

	void setWorldUp(const vec3 &aWorldUp);

	void lookAt(const dvec3 &target);
	void lookAt(const dvec3 &aEyePoint, const dvec3 &target);
	void lookAt(const dvec3 &aEyePoint, const dvec3 &target, const dvec3 &aUp);
	void setViewDirection(const vec3 &aViewDirection);
	void setOrientation(const quat &aOrientation);
	void setFov(r32 aFov) { fov = aFov;  projectionCached = false; }
	
	r32 getFovHorizontal() const
	{
		return RAD_TO_DEGf * (2.0f * atanf(tanf(fov * DEG_TO_RADf * 0.5f) * aspectRatio));
	}
	
	void setFovHorizontal(r32 aFov)
	{
		fov = RAD_TO_DEGf * (2.0f * atanf(tanf(aFov * DEG_TO_RADf * 0.5f) / aspectRatio));
		projectionCached = false;
	}

	void setAspectRatio(r32 aAspectRatio) { aspectRatio = aAspectRatio; projectionCached = false; }
	void setNearClip(r32 aNearClip) { nearClip = aNearClip; projectionCached = false; }
	void setFarClip(r32 aFarClip) { farClip = aFarClip; projectionCached = false; }

	void getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);
	void getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);

	// Returns the coordinates of the camera's frustum, suitable for passing to \c glFrustum
	void getFrustum(
		r32 *leftPlane,
		r32 *topPlane,
		r32 *rightPlane,
		r32 *bottomPlane,
		r32 *nearPlane,
		r32 *farPlane);
	
	//Ray generateRay(r32 u, r32 v, r32 imagePlaneAspectRatio) const;
	void getBillboardVectors(dvec3 *billboardRight, dvec3 *billboardUp) const;

	// Converts a world-space coordinate \a worldCoord to screen coordinates as viewed by the camera, based ona s screen which is \a screenWidth x \a screenHeight pixels.
	vec2 worldToScreen(const dvec3 &worldCoord, r32 screenWidth, r32 screenHeight) const;
	// Converts a world-space coordinate \a worldCoord to eye-space, also known as camera-space. -Z is along the view direction.
	vec3 worldToEye(const dvec3 &worldCoord);
	// Converts a world-space coordinate \a worldCoord to the z axis of eye-space, also known as camera-space. -Z is along the view direction. Suitable for depth sorting.
	r32	worldToEyeDepth(const dvec3 &worldCoord) const;
	// Converts a world-space coordinate \a worldCoord to normalized device coordinates
	vec3 worldToNdc(const dvec3 &worldCoord);

	//r32 getScreenRadius(const class Sphere &sphere, r32 screenWidth, r32 screenHeight) const;
	void calcModelView();
	void calcInverseModelView();
	void calcPerspProjection();

	void calcMatrices()
	{
		if (!modelViewCached) { calcModelView(); }
		if (!projectionCached) { calcPerspProjection(); }
		// note: calculation of the inverse modelview matrices is postponed until actually requested
		//if (!mInverseModelViewCached) { calcInverseModelView(); }
	}
};


struct CameraPersp : public Camera
{
	vec3 getEulerAngles() const;

	void setTranslationYawPitchRoll(
		const dvec3& position,
		r64 yaw,
		r64 pitch,
		r64 roll);

	void lookAt(
		const dvec3 &position,
		const dvec3 &target);

	void extractFrustumPlanes();

	void setPerspective(
		r32 verticalFovDegrees,
		r32 aspect,
		r32 nearPlane,
		r32 farPlane);
	
	void setOrtho(
		r32 left,
		r32 right,
		r32 bottom,
		r32 top,
		r32 nearPlane,
		r32 farPlane);

	/** Returns both the horizontal and vertical lens shift.
	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
	void getLensShift(r32 *horizontal, r32 *vertical) const { *horizontal = lensShift.x; *vertical = lensShift.y; }
	
	/** Returns both the horizontal and vertical lens shift.
	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
	vec2 getLensShift() const { return vec2{ lensShift.x, lensShift.y }; }
	
	/** Sets both the horizontal and vertical lens shift.
	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
	void setLensShift(r32 horizontal, r32 vertical);
	
	/** Sets both the horizontal and vertical lens shift.
	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
	void setLensShift(const vec2 &shift) { setLensShift(shift.x, shift.y); }
	
	// Returns the horizontal lens shift. A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
	r32	getLensShiftHorizontal() const { return lensShift.x; }
	
	/** Sets the horizontal lens shift.
	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport. */
	void setLensShiftHorizontal(r32 horizontal) { setLensShift(horizontal, lensShift.y); }
	
	// Returns the vertical lens shift. A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport.
	r32	getLensShiftVertical() const { return lensShift.y; }
	
	/** Sets the vertical lens shift.
	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
	void setLensShiftVertical(r32 vertical) { setLensShift(lensShift.x, vertical); }

	//CameraPersp	getFrameSphere(const class Sphere &worldSpaceSphere, int maxIterations = 20) const;
};

// struct CameraStereo : public CameraPersp
// {
// 	mat4		projectionMatrixLeft, inverseProjectionMatrixLeft;
// 	mat4		projectionMatrixRight, inverseProjectionMatrixRight;
// 	dmat4		modelViewMatrixLeft, inverseModelViewMatrixLeft;
// 	dmat4		modelViewMatrixRight, inverseModelViewMatrixRight;

// 	bool		isStereo;
// 	bool		isLeft;

// 	r32			convergence;
// 	r32			eyeSeparation;


// 	CameraStereo() :
// 		convergence(1.0f), eyeSeparation(0.05f), isStereo(false), isLeft(true)
// 	{}
// 	CameraStereo(int pixelWidth, int pixelHeight, r32 fov) :
// 		CameraPersp(pixelWidth, pixelHeight, fov),
// 		convergence(1.0f), eyeSeparation(0.05f), isStereo(false), isLeft(true) // constructs screen-aligned camera
// 	{}
// 	CameraStereo(int pixelWidth, int pixelHeight, r32 fov, r32 nearPlane, r32 farPlane) :
// 		CameraPersp(pixelWidth, pixelHeight, fov, nearPlane, farPlane),
// 		convergence(1.0f), eyeSeparation(0.05f), isStereo(false), isLeft(true) // constructs screen-aligned camera
// 	{}

// 	// Returns the current convergence, which is the distance at which there is no parallax.
// 	r32			getConvergence() const { return convergence; }
// 	// Sets the convergence of the camera, which is the distance at which there is no parallax.
// 	void			setConvergence(r32 distance, bool adjustEyeSeparation = false) {
// 						convergence = distance; projectionCached = false;
// 						if (adjustEyeSeparation) { eyeSeparation = convergence / 30.0f; }
// 					}
// 	// Returns the distance between the camera's for the left and right eyes.
// 	r32			getEyeSeparation() const { return eyeSeparation; }
// 	// Sets the distance between the camera's for the left and right eyes. This affects the parallax effect. 
// 	void			setEyeSeparation(r32 distance) { eyeSeparation = distance; modelViewCached = false; projectionCached = false; }
// 	// Returns the location of the currently enabled eye camera.
// 	dvec3			getEyePointShifted() const;

// 	// Enables the left eye camera.
// 	void			enableStereoLeft() { isStereo = true; isLeft = true; }
// 	bool			isStereoLeftEnabled() const { return isStereo && isLeft; }
// 	// Enables the right eye camera.
// 	void			enableStereoRight() { isStereo = true; isLeft = false; }
// 	bool			isStereoRightEnabled() const { return isStereo && !isLeft; }
// 	// Disables stereoscopic rendering, converting the camera to a standard CameraPersp.
// 	void			disableStereo() { isStereo = false; }
// 	bool			isStereoEnabled() const { return isStereo; }

// 	virtual void	getNearClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);
// 	virtual void	getFarClipCoordinates(dvec3 *topLeft, dvec3 *topRight, dvec3 *bottomLeft, dvec3 *bottomRight);

// 	virtual const mat4&		getProjectionMatrix() const;
// 	virtual const dmat4&	getModelViewMatrix() const;
// 	virtual const dmat4&	getInverseModelViewMatrix() const;

// 	virtual void	calcModelView();
// 	virtual void	calcInverseModelView();
// 	virtual void	calcProjection();
// };

#endif