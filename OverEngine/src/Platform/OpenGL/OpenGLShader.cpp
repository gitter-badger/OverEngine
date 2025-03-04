#include "pcheader.h"
#include "OpenGLShader.h"

#include "OpenGLIntermediateShader.h"

#include "OverEngine/Core/FileSystem/FileSystem.h"

#include <glad/gl.h>
#include <fstream>

namespace OverEngine
{
	static GLenum ShaderTypeFromString(const String& type)
	{
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;

		OE_CORE_ASSERT(false, "Unknown shader type '{0}'!", type);
		return 0;
	}

	OpenGLShader::OpenGLShader(const String& filePath)
		: m_FilePath(filePath)
	{
		Compile(PreProcess(FileSystem::ReadFile(filePath)));

		auto lastSlash = filePath.find_last_of("/\\");
		lastSlash = lastSlash == String::npos ? 0 : lastSlash + 1;
		auto lastDot = filePath.rfind('.');
		auto count = lastDot == String::npos ? filePath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filePath.substr(lastSlash, count);
	}

	OpenGLShader::OpenGLShader(const String& name, const String& vertexSrc, const String& fragmentSrc)
		: m_Name(name)
	{
		UnorderedMap<GLenum, const char*> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc.c_str();
		sources[GL_FRAGMENT_SHADER] = fragmentSrc.c_str();
		Compile(sources);
	}

	OpenGLShader::OpenGLShader(const String& name, const char* vertexSrc, const char* fragmentSrc)
	{
		UnorderedMap<GLenum, const char*> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);
	}

	OpenGLShader::OpenGLShader(const String& name, const Ref<IntermediateShader>& vertexShader, const Ref<IntermediateShader>& fragmentShader)
		: m_Name(name)
	{
		m_RendererID = glCreateProgram();

		uint32_t vsID = vertexShader->GetRendererID();
		uint32_t fsID = fragmentShader->GetRendererID();

		glAttachShader(m_RendererID, vsID);
		glAttachShader(m_RendererID, fsID);

		glLinkProgram(m_RendererID);

		GLint isLinked = 0;
		glGetProgramiv(m_RendererID, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &maxLength);

			Vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram(m_RendererID);

			OE_CORE_ERROR("{0}", infoLog.data());
			OE_CORE_ASSERT(false, "Shader link failure!");
			return;
		}

		glDetachShader(m_RendererID, vsID);
		glDetachShader(m_RendererID, fsID);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	UnorderedMap<GLenum, String> OpenGLShader::PreProcess(const String& source)
	{
		UnorderedMap<GLenum, String> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0); // Start of shader type declaration line
		while (pos != String::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos); // End of shader type declaration line
			OE_CORE_ASSERT(eol != String::npos, "Syntax error!");
			size_t begin = pos + typeTokenLength + 1; // Start of shader type name (after "#type " keyword)
			String type = source.substr(begin, eol - begin);
			OE_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified!");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol); // Start of shader code after shader type declaration line
			OE_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
			pos = source.find(typeToken, nextLinePos); // Start of next shader type declaration line
			shaderSources[ShaderTypeFromString(type)] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
		}

		return shaderSources;
	}

	void OpenGLShader::Compile(const UnorderedMap<GLenum, String>& shaderSources)
	{
		UnorderedMap<GLenum, const char*> sources;
		for (auto& src : shaderSources)
			sources[src.first] = src.second.c_str();
		Compile(sources);
	}

	void OpenGLShader::Compile(const UnorderedMap<GLenum, const char*>& shaderSources)
	{
		GLuint program = glCreateProgram();

		OE_CORE_ASSERT(shaderSources.size() <= 2, "{0} shader sources got but 2 is maximim", shaderSources.size());

		std::array<GLenum, 2> glShaderIDs;
		int glShaderIdIndex = 0;
		bool allCompiled = true;

		for (auto& src : shaderSources)
		{
			GLenum type = src.first;
			const char* source = src.second;

			GLuint shader = glCreateShader(type);

			glShaderSource(shader, 1, &source, nullptr);

			glCompileShader(shader);

			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

				Vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

				glDeleteShader(shader);
				glDeleteProgram(program);

				OE_CORE_ERROR("{0}", infoLog.data());
				OE_THROW(Exception("Shader compilation failure!"));
				allCompiled = false;

				break;
			}

			glAttachShader(program, shader);

			glShaderIDs[glShaderIdIndex++] = shader;
		}

		if (allCompiled)
		{
			m_RendererID = program;

			glLinkProgram(program);

			GLint isLinked = 0;
			glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
			if (isLinked == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

				Vector<GLchar> infoLog(maxLength);
				glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());

				glDeleteProgram(program);

				for (auto id : glShaderIDs)
					glDeleteShader(id);

				std::ofstream a("log.txt");
				a << infoLog.data();
				a.flush();
				a.close();

				OE_CORE_ERROR("{0}", infoLog.data());
				OE_CORE_ASSERT(false, "Shader link failure!");
				return;
			}

			for (auto id : glShaderIDs)
			{
				glDetachShader(program, id);
				glDeleteShader(id);
			}
		}
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::UploadUniformInt(const char* name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const char* name, const int* value, int count)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniform1iv(location, count, value);
	}

	void OpenGLShader::UploadUniformFloat(const char* name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const char* name, const Math::Vector2& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat3(const char* name, const Math::Vector3& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat4(const char* name, const Math::Vector4& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::UploadUniformMat3(const char* name, const Math::Mat3x3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const char* name, const Math::Mat4x4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name);
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	bool OpenGLShader::Reload(String filePath)
	{
		if (filePath.empty())
		{
			if (m_FilePath.empty())
				return false;

			Compile(PreProcess(FileSystem::ReadFile(m_FilePath)));
			return true;
		}

		Compile(PreProcess(FileSystem::ReadFile(filePath)));
		return true;
	}

	bool OpenGLShader::Reload(const String& vertexSrc, const String& fragmentSrc)
	{
		UnorderedMap<GLenum, const char*> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc.c_str();
		sources[GL_FRAGMENT_SHADER] = fragmentSrc.c_str();
		Compile(sources);
		return true;
	}

	bool OpenGLShader::Reload(const char* vertexSrc, const char* fragmentSrc)
	{
		UnorderedMap<GLenum, const char*> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);
		return true;
	}
}
