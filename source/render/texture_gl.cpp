#include <GL/glew.h>
#include "texture_gl.h"
#include "dds.h"
#include "utility/logger.h"

namespace render
{
	// Global variables

	/**
	* Table of texture pixel formats
	* first dimension is RGBA vs. BGRA
	* second dimension is # components
	*/
	const GLenum g_formatLookup[2][4] = { { GL_RED, GL_RG, GL_RGB, GL_RGBA }, { 0, 0, GL_BGR, GL_BGRA } };

	/**
	* Table of internal formats
	* first dimension is # components
	* second dimension is component size
	* third dimension is data type
	*/
	const GLint g_internalFormatLookup[4][3][5] = {
		{
		/* R ----- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
		/* 8  */ { { GL_R8 },    { 0 },          { GL_R8I },     { GL_R8UI },     { GL_SLUMINANCE8 } },
		/* 16 */ { { 0 },        { GL_R16F },    { GL_R16I },    { GL_R16UI },    { 0 } },
		/* 32 */ { { 0 },        { GL_R32F },    { GL_R32I },    { GL_R32UI },    { 0 } }
		},
		{
		/* RG ---- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
		/* 8  */ { { GL_RG8 },   { 0 },          { GL_RG8I },    { GL_RG8UI },    { GL_SLUMINANCE8_ALPHA8 } },
		/* 16 */ { { 0 },        { GL_RG16F },   { GL_RG16I },   { GL_RG16UI },   { 0 } },
		/* 32 */ { { 0 },        { GL_RG32F },   { GL_RG32I },   { GL_RG32UI },   { 0 } }
		},
		{
		/* RGB --- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
		/* 8  */ { { GL_RGB8 },  { 0 },          { GL_RGB8I },   { GL_RGB8UI },   { GL_SRGB8 } },
		/* 16 */ { { 0 },        { GL_RGB16F },  { GL_RGB16I },  { GL_RGB16UI },  { 0 } },
		/* 32 */ { { 0 },        { GL_RGB32F },  { GL_RGB32I },  { GL_RGB32UI },  { 0 } }
		},
		{
		/* RGBA -- standard ---- float --------- int ----------- uint ----------- sRGB -------------------- */
		/* 8  */ { { GL_RGBA8 }, { 0 },          { GL_RGBA8I },  { GL_RGBA8UI },  { GL_SRGB8_ALPHA8 } },
		/* 16 */ { { 0 },        { GL_RGBA16F }, { GL_RGBA16I }, { GL_RGBA16UI }, { 0 } },
		/* 32 */ { { 0 },        { GL_RGBA32F }, { GL_RGBA32I }, { GL_RGBA32UI }, { 0 } }
		}
	};


	// Texture2D_GL Functions

	void Texture2D_GL::setTextureParameters()
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (numMipmaps > 0) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMipmaps);
		}
		else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		
		// TODO: TEMP, do NOT keep this here as it will be queried at runtime, move to OpenGL setup and set a flag
		if (glewIsExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
			r32 fLargest = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
		}

		ASSERT_GL_ERROR;
	}

	bool Texture2D_GL::createFromMemory(
		u8* data,
		size_t size,
		u32 width,
		u32 height,
		u8 components,
		u8 componentSize,
		u8 levels,
		u8 flags,
		u32 format)
	{
		assert(components > 0 && components <= 4 && "components out of range");
		assert(componentSize > 0 && componentSize <= 8 && "componentSize out of range");
		assert(levels > 0 && levels <= 13 && "level (probably) out of range, need more than 13 mipmaps?");

		sizeBytes = size;
		width = width;
		height = height;
		components = components;
		numMipmaps = levels - 1;

		// get number of mipmaps to be generated
		if (levels == 1 && (flags & Texture2DFlag_GenerateMipmaps)) {
			numMipmaps = (u32)(floorf(log2f((r32)(width > height ? width : height))));
		}

		u32 componentsLookup = components - 1;
		u32 componentSizeLookup = componentSize / 2; // 1 => 0,  2 => 1,  4 => 2
		u32 typeLookup = 
			(flags & Texture2DFlag_Float ? 1 :
			(flags & Texture2DFlag_Int   ? 2 :
			(flags & Texture2DFlag_UInt  ? 3 :
			(flags & Texture2DFlag_sRGB  ? 4 : 0))));

		if (format == 0) {
			format = g_formatLookup[(flags & Texture2DFlag_BGRA) ? 1 : 0][componentsLookup];
		}
		GLint internalFormat = g_internalFormatLookup[componentsLookup][componentSizeLookup][typeLookup];
		
		if (format == 0 || internalFormat == 0) {
			logger::warn(logger::Category_Render, "invalid texture format");
			return false;
		}

		// create texture storage
		glGenTextures(1, &glTexture);
		glBindTexture(GL_TEXTURE_2D, glTexture);
		glTexStorage2D(GL_TEXTURE_2D, numMipmaps + 1, internalFormat, width, height);

		// upload pixel data
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

		if (levels == 1 && numMipmaps > 0) {
			glGenerateTextureMipmap(glTexture);
		}
		else if (numMipmaps > 0) {
			u8* pData = data + (width * height * components * componentSize);
			u32 mipWidth = width / 2;
			u32 mipHeight = height / 2;

			for (u32 level = 1; level <= numMipmaps; ++level) {
				glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, format, GL_UNSIGNED_BYTE, pData);
				pData += (mipWidth * mipHeight * components * componentSize);
				mipWidth /= 2;
				mipHeight /= 2;
			}

			assert(width == 1 && height == 1 && data - pData == size && "problem with mipmap chain data");
		}

		setTextureParameters();

		ASSERT_GL_ERROR;
		return true;
	}

	bool Texture2D_GL::loadDDSFromMemory(
		u8* data,
		size_t size,
		bool sRGB)
	{
		// if image is already loaded, do some cleanup?

		glGenTextures(1, &glTexture);
		glBindTexture(GL_TEXTURE_2D, glTexture);
		
		DDSImage image;
		bool ok = image.loadFromMemory(data, true, sRGB)
					&& !image.cubemap && !image.volume
					&& image.upload_texture2D();

		if (ok) {
			sizeBytes = size;
			width = image.get_width();
			height = image.get_height();
			numMipmaps = image.get_num_mipmaps();
			components = image.components;

			setTextureParameters();
		}
		else {
			logger::warn(logger::Category_Render, "texture loading error");
		}

		ASSERT_GL_ERROR;
		return ok;
	}

	bool Texture2D_GL::loadDDSFromFile(
		MemoryArena& transient,
		const char* filename,
		bool sRGB)
	{
		// if image is already loaded, do some cleanup?

		glGenTextures(1, &glTexture);
		glBindTexture(GL_TEXTURE_2D, glTexture);
		DDSImage image;
		bool ok = image.loadFromFile(transient, filename, true, sRGB)
					&& !image.cubemap && !image.volume
					&& image.upload_texture2D();

		if (ok) {
			sizeBytes = image.get_size();
			width = image.get_width();
			height = image.get_height();
			numMipmaps = image.get_num_mipmaps();
			components = image.components;

			setTextureParameters();
		}
		else {
			logger::warn(logger::Category_Render, "texture loading error");
		}

		return ok;
	}

	void Texture2D_GL::bind(u32 textureSlot) const
	{
		assert(glTexture != 0 && textureSlot >= 0 && textureSlot < 32 && "textureSlot must be in 0-31 range");

		glActiveTexture(GL_TEXTURE0 + textureSlot);
		glBindTexture(GL_TEXTURE_2D, glTexture);

		ASSERT_GL_ERROR;
	}

	void Texture2D_GL::destroy()
	{
		if (glTexture != 0) {
			glDeleteTextures(1, &glTexture);
			glTexture = 0;
		}
	}


	// TextureCubeMap_GL Functions

	void TextureCubeMap_GL::setTextureParameters()
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (numMipmaps > 0) {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, numMipmaps);
		}
		else {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		// TODO: TEMP, do NOT keep this here as it will be queried at runtime, move to OpenGL setup and set a flag
		if (glewIsExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
			r32 fLargest = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
			glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
		}
	}

	bool TextureCubeMap_GL::loadDDSFromMemory(
		u8* data,
		size_t size,
		bool swapY,
		bool sRGB)
	{
		// if image is already loaded, do some cleanup?

		glGenTextures(1, &glTexture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, glTexture);
		DDSImage image;
		bool ok = image.loadFromMemory(data, true, sRGB)
					&& image.cubemap
					&& image.upload_textureCubemap(swapY);

		if (ok) {
			sizeBytes = size;
			width = image.get_width();
			height = image.get_height();
			numMipmaps = image.get_num_mipmaps();
			components = image.components;

			setTextureParameters();
		}
		else {
			logger::warn(logger::Category_Render, "texture loading error");
		}

		return ok;
	}

	bool TextureCubeMap_GL::loadDDSFromFile(
		MemoryArena& transient,
		const char* filename,
		bool swapY,
		bool sRGB)
	{
		// if image is already loaded, do some cleanup?

		glGenTextures(1, &glTexture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, glTexture);
		DDSImage image;
		bool ok = image.loadFromFile(transient, filename, true, sRGB)
					&& image.cubemap
					&& image.upload_textureCubemap(swapY);

		if (ok) {
			sizeBytes = image.get_size();
			width = image.get_width();
			height = image.get_height();
			numMipmaps = image.get_num_mipmaps();
			components = image.components;

			setTextureParameters();
		}
		else {
			logger::warn(logger::Category_Render, "texture loading error");
		}

		return ok;
	}

	void TextureCubeMap_GL::bind(u32 textureSlot) const
	{
		assert(glTexture != 0 && textureSlot >= 0 && textureSlot < 32 && "textureSlot must be in 0-31 range");

		glActiveTexture(GL_TEXTURE0 + textureSlot);
		glBindTexture(GL_TEXTURE_CUBE_MAP, glTexture);
	}

	void TextureCubeMap_GL::destroy()
	{
		if (glTexture != 0) {
			glDeleteTextures(1, &glTexture);
			glTexture = 0;
		}
	}
}

#include "dds.cpp"
