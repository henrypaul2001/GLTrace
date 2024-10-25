#pragma once
#include <glad/glad.h>

class Texture2D {
public:
	Texture2D(GLint wrap_s = GL_CLAMP_TO_EDGE, GLint wrap_t = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, GLint internalFormat = GL_RGBA32F, GLenum format = GL_RGBA, GLenum type = GL_FLOAT) : generated(false), wrap_s(wrap_s), wrap_t(wrap_t), minFilter(minFilter), magFilter(magFilter), internalFormat(internalFormat), format(format), type(type), texID(0), width(0), height(0), texture_type(GL_TEXTURE_2D) {}
	Texture2D(const unsigned int width, const unsigned int height, GLint wrap_s = GL_CLAMP_TO_EDGE, GLint wrap_t = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, GLint internalFormat = GL_RGBA32F, GLenum format = GL_RGBA, GLenum type = GL_FLOAT) : generated(false), wrap_s(wrap_s), wrap_t(wrap_t), minFilter(minFilter), magFilter(magFilter), internalFormat(internalFormat), format(format), type(type), texture_type(GL_TEXTURE_2D) {
		GenerateTexture();
		ResizeTexture(width, height);
	}
	virtual ~Texture2D() {
		glDeleteTextures(1, &texID);
	}

	virtual void GenerateTexture() {
		if (!generated) {
			texID = 0;
			glGenTextures(1, &texID);
			generated = true;
		}
	}

	virtual void ResizeTexture(const unsigned int newWidth, const unsigned int newHeight) {
		if (generated) {
			width = newWidth;
			height = newHeight;
			SetupImage();
		}
	}

	virtual void SetupImage() {
		if (generated) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(texture_type, texID);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_s);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_t);
			glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexImage2D(texture_type, 0, internalFormat, width, height, 0, format, type, nullptr);
			glBindTexture(texture_type, 0);
		}
	}

	virtual void BindToSlot(const unsigned int textureSlot) const {
		if (generated) {
			glActiveTexture(GL_TEXTURE0 + textureSlot);
			glBindTexture(texture_type, texID);
		}
	}

	virtual void Bind() const {
		if (generated) {
			glBindTexture(texture_type, texID);
		}
	}

	virtual void BindImage(const GLenum access = GL_READ_WRITE, const unsigned int slot = 0, const bool layered = false, const unsigned int layer = 0) const {
		if (generated) {
			glBindImageTexture(slot, texID, 0, layered, layer, access, internalFormat);
		}
	}

	virtual void GenMipmaps() const {
		if (generated) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(texture_type, texID);
			glGenerateMipmap(texture_type);
		}
	}

	const unsigned int ID() const { return texID; }

	const GLint Wrap_S() const							{ return wrap_s; }
	const GLint Wrap_T() const							{ return wrap_t; }
	const GLint MinFilter() const						{ return minFilter; }
	const GLint MagFilter() const						{ return magFilter; }
	const GLint InternalFormat() const					{ return internalFormat; }
	const GLenum Format() const							{ return format; }
	const GLenum Type() const							{ return type; }

	void SetWrap_S(const GLint wrap)					{ wrap_s = wrap; }
	void SetWrap_T(const GLint wrap)					{ wrap_t = wrap; }
	void SetMinFilter(const GLint min)					{ minFilter = min; }
	void SetMagFilter(const GLint mag)					{ magFilter = mag; }
	void SetInternalFormat(const GLint internalForm)	{ internalFormat = internalForm; }
	void SetFormat(const GLenum form)					{ format = form; }
	void SetType(const GLenum t)						{ type = t; }

protected:
	GLint wrap_s;
	GLint wrap_t;
	GLint minFilter;
	GLint magFilter;
	GLint internalFormat;
	GLenum format;
	GLenum type;
	GLenum texture_type;

	unsigned int width, height, texID;
	bool generated;
};