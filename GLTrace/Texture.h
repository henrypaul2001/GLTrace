#pragma once
#include <glad/glad.h>

class Texture2D {
public:
	Texture2D(GLint wrap_s = GL_CLAMP_TO_EDGE, GLint wrap_t = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, GLint internalFormat = GL_RGBA32F, GLenum format = GL_RGBA, GLenum type = GL_FLOAT) : generated(false), wrap_s(wrap_s), wrap_t(wrap_t), minFilter(minFilter), magFilter(magFilter), internalFormat(internalFormat), format(format), type(type), texID(0), width(0), height(0) {}
	Texture2D(const unsigned int width, const unsigned int height, GLint wrap_s = GL_CLAMP_TO_EDGE, GLint wrap_t = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, GLint internalFormat = GL_RGBA32F, GLenum format = GL_RGBA, GLenum type = GL_FLOAT) : generated(false), wrap_s(wrap_s), wrap_t(wrap_t), minFilter(minFilter), magFilter(magFilter), internalFormat(internalFormat), format(format), type(type) {
		GenerateTexture();
		ResizeTexture(width, height);
	}
	~Texture2D() {
		glDeleteTextures(1, &texID);
	}

	void GenerateTexture() {
		if (!generated) {
			texID = 0;
			glGenTextures(1, &texID);
			generated = true;
		}
	}

	void ResizeTexture(const unsigned int newWidth, const unsigned int newHeight) {
		if (generated) {
			width = newWidth;
			height = newHeight;
			SetupImage();
		}
	}

	void SetupImage() {
		if (generated) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void BindToSlot(const unsigned int textureSlot) const {
		if (generated) {
			glActiveTexture(GL_TEXTURE0 + textureSlot);
			glBindTexture(GL_TEXTURE_2D, texID);
		}
	}

	void Bind() const {
		if (generated) {
			glBindTexture(GL_TEXTURE_2D, texID);
		}
	}

	void BindImage(const GLenum access = GL_READ_WRITE, const GLenum slot = GL_TEXTURE0) const {
		if (generated) {
			glActiveTexture(slot);
			glBindImageTexture(0, texID, 0, GL_FALSE, 0, access, internalFormat);
		}
	}

	void GenMipmaps() const {
		if (generated) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texID);
			glGenerateMipmap(GL_TEXTURE_2D);
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

private:
	GLint wrap_s;
	GLint wrap_t;
	GLint minFilter;
	GLint magFilter;
	GLint internalFormat;
	GLenum format;
	GLenum type;

	unsigned int width, height, texID;
	bool generated;
};