#ifndef _SHADER_GL_H
#define _SHADER_GL_H

#include "utility/common.h"


typedef h32 ShaderId;

namespace render
{
	/**
	 * Enum of all standard uniform blocks. These are bound by name using an enum-to-string macro,
	 * so exact name matching is important. The value corresponds to the binding point index with
	 * 1 added to the value.
	 */
	enum UBOType : u8 {
		CameraUniforms = 0,
		ObjectUniforms,
		_UBOType_Count
	};

	/**
	 * Standard vertex buffer layouts.
	 */
	enum VertexLayoutLocation : u8 {
		VertexLayout_Position      = 0,
		VertexLayout_Normal        = 1,
		VertexLayout_Tangent       = 2,
		VertexLayout_Bitangent     = 3,
		VertexLayout_TextureCoords = 4,  // consumes up to 8 locations
		VertexLayout_Colors        = 12  // consumes up to 8 locations
	};

	enum SamplerBindingLocation : u8 {
		SamplerBinding_Diffuse1 = 4, // sampler bindings start at 4 because 0-3 are for G-buffer MRT
		SamplerBinding_Diffuse2,
		SamplerBinding_Diffuse3,
		SamplerBinding_Diffuse4
	};

	/**
	 * GLSL layout(std140)
	 */
	struct CameraUniformsUBO {
		mat4	projection;
		mat4	viewProjection;
		r32		frustumNear;
		r32		frustumFar;
		r32		inverseFrustumDistance;
		r32		_padding;
	};

	/**
	 * GLSL layout(std140)
	 */
	struct ObjectUniformsUBO {
		mat4	modelToWorld;
		mat4	modelView;
		mat4	modelViewProjection;
		mat4	normalMatrix;
	};


	struct Shader_GL
	{
		u32		shaderId;
		u32		shaderType;


		bool compileShader(
			const char* shaderSource,
			u32 shaderType);

		void destroy();
	};
	

	struct ShaderProgram_GL
	{
		u32			programId;
		u32			numShaders;
		u32			blockIndex[_UBOType_Count];	// block index of each UBO type
		Shader_GL	shaders[5];
		char*		shaderCode;
		char*		preprocessorMacros;
		char*		programPath;


		/**
		 * @param	shaderCode	null-terminated string containing shader code, if nullptr is
		 *	passed, code is taken from shaderCode which must have been set at construction.
		 * @returns	true if compilation and link succeed, false on failure
		 */
		bool compileAndLinkProgram(const char* shaderCode = nullptr);

		void bindUniformBuffer(UBOType uboType, u32 uboHandle);

		bool loadProgramBinaryFromMemory(u8* data, size_t size);
		bool loadProgramBinaryFromFile(const char* filename);
		bool writeProgramBinaryFile(const char* filename) const;

//		void deserialize(std::istream& in);
//		void serialize(std::ostream& out);

		void addPreprocessorMacro(const char* preprocessor)
		{
			// TODO: implement this with c strings
			//preprocessorMacros += string(preprocessor) + "\n";
		}

		void useProgram() const;
		
		void destroy();
	};

}

#endif