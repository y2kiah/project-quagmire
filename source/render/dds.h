#ifndef _DDS_H
#define _DDS_H

#include "../utility/common.h"
#include "../utility/memory.h"

namespace render
{
	
	// The typedefs below are not currently used but could be

	//typedef struct { // DDCOLORKEY
	//	u32		dw1;
	//	u32		dw2;
	//} ddColorKey;

	//typedef struct  { // DDSCAPS2
	//	u32		dwCaps1;
	//	u32		dwCaps2;
	//	u32		Reserved[2];
	//} ddCaps2;

	//typedef struct _DDPIXELFORMAT { // DDPIXELFORMAT
	//	u32		dwSize;
	//	u32		dwFlags;
	//	u32		dwFourCC;
	//	union {
	//		u32	dwRGBBitCount;
	//		u32	dwYUVBitCount;
	//		u32	dwZBufferBitDepth;
	//		u32	dwAlphaBitDepth;
	//		u32	dwLuminanceBitCount;
	//		u32	dwBumpBitCount;
	//		u32	dwPrivateFormatBitCount;
	//	};
	//	union {
	//		u32	dwRBitMask;
	//		u32	dwYBitMask;
	//		u32	dwStencilBitDepth;
	//		u32	dwLuminanceBitMask;
	//		u32	dwBumpDuBitMask;
	//		u32	dwOperations;
	//	};
	//	union {
	//		u32	dwGBitMask;
	//		u32	dwUBitMask;
	//		u32	dwZBitMask;
	//		u32	dwBumpDvBitMask;
	//		struct {
	//			int32_t	wFlipMSTypes;
	//			int32_t	wBltMSTypes;
	//		} MultiSampleCaps;
	//	};
	//	union {
	//		u32	dwBBitMask;
	//		u32	dwVBitMask;
	//		u32	dwStencilBitMask;
	//		u32	dwBumpLuminanceBitMask;
	//	};
	//	union {
	//		u32	dwRGBAlphaBitMask;
	//		u32	dwYUVAlphaBitMask;
	//		u32	dwLuminanceAlphaBitMask;
	//		u32	dwRGBZBitMask;
	//		u32	dwYUVZBitMask;
	//	};
	//} ddPixelFormat;

	//typedef struct ddSurface // this is lifted and adapted from DDSURFACEDESC2
	//{
	//	u32		dwSize;                 // size of the DDSURFACEDESC structure
	//	u32		dwFlags;                // determines what fields are valid
	//	u32		dwHeight;               // height of surface to be created
	//	u32		dwWidth;                // width of input surface
	//	union {
	//		int32_t		lPitch;                 // distance to start of next line (return value only)
	//		u32	dwLinearSize;           // Formless late-allocated optimized surface size
	//	};
	//	union {
	//		u32	dwBackBufferCount;      // number of back buffers requested
	//		u32	dwDepth;                // the depth if this is a volume texture 
	//	};
	//	union {
	//		u32	dwMipMapCount;          // number of mip-map levels requestde
	//		// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
	//		u32	dwRefreshRate;          // refresh rate (used when display mode is described)
	//		u32	dwSrcVBHandle;          // The source used in VB::Optimize
	//	};
	//	u32		dwAlphaBitDepth;        // depth of alpha buffer requested
	//	u32		dwReserved;             // reserved
	//	u32		lpSurface;              // pointer to the associated surface memory
	//	union {
	//		ddColorKey	ddckCKDestOverlay;      // color key for destination overlay use
	//		u32	dwEmptyFaceColor;       // Physical color for empty cubemap faces
	//	};
	//	ddColorKey		ddckCKDestBlt;          // color key for destination blt use
	//	ddColorKey		ddckCKSrcOverlay;       // color key for source overlay use
	//	ddColorKey		ddckCKSrcBlt;           // color key for source blt use
	//	union {
	//		ddPixelFormat ddpfPixelFormat;      // pixel format description of the surface
	//		u32	dwFVF;                  // vertex format description of vertex buffers
	//	};
	//	ddCaps2			ddsCaps;                // direct draw surface capabilities
	//	u32		dwTextureStage;         // stage in multitexture cascade
	//} ddSurface;

	//enum { FOURCC_DXT1 = 0x31545844, FOURCC_DXT3 = 0x33545844, FOURCC_DXT5 = 0x35545844 };

	// 12 mipmap levels plus the top level image gives a max resolution of 8192
	#define DDS_MAX_MIPMAPS		12

	const u32 DDS_FOURCC  = 0x00000004;
	const u32 DDS_RGB     = 0x00000040;
	const u32 DDS_RGBA    = 0x00000041;
	const u32 DDS_DEPTH   = 0x00800000;

	const u32 DDS_COMPLEX = 0x00000008;
	const u32 DDS_CUBEMAP = 0x00000200;
	const u32 DDS_VOLUME  = 0x00200000;

	const u32 FOURCC_DXT1 = 0x31545844; //(MAKEFOURCC('D','X','T','1'))
	const u32 FOURCC_DXT3 = 0x33545844; //(MAKEFOURCC('D','X','T','3'))
	const u32 FOURCC_DXT5 = 0x35545844; //(MAKEFOURCC('D','X','T','5'))

	struct DDSPixelFormat
	{
		u32		dwSize;
		u32		dwFlags;
		u32		dwFourCC;
		u32		dwRGBBitCount;
		u32		dwRBitMask;
		u32		dwGBitMask;
		u32		dwBBitMask;
		u32		dwABitMask;
	};

	struct DXTColBlock
	{
		u16		col0;
		u16		col1;
		u8		row[4];
	};

	struct DXT3AlphaBlock
	{
		u16		row[4];
	};

	struct DXT5AlphaBlock
	{
		u8		alpha0;
		u8		alpha1;
		u8		row[6];
	};

	struct DDSHeader
	{
		u32		dwSize;
		u32		dwFlags;
		u32		dwHeight;
		u32		dwWidth;
		u32		dwPitchOrLinearSize;
		u32		dwDepth;
		u32		dwMipMapCount;
		u32		dwReserved1[11];
		DDSPixelFormat	ddspf;
		u32		dwCaps1;
		u32		dwCaps2;
		u32		dwReserved2[3];
	};

	struct DDSSurface
	{
		u8*		pixels;
		
		u32		width;
		u32		height;
		u32		depth;
		u32		size;

		operator u8*();

		void init(
			u32 w, u32 h, u32 d,
			u32 imgsize,
			u8* data);
	};


	struct DDSTexture
		: public DDSSurface
	{
		DDSSurface	mipmaps[DDS_MAX_MIPMAPS];
		u32			numMipmaps;
		
		u32			_padding;
		

		DDSSurface& get_mipmap(u32 index)
		{
			assert(index < numMipmaps);
			return mipmaps[index];
		}
	};

	struct DDSImage
	{
		DDSTexture	images[6];
		u32			numImages;
		
		u32 		format;
		u32 		internalFormat;
		u32 		components;
		
		u8			compressed;
		u8			cubemap;
		u8			volume;
		u8			valid;


		bool loadFromMemory(
			u8* data,
			u32 dataSize,
			bool flipImage = true,
			bool sRGB = false);

		bool loadFromFile(
			MemoryArena& transient,
			const char* filename,
			bool flipImage = true,
			bool sRGB = false);

		operator u8*();
		DDSTexture& operator[](u32 index);

		u32 clamp_size(u32 size);
		u32 get_line_width(u32 width, u32 bpp);
		u32 size_dxtc(u32 width, u32 height);
		u32 size_rgb(u32 width, u32 height);
		//void align_memory(DDSTexture* surface);

		void flip(u8* image, u32 width, u32 height, u32 depth, u32 size);

		//void swap(void* byte1, void* byte2, u32 size);
		void flip_blocks_dxtc1(DXTColBlock* line, u32 numBlocks);
		void flip_blocks_dxtc3(DXTColBlock* line, u32 numBlocks);
		void flip_blocks_dxtc5(DXTColBlock* line, u32 numBlocks);
		void flip_dxt5_alpha(DXT5AlphaBlock* block);

		void upload_texture1D();
		void upload_texture2D(u32 imageIndex = 0, u32 glTarget = 0x0DE1); // GLenum = GL_TEXTURE_2D
		void upload_textureRectangle();
		void upload_texture3D();
		void upload_textureCubemap(bool swapY = false);

		u32 get_width()
		{
			assert(valid && numImages > 0);
			return images[0].width;
		}

		u32 get_height()
		{
			assert(valid && numImages > 0);
			return images[0].height;
		}

		u32 get_depth()
		{
			assert(valid && numImages > 0);
			return images[0].depth;
		}

		u32 get_size()
		{
			assert(valid && numImages > 0);
			return images[0].size;
		}

		u32 get_num_mipmaps()
		{
			assert(valid && numImages > 0);
			return images[0].numMipmaps;
		}

		DDSSurface& get_mipmap(u32 index)
		{
			assert(valid && numImages > 0 && index < images[0].numMipmaps);
			return images[0].get_mipmap(index);
		}
	};
}

#endif