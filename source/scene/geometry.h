#ifndef _GEOMETRY_H
#define _GEOMETRY_H

#include "utility/types.h"
#include "math/conversions.h"


// Sphere

/**
 * Represent sphere in traditional AoS format, useful for testing frustum intersection using the
 * frustum_intersectSpheres_sse function.
 */
struct Sphere {
	union {
		struct {
			vec3	center;
			r32		radius;
		};
		r32			E[4];
	};
};


// Plane

struct Plane {
	// plane equation nx*x+ny*y+nz*z = d
	// put another way, n*p - d = 0  where p is any point on the plane
	union {
		struct { r32 nx, ny, nz, d; };
		struct { vec3 n; r32 d; };
	};
};

/**
 * In order to obtain a "true" distance, the plane must be normalized.
 * Returns distance in units of magnitude of the plane's normal vector {nx,ny,nz}.
 */
r32 plane_distanceToPoint(
	const Plane& plane,
	vec3 p)
{
	return dot(plane.n, p) - plane.d;
}

void plane_normalize(
	Plane& plane)
{
	assert(plane.nx != 0.0f || plane.ny != 0.0f || plane.nz != 0.0f);
	r32 iMag = 1.0f / length(plane.n);
	plane.n *= iMag;
}

/**
 * nx, ny, nz and d arrays must be 16-byte aligned
 */
void plane_normalize_4_sse(
	r32 nx[4],
	r32 ny[4],
	r32 nz[4],
	r32 d[4])
{
	const __m128 mm_nx = _mm_load_ps(nx);
	const __m128 mm_ny = _mm_load_ps(ny);
	const __m128 mm_nz = _mm_load_ps(nz);
	const __m128 mm_d  = _mm_load_ps(d);

	// n*n
	__m128 mm_dx = _mm_mul_ps(mm_nx, mm_nx); // dx = nx * nx
	__m128 mm_dy = _mm_mul_ps(mm_ny, mm_ny); // dy = ny * ny
	__m128 mm_dz = _mm_mul_ps(mm_nz, mm_nz); // dz = nz * nz
	__m128 dots  = _mm_add_ps(_mm_add_ps(mm_dx, mm_dy), mm_dz); // dots = dx + dy + dz
	// sqrt(dots)
	__m128 lens  = _mm_sqrt_ps(dots);
	// n / lens
	mm_dx        = _mm_div_ps(mm_nx, lens); // dx = nx / len(n)
	mm_dy        = _mm_div_ps(mm_ny, lens); // dy = ny / len(n)
	mm_dz        = _mm_div_ps(mm_nz, lens); // dz = nz / len(n)
	__m128 mm_dd = _mm_div_ps(mm_d,  lens); // a0 = d  / len(n)

	_mm_store_ps(nx, mm_dx);
	_mm_store_ps(ny, mm_dy);
	_mm_store_ps(nz, mm_dz);
	_mm_store_ps(d, mm_dd);
}

Plane plane_fromPoints(
	const vec3& p1,
	const vec3& p2,
	const vec3& p3)
{
	vec3 v1 = p2 - p1;
	vec3 v2 = p3 - p1;
	v1 = normalize(cross(v1, v2));
	return Plane{ v1.x, v1.y, v1.z, dot(v1, p1) };
}


// Frustum

enum FrustumPlane : u8
{
	Near = 0, Far, Left, Right, Top, Bottom
};


struct alignas(16) Frustum
{
	// planes stored as SoA instead of AoS
	r32 nx[6];
	r32 ny[6];
	r32 nz[6];
	r32 d[6];
};


struct FrustumPoints {
	union {
		struct { dvec3 eye, ftl, ftr, fbl, fbr, ntl, ntr, nbl, nbr; };
		dvec3 E[9];
	};
};


Plane frustum_getPlane(
	const Frustum& f,
	FrustumPlane p)
{
	return Plane{ f.nx[p], f.ny[p], f.nz[p], f.d[p] };
}

void frustum_getPlanes(
	const Frustum& f,
	Plane* outPlanes)
{
	assert(outPlanes);
	for (int p = 0; p < 6; ++p) {
		Plane &plane = outPlanes[p];
		plane.nx = f.nx[p];
		plane.ny = f.ny[p];
		plane.nz = f.nz[p];
		plane.d = f.d[p];
	}
}

void frustum_getPoints(
	const dmat4& view,
	const dvec3& eyePoint, // camera position
	r64 nearClip,
	r64 farClip,
	r32 fovDegreesVertical,
	r32 aspectRatio,
	FrustumPoints& outPoints)
{
	dvec3 f, u, r;
	getForwardUpRight(view, f, u, r);
	f.normalize();
	u.normalize();
	r.normalize();
	
	// get the half-width and half-height of the near and far planes
	r64 hNear = tan(fovDegreesVertical * DEG_TO_RAD * 0.5) * nearClip;
	r64 wNear = hNear * aspectRatio;
	r64 hFar  = tan(fovDegreesVertical * DEG_TO_RAD * 0.5) * farClip;
	r64 wFar  = hFar * aspectRatio;
	// get center point on planes along forward vector
	dvec3 cNear = eyePoint + (f * nearClip);
	dvec3 cFar  = eyePoint + (f * farClip);
	// get up and right vectors scaled by near and far planes
	dvec3 uNear = u * hNear;
	dvec3 rNear = r * hNear;
	dvec3 uFar  = u * hFar;
	dvec3 rFar  = r * hFar;

	outPoints.eye = eyePoint;
	outPoints.ftl = cFar  + uFar  - rFar;	// Far Top Left
	outPoints.ftr = cFar  + uFar  + rFar;	// Far Top Right
	outPoints.fbl = cFar  - uFar  - rFar;	// Far Bottom Left
	outPoints.fbr = cFar  - uFar  + rFar;	// Far Bottom Right
	outPoints.ntl = cNear + uNear - rNear;	// Near Top Left
	outPoints.ntr = cNear + uNear + rNear;	// Near Top Right
	outPoints.nbl = cNear - uNear - rNear;	// Near Bottom Left
	outPoints.nbr = cNear + uNear + rNear;	// Near Bottom Right
}

/**
 * Matrix is column-major order for GL, row-major for D3D.
 * For input of projection matrix, planes are extracted in view/camera space.
 * For input of view*projection matrix, world space.
 * For input of model*view*projection matrix, object space.
 * see "Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix"
 */
Frustum frustum_extractFromMatrixGL(
	r32 matrix[16],
	bool normalize = true)
{
	Frustum f;
	const MatrixColumnMajor& m = *(MatrixColumnMajor*)matrix;
				
	f.nx[Near]   = m._41 + m._31;
	f.ny[Near]   = m._42 + m._32;
	f.nz[Near]   = m._43 + m._33;
	f.d[Near]    = m._44 + m._34;

	f.nx[Far]    = m._41 + m._31;
	f.ny[Far]    = m._42 + m._32;
	f.nz[Far]    = m._43 + m._33;
	f.d[Far]     = m._44 + m._34;

	f.nx[Left]   = m._41 + m._11;
	f.ny[Left]   = m._42 + m._12;
	f.nz[Left]   = m._43 + m._13;
	f.d[Left]    = m._44 + m._14;

	f.nx[Right]  = m._41 - m._11;
	f.ny[Right]  = m._42 - m._12;
	f.nz[Right]  = m._43 - m._13;
	f.d[Right]   = m._44 - m._14;

	f.nx[Top]    = m._41 - m._21;
	f.ny[Top]    = m._42 - m._22;
	f.nz[Top]    = m._43 - m._23;
	f.d[Top]     = m._44 - m._24;

	f.nx[Bottom] = m._41 + m._21;
	f.ny[Bottom] = m._42 + m._22;
	f.nz[Bottom] = m._43 + m._23;
	f.d[Bottom]  = m._44 + m._24;

	if (normalize) {
		plane_normalize_4_sse(f.nx, f.ny, f.nz, f.d);
		plane_normalize_4_sse(f.nx+2, f.ny+2, f.nz+2, f.d+2);

		// Plane planes[6] = {};
		// frustum_getPlanes(f, planes);
		// for (int p = 0; p < 6; ++p) {
		// 	plane_normalize(planes[p]);
		// 	f.nx[p] = planes[p].nx;
		// 	f.ny[p] = planes[p].ny;
		// 	f.nz[p] = planes[p].nz;
		// 	f.d[p] = planes[p].d;
		// }
	}
	return f;
}

Frustum frustum_extractFromMatrixD3D(
	r32 matrix[16],
	bool normalize = true)
{
	Frustum f;
	const MatrixRowMajor& m = *(MatrixRowMajor*)matrix;
				
	f.nx[Near]   = m._13;
	f.ny[Near]   = m._23;
	f.nz[Near]   = m._33;
	f.d[Near]    = m._43;

	f.nx[Far]    = m._14 - m._13;
	f.ny[Far]    = m._24 - m._23;
	f.nz[Far]    = m._34 - m._33;
	f.d[Far]     = m._44 - m._43;

	f.nx[Left]   = m._14 + m._11;
	f.ny[Left]   = m._24 + m._21;
	f.nz[Left]   = m._34 + m._31;
	f.d[Left]    = m._44 + m._41;

	f.nx[Right]  = m._14 - m._11;
	f.ny[Right]  = m._24 - m._21;
	f.nz[Right]  = m._34 - m._31;
	f.d[Right]   = m._44 - m._41;

	f.nx[Top]    = m._14 - m._12;
	f.ny[Top]    = m._24 - m._22;
	f.nz[Top]    = m._34 - m._32;
	f.d[Top]     = m._44 - m._42;

	f.nx[Bottom] = m._14 + m._12;
	f.ny[Bottom] = m._24 + m._22;
	f.nz[Bottom] = m._34 + m._32;
	f.d[Bottom]  = m._44 + m._42;

	if (normalize) {
		plane_normalize_4_sse(f.nx, f.ny, f.nz, f.d);
		plane_normalize_4_sse(f.nx+2, f.ny+2, f.nz+2, f.d+2);

		// Plane planes[6] = {};
		// frustum_getPlanes(f, planes);
		// for (int p = 0; p < 6; ++p) {
		// 	plane_normalize(planes[p]);
		// 	f.nx[p] = planes[p].nx;
		// 	f.ny[p] = planes[p].ny;
		// 	f.nz[p] = planes[p].nz;
		// 	f.d[p] = planes[p].d;
		// }
	}
	return f;
}


#endif