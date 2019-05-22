#ifndef _CAMERA_H
#define _CAMERA_H

#include "../math/qmath.h"


struct CameraFrame
{
	dmat4	view;				// transform from world space to view space (camera at origin)
	dmat4	inverseView;		// transform from view space to world space
	mat4	projection;
	mat4	inverseProjection;
	mat4	viewProjection;
	quat	viewRotationQuat;
};

struct Camera
{
	dvec3	eyePoint;
	vec3	viewDirection;
	quat	orientation;
	
	vec3	worldUp;

	r32		fovDegreesVertical;
	r32		aspectRatio;
	r32		nearClip;
	r32		farClip;

	CameraFrame	frame;
};


void calcPerspProjection(
	Camera& cam,
	r32 fovDegreesVertical,
	r32 aspectRatio,
	r32 nearClip,
	r32 farClip);

void setViewDirection(
	Camera& cam,
	const vec3 &viewDirection);

void setOrientation(
	Camera& cam,
	const quat &orientation);

void lookAt(
	Camera& cam,
	const dvec3 &eyePoint,
	const dvec3 &target,
	const dvec3 &worldUp);

/**
 * Calculates matrices related to viewspace (camera centered at origin).
 */
void calcView(
	Camera& cam);

CameraFrame& calcCameraFrame(
	Camera& cam,
	const dvec3& eyePoint,
	const quat& orientation);

Camera makePerspectiveCamera(
	r32 aspectRatio,
	r32 fovDegreesVertical,
	r32 nearClip,
	r32 farClip,
	const dvec3& eyePoint,
	const quat& orientation);

r32 getFovDegreesHorizontal(
	r32 fovDegreesVertical,
	r32 aspectRatio);

// struct CameraPersp : public Camera
// {
// 	vec3 getEulerAngles() const;

// 	void setTranslationYawPitchRoll(
// 		const dvec3& position,
// 		r64 yaw,
// 		r64 pitch,
// 		r64 roll);

// 	void lookAt(
// 		const dvec3 &position,
// 		const dvec3 &target);

// 	void extractFrustumPlanes();

// 	void setPerspective(
// 		r32 verticalFovDegrees,
// 		r32 aspect,
// 		r32 nearPlane,
// 		r32 farPlane);
	
// 	void setOrtho(
// 		r32 left,
// 		r32 right,
// 		r32 bottom,
// 		r32 top,
// 		r32 nearPlane,
// 		r32 farPlane);

// 	/** Returns both the horizontal and vertical lens shift.
// 	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
// 	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
// 	void getLensShift(r32 *horizontal, r32 *vertical) const { *horizontal = lensShift.x; *vertical = lensShift.y; }
	
// 	/** Returns both the horizontal and vertical lens shift.
// 	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
// 	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
// 	vec2 getLensShift() const { return vec2{ lensShift.x, lensShift.y }; }
	
// 	/** Sets both the horizontal and vertical lens shift.
// 	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
// 	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
// 	void setLensShift(r32 horizontal, r32 vertical);
	
// 	/** Sets both the horizontal and vertical lens shift.
// 	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
// 	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
// 	void setLensShift(const vec2 &shift) { setLensShift(shift.x, shift.y); }
	
// 	// Returns the horizontal lens shift. A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport.
// 	r32	getLensShiftHorizontal() const { return lensShift.x; }
	
// 	/** Sets the horizontal lens shift.
// 	A horizontal lens shift of 1 (-1) will shift the view right (left) by half the width of the viewport. */
// 	void setLensShiftHorizontal(r32 horizontal) { setLensShift(horizontal, lensShift.y); }
	
// 	// Returns the vertical lens shift. A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport.
// 	r32	getLensShiftVertical() const { return lensShift.y; }
	
// 	/** Sets the vertical lens shift.
// 	A vertical lens shift of 1 (-1) will shift the view up (down) by half the height of the viewport. */
// 	void setLensShiftVertical(r32 vertical) { setLensShift(lensShift.x, vertical); }

// 	//CameraPersp	getFrameSphere(const class Sphere &worldSpaceSphere, int maxIterations = 20) const;
// };

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