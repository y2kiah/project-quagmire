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

#define DEG_TO_RAD					0.01745329251994329576923690768489
#define DEG_TO_RADf					0.01745329251994329576923690768489f
#define RAD_TO_DEG					57.295779513082320876798154814105
#define RAD_TO_DEGf					57.295779513082320876798154814105f


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


r32 clamp(r32 x, r32 minVal, r32 maxVal)
{
	return min(max(x, minVal), maxVal);
}

r32 mix(r32 x, r32 y, r32 a)
{
	return x + a * (y - x);
}

r32 lerp(r32 x, r32 y, r32 a)
{
	return x + a * (y - x);
}

r32 step(r32 edge, r32 x)
{
	return mix(1.0f, 0.0f, (x < edge));
}

r32 smoothstep(r32 edge0, r32 edge1, r32 x)
{
	r32 tmp = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return tmp * tmp * (3.0f - 2.0f * tmp);
}


#endif