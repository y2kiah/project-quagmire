#include <GL/glew.h>
#include "shader_gl.h"
#include "utility/logger.h"


namespace render
{

	// Shader_GL Functions

	void Shader_GL::destroy()
	{
		logger::verbose(logger::Category_Render, "deleting shader with shaderId = %d", shaderId);
		if (shaderId != 0) {
			logger::verbose(logger::Category_Render, "deleting shader in opengl land");
			glDeleteShader(shaderId);
			shaderId = 0;
		}
	}

	bool Shader_GL::compileShader(const char* shaderSource, u32 shaderType)
	{
		GLint result = GL_FALSE;
		int infoLogLength = 0;

		// Compile Shader
		logger::verbose(logger::Category_Render, "  compiling shader %u", shaderType);

		GLuint shaderId = glCreateShader(shaderType);
		
		glShaderSource(shaderId, 1, &shaderSource, nullptr);
		glCompileShader(shaderId);

		// Check Shader
		glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0) {
			vector<char> shaderErrorMessage(infoLogLength);
			glGetShaderInfoLog(shaderId, infoLogLength, nullptr, &shaderErrorMessage[0]);
			logger::error(logger::Category_Render, &shaderErrorMessage[0]);
		}

		if (result == GL_TRUE) {
			shaderId = shaderId;
			shaderType = shaderType;
			return true;
		}
		
		return false;
	}


	// ShaderProgram_GL Functions

	void ShaderProgram_GL::destroy()
	{
		if (m_programId != 0) {
			logger::verbose(logger::Category_Render, "deleting program");
			glDeleteProgram(m_programId);
		}
	}

	bool ShaderProgram_GL::compileAndLinkProgram(const char* shaderCode) {
		if (shaderCode == nullptr) {
			shaderCode = m_shaderCode.c_str();
			assert(m_shaderCode.length() > 0 && "no shader code set");
		}
		else {
			m_shaderCode = shaderCode;
		}
		
		string programPath(m_programPath.begin(), m_programPath.end());
		logger::verbose(logger::Category_Render, "compiling program %s", programPath.c_str());

		bool hasGeometryStage    = (m_shaderCode.find("_GEOMETRY_", 0) != string::npos);
		bool hasTessControlStage = (m_shaderCode.find("_TESS_CONTROL_", 0) != string::npos);
		bool hasTessEvalStage    = (m_shaderCode.find("_TESS_EVAL_", 0) != string::npos);

		// Compile code as vertex shader (defines _VERTEX_)
		int currentShader = 0;
		string shaderSource = "#version 440 core\n#define _VERTEX_\n" + m_preprocessorMacros + m_shaderCode;
		bool ok = m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_VERTEX_SHADER);
		
		if (hasTessControlStage) {
			// Compile code as tesselation control shader (defines _TESS_CONTROL_)
			++currentShader;
			shaderSource = "#version 440 core\n#define _TESS_CONTROL_\n" + m_preprocessorMacros + m_shaderCode;
			ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_TESS_CONTROL_SHADER);
		}

		if (hasTessEvalStage) {
			// Compile code as tesselation evaluation shader (defines _TESS_EVAL_)
			++currentShader;
			shaderSource = "#version 440 core\n#define _TESS_EVAL_\n" + m_preprocessorMacros + m_shaderCode;
			ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_TESS_EVALUATION_SHADER);
		}

		if (hasGeometryStage) {
			// Compile code as geometry shader (defines _GEOMETRY_)
			++currentShader;
			shaderSource = "#version 440 core\n#define _GEOMETRY_\n" + m_preprocessorMacros + m_shaderCode;
			ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_GEOMETRY_SHADER);
		}

		// Compile code as fragment shader (defines _FRAGMENT_)
		++currentShader;
		shaderSource = "#version 440 core\n#define _FRAGMENT_\n" + m_preprocessorMacros + m_shaderCode;
		ok = ok && m_shaders[currentShader].compileShader(shaderSource.c_str(), GL_FRAGMENT_SHADER);

		m_numShaders = currentShader + 1;

		// Link the program
		if (ok) {
			logger::verbose(logger::Category_Render, "  linking program %s", programPath.c_str());
			GLuint programId = glCreateProgram();
			for (const auto& s : m_shaders) {
				if (s.getShaderType() != 0) {
					glAttachShader(programId, s.getShaderId());
				}
			}
			glLinkProgram(programId);

			// Check the program
			GLint result = GL_FALSE;
			int infoLogLength = 0;

			glGetProgramiv(programId, GL_LINK_STATUS, &result);
			glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0) {
				vector<char> programErrorMessage(infoLogLength);
				glGetProgramInfoLog(programId, infoLogLength, nullptr, &programErrorMessage[0]);
				logger::error(logger::Category_Render, &programErrorMessage[0]);
			}

			/*if (hasGeometryStage) {
				glProgramParameteriEXT(programId, GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
				glProgramParameteriEXT(programId, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
				glProgramParameteriEXT(programId, GL_GEOMETRY_VERTICES_OUT_EXT, 3);
			}*/

			ok = (result == GL_TRUE);
			if (ok) {
				m_programId = programId;

				// get block index for all uniform blocks
				for (int ubo = 0; ubo < UBOTypeCount; ++ubo) {
					m_blockIndex[ubo] = glGetUniformBlockIndex(m_programId, UBOTypeToString(static_cast<UBOType>(ubo)));

					#ifdef _DEBUG
					if (m_blockIndex[ubo] != -1) {
						GLint blockSize = 0;
						glGetActiveUniformBlockiv(m_programId, m_blockIndex[ubo], GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
						if (ubo == CameraUniforms) {
							assert(blockSize == sizeof(CameraUniformsUBO));
						}
						else if (ubo == ObjectUniforms) {
							assert(blockSize == sizeof(ObjectUniformsUBO));
						}
						logger::verbose(logger::Category_Render, "  uniform block \"%s\" index %u", UBOTypeToString(static_cast<UBOType>(ubo)), m_blockIndex[ubo]);
					}
					#endif
				}
			}
		}

		ASSERT_GL_ERROR;
		return ok;
	}

	void ShaderProgram_GL::bindUniformBuffer(UBOType uboType, u32 uboHandle)
	{
		if (m_blockIndex[uboType] != -1 && uboHandle != 0) {
			int m = 0;
			glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &m);
			logger::debug(logger::Category_Video, "GL_MAX_UNIFORM_BUFFER_BINDINGS = %d", m);
			glUniformBlockBinding(m_programId, m_blockIndex[uboType], uboType+1);
			glBindBufferBase(GL_UNIFORM_BUFFER, uboType+1, uboHandle);
			
			assert(uboType + 1 <= 84); // 84 is the minimum number of binding points, should be more than enough
		}

		ASSERT_GL_ERROR;
	}


	/**
	 * Shader_GL binary file header, contains all properties needed for serialization
	 */
	struct Shader_GL_Header {
		u32 binaryFormat;
		int programBinarySize;
	};

	
	bool ShaderProgram_GL::loadProgramBinaryFromMemory(u8* data, size_t size)
	{
		Shader_GL_Header* header = (Shader_GL_Header*)data;
		void* shaderBinary = header + sizeof(Shader_GL_Header);

		// load the binary into the program object
		glProgramBinary(m_programId, header->binaryFormat, shaderBinary, header->programBinarySize);

		GLint success = 0;
		glGetProgramiv(m_programId, GL_LINK_STATUS, &success);

		if (!success) {
		
			// Something must have changed since the program binaries
			// were cached away.  Fallback to source shader loading path,
			// and then retrieve and cache new program binaries once again.
		}

		return (success == 1);
	}


	bool ShaderProgram_GL::loadProgramBinaryFromFile(const char* filename)
	{
		// read the program binary
		FILE* inFile = nullptr;
		
		if (_fopen_s(&inFile, filename, "rb")) {
			fseek(inFile, 0, SEEK_END);
			auto size = ftell(inFile);
			auto dataPtr = std::make_unique<unsigned char[]>(size);
			
			// move this part to deserialize function, switch to istream file
			fseek(inFile, 0, SEEK_SET);
			fread((void*)dataPtr.get(), 1, size, inFile);
			fclose(inFile);

			return loadProgramBinaryFromMemory(dataPtr.get(), size);
		}
		return false;
	}
	

	bool ShaderProgram_GL::writeProgramBinaryFile(const char* filename) const
	{
		if (m_programId == 0) {
			return false;
		}

		// get the program binary
		GLint binaryLength = 0;
		GLenum binaryFormat = 0;
		glGetProgramiv(m_programId, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
		auto binary = std::make_unique<char[]>(binaryLength);
		glGetProgramBinary(m_programId, binaryLength, nullptr, &binaryFormat, (void*)binary.get());

		// write the program binary file
		FILE* outFile = nullptr;
		if (!_fopen_s(&outFile, filename, "wb")) {
			return false;
		}

		// move this part to serialize function, switch to ostream file
		Shader_GL_Header header{};
		header.binaryFormat = binaryFormat;
		header.programBinarySize = binaryLength;

		fwrite(&header, sizeof(Shader_GL_Header), 1, outFile);
		fwrite(binary.get(), 1, binaryLength, outFile);
		fclose(outFile);

		return true;
	}
	
	
	void ShaderProgram_GL::deserialize(std::istream& in)
	{}


	void ShaderProgram_GL::serialize(std::ostream& out)
	{}


	void ShaderProgram_GL::useProgram() const
	{
		glUseProgram(m_programId);

		// TODO, do we need to bind uniform buffers again for each program change?
		//glBindBuffer(GL_UNIFORM_BUFFER, renderSystem.getUBOHandle(ObjectUniforms));
	}

}