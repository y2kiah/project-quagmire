#ifndef _VEC3_H
#define _VEC3_H

#include "../utility/common.h"
#include "vec2.h"


struct vec3
{
	union {
		struct { r32 x, y, z; };
		struct { r32 r, g, b; };
		struct { r32 s, t, p; };
		r32 E[3];

		SWIZZLE_vec3(x, y, z)
		SWIZZLE_vec3(r, g, b)
		SWIZZLE_vec3(s, t, p)
	};

	r32& operator[](size_t e) { assert(e < 3); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 3); return E[e]; }

	vec3& operator=(const _vec3& rhs) {
		*this = (vec3&)rhs;
		return *this;
	}
};

static_assert(sizeof(vec3) == 12, "");


#endif