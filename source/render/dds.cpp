// Reference: nv_dds.cpp
// Description:
// 
// Loads DDS images (DXTC1, DXTC3, DXTC5, RGB (888, 888X), and RGBA (8888) are
// supported) for use in OpenGL. Image is flipped when its loaded as DX images
// are stored with different coordinate system. If file has mipmaps and/or 
// cubemaps then these are loaded as well. Volume textures can be loaded as 
// well but they must be uncompressed.
//
// When multiple textures are loaded (i.e a volume or cubemap texture), 
// additional faces can be accessed using the array operator. 
//
// The mipmaps for each face are also stored in a list and can be accessed like 
// so: image.get_mipmap() (which accesses the first mipmap of the first 
// image). To get the number of mipmaps call the get_num_mipmaps function for
// a given texture.
//
// Call the is_volume() or is_cubemap() function to check that a loaded image
// is a volume or cubemap texture respectively. If a volume texture is loaded
// then the get_depth() function should return a number greater than 1. 
// Mipmapped volume textures and DXTC compressed volume textures are supported.
//
//
// Sample usage
//
// Loading a compressed texture:
//
// DDSImage image;
// GLuint texobj;
//
// image.load("compressed.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glCompressedTexImage2D(GL_TEXTURE_2D, 0, image.get_format(), 
//     image.get_width(), image.get_height(), 0, image.get_size(), 
//     image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glCompressedTexImage2D(GL_TEXTURE_2D, i+1, image.get_format(), 
//         image.get_mipmap(i).get_width(), image.get_mipmap(i).get_height(), 0, 
//         image.get_mipmap(i).get_size(), image.get_mipmap(i));
// } 
// 
// Loading an uncompressed texture:
//
// DDSImage image;
// GLuint texobj;
//
// image.load("uncompressed.dds");
//
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_2D);
// glBindTexture(GL_TEXTURE_2D, texobj);
//
// glTexImage2D(GL_TEXTURE_2D, 0, image.get_components(), image.get_width(), 
//     image.get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE, image);
//
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage2D(GL_TEXTURE_2D, i+1, image.get_components(), 
//         image.get_mipmap(i).get_width(), image.get_mipmap(i).get_height(), 
//         0, image.get_format(), GL_UNSIGNED_BYTE, image.get_mipmap(i));
// }
//
// 
// Loading an uncompressed cubemap texture:
//
// DDSImage image;
// GLuint texobj;
// GLenum target;
// 
// image.load("cubemap.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_CUBE_MAP);
// glBindTexture(GL_TEXTURE_CUBE_MAP, texobj);
// 
// for (int n = 0; n < 6; n++)
// {
//     target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+n;
// 
//     glTexImage2D(target, 0, image.get_components(), image[n].get_width(), 
//         image[n].get_height(), 0, image.get_format(), GL_UNSIGNED_BYTE, 
//         image[n]);
// 
//     for (int i = 0; i < image[n].get_num_mipmaps(); i++)
//     {
//         glTexImage2D(target, i+1, image.get_components(), 
//             image[n].get_mipmap(i).get_width(), 
//             image[n].get_mipmap(i).get_height(), 0,
//             image.get_format(), GL_UNSIGNED_BYTE, image[n].get_mipmap(i));
//     }
// }
//
// 
// Loading a volume texture:
//
// DDSImage image;
// GLuint texobj;
// 
// image.load("volume.dds");
// 
// glGenTextures(1, &texobj);
// glEnable(GL_TEXTURE_3D);
// glBindTexture(GL_TEXTURE_3D, texobj);
// 
// PFNGLTEXIMAGE3DPROC glTexImage3D;
// glTexImage3D(GL_TEXTURE_3D, 0, image.get_components(), image.get_width(), 
//     image.get_height(), image.get_depth(), 0, image.get_format(), 
//     GL_UNSIGNED_BYTE, image);
// 
// for (int i = 0; i < image.get_num_mipmaps(); i++)
// {
//     glTexImage3D(GL_TEXTURE_3D, i+1, image.get_components(), 
//         image[0].get_mipmap(i).get_width(), 
//         image[0].get_mipmap(i).get_height(), 
//         image[0].get_mipmap(i).get_depth(), 0, image.get_format(), 
//         GL_UNSIGNED_BYTE, image[0].get_mipmap(i));
// }

#include "dds.h"
#include <GL/glew.h>
#include <cstdio>


static void swap(void* a, void* b, u32 size)
{
	u8 tmp;
	u8* pa = (u8*)a;
	u8* pb = (u8*)b;
	while (size--) {
		tmp = *pa;
		*pa++ = *pb;
		*pb++ = tmp;
	}
}


namespace render
{
	// DDSImage Functions

	bool DDSImage::loadFromMemory(
		u8* data,
		u32 dataSize,
		bool flipImage,
		bool sRGB)
	{
		u32 (DDSImage::*sizefunc)(u32, u32);

		u8* dp = data;

		// read in file marker, make sure its a DDS file
		if (strncmp((char*)dp, "DDS ", 4) != 0) {
			return false;
		}
		dp += 4;

		// read in DDS header
		DDSHeader& ddsh = *(DDSHeader*)dp;
		dp += sizeof(DDSHeader);

		// check if image is a cubempa
		if (ddsh.dwCaps2 & DDS_CUBEMAP) {
			cubemap = true;
		}

		// check if image is a volume texture
		if ((ddsh.dwCaps2 & DDS_VOLUME) && (ddsh.dwDepth > 0)) {
			volume = true;
		}

		// figure out what the image format is
		if (ddsh.ddspf.dwFlags & DDS_FOURCC) {
			switch (ddsh.ddspf.dwFourCC) {
				case FOURCC_DXT1:
					internalFormat = (sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
					components = 3;
					compressed = true;
					break;
				case FOURCC_DXT3:
					internalFormat = (sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
					components = 4;
					compressed = true;
					break;
				case FOURCC_DXT5:
					internalFormat = (sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
					components = 4;
					compressed = true;
					break;
				default:
					return false;
			}
		}
		else if (ddsh.ddspf.dwFlags == DDS_RGBA && ddsh.ddspf.dwRGBBitCount == 32) {
			format = GL_BGRA;
			internalFormat = (sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8);
			compressed = false;
			components = 4;
		}
		else if (ddsh.ddspf.dwFlags == DDS_RGB  && ddsh.ddspf.dwRGBBitCount == 32) {
			format = GL_BGRA;
			internalFormat = (sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8);
			compressed = false;
			components = 4;
		}
		else if (ddsh.ddspf.dwFlags == DDS_RGB  && ddsh.ddspf.dwRGBBitCount == 24) {
			format = GL_BGR;
			internalFormat = (sRGB ? GL_SRGB8 : GL_RGB8);
			compressed = false;
			components = 3;
		}
		else if (ddsh.ddspf.dwRGBBitCount == 8) {
			format = GL_RED;
			internalFormat = (sRGB ? GL_SLUMINANCE8 : GL_R8);
			compressed = false;
			components = 1;
		}
		else {
			return false;
		}

		// store primary surface width/height/depth
		u32 width = ddsh.dwWidth;
		u32 height = ddsh.dwHeight;
		u32 depth = clamp_size(ddsh.dwDepth);   // set to 1 if 0

		// use correct size calculation function depending on whether image is compressed
		sizefunc = (compressed ? &DDSImage::size_dxtc : &DDSImage::size_rgb);

		// load all surfaces for the image (6 surfaces for cubemaps)
		numImages = (cubemap ? 6 : 1);
		for (u32 n = 0;
			n < numImages;
			++n)
		{
			// calculate surface size
			u32 size = (this->*sizefunc)(width, height)*depth;

			// load surface
			DDSTexture& img = images[n];
			img.init(width, height, depth, size, dp);
			dp += size;

			// TODO: why is this done and should I bring it back? requires a buffer allocation
			//align_memory(&img);

			if (!cubemap && flipImage) {
				flip(img, width, height, depth, size);
			}

			u32 w = clamp_size(width >> 1);
			u32 h = clamp_size(height >> 1);
			u32 d = clamp_size(depth >> 1);

			// number of mipmaps in file includes main surface so decrease count by one
			img.numMipmaps = ddsh.dwMipMapCount - 1;

			assert(img.numMipmaps <= DDS_MAX_MIPMAPS && "max mipmaps reached, lower resolution or allow more mipmaps");

			// load all mipmaps for current surface
			for (u32 i = 0;
				i < img.numMipmaps;
				++i)
			{
				// calculate mipmap size
				size = (this->*sizefunc)(w, h)*d;

				DDSSurface& mipmap = img.mipmaps[i];
				mipmap.init(w, h, d, size, dp);
				dp += size;

				if (!cubemap && flipImage) {
					flip(mipmap, w, h, d, size);
				}

				// shrink to next power of 2
				w = clamp_size(w >> 1);
				h = clamp_size(h >> 1);
				d = clamp_size(d >> 1);
			}
		}

		// swap cubemaps on y axis (since image is flipped in OGL)
		if (cubemap && flipImage) {
			DDSTexture tmp;
			tmp = images[3];
			images[3] = images[2];
			images[2] = tmp;
		}

		valid = true;

		assert(dp - data == dataSize);
		return true;
	}

	// loads DDS image
	//
	// filename - fully qualified name of DDS image
	// flipImage - specifies whether image is flipped on load, default is true
	bool DDSImage::loadFromFile(
		MemoryArena& transient,
		const char* filename,
		bool flipImage,
		bool sRGB)
	{
		FILE* fp = nullptr;
		if (fopen_s(&fp, filename, "rb")) {
			// read in file
			fseek(fp, 0, SEEK_END);
			
			u32 size = ftell(fp);
			ScopedTemporaryMemory temp = scopedTemporaryMemory(transient);
			u8* dataPtr = (u8*)allocArrayOfType(transient, u8, size);
			
			fread(dataPtr, 1, size, fp);
			fclose(fp);

			return loadFromMemory(dataPtr, size, flipImage, sRGB);
		}
		return false;
	}

	// returns individual texture when multiple textures are loaded (as is the case
	// with volume textures and cubemaps)
	DDSTexture &DDSImage::operator[](u32 index)
	{
		// make sure an image has been loaded
		assert(valid && index < (u32)images.size());

		return images[index];
	}

	// returns pointer to main image
	DDSImage::operator u8*()
	{
		assert(valid);

		return images[0];
	}

	// uploads a compressed/uncompressed 1D texture
	bool DDSImage::upload_texture1D()
	{
		assert(valid && images[0] && images[0].height == 1 && images[0].width > 0);

		u32 numMipMaps = images[0].numMipmaps;

		glTexStorage1D(GL_TEXTURE_1D, numMipMaps + 1, internalFormat, images[0].width);

		if (compressed) {
			glCompressedTexSubImage1D(
				GL_TEXTURE_1D, 0, 0,
				images[0].width,
				internalFormat,
				images[0].size,
				images[0]);
			
			// load all mipmaps
			if (numMipMaps > 0) {
				for (u32 i = 0; i < numMipMaps; ++i) {
					glCompressedTexSubImage1D(
						GL_TEXTURE_1D, i + 1, 0,
						images[0].mipmaps[i].width,
						internalFormat,
						images[0].mipmaps[i].size,
						images[0].mipmaps[i]);
				}
				// TODO: this would be in Texture1D_GL
				//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
				//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, numMipMaps);
			}
		}
		else {
			glTexSubImage1D(GL_TEXTURE_1D, 0, 0, images[0].width, format, GL_UNSIGNED_BYTE, images[0]);

			// load all mipmaps
			if (numMipMaps > 0) {
				for (u32 i = 0; i < numMipMaps; ++i) {
					glTexSubImage1D(GL_TEXTURE_1D, i + 1, 0, images[0].width, format, GL_UNSIGNED_BYTE, images[0].mipmaps[i]);
				}
				// TODO: this would be in Texture1D_GL
				//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
				//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, numMipMaps);
			}
		}

		return true;
	}

	// uploads a compressed/uncompressed 2D texture
	//
	// imageIndex - allows you to optionally specify other loaded surfaces for 2D
	//              textures such as a face in a cubemap or a slice in a volume
	//
	//              default: 0
	//
	// target     - allows you to optionally specify a different texture target for
	//              the 2D texture such as a specific face of a cubemap
	//
	//              default: GL_TEXTURE_2D
	bool DDSImage::upload_texture2D(u32 imageIndex, GLenum target)
	{
		assert(valid && imageIndex >= 0 && imageIndex < (u32)images.size());
		assert(images[imageIndex] && images[imageIndex].height > 0 && images[imageIndex].width > 0);
		assert(target == GL_TEXTURE_2D || (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));

		u32 numMipMaps = (u32)images[imageIndex].numMipmaps;

		if (target == GL_TEXTURE_2D) {
			glTexStorage2D(target, numMipMaps + 1, internalFormat, images[imageIndex].width, images[imageIndex].height);
		}

		if (compressed) {
			glCompressedTexSubImage2D(
				target, 0, 0, 0,
				images[imageIndex].width, images[imageIndex].height,
				internalFormat, images[imageIndex].size,
				images[imageIndex]);
									
			// load all mipmaps
			if (numMipMaps > 0) {
				for (u32 i = 0; i < numMipMaps; ++i) {
					glCompressedTexSubImage2D(
						target, i + 1, 0, 0,
						images[imageIndex].mipmaps[i].width, images[imageIndex].mipmaps[i].height,
						internalFormat, images[imageIndex].mipmaps[i].size,
						images[imageIndex].mipmaps[i]);
				}
			}
		}
		else {
			glTexSubImage2D(
				target, 0, 0, 0,
				images[imageIndex].width, images[imageIndex].height,
				format, GL_UNSIGNED_BYTE, images[imageIndex]);
									
			// load all mipmaps
			if (numMipMaps > 0) {
				for (u32 i = 0; i < numMipMaps; ++i) {
					glTexSubImage2D(
						target, i + 1, 0, 0,
						images[imageIndex].mipmaps[i].width, images[imageIndex].mipmaps[i].height,
						format, GL_UNSIGNED_BYTE, images[imageIndex].mipmaps[i]);
				}
			}
		}

		ASSERT_GL_ERROR;
		return true;
	}

	bool DDSImage::upload_textureRectangle()
	{
		assert(valid && images.size() >= 1);

		if (!upload_texture2D(0, GL_TEXTURE_RECTANGLE)) {
			return false;
		}

		return true;
	}

	// uploads a compressed/uncompressed cubemap texture
	bool DDSImage::upload_textureCubemap(bool swapY)
	{
		assert(valid && cubemap && images.size() == 6);

		bool result = true;
		u32 numMipMaps = images[0].numMipmaps;

		glTexStorage2D(GL_TEXTURE_CUBE_MAP, numMipMaps + 1, internalFormat, images[0].width, images[0].height);

		GLenum target = 0;

		// loop through cubemap faces and load them as 2D textures 
		for (u32 n = 0; n < 6; ++n) {
			// specify cubemap face
			target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + n;

			if (swapY && target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y) {
				target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
			}
			else if (swapY && target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y) {
				target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
			}

			result = upload_texture2D(n, target);
		}

		ASSERT_GL_ERROR;
		return result;
	}

	// uploads a compressed/uncompressed 3D texture
	bool DDSImage::upload_texture3D()
	{
		assert(valid && volume && images[0].depth >= 1);

		u32 numMipMaps = images[0].numMipmaps;

		glTexStorage3D(GL_TEXTURE_3D, numMipMaps + 1, internalFormat, images[0].width, images[0].height, images[0].depth);

		if (compressed) {
			glCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
									images[0].width, images[0].height, images[0].depth,
									internalFormat, images[0].size, images[0]);

			// load all mipmap volumes
			if (numMipMaps > 0) {
				for (u32 i = 0; i < numMipMaps; ++i) {
					glCompressedTexSubImage3D(GL_TEXTURE_3D, i + 1, 0, 0, 0,
											images[0].mipmaps[i].width, images[0].mipmaps[i].height, images[0].depth,
											internalFormat, images[0].mipmaps[i].size, images[0].mipmaps[i]);
				}
				// TODO: this would be in Texture3D_GL
				//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
				//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, numMipMaps);
			}
		}
		else {
			glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
							images[0].width, images[0].height, images[0].depth,
							format, GL_UNSIGNED_BYTE, images[0]);

			// load all mipmap volumes
			if (numMipMaps > 0) {
				for (u32 i = 0; i < numMipMaps; ++i) {
					glTexSubImage3D(GL_TEXTURE_3D, i + 1, 0, 0, 0,
									images[0].mipmaps[i].width, images[0].mipmaps[i].height, images[0].mipmaps[i].depth,
									format, GL_UNSIGNED_BYTE, images[0].mipmaps[i]);
				}
				// TODO: this would be in Texture3D_GL
				//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
				//glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, numMipMaps);
			}
		}
		return true;
	}

	// clamps input size to [1-size]
	inline u32 DDSImage::clamp_size(u32 size)
	{
		if (size <= 0) {
			size = 1;
		}
		return size;
	}

	// DDSImage private functions

	// calculates 4-byte aligned width of image
	inline u32 DDSImage::get_line_width(u32 width, u32 bpp)
	{
		return ((width * bpp + 31) & -32) >> 3;
	}

	// calculates size of DXTC texture in bytes
	inline u32 DDSImage::size_dxtc(u32 width, u32 height)
	{
		return ((width + 3) / 4) * ((height + 3) / 4) *
			(internalFormat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT ||
			internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
	}

	// calculates size of uncompressed RGB texture in bytes
	inline u32 DDSImage::size_rgb(u32 width, u32 height)
	{
		return width*height*components;
	}

	// align to 4 byte boundary (add pad bytes to end of each line in the image)
/*	void DDSImage::align_memory(DDSTexture* surface)
	{
		// don't bother with compressed images, volume textures, or cubemaps
		if (compressed || volume || cubemap) {
			return;
		}

		// calculate new image size
		u32 linesize = get_line_width(surface->width, components * 8);
		u32 imagesize = linesize*surface->height;

		// exit if already aligned
		if (surface->size == imagesize) {
			return;
		}

		// create new image of new size
		DDSTexture newSurface{};
		newSurface.create(surface->width, surface->height, surface->depth, imagesize);

		// add pad bytes to end of each line
		u8* srcimage = (u8*)*surface;
		u8* dstimage = (u8*)newSurface;
		for (u32 n = 0; n < surface->depth; ++n) {
			u8* curline = srcimage;
			u8* newline = dstimage;

			u32 imsize = surface->size / surface->depth;
			u32 lnsize = imsize / surface->height;

			for (u32 i = 0; i < surface->height; ++i) {
				memcpy(newline, curline, lnsize);
				newline += linesize;
				curline += lnsize;
			}
		}

		// save padded image
		*surface = newSurface;
	}
*/

	// flip image around X axis
	void DDSImage::flip(u8* image, u32 width, u32 height, u32 depth, u32 size)
	{
		u32 linesize;
		u32 offset;

		if (!compressed) {
			assert(depth > 0);

			u32 imagesize = size / depth;
			linesize = imagesize / height;

			for (u32 n = 0; n < depth; ++n) {
				offset = imagesize*n;
				u8* top = image + offset;
				u8* bottom = top + (imagesize - linesize);

				for (u32 i = 0; i < (height >> 1); ++i) {
					swap(bottom, top, linesize);

					top += linesize;
					bottom -= linesize;
				}
			}
		}
		else {
			void (DDSImage::*flipblocks)(DXTColBlock*, u32);
			u32 xblocks = width / 4;
			u32 yblocks = height / 4;
			u32 blocksize;

			switch (internalFormat) {
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: // yes, the fall through is on purpose
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
					blocksize = 8;
					flipblocks = &DDSImage::flip_blocks_dxtc1;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
					blocksize = 16;
					flipblocks = &DDSImage::flip_blocks_dxtc3;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
					blocksize = 16;
					flipblocks = &DDSImage::flip_blocks_dxtc5;
					break;
				default:
					return;
			}

			linesize = xblocks * blocksize;

			DXTColBlock* top;
			DXTColBlock* bottom;

			for (u32 j = 0; j < (yblocks >> 1); ++j) {
				top = (DXTColBlock*)(image + j * linesize);
				bottom = (DXTColBlock*)(image + (((yblocks - j) - 1) * linesize));

				(this->*flipblocks)(top, xblocks);
				(this->*flipblocks)(bottom, xblocks);

				swap(bottom, top, linesize);
			}
		}
	}

	// swap to sections of memory
	/*void DDSImage::swap(void* byte1, void* byte2, u32 size)
	{
		// TODO: get from temp transient memory
		u8* tmp = new u8[size];

		memcpy(tmp, byte1, size);
		memcpy(byte1, byte2, size);
		memcpy(byte2, tmp, size);

		delete[] tmp;
	}*/

	// flip a DXT1 color block
	void DDSImage::flip_blocks_dxtc1(DXTColBlock* line, u32 numBlocks)
	{
		DXTColBlock* curblock = line;

		for (u32 i = 0; i < numBlocks; ++i) {
			swap(&curblock->row[0], &curblock->row[3], sizeof(u8));
			swap(&curblock->row[1], &curblock->row[2], sizeof(u8));

			++curblock;
		}
	}

	// flip a DXT3 color block
	void DDSImage::flip_blocks_dxtc3(DXTColBlock* line, u32 numBlocks)
	{
		DXTColBlock* curblock = line;
		DXT3AlphaBlock* alphablock;

		for (u32 i = 0; i < numBlocks; ++i) {
			alphablock = (DXT3AlphaBlock*)curblock;

			swap(&alphablock->row[0], &alphablock->row[3], sizeof(u16));
			swap(&alphablock->row[1], &alphablock->row[2], sizeof(u16));

			++curblock;

			swap(&curblock->row[0], &curblock->row[3], sizeof(u8));
			swap(&curblock->row[1], &curblock->row[2], sizeof(u8));

			++curblock;
		}
	}

	// flip a DXT5 alpha block
	void DDSImage::flip_dxt5_alpha(DXT5AlphaBlock* block)
	{
		u8 gBits[4][4];

		const u32 mask = 0x00000007;          // bits = 00 00 01 11
		u32 bits = 0;
		memcpy(&bits, &block->row[0], sizeof(u8) * 3);

		gBits[0][0] = (u8)(bits & mask);
		bits >>= 3;
		gBits[0][1] = (u8)(bits & mask);
		bits >>= 3;
		gBits[0][2] = (u8)(bits & mask);
		bits >>= 3;
		gBits[0][3] = (u8)(bits & mask);
		bits >>= 3;
		gBits[1][0] = (u8)(bits & mask);
		bits >>= 3;
		gBits[1][1] = (u8)(bits & mask);
		bits >>= 3;
		gBits[1][2] = (u8)(bits & mask);
		bits >>= 3;
		gBits[1][3] = (u8)(bits & mask);

		bits = 0;
		memcpy(&bits, &block->row[3], sizeof(u8) * 3);

		gBits[2][0] = (u8)(bits & mask);
		bits >>= 3;
		gBits[2][1] = (u8)(bits & mask);
		bits >>= 3;
		gBits[2][2] = (u8)(bits & mask);
		bits >>= 3;
		gBits[2][3] = (u8)(bits & mask);
		bits >>= 3;
		gBits[3][0] = (u8)(bits & mask);
		bits >>= 3;
		gBits[3][1] = (u8)(bits & mask);
		bits >>= 3;
		gBits[3][2] = (u8)(bits & mask);
		bits >>= 3;
		gBits[3][3] = (u8)(bits & mask);

		// clear existing alpha bits
		memset(block->row, 0, sizeof(u8) * 6);

		u32 *pBits = ((u32*)&(block->row[0]));

		*pBits = *pBits | (gBits[3][0] << 0);
		*pBits = *pBits | (gBits[3][1] << 3);
		*pBits = *pBits | (gBits[3][2] << 6);
		*pBits = *pBits | (gBits[3][3] << 9);

		*pBits = *pBits | (gBits[2][0] << 12);
		*pBits = *pBits | (gBits[2][1] << 15);
		*pBits = *pBits | (gBits[2][2] << 18);
		*pBits = *pBits | (gBits[2][3] << 21);

		pBits = ((u32*)&(block->row[3]));

		*pBits = *pBits | (gBits[1][0] << 0);
		*pBits = *pBits | (gBits[1][1] << 3);
		*pBits = *pBits | (gBits[1][2] << 6);
		*pBits = *pBits | (gBits[1][3] << 9);

		*pBits = *pBits | (gBits[0][0] << 12);
		*pBits = *pBits | (gBits[0][1] << 15);
		*pBits = *pBits | (gBits[0][2] << 18);
		*pBits = *pBits | (gBits[0][3] << 21);
	}

	// flip a DXT5 color block
	void DDSImage::flip_blocks_dxtc5(DXTColBlock* line, u32 numBlocks)
	{
		DXTColBlock* curblock = line;
		DXT5AlphaBlock* alphablock;

		for (u32 i = 0; i < numBlocks; ++i) {
			alphablock = (DXT5AlphaBlock*)curblock;

			flip_dxt5_alpha(alphablock);

			++curblock;

			swap(&curblock->row[0], &curblock->row[3], sizeof(u8));
			swap(&curblock->row[1], &curblock->row[2], sizeof(u8));

			++curblock;
		}
	}


	// DDSSurface Functions

	DDSSurface::operator u8*()
	{
		return pixels;
	}

	// TODO: make a non-member function
	void DDSSurface::init(
		u32 w, u32 h, u32 d,
		u32 imgsize,
		u8* data)
	{
		width = w;
		height = h;
		depth = d;
		size = imgsize;
		pixels = data;
	}

}