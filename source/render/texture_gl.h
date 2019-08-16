#ifndef _TEXTURE_GL_H
#define _TEXTURE_GL_H

#include "utility/common.h"
#include "utility/sparse_handle_map_16.h"
#include "asset/asset.h"
#include "render/dds.h"


typedef h32 TextureId;

namespace render
{

	enum TextureFlags : u32 {
		TextureFlag_None            = 0x00,
		TextureFlag_GenerateMipmaps = 0x01,	// generate mipmaps, do not use if mipmaps are included in buffer
		TextureFlag_FlipY           = 0x02,	// flip the Y direction on load
		TextureFlag_BGRA            = 0x04,	// buffer format is ordered BGRA instead of RGBA (default)
		// internal format flags are mutually exclusive
		TextureFlag_Float           = 0x08,	// float internal format
		TextureFlag_Int             = 0x10,	// int internal format
		TextureFlag_UInt            = 0x20,	// unsigned int internal format
		TextureFlag_sRGB            = 0x40	// texture in sRGB colorspace, use sRGB or sRGB_ALPHA format
	};


	struct Texture2D_GL {
		u32			sizeBytes;
		u32			glTexture;
		u32			width;
		u32			height;
		u32			numMipmaps;
		u32			components;
		u32			flags;
		AssetHnd	asset;

		/**
		 * Bind to an active texture slot (0 - 31) to be sampled from a shader
		 * @param textureSlot	which texture slot to bind into, added to GL_TEXTURE0
		 */
		void bind(u32 textureSlot = 0);

		void setTextureParameters();

		void destroy();

	};
	
	/**
	 * Create an image from memory as a new OpenGL texture
	 * @param format	OpenGL pixel format for incoming data (ex. GL_BGRA), 0 = auto-detect
	 * @param asset		if the texture will be owned by the assetStore, provide the asset handle
	 * @param flags		texture creation options
	 */
	Texture2D_GL createFromMemory(
		u8* data,
		u32 size,
		u32 width,
		u32 height,
		u8 components = 4,
		u8 componentSize = 1,
		u8 levels = 1,
		u32 format = 0,
		AssetHnd asset = null_h32,
		u32 flags = TextureFlag_None);

	/**
	 * Init a Texture2D_GL from a DDSImage already loaded in memory
	 */
	bool initFromDDS(
		Texture2D_GL& tex,
		DDSImage& dds,
		u32 size,
		AssetHnd asset = null_h32,
		u32 flags = TextureFlag_None);


	struct TextureCubeMap_GL {
		u32			sizeBytes;
		u32			glTexture;
		u32			width;
		u32			height;
		u32			numMipmaps;
		u32			components;
		u32			flags;
		AssetHnd	asset;

		/**
		 * Bind to an active texture slot (0 - 31) to be sampled from a shader
		 * @var	textureSlot		which texture slot to bind into, added to GL_TEXTURE0
		 */
		void bind(u32 textureSlot = 0);

		void setTextureParameters();

		void destroy();
	};

	/**
	 * Init a TextureCubeMap_GL from a DDSImage already loaded in memory
	 */
	bool initFromDDS(
		TextureCubeMap_GL& tex,
		DDSImage& dds,
		u32 size,
		AssetHnd asset = null_h32,
		u32 flags = TextureFlag_None);


	SparseHandleMap16Typed(Texture2D_GL, Texture2D_HandleMap, TextureId, 0);
	SparseHandleMap16Typed(TextureCubeMap_GL, TextureCubeMap_HandleMap, TextureId, 1);


	// Texture Asset Functions

	TextureId createTexture2DAsset(
		AssetStore& store,
		Texture2D_HandleMap& textures2D,
		h32 assetPack,
		u32 assetId,
		u32 textureParams = TextureFlag_None);

}


#endif