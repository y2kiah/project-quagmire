#ifndef _MATERIAL_GL_H_
#define _MATERIAL_GL_H_

#include "../math/qmath.h"
//#include <utility/container/handle_map.h>

		
#define MAX_MATERIAL_TEXTURES			12
#define MAX_MATERIAL_TEXTURE_NAME_SIZE	64


enum MaterialTextureType : u16
{
	MaterialTexture_None = 0,
	MaterialTexture_Diffuse,				// RGB - diffuse surface color,	A - unused
	MaterialTexture_Diffuse_Opacity,		// RGB - diffuse surface color,	A - opacity (alpha blend)
	MaterialTexture_Diffuse_OpacityMask,	// RGB - diffuse surface color,	A - opacity (no blend, alpha test only)
	MaterialTexture_Diffuse_Occlusion,		// RGB - diffuse surface color,	A - occlusion
	MaterialTexture_Diffuse_Height,			// RGB - diffuse surface color,	A - height
	MaterialTexture_Diffuse_Specular,		// RGB - diffuse surface color,	A - specular
	MaterialTexture_Emissive,				// RGB - emissive light color,	A - brightness
	MaterialTexture_Normal,					// RGB - normal map,			A - unused
	MaterialTexture_Normal_Height,			// RGB - normal map,			A - height
	MaterialTexture_Normal_Specular,		// RGB - normal map,			A - specular
	MaterialTexture_Specular_Metallic_Reflectivity_Occlusion	// R - specular, G - metallic, B - reflectivity, A - occlusion
};

enum MaterialTextureMappingMode : u8
{
	MaterialTextureMappingMode_None = 0,
	MaterialTextureMappingMode_Wrap,
	MaterialTextureMappingMode_Clamp,
	MaterialTextureMappingMode_Decal,
	MaterialTextureMappingMode_Mirror
};

/**
 * All of the parameters in this key tie in to the ubershader with a compile time ifdef
 * to generate a unique permutation of the shader for each unique key. The shader manager
 * stores and identifies needed shaders by this key. Non-ubershader programs just use key
 * value of 0.
 * numDiffuseTextures: Up to 4 diffuse textures can be blended. for the first diffuse
 * texture, the channels are interpreted by the presence of either the hasFirstDiffuseMap or
 * hasFirstDiffuseOpacityMap flag. For the next 3 diffuse textures, alpha is always used for
 * blending with the base color
 */
struct ShaderKey {
	union {
		struct {
			u8	isUbershader				: 1;	// 1 = shader is an ubershader permutation, 0 = unique shader
			u8	hasFirstDiffuseMap			: 1;	// mutually exclusive with hasFirstDiffuseOpacityMap and hasFirstDiffuseAOMap
			u8	hasFirstDiffuseOpacityMap	: 1;	// mutually exclusive with hasFirstDiffuseMap and hasFirstDiffuseAOMap
			u8	hasFirstDiffuseAOMap		: 1;	// mutually exclusive with hasFirstDiffuseMap and hasFirstDiffuseOpacityMap
			u8	hasSpecularMap				: 1;
			u8	hasEmissiveMap				: 1;
			u8	hasNormalMap				: 1;
			u8	hasNormalHeightMap			: 1;
			// 1
			u8	hasMetallicReflectiveAOMap	: 1;
			u8	numDiffuseTextures			: 2;
			u8	usesVertexColorForDiffuse	: 1;	// whether or not to multiply vertex color channel 1 into base diffuse, TODO: support or no? Use uniform for color channel index
			u8	isLit						: 1;	// accepts scene lighting, if 0 object masked off from lighting using stencil buffer
			u8	isReflective				: 1;	// reflects environment map
			u8	isTranslucent				: 1;	// uses alpha blend and depth sort, rendered after deferred pass, either mat. opacity < 1 or usesAlphaBlend=1 cause this to be 1
			u8	isShadowed					: 1;	// accepts shadows from scene, if 0 object masked off from shadows using stencil buffer???
			// 2
			u8	castsShadow					: 1;	// casts shadow in scene, if 0 object does not cast a shadow
			u8	usesAlphaBlend				: 1;	// when hasFirstDiffuseOpacityMap=1, alpha channel of diffuse texture 0 is used for alpha blend
			u8	usesAlphaTest				: 1;	// when hasFirstDiffuseOpacityMap=1, alpha channel of diffuse texture 0 is treated as on/off alpha mask
			u8	usesBumpMapping				: 1;	// uses normalmap to do bumpmapping
			u8	usesDisplacementMapping		: 1;	// uses normalmap and heightmap to do displacement mapping

			u8	_padding					: 3;
			// 3

			u8	_padding_end[5];
			// 8
			// TODO: how to handle LOD, e.g. turn off bump/displacement mapping with distance from camera
		};

		u64		value;
	};
};


struct MaterialTexture
{
	Id_T						textureResourceHandle;
	MaterialTextureType			textureType;
	u8							uvChannelIndex;
	MaterialTextureMappingMode	textureMappingModeU;
	MaterialTextureMappingMode	textureMappingModeV;
	u8							_padding[3];
	char						name[MAX_MATERIAL_TEXTURE_NAME_SIZE];
};

struct Material_GL
{
	vec3		diffuseColor;
	vec3		ambientColor;
	vec3		specularColor;
	vec3		emissiveColor;
	// 48
	r32			opacity;		// opacity should be defaulted to 1.0f		
	r32			reflectivity;
	r32			shininess;
	r32			metallic;
	// 64
	Id_T		shaderResourceHandle;
	ShaderKey	shaderKey;		// shader key combines vertex and material flags, shader manager stores ubershader by key
	u8			numTextures;
	u8			_padding[7];
	// 88
	MaterialTexture textures[MAX_MATERIAL_TEXTURES]; // 80 * 12 = 960
	// 1048
};
static_assert_aligned_size(Material_GL,4);


#endif