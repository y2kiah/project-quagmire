#ifndef _BUFFERS_GL_H
#define _BUFFERS_GL_H

#include "utility/common.h"
#include <GL/glew.h>

namespace render
{

	struct IndexBuffer_GL
	{
		enum IndexBufferFlags : u8 {
			IndexBuffer_None  = 0,
			IndexBuffer_8bit  = 1, // 8bit, 16bit and 32bit are mutually exclusive
			IndexBuffer_16bit = 2,
			IndexBuffer_32bit = 4
		};

		size_t	sizeBytes;
		u32		glIndexBuffer; // gl index buffer id
		u8		flags;

		u8		_padding[3];


		u32 getIndexType() const;

		bool loadFromMemory(
			const u8* data,
			size_t size,
			u32 sizeOfElement = sizeof(u32));

		void bind() const;
		void destroy();

		inline static IndexBufferFlags getSizeFlag(u32 sizeOfElement);
		inline static u32 getSizeOfElement(IndexBufferFlags flags);
	};


	IndexBuffer_GL::IndexBufferFlags IndexBuffer_GL::getSizeFlag(u32 sizeOfElement)
	{
		if (sizeOfElement == sizeof(u8)) {
			return IndexBuffer_8bit;
		}
		else if (sizeOfElement == sizeof(u16)) {
			return IndexBuffer_16bit;
		}
		return IndexBuffer_32bit;
	}
	
	u32 IndexBuffer_GL::getSizeOfElement(IndexBuffer_GL::IndexBufferFlags flags)
	{
		if (flags & IndexBuffer_8bit) {
			return sizeof(u8);
		}
		else if (flags & IndexBuffer_16bit) {
			return sizeof(u16);
		}
		return sizeof(u32);
	}

	u32 IndexBuffer_GL::getIndexType() const
	{
		GLenum type = GL_UNSIGNED_INT;
		if (flags & IndexBuffer_16bit) {
			type = GL_UNSIGNED_SHORT;
		}
		else if (flags & IndexBuffer_8bit) {
			type = GL_UNSIGNED_BYTE;
		}
		return type;
	}

	bool IndexBuffer_GL::loadFromMemory(
		const u8* data,
		size_t size,
		u32 sizeOfElement)
	{
		// generate the buffer
		glGenBuffers(1, &glIndexBuffer);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glIndexBuffer);

		// send data to OpenGL
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

		sizeBytes = size;

		flags |= (u8)getSizeFlag(sizeOfElement);

		ASSERT_GL_ERROR;
		return (glIndexBuffer != 0);
	}

	void IndexBuffer_GL::bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glIndexBuffer);
		ASSERT_GL_ERROR;
	}

	void IndexBuffer_GL::destroy()
	{
		if (glIndexBuffer != 0) {
			glDeleteBuffers(1, &glIndexBuffer);
			glIndexBuffer = 0;
		}
	}


	/**
	 * Vertex buffers do not necessarily store homogeneous vertex data. There may be ranges of
	 * vertices within the buffer that include different components (e.g. position, normal,
	 * color(s), texture coordinate(s), tangent and bitangent). The DrawSet structure contains
	 * the information necessary to interpret a range of vertex data within the buffer.
	 */
	struct VertexBuffer_GL
	{
		size_t	sizeBytes;
		u32		glVertexBuffer; // gl vertex buffer id


		bool loadFromMemory(
			const u8* data,
			size_t size);

		void bind() const;
		void destroy();
	};


	bool VertexBuffer_GL::loadFromMemory(
		const u8* data,
		size_t size)
	{
		// generate the buffer
		glGenBuffers(1, &glVertexBuffer);

		glBindBuffer(GL_ARRAY_BUFFER, glVertexBuffer);

		// send data to OpenGL
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

		sizeBytes = size;

		ASSERT_GL_ERROR;
		return (glVertexBuffer != 0);
	}

	void VertexBuffer_GL::bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, glVertexBuffer);
		ASSERT_GL_ERROR;
	}

	void VertexBuffer_GL::destroy()
	{
		if (glVertexBuffer != 0) {
			glDeleteBuffers(1, &glVertexBuffer);
			glVertexBuffer = 0;
		}
	}
}

#endif