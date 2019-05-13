#ifndef _QMATH_H
#define _QMATH_H

#include "vec4.h"
#include "dvec4.h"
#include "mat4_transforms.h"
#include "dmat4_transforms.h"
#include "quat.h"

// conversion functions

vec3 _vec3(const dvec4& v)
{
    return vec3{ (r32)v.x, (r32)v.y, (r32)v.z };
}

dvec4 _dvec4(const dvec3& v, r64 w)
{
    return dvec4{ v.x, v.y, v.z, w };
}

#endif