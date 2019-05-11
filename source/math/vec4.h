#ifndef _VEC4_H
#define _VEC4_H

#include "../utility/common.h"
#include "vec3.h"


struct vec4
{
	union {
		struct { r32 x, y, z, w; };
		struct { r32 r, g, b, a; };
		struct { r32 s, t, p, q; };
		r32 E[4];

		SWIZZLE_vec4(x, y, z, w)
		SWIZZLE_vec4(r, g, b, a)
		SWIZZLE_vec4(s, t, p, q)
	};

	r32& operator[](size_t e) { assert(e < 4); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 4); return E[e]; }
	
	vec4& operator=(const vec2& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *(vec4*)this;
	}
	
	vec4& operator=(const vec3& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *(vec4*)this;
	}

	vec4& operator=(const _vec4& rhs) {
		*this = (vec4&)rhs;
		return *this;
	}
};

static_assert(sizeof(vec4) == 16, "");


#endif