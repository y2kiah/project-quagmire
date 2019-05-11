#ifndef _VEC_H
#define _VEC_H

#include "../utility/types.h"

#define SWIZZLE_2_2_RW(Type, A, B, VecType) \
	struct swizzle_##A##B {\
		union {\
			struct { Type x, y; };\
			struct { Type u, v; };\
			struct { Type r, g; };\
			struct { Type s, t; };\
		};\
		vec2& operator=(const vec2& rhs) {\
			A = rhs.x;\
			B = rhs.y;\
			return *(vec2*)this;\
		}\
		operator vec2() { return vec2{ A, B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_2_2_RO(Type, A, B, vec2) \
	struct swizzle_##A##B {\
		union {\
			struct { Type x, y; };\
			struct { Type u, v; };\
			struct { Type r, g; };\
			struct { Type s, t; };\
		};\
		operator vec2() { return vec2{ A, B }; }\
	};\
	swizzle_##A##B A##B;


#define SWIZZLE_3_3_RW(Type, A, B, C, VecType) \
	struct swizzle_##A##B##C {\
		union {\
			struct { Type x, y, z; };\
			struct { Type r, g, b; };\
			struct { Type s, t, p; };\
		};\
		VecType& operator=(const VecType& rhs) {\
			A = rhs.x;\
			B = rhs.y;\
			C = rhs.z;\
			return *(VecType*)this;\
		}\
		operator VecType() { return VecType{ A, B, C }; }\
	};\
	swizzle_##A##B##C A##B##C;

#define SWIZZLE_3_3_RO(Type, A, B, C, VecType) \
	struct swizzle_##A##B##C {\
		union {\
			struct { Type x, y, z; };\
			struct { Type r, g, b; };\
			struct { Type s, t, p; };\
		};\
		operator VecType() { return VecType{ A, B, C }; }\
	};\
	swizzle_##A##B##C A##B##C;

#define SWIZZLE_3_2_RW(Type, A, B, PVecType, SVecType) \
	struct swizzle_##A##B {\
		union {\
			struct { Type x, y, z; };\
			struct { Type r, g, b; };\
			struct { Type s, t, p; };\
		};\
		PVecType& operator=(const SVecType& rhs) {\
			A = rhs.x;\
			B = rhs.y;\
			return *(PVecType*)this;\
		}\
		operator SVecType() { return SVecType{ A, B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_3_2_RO(Type, A, B, PVecType, SVecType) \
	struct swizzle_##A##B {\
		union {\
			struct { Type x, y, z; };\
			struct { Type r, g, b; };\
			struct { Type s, t, p; };\
		};\
		operator SVecType() { return SVecType{ A, B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_vec3(x, y, z) \
	SWIZZLE_3_3_RW(r32, x, z, y, vec3)\
	SWIZZLE_3_3_RW(r32, y, x, z, vec3)\
	SWIZZLE_3_3_RW(r32, y, z, x, vec3)\
	SWIZZLE_3_3_RW(r32, z, x, y, vec3)\
	SWIZZLE_3_3_RW(r32, z, y, x, vec3)\
	\
	SWIZZLE_3_3_RO(r32, x, x, x, vec3)\
	SWIZZLE_3_3_RO(r32, y, y, y, vec3)\
	SWIZZLE_3_3_RO(r32, z, z, z, vec3)\
	\
	SWIZZLE_3_3_RO(r32, x, x, y, vec3)\
	SWIZZLE_3_3_RO(r32, x, x, z, vec3)\
	SWIZZLE_3_3_RO(r32, x, y, x, vec3)\
	SWIZZLE_3_3_RO(r32, x, z, x, vec3)\
	SWIZZLE_3_3_RO(r32, y, x, x, vec3)\
	SWIZZLE_3_3_RO(r32, z, x, x, vec3)\
	\
	SWIZZLE_3_3_RO(r32, y, y, x, vec3)\
	SWIZZLE_3_3_RO(r32, y, y, z, vec3)\
	SWIZZLE_3_3_RO(r32, y, x, y, vec3)\
	SWIZZLE_3_3_RO(r32, y, z, y, vec3)\
	SWIZZLE_3_3_RO(r32, x, y, y, vec3)\
	SWIZZLE_3_3_RO(r32, z, y, y, vec3)\
	\
	SWIZZLE_3_2_RW(r32, x, y, vec3, vec2)\
	SWIZZLE_3_2_RW(r32, x, z, vec3, vec2)\
	SWIZZLE_3_2_RW(r32, y, x, vec3, vec2)\
	SWIZZLE_3_2_RW(r32, y, z, vec3, vec2)\
	SWIZZLE_3_2_RW(r32, z, x, vec3, vec2)\
	SWIZZLE_3_2_RW(r32, z, y, vec3, vec2)\
	\
	SWIZZLE_3_2_RO(r32, x, x, vec3, vec2)\
	SWIZZLE_3_2_RO(r32, y, y, vec3, vec2)\
	SWIZZLE_3_2_RO(r32, z, z, vec3, vec2)

struct vec2
{
	union {
		struct { r32 x, y; };
		struct { r32 u, v; };
		struct { r32 r, g; };
		struct { r32 s, t; };
		r32 E[2];

		SWIZZLE_2_2_RW(r32, y, x, vec2)
		SWIZZLE_2_2_RO(r32, x, x, vec2)
		SWIZZLE_2_2_RO(r32, y, y, vec2)
	};

	r32& operator[](size_t e) { assert(e < 2); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 2); return E[e]; }
};

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
};

struct vec4
{
	union {
		struct { r32 x, y, z, w; };
		struct { r32 r, g, b, a; };
		struct { r32 s, t, p, q; };
		r32 E[4];

		struct swizzle_xzw {
			r32 x, y, z, w;

			vec4& operator=(const vec4& rhs) {
				x = rhs.x;
				z = rhs.y;
				w = rhs.z;
				return *(vec4*)this;
			}

			operator vec3() { return vec3{ x, z, w }; }
		};
		swizzle_xzw xzw;
	};

	vec4& operator=(const vec3& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *(vec4*)this;
	}

	r32& operator[](size_t e) { assert(e < 4); return E[e]; }
	r32  operator[](size_t e) const { assert(e < 4); return E[e]; }
};


#endif