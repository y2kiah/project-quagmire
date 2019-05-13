#ifndef _SWIZZLE_H
#define _SWIZZLE_H

#include "../utility/common.h"

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


#define SWIZZLE__vec2(x, y, vec2, _vec3, _vec4) \
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

#define SWIZZLE_vec2(x, y) \
    SWIZZLE__vec2(x, y, vec2, _vec3, _vec4)

#define SWIZZLE_dvec2(x, y) \
    SWIZZLE__vec2(x, y, dvec2, _dvec3, _dvec4)


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


#define SWIZZLE__vec3(x, y, z, vec3, vec2, _vec4) \
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

#define SWIZZLE_vec3(x, y, z) \
    SWIZZLE__vec3(x, y, z, vec3, vec2, _vec4)

#define SWIZZLE_dvec3(x, y, z) \
    SWIZZLE__vec3(x, y, z, dvec3, dvec2, _dvec4)


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


#define SWIZZLE__vec4(x, y, z, w, vec4, vec3, vec2) \
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
	SWIZZLE_4_3_RW(x, y, z, vec4, vec3)\
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
	SWIZZLE_4_3_RO(w, w, w, vec4, vec3)\
	\
	SWIZZLE_4_2_RW(x, y, vec4, vec2)\
	SWIZZLE_4_2_RW(x, z, vec4, vec2)\
	SWIZZLE_4_2_RW(y, x, vec4, vec2)\
	SWIZZLE_4_2_RW(y, z, vec4, vec2)\
	SWIZZLE_4_2_RW(z, x, vec4, vec2)\
	SWIZZLE_4_2_RW(z, y, vec4, vec2)\
	\
	SWIZZLE_4_2_RO(x, x, vec4, vec2)\
	SWIZZLE_4_2_RO(y, y, vec4, vec2)\
	SWIZZLE_4_2_RO(z, z, vec4, vec2)\
	SWIZZLE_4_2_RO(w, w, vec4, vec2)
	// too many permutations, here are a few to get started, define them as needed

#define SWIZZLE_vec4(x, y, z, w) \
    SWIZZLE__vec4(x, y, z, w, vec4, vec3, vec2)

#define SWIZZLE_dvec4(x, y, z, w) \
    SWIZZLE__vec4(x, y, z, w, dvec4, dvec3, dvec2)

#endif