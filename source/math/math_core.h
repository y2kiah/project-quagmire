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


#endif