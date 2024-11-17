#pragma once
#include "AbstractShader.h"
class Shader : public AbstractShader
{
public:
	Shader() : AbstractShader() {}
	Shader(const char* vPath, const char* fPath) : AbstractShader() {
		LoadShader(vPath, fPath);
	}
	Shader(const char* vPath, const char* fPath, const char* gPath) : AbstractShader() {
		LoadShader(vPath, fPath, gPath);
	}
	~Shader() {}

	bool LoadShader(const char* vPath, const char* fPath) {
		// 1. retrieve source code
		std::string vCode;
		std::string fCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		// ensure ifstream objects can throw exceptions
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			vShaderFile.open(vPath);
			fShaderFile.open(fPath);
			std::stringstream vShaderStream, fShaderStream;

			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();

			vShaderFile.close();
			fShaderFile.close();

			vCode = vShaderStream.str();
			fCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e) {
			Logger::LogError("SHADER::FILE_NOT_READ_SUCCESFULLY");
			setupStatus = MISSING_FILE;
			return false;
		}

		const char* vShaderCode = vCode.c_str();
		const char* fShaderCode = fCode.c_str();

		// 2. compile shaders
		unsigned int vertex, fragment;
		int success;
		char infoLog[512];

		// vertex
		// ------
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		// print compile errors
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			std::string message = "SHADER::VERTEX::COMPILATION_FAILED\n" + std::string(infoLog);
			Logger::LogError(message.c_str());
			setupStatus = COMPILATION_FAILED;
			glDeleteShader(vertex);
			return false;
		}

		// fragment
		// --------
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		// print compile errors
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			std::string message = "SHADER::FRAGMENT::COMPILATION_FAILED\n" + std::string(infoLog);
			Logger::LogError(message.c_str());
			glDeleteShader(vertex);
			glDeleteShader(fragment);
			setupStatus = COMPILATION_FAILED;
			return false;
		}

		// shader program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);

		// print linking errors
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			std::string message = "SHADER::PROGRAM::LINKING_FAILED\n" + std::string(infoLog);
			Logger::LogError(message.c_str());
			setupStatus = COMPILED_NOT_LINKED;
			glDeleteShader(vertex);
			glDeleteShader(fragment);
			glDeleteProgram(ID);
			return false;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);

		setupStatus = LINKED;

		return setupStatus == LINKED;
	}
	bool LoadShader(const char* vPath, const char* fPath, const char* gPath) {
		// 1. retrieve source code from files
		std::string vertexCode;
		std::string fragmentCode;
		std::string geometryCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		std::ifstream gShaderFile;

		// ensure ifstream objects can throw exceptions
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			vShaderFile.open(vPath);
			fShaderFile.open(fPath);
			gShaderFile.open(gPath);
			std::stringstream vShaderStream, fShaderStream, gShaderStream;

			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			gShaderStream << gShaderFile.rdbuf();

			vShaderFile.close();
			fShaderFile.close();
			gShaderFile.close();

			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
			geometryCode = gShaderStream.str();
		}
		catch (std::ifstream::failure e) {
			Logger::LogError("SHADER::FILE_NOT_SUCCESFULLY_READ");
			setupStatus = MISSING_FILE;
			return false;
		}

		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();
		const char* gShaderCode = geometryCode.c_str();

		// 2. compile shaders
		unsigned int vertex, fragment, geometry;
		int success;
		char infoLog[512];

		// vertex
		// ------
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);

		// print compile errors
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			std::string message = "SHADER::VERTEX::COMPILATION_FAILED\n" + std::string(infoLog);
			Logger::LogError(message.c_str());
			setupStatus = COMPILATION_FAILED;
			glDeleteShader(vertex);
			return false;
		}

		// geometry
		// --------
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, NULL);
		glCompileShader(geometry);

		glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(geometry, 512, NULL, infoLog);
			std::string message = "SHADER::GEOMETRY::COMPILATION_FAILED\n" + std::string(infoLog);
			Logger::LogError(message.c_str());
			glDeleteShader(vertex);
			glDeleteShader(geometry);
			setupStatus = COMPILATION_FAILED;
			return false;
		}

		// fragment
		// --------
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);

		// print compile errors
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			std::string message = "SHADER::FRAGMENT::COMPILATION_FAILED\n" + std::string(infoLog);
			Logger::LogError(message.c_str());
			glDeleteShader(vertex);
			glDeleteShader(fragment);
			glDeleteShader(geometry);
			setupStatus = COMPILATION_FAILED;
			return false;
		}

		// shader program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glAttachShader(ID, geometry);
		glLinkProgram(ID);

		// print linking errors
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			std::string message = "SHADER::PROGRAM::LINKING_FAILED\n" + std::string(infoLog);
			Logger::LogError(message.c_str());
			setupStatus = COMPILED_NOT_LINKED;
			glDeleteShader(vertex);
			glDeleteShader(fragment);
			glDeleteShader(geometry);
			glDeleteProgram(ID);
			return false;
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);
		glDeleteShader(geometry);

		setupStatus = LINKED;

		return setupStatus == LINKED;
	}
};