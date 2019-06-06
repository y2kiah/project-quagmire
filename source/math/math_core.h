#ifndef _MATH_CORE_H
#define _MATH_CORE_H

#include "../utility/common.h"
#include <cmath>


/**
 * Note: You will often see vector equality implemented in this way using FLT_EPSILON. That is
 * probably incorrect; see this article for details on why that is:
 * https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/.
 * 
 * It's expensive to do a truly safe and meaningful floating point comparison. It's best to use a
 * context-specific delta.
 * 
 * Here we are assuming a scale of meters and setting an absolute diff of 1/10th of a millimeter,
 * since this is probably small enough that a visual difference can't be discerned on screen
 * between two positions that close to each other.
 */
#define VEC_COMPARISON_DELTA		0.0001f
#define DVEC_COMPARISON_DELTA		0.0001

/**
 * Quaternion elements range -1 to 1, so epsilon is not a bad choice for quaternion comparisons.
 */
#define QUAT_COMPARISON_DELTA		FLT_EPSILON
#define DQUAT_COMPARISON_DELTA		DBL_EPSILON

#define PI							3.14159265358979323846264338327950288
#define PIf							3.14159265358979323846264338327950288f

#define INV_LN_HALFf				-1.44269504088896340736f	// 1 / ln(.5)

#define DEG_TO_RAD					0.01745329251994329576923690768489
#define DEG_TO_RADf					0.01745329251994329576923690768489f
#define RAD_TO_DEG					57.295779513082320876798154814105
#define RAD_TO_DEGf					57.295779513082320876798154814105f

#define SQRT_2						1.4142135623730950488016887242097
#define SQRT_2f						1.4142135623730950488016887242097f


struct _vec2 {
	union {
		struct { r32 x, y; };
		struct { r32 r, g; };
		struct { r32 s, t; };
	};
};

struct _vec3 {
	union {
		struct { r32 x, y, z; };
		struct { r32 r, g, b; };
		struct { r32 s, t, p; };
	};
};

struct _vec4 {
	union {
		struct { r32 x, y, z, w; };
		struct { r32 r, g, b, a; };
		struct { r32 s, t, p, q; };
	};
};

struct _dvec2 {
	union {
		struct { r64 x, y; };
		struct { r64 r, g; };
		struct { r64 s, t; };
	};
};

struct _dvec3 {
	union {
		struct { r64 x, y, z; };
		struct { r64 r, g, b; };
		struct { r64 s, t, p; };
	};
};

struct _dvec4 {
	union {
		struct { r64 x, y, z, w; };
		struct { r64 r, g, b, a; };
		struct { r64 s, t, p, q; };
	};
};


i32 clamp(i32 x, i32 minVal, i32 maxVal)
{
	return min(max(x, minVal), maxVal);
}

r32 clamp(r32 x, r32 minVal, r32 maxVal)
{
	return min(max(x, minVal), maxVal);
}

i64 clamp(i64 x, i64 minVal, i64 maxVal)
{
	return min(max(x, minVal), maxVal);
}

r64 clamp(r64 x, r64 minVal, r64 maxVal)
{
	return min(max(x, minVal), maxVal);
}

// Interpolation Functions

r32 mix(r32 x, r32 y, r32 t)
{
	return x + t * (y - x);
}

r64 mix(r64 x, r64 y, r64 t)
{
	return x + t * (y - x);
}

r32 lerp(r32 x, r32 y, r32 t)
{
	return x + t * (y - x);
}

r64 lerp(r64 x, r64 y, r64 t)
{
	return x + t * (y - x);
}

r32 step(r32 edge, r32 t)
{
	return (r32)(t >= edge);
}

r64 step(r64 edge, r64 t)
{
	return (r64)(t >= edge);
}

r32 smoothstep(r32 edge0, r32 edge1, r32 t)
{
	r32 tmp = clamp((t - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return tmp * tmp * (3.0f - 2.0f * tmp);
}

r64 smoothstep(r64 edge0, r64 edge1, r64 x)
{
	r64 tmp = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return tmp * tmp * (3.0 - 2.0 * tmp);
}

r32 boxStep(r32 a, r32 b, r32 t)
{
	assert(b != a);
	return clamp(0.0f, 1.0f, (t - a) / (b - a));
}

r32 pulse(r32 a, r32 b, r32 t)
{
	return (r32)((t >= a) - (t >= b));
}

r32 bias(r32 a, r32 b)
{
	return powf(a, logf(b) * INV_LN_HALFf);
}

r32 gamma(r32 a, r32 g)
{
	return powf(a, 1.0f / g);
}

r32 expose(r32 l, r32 k)
{
	return (1.0f - expf(-l * k));
}

// Cubic S-curve = 3t^2 - 2t^3 : 2nd derivative is discontinuous at t=0 and t=1 causing visual artifacts at boundaries
r32 sCurve(r32 t)
{
	return t * t * (3.0f - 2.0f * t);
}

// Cubic curve 1st derivative = 6t - 6t^2
r32 sCurveDeriv(r32 t)
{
	return 6.0f * t * (1.0f - t);
}

// Quintic curve = 6t^5 - 15t^4 + 10t^3
r32 qCurve(r32 t)
{
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Quintic curve 1st derivative = 30t^4 - 60t^3 + 30t^2
r32 qCurveDeriv(r32 t)
{
	return t * t * (t * (t * 30.0f - 60.0f) + 30.0f);
}

// Cosine curve
r32 cosCurve(r32 t)
{
	return (1.0f - cosf(t * PIf)) * 0.5f;
}

#endif