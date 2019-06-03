#ifndef _NOISE_H
#define _NOISE_H

#include "../utility/types.h"


enum CoherentNoiseType : u8 {
	CoherentNoiseType_Linear = 0,
	CoherentNoiseType_Cosine,
	CoherentNoiseType_Cubic,
	CoherentNoiseType_Quintic,
	CoherentNoiseType_Perlin,
	CoherentNoiseType_Simplex,
	CoherentNoiseType_Test,
	CoherentNoiseType_Cached
};

enum MultiFractalOperation : u8 {
	MultiFractalOperation_Add = 0,
	MultiFractalOperation_Multiply,
	MultiFractalOperation_Add_Abs,
	MultiFractalOperation_Multiply_Abs,
	MultiFractalOperation_Pow,
	MultiFractalOperation_Exp
};


// Function Declarations

// TEMP
//void initGrad4();

// Integer noise methods (discrete)
r32 integerNoise1(
	i32 x, i32 seed = 0);
r32 integerNoise2(
	i32 x, i32 y,
	i32 xSeed = 0, i32 ySeed = 0);
r32 integerNoise3(
	i32 x, i32 y, i32 z,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0);
r32 integerNoise4(
	i32 x, i32 y, i32 z, i32 w,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0, i32 wSeed = 0);

// Coherent noise methods (continuous, interpolate integer noise)
//  these can be optimized for generating large buffers by caching integer noise results, then interpolating cached noise
r32 linearNoise1(
	r32 x, i32 seed = 0);
r32 linearNoise2(
	r32 x, r32 y,
	i32 xSeed = 0, i32 ySeed = 0);
r32 linearNoise3(
	r32 x, r32 y, r32 z,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0);
r32 linearNoise4(
	r32 x, r32 y, r32 z, r32 w,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0, i32 wSeed = 0);

r32 cosineNoise1(
	r32 x,
	i32 seed = 0);
r32 cosineNoise2(
	r32 x, r32 y,
	i32 xSeed = 0, i32 ySeed = 0);
r32 cosineNoise3(
	r32 x, r32 y, r32 z,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0);
r32 cosineNoise4(
	r32 x, r32 y, r32 z, r32 w,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0, i32 wSeed = 0);

r32 cubicNoise1(
	r32 x,
	i32 seed = 0);
r32 cubicNoise2(
	r32 x, r32 y,
	i32 xSeed = 0, i32 ySeed = 0);
r32 cubicNoise3(
	r32 x, r32 y, r32 z,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0);
r32 cubicNoise4(
	r32 x, r32 y, r32 z, r32 w,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0, i32 wSeed = 0);

r32 quinticNoise1(
	r32 x,
	i32 seed = 0);
r32 quinticNoise2(
	r32 x, r32 y,
	i32 xSeed = 0, i32 ySeed = 0);
r32 quinticNoise3(
	r32 x, r32 y, r32 z,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0);
r32 quinticNoise4(
	r32 x, r32 y, r32 z, r32 w,
	i32 xSeed = 0, i32 ySeed = 0, i32 zSeed = 0, i32 wSeed = 0);

// fast gradient noise as implemented by Perlin, "improved" version
r32 perlinNoise1(r32 x);
r32 perlinNoise2(r32 x, r32 y);
r32 perlinNoise3(r32 x, r32 y, r32 z);
r32 perlinNoise4(r32 x, r32 y, r32 z, r32 w);

r32 simplexNoise1(r32 x);
r32 simplexNoise2(r32 x, r32 y);
r32 simplexNoise3(r32 x, r32 y, r32 z);
r32 simplexNoise4(r32 x, r32 y, r32 z, r32 w);

/*r32 testNoise1(r32 x);
r32 testNoise2(r32 x, r32 y);
r32 testNoise3(r32 x, r32 y, r32 z);
r32 testNoise4(r32 x, r32 y, r32 z, r32 w);

r32 cachedNoise1(r32 x);
r32 cachedNoise2(r32 x, r32 y);
r32 cachedNoise3(r32 x, r32 y, r32 z);
r32 cachedNoise4(r32 x, r32 y, r32 z, r32 w);*/

// Wrapper for coherent noise functions
r32 coherentNoise(
	i32 numDimensions, r32 *v,
	CoherentNoiseType noiseType = CoherentNoiseType_Simplex);

// Fractal methods (coherent noise combined in octaves)
r32 fBm(i32 numDimensions, r32 *v, i32 octaves, r32 lacunarity, r32 persistence, r32 amplitudeStart,
	MultiFractalOperation operation = MultiFractalOperation_Add,
	CoherentNoiseType noiseType = CoherentNoiseType_Simplex);

r32 multiFractal(i32 numDimensions, r32 *v, i32 octaves, r32 lacunarity, r32 roughness, r32 amplitudeStart,
	MultiFractalOperation operation = MultiFractalOperation_Add,
	CoherentNoiseType noiseType = CoherentNoiseType_Simplex);

struct Noise3Deriv {
	r32 n;
	r32 dx;
	r32 dy;
	r32 dz;
};

Noise3Deriv perlinNoise3Deriv(
	r32 x, r32 y, r32 z);

r32 swissTurbulence(
	r32 x, r32 y, r32 z, i32 octaves,
	r32 lacunarity = 2.0f, r32 persistence = 0.5f, r32 warp = 0.15f);

r32 jordanTurbulence(
	r32 x, r32 y, r32 z,
	i32 octaves, r32 lacunarity = 2.0f,
	r32 gain1 = 0.8f, r32 gain = 0.5f,
	r32 warp0 = 0.4f, r32 warp = 0.35f,
	r32 damp0 = 1.0f, r32 damp = 0.8f,
	r32 damp_scale = 1.0f);

// Inline Function Definitions

inline r32 integerNoise1(
	i32 x, i32 seed)
{
	x += seed;
	x = (x << 13) ^ x;
	return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
}

inline r32 integerNoise2(
	i32 x, i32 y, i32 xSeed, i32 ySeed)
{
	x += xSeed;
	y += ySeed;
	x += y * 47;
	x = (x << 13) ^ x;
	return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
}

inline r32 integerNoise3(
	i32 x, i32 y, i32 z,
	i32 xSeed, i32 ySeed, i32 zSeed)
{
	x += xSeed;
	y += ySeed;
	z += zSeed;
	x += (y * 47) + (z * 59);
	x = (x << 13) ^ x;
	return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
}

inline r32 integerNoise4(
	i32 x, i32 y, i32 z, i32 w,
	i32 xSeed, i32 ySeed, i32 zSeed, i32 wSeed)
{
	x += xSeed;
	y += ySeed;
	z += zSeed;
	w += wSeed;
	x += (y * 47) + (z * 59) + (w * 131);
	x = (x << 13) ^ x;
	return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) * 0.000000000931322574615478515625f);
}


#endif