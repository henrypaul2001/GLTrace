#pragma once
#include "AbstractShader.h"
#include <unordered_map>
struct ShaderStorageBuffer {
public:
	ShaderStorageBuffer() { id = 0; binding = 0; }
	ShaderStorageBuffer(const unsigned int binding) {
		id = 0;
		this->binding = binding;
		glGenBuffers(1, &id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	~ShaderStorageBuffer() {
		glDeleteBuffers(1, &id);
	}

	// Initialise buffer as immutable
	void BufferStorage(const void* data, const GLsizeiptr dataSize, const GLbitfield flags) const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, dataSize, data, flags);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// Buffer data into entire GPU buffer
	void BufferData(const void* data, const GLsizeiptr dataSize, const GLenum usage = GL_STATIC_DRAW) const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		glBufferData(GL_SHADER_STORAGE_BUFFER, dataSize, data, usage);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// Buffer data into sub section of GPU buffer
	void BufferSubData(const void* data, const GLsizeiptr dataSize, const GLintptr offset) const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, dataSize, data);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	// Reads the entire buffer into CPU
	void ReadBufferData(void* dataOutput) const {
		if (dataOutput != nullptr) {
			GLsizeiptr size = GetBufferSizeInBytes();
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, dataOutput);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}

	// Reads sub section of buffer into CPU
	void ReadBufferSubData(void* dataOutput, const GLsizeiptr dataSize, const GLintptr offset) const {
		if (dataOutput != nullptr) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, dataSize, dataOutput);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}

	void UnmapBuffer() const { glBindBuffer(GL_SHADER_STORAGE_BUFFER, id); glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); }

	// Map buffer to pointer for reading, bufferSize = -1 will automatically get buffer size. Call UnmapBuffer() after using
	void* MapBufferForRead(GLint bufferSize = -1, const GLintptr offset = 0) const {
		if (bufferSize == -1) { bufferSize = GetBufferSizeInBytes(); }
		void* ptr = MapBuffer(bufferSize, offset, GL_MAP_READ_BIT);

		if (ptr) {
			return ptr;
		}
		else {
			std::cout << "ERROR::ShaderStorageBuffer::Failed to map buffer for reading. ID (" << id << "), Binding (" << binding << ")" << std::endl;
			return nullptr;
		}
	}

	// Map buffer to pointer for writing, bufferSize = -1 will automatically get buffer size. Call UnmapBuffer() after using
	void* MapBufferForWrite(GLint bufferSize = -1, const GLintptr offset = 0) const {
		if (bufferSize == -1) { bufferSize = GetBufferSizeInBytes(); }
		void* ptr = MapBuffer(bufferSize, offset, GL_MAP_WRITE_BIT);

		if (ptr) {
			return ptr;
		}
		else {
			std::cout << "ERROR::ShaderStorageBuffer::Failed to map buffer for writing. ID (" << id << "), Binding (" << binding << ")" << std::endl;
			return nullptr;
		}
	}

	// Map buffer to pointer for reading/writing, bufferSize = -1 will automatically get buffer size. Call UnmapBuffer() after using
	void* MapBufferForReadAndWrite(GLint bufferSize = -1, const GLintptr offset = 0) const {
		if (bufferSize == -1) { bufferSize = GetBufferSizeInBytes(); }
		void* ptr = MapBuffer(bufferSize, offset, (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

		if (ptr) {
			return ptr;
		}
		else {
			std::cout << "ERROR::ShaderStorageBuffer::Failed to map buffer for read/write. ID (" << id << "), Binding (" << binding << ")" << std::endl;
			return nullptr;
		}
	}

	// Map buffer to pointer with explicit access type, bufferSize = -1 will automatically get buffer size. Call UnmapBuffer() after using
	void* MapBufferWithAccess(const GLbitfield access, GLint bufferSize = -1, const GLintptr offset = 0) const {
		if (bufferSize == -1) { bufferSize = GetBufferSizeInBytes(); }
		void* ptr = MapBuffer(bufferSize, offset, access);

		if (ptr) {
			return ptr;
		}
		else {
			std::cout << "ERROR::ShaderStorageBuffer::Failed to map buffer. Access (" << access << ")" << ", ID(" << id << "), Binding(" << binding << ")" << std::endl;
			return nullptr;
		}
	}

	const GLint GetBufferSizeInBytes() const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		GLint bufferSize = 0;
		glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &bufferSize);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return bufferSize;
	}
	void BindForDispatch() const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
	}
	const unsigned int GetID() const { return id; }
	const unsigned int GetBinding() const { return binding; }
private:
	void* MapBuffer(GLint bufferSize, const GLintptr offset, const GLbitfield access) const {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
		void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, offset, bufferSize, access);

		if (ptr) {
			return ptr;
		}
		else {
			return nullptr;
		}
	}

	unsigned int id;
	unsigned int binding;
};

class ComputeShader : public AbstractShader
{
public:
	ComputeShader() : AbstractShader() {}
	ComputeShader(const char* cPath) : AbstractShader() {
		LoadShader(cPath);
	}
	~ComputeShader() {
		std::unordered_map<unsigned int, ShaderStorageBuffer*>::const_iterator ssboIt = shaderStorageBufferMap.begin();
		while (ssboIt != shaderStorageBufferMap.end()) {
			delete ssboIt->second;
			ssboIt++;
		}
	}

	bool LoadShader(const char* cPath) {
		// retrieve source code from file
		std::string computeCode;
		std::ifstream cShaderFile;

		// ensure ifstream objects can throw exceptions
		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			// open file
			cShaderFile.open(cPath);
			std::stringstream cShaderStream;

			// read files buffer contents into stream
			cShaderStream << cShaderFile.rdbuf();

			// close file
			cShaderFile.close();

			// convert stream into string
			computeCode = cShaderStream.str();
		}
		catch (std::ifstream::failure e) {
			std::cout << "ERROR::COMPUTESHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
			setupStatus = MISSING_FILE;
			return false;
		}

		const char* cShaderCode = computeCode.c_str();

		//  compile shader
		unsigned int compute;
		int success;
		char infoLog[512];

		// vertex
		compute = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(compute, 1, &cShaderCode, NULL);
		glCompileShader(compute);

		// print compile errors
		glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(compute, 512, NULL, infoLog);
			std::cout << "ERROR::COMPUTESHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
			setupStatus = COMPILATION_FAILED;
			glDeleteShader(compute);
			return false;
		}

		ID = glCreateProgram();
		glAttachShader(ID, compute);
		glLinkProgram(ID);

		// print linking errors
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(ID, 512, NULL, infoLog);
			std::cout << "ERROR::COMPUTESHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			setupStatus = COMPILED_NOT_LINKED;
			glDeleteShader(compute);
			glDeleteProgram(ID);
			return false;
		}

		// delete shader (no longer needed after being linked to program)
		glDeleteShader(compute);

		setupStatus = LINKED;

		return setupStatus == LINKED;
	}

	static void InitOpenGLConstants() {
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroupsX);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGroupsY);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGroupsZ);
	}
	void DispatchCompute(const unsigned int xGroups = 1, const unsigned int yGroups = 1, const unsigned int zGroups = 1, GLbitfield barrierBits = GL_ALL_BARRIER_BITS) {
		//SCOPE_TIMER("ComputeShader::DispatchCompute");
		assert(xGroups >= 1, "ERROR::ComputeShader::xGroups cannot be less than one");
		assert(yGroups >= 1, "ERROR::ComputeShader::yGroups cannot be less than one");
		assert(zGroups >= 1, "ERROR::ComputeShader::zGroups cannot be less than one");

		assert(xGroups < maxWorkGroupsX, "ERROR::ComputeShader::xGroups exceeds max group count: " << maxWorkGroupsX);
		assert(yGroups < maxWorkGroupsY, "ERROR::ComputeShader::yGroups exceeds max group count: " << maxWorkGroupsY);
		assert(zGroups < maxWorkGroupsZ, "ERROR::ComputeShader::zGroups exceeds max group count: " << maxWorkGroupsZ);

		Use();
		BindStorageBuffersForDispatch();
		glDispatchCompute(xGroups, yGroups, zGroups);

		glMemoryBarrier(barrierBits);

		sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	}

	const GLenum Sync(const GLuint64 nanoSecondsTimeout = 1000000) const {
		//SCOPE_TIMER("ComputeShader::Sync");
		GLenum waitReturn = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, nanoSecondsTimeout);
		glDeleteSync(sync);
		return waitReturn;
	}

	const ShaderStorageBuffer* AddNewSSBO(const unsigned int binding) {
		if (shaderStorageBufferMap.find(binding) != shaderStorageBufferMap.end()) {
			std::cout << "ERROR::ComputeShader::SSBO binding (" << binding << ") already exists" << std::endl;
			return nullptr;
		}
		else {
			shaderStorageBufferMap[binding] = new ShaderStorageBuffer(binding);
			return shaderStorageBufferMap[binding];
		}
	}
	const ShaderStorageBuffer* GetSSBO(const unsigned int binding) {
		if (shaderStorageBufferMap.find(binding) != shaderStorageBufferMap.end()) {
			return shaderStorageBufferMap.at(binding);
		}
		else {
			std::cout << "ERROR:ComputeShader::Buffer at binding (" << binding << ") does not exist" << std::endl;
			return nullptr;
		}
	}

	void BindStorageBuffersForDispatch() const {
		std::unordered_map<unsigned int, ShaderStorageBuffer*>::const_iterator ssboIt = shaderStorageBufferMap.begin();
		while (ssboIt != shaderStorageBufferMap.end()) {
			ssboIt->second->BindForDispatch();
			ssboIt++;
		}
	}
protected:
	std::unordered_map<unsigned int, ShaderStorageBuffer*> shaderStorageBufferMap; // <binding, buffer>

private:
	GLsync sync;

	static int maxWorkGroupsX;
	static int maxWorkGroupsY;
	static int maxWorkGroupsZ;
};