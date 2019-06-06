#ifndef _TEXTURE_GL_H
#define _TEXTURE_GL_H

#include "utility/common.h"

namespace render
{
	typedef h32 TextureId;

	enum Texture2D_Flags : u8 {
		Texture2DFlag_None            = 0x00,
		Texture2DFlag_GenerateMipmaps = 0x01,	// generate mipmaps, do not use if mipmaps are included in buffer
		Texture2DFlag_BGRA            = 0x02,	// buffer format is ordered BGRA instead of RGBA (default)
		// internal format flags are mutually exclusive
		Texture2DFlag_Float           = 0x04,	// float internal format
		Texture2DFlag_Int             = 0x08,	// int internal format
		Texture2DFlag_UInt            = 0x10,	// unsigned int internal format
		Texture2DFlag_sRGB            = 0x20	// texture in sRGB colorspace, use sRGB or sRGB_ALPHA format
	};


	struct Texture2D_GL {
		size_t		sizeBytes;
		u32			glTexture;
		u32			width;
		u32			height;
		u32			numMipmaps;
		u32			components;


		/**
		 * create an image from memory as a new OpenGL texture
		 * @param flags		texture creation options
		 * @param format	OpenGL pixel format for incoming data (ex. GL_BGRA), 0 = auto-detect
		 */
		bool createFromMemory(
			u8* data,
			size_t size,
			u32 width,
			u32 height,
			u8 components = 4,
			u8 componentSize = 1,
			u8 levels = 1,
			u8 flags = Texture2DFlag_None,
			u32 format = 0);

		/**
		 * load a DDS image from memory as a new OpenGL texture
		 */
		bool loadDDSFromMemory(
			u8* data,
			size_t size,
			bool sRGB = false);

		/**
		 * load a DDS image file directly as a new OpenGL texture
		 */
		bool loadDDSFromFile(
			MemoryArena& transient,
			const char* filename,
			bool sRGB = false);

		/**
		 * Bind to an active texture slot (0 - 31) to be sampled from a shader
		 * @param textureSlot	which texture slot to bind into, added to GL_TEXTURE0
		 */
		void bind(u32 textureSlot = 0) const;

		void setTextureParameters();

		void destroy();
	};


	struct TextureCubeMap_GL {
		size_t		sizeBytes;
		u32			glTexture;
		u32			width;
		u32			height;
		u32			numMipmaps;
		u32			components;


		/**
		 * load an image from memory as a new OpenGL texture
		 */
		bool loadDDSFromMemory(
			u8* data,
			size_t size,
			bool swapY = false,
			bool sRGB = false);

		/**
		 * load an image file directly as a new OpenGL texture
		 */
		bool loadDDSFromFile(
			MemoryArena& transient,
			const char* filename,
			bool swapY = false,
			bool sRGB = false);

		/**
		 * Bind to an active texture slot (0 - 31) to be sampled from a shader
		 * @var	textureSlot		which texture slot to bind into, added to GL_TEXTURE0
		 */
		void bind(u32 textureSlot = 0) const;

		void setTextureParameters();

		void destroy();
	};

}


#endif