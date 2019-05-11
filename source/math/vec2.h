#ifndef _VEC2_H
#define _VEC2_H

#include "../utility/common.h"
#include "swizzle.h"


struct vec2
{
	union {
		struct { r32 x, y; };
		struct { r32 r, g; };
		struct { r32 s, t; };
		r32 E[2];

		SWIZZLE_vec2(x, y)
		SWIZZLE_vec2(r, g)
		SWIZZLE_vec2(s, t)
	};

	r32& operator[](size_t e) { assert(e < 2); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 2); return E[e]; }

	vec2& operator=(const _vec2& rhs) {
		*this = (vec2&)rhs;
		return *this;
	}
};

static_assert(sizeof(vec2) == 8, "");


#endif