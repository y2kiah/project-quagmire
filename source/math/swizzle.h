#ifndef _SWIZZLE_H
#define _SWIZZLE_H

#include "../utility/common.h"

/**
 * Storage classes
 */

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


/**
 * vec2 swizzling
 */

#define SWIZZLE_2_2_RW(A, B, VecType) \
	struct swizzle_##A##B {\
		_##VecType V;\
		VecType& operator=(const VecType& rhs) {\
			V.A = rhs.x;\
			V.B = rhs.y;\
			return *(VecType*)this;\
		}\
		operator VecType() { return VecType{ V.A, V.B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_2_2_RO(A, B, VecType) \
	struct swizzle_##A##B {\
		_##VecType V;\
		operator VecType() { return VecType{ V.A, V.B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_2_3_RO(A, B, C, VecType, SVecType) \
	struct swizzle_##A##B##C {\
		_##VecType V;\
		operator SVecType() { return SVecType{ V.A, V.B, V.C }; }\
	};\
	swizzle_##A##B##C A##B##C;

#define SWIZZLE_2_4_RO(A, B, C, D, VecType, SVecType) \
	struct swizzle_##A##B##C##D {\
		_##VecType V;\
		operator SVecType() { return SVecType{ V.A, V.B, V.C, V.D }; }\
	};\
	swizzle_##A##B##C##D A##B##C##D;


#define SWIZZLE_vec2(x, y) \
	SWIZZLE_2_2_RW(y, x, vec2)\
	SWIZZLE_2_2_RO(x, x, vec2)\
	SWIZZLE_2_2_RO(y, y, vec2)\
	\
	SWIZZLE_2_3_RO(x, x, x, vec2, _vec3)\
	SWIZZLE_2_3_RO(x, x, y, vec2, _vec3)\
	SWIZZLE_2_3_RO(x, y, x, vec2, _vec3)\
	SWIZZLE_2_3_RO(y, x, x, vec2, _vec3)\
	SWIZZLE_2_3_RO(y, y, y, vec2, _vec3)\
	SWIZZLE_2_3_RO(y, y, x, vec2, _vec3)\
	SWIZZLE_2_3_RO(y, x, y, vec2, _vec3)\
	SWIZZLE_2_3_RO(x, y, y, vec2, _vec3)\
	\
	SWIZZLE_2_4_RO(x, x, x, x, vec2, _vec4)\
	SWIZZLE_2_4_RO(x, x, x, y, vec2, _vec4)\
	SWIZZLE_2_4_RO(x, x, y, x, vec2, _vec4)\
	SWIZZLE_2_4_RO(x, y, x, x, vec2, _vec4)\
	SWIZZLE_2_4_RO(y, x, x, x, vec2, _vec4)\
	SWIZZLE_2_4_RO(x, x, y, y, vec2, _vec4)\
	SWIZZLE_2_4_RO(x, y, x, y, vec2, _vec4)\
	SWIZZLE_2_4_RO(y, x, x, y, vec2, _vec4)\
	SWIZZLE_2_4_RO(y, y, x, x, vec2, _vec4)\
	SWIZZLE_2_4_RO(x, y, y, x, vec2, _vec4)\
	SWIZZLE_2_4_RO(x, y, y, y, vec2, _vec4)\
	SWIZZLE_2_4_RO(y, x, y, y, vec2, _vec4)\
	SWIZZLE_2_4_RO(y, y, x, y, vec2, _vec4)\
	SWIZZLE_2_4_RO(y, y, y, x, vec2, _vec4)\
	SWIZZLE_2_4_RO(y, y, y, y, vec2, _vec4)


/**
 * vec3 swizzling
 */

#define SWIZZLE_3_3_RW(A, B, C, VecType) \
	struct swizzle_##A##B##C {\
		_##VecType V;\
		VecType& operator=(const VecType& rhs) {\
			V.A = rhs.x;\
			V.B = rhs.y;\
			V.C = rhs.z;\
			return *(VecType*)this;\
		}\
		operator VecType() { return VecType{ V.A, V.B, V.C }; }\
	};\
	swizzle_##A##B##C A##B##C;

#define SWIZZLE_3_3_RO(A, B, C, VecType) \
	struct swizzle_##A##B##C {\
		_##VecType V;\
		operator VecType() { return VecType{ V.A, V.B, V.C }; }\
	};\
	swizzle_##A##B##C A##B##C;

#define SWIZZLE_3_2_RW(A, B, VecType, SVecType) \
	struct swizzle_##A##B {\
		_##VecType V;\
		VecType& operator=(const SVecType& rhs) {\
			V.A = rhs.x;\
			V.B = rhs.y;\
			return *(VecType*)this;\
		}\
		operator SVecType() { return SVecType{ V.A, V.B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_3_2_RO(A, B, VecType, SVecType) \
	struct swizzle_##A##B {\
		_##VecType V;\
		operator SVecType() { return SVecType{ V.A, V.B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_3_4_RO(A, B, C, D, VecType, SVecType) \
	struct swizzle_##A##B##C##D {\
		_##VecType V;\
		operator SVecType() { return SVecType{ V.A, V.B, V.C, V.D }; }\
	};\
	swizzle_##A##B##C##D A##B##C##D;


#define SWIZZLE_vec3(x, y, z) \
	SWIZZLE_3_3_RW(x, z, y, vec3)\
	SWIZZLE_3_3_RW(y, x, z, vec3)\
	SWIZZLE_3_3_RW(y, z, x, vec3)\
	SWIZZLE_3_3_RW(z, x, y, vec3)\
	SWIZZLE_3_3_RW(z, y, x, vec3)\
	\
	SWIZZLE_3_3_RO(x, x, x, vec3)\
	SWIZZLE_3_3_RO(y, y, y, vec3)\
	SWIZZLE_3_3_RO(z, z, z, vec3)\
	\
	SWIZZLE_3_3_RO(x, x, y, vec3)\
	SWIZZLE_3_3_RO(x, x, z, vec3)\
	SWIZZLE_3_3_RO(x, y, x, vec3)\
	SWIZZLE_3_3_RO(x, z, x, vec3)\
	SWIZZLE_3_3_RO(y, x, x, vec3)\
	SWIZZLE_3_3_RO(z, x, x, vec3)\
	\
	SWIZZLE_3_3_RO(y, y, x, vec3)\
	SWIZZLE_3_3_RO(y, y, z, vec3)\
	SWIZZLE_3_3_RO(y, x, y, vec3)\
	SWIZZLE_3_3_RO(y, z, y, vec3)\
	SWIZZLE_3_3_RO(x, y, y, vec3)\
	SWIZZLE_3_3_RO(z, y, y, vec3)\
	\
	SWIZZLE_3_2_RW(x, y, vec3, vec2)\
	SWIZZLE_3_2_RW(x, z, vec3, vec2)\
	SWIZZLE_3_2_RW(y, x, vec3, vec2)\
	SWIZZLE_3_2_RW(y, z, vec3, vec2)\
	SWIZZLE_3_2_RW(z, x, vec3, vec2)\
	SWIZZLE_3_2_RW(z, y, vec3, vec2)\
	\
	SWIZZLE_3_2_RO(x, x, vec3, vec2)\
	SWIZZLE_3_2_RO(y, y, vec3, vec2)\
	SWIZZLE_3_2_RO(z, z, vec3, vec2)\
	\
	SWIZZLE_3_4_RO(x, x, x, x, vec3, _vec4)\
	SWIZZLE_3_4_RO(y, y, y, y, vec3, _vec4)\
	SWIZZLE_3_4_RO(z, z, z, z, vec3, _vec4)
	// too many permutations, here are a few to get started, define them as needed


/**
 * vec4 swizzling
 */

#define SWIZZLE_4_4_RW(A, B, C, D, VecType) \
	struct swizzle_##A##B##C##D {\
		_##VecType V;\
		VecType& operator=(const VecType& rhs) {\
			V.A = rhs.x;\
			V.B = rhs.y;\
			V.C = rhs.z;\
			V.D = rhs.w;\
			return *(VecType*)this;\
		}\
		operator VecType() { return VecType{ V.A, V.B, V.C, V.D }; }\
	};\
	swizzle_##A##B##C##D A##B##C##D;

#define SWIZZLE_4_4_RO(A, B, C, D, VecType) \
	struct swizzle_##A##B##C##D {\
		_##VecType V;\
		operator VecType() { return VecType{ V.A, V.B, V.C, V.D }; }\
	};\
	swizzle_##A##B##C##D A##B##C##D;

#define SWIZZLE_4_3_RW(A, B, C, VecType, SVecType) \
	struct swizzle_##A##B##C {\
		_##VecType V;\
		VecType& operator=(const SVecType& rhs) {\
			V.A = rhs.x;\
			V.B = rhs.y;\
			V.C = rhs.z;\
			return *(VecType*)this;\
		}\
		operator SVecType() { return SVecType{ V.A, V.B, V.C }; }\
	};\
	swizzle_##A##B##C A##B##C;

#define SWIZZLE_4_3_RO(A, B, C, VecType, SVecType) \
	struct swizzle_##A##B##C {\
		_##VecType V;\
		operator SVecType() { return SVecType{ V.A, V.B, V.C }; }\
	};\
	swizzle_##A##B##C A##B##C;

#define SWIZZLE_4_2_RW(A, B, VecType, SVecType) \
	struct swizzle_##A##B {\
		_##VecType V;\
		VecType& operator=(const SVecType& rhs) {\
			V.A = rhs.x;\
			V.B = rhs.y;\
			return *(VecType*)this;\
		}\
		operator SVecType() { return SVecType{ V.A, V.B }; }\
	};\
	swizzle_##A##B A##B;

#define SWIZZLE_4_2_RO(A, B, VecType, SVecType) \
	struct swizzle_##A##B {\
		_##VecType V;\
		operator SVecType() { return SVecType{ V.A, V.B }; }\
	};\
	swizzle_##A##B A##B;


#define SWIZZLE_vec4(x, y, z, w) \
	SWIZZLE_4_4_RW(x, z, y, w, vec4)\
	SWIZZLE_4_4_RW(y, x, z, w, vec4)\
	SWIZZLE_4_4_RW(y, z, x, w, vec4)\
	SWIZZLE_4_4_RW(z, x, y, w, vec4)\
	SWIZZLE_4_4_RW(z, y, x, w, vec4)\
	\
	SWIZZLE_4_4_RO(x, x, x, w, vec4)\
	SWIZZLE_4_4_RO(y, y, y, w, vec4)\
	SWIZZLE_4_4_RO(z, z, z, w, vec4)\
	\
	SWIZZLE_4_4_RO(x, x, y, w, vec4)\
	SWIZZLE_4_4_RO(x, x, z, w, vec4)\
	SWIZZLE_4_4_RO(x, y, x, w, vec4)\
	SWIZZLE_4_4_RO(x, z, x, w, vec4)\
	SWIZZLE_4_4_RO(y, x, x, w, vec4)\
	SWIZZLE_4_4_RO(z, x, x, w, vec4)\
	\
	SWIZZLE_4_4_RO(y, y, x, w, vec4)\
	SWIZZLE_4_4_RO(y, y, z, w, vec4)\
	SWIZZLE_4_4_RO(y, x, y, w, vec4)\
	SWIZZLE_4_4_RO(y, z, y, w, vec4)\
	SWIZZLE_4_4_RO(x, y, y, w, vec4)\
	SWIZZLE_4_4_RO(z, y, y, w, vec4)\
	\
	SWIZZLE_4_3_RW(x, y, w, vec4, vec3)\
	SWIZZLE_4_3_RW(x, z, w, vec4, vec3)\
	SWIZZLE_4_3_RW(y, x, w, vec4, vec3)\
	SWIZZLE_4_3_RW(y, z, w, vec4, vec3)\
	SWIZZLE_4_3_RW(z, x, w, vec4, vec3)\
	SWIZZLE_4_3_RW(z, y, w, vec4, vec3)\
	\
	SWIZZLE_4_3_RO(x, x, w, vec4, vec3)\
	SWIZZLE_4_3_RO(y, y, w, vec4, vec3)\
	SWIZZLE_4_3_RO(z, z, w, vec4, vec3)\
	SWIZZLE_4_3_RO(x, x, x, vec4, vec3)\
	SWIZZLE_4_3_RO(y, y, y, vec4, vec3)\
	SWIZZLE_4_3_RO(z, z, z, vec4, vec3)\
	SWIZZLE_4_3_RO(w, w, w, vec4, vec3)
	// too many permutations, here are a few to get started, define them as needed


#endif