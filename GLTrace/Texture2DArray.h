#pragma once

#include "Texture.h"

class Texture2DArray : public Texture2D {
public:
	Texture2DArray(const unsigned int num_layers = 2, GLint wrap_s = GL_CLAMP_TO_EDGE, GLint wrap_t = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, GLint internalFormat = GL_RGBA32F, GLenum format = GL_RGBA, GLenum type = GL_FLOAT) : Texture2D(), num_layers(num_layers) {
		texture_type = GL_TEXTURE_2D_ARRAY;
	}
	Texture2DArray(const unsigned int width, const unsigned int height, const unsigned int num_layers, GLint wrap_s = GL_CLAMP_TO_EDGE, GLint wrap_t = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, GLint internalFormat = GL_RGBA32F, GLenum format = GL_RGBA, GLenum type = GL_FLOAT) : Texture2D(wrap_s, wrap_t, minFilter, magFilter, internalFormat, format, type), num_layers(num_layers) {
		texture_type = GL_TEXTURE_2D_ARRAY;
		this->width = width;
		this->height = height;
		GenerateTexture();
		ResizeTexture(width, height);
	}

	virtual void SetupImage() override {
		if (generated) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(texture_type, texID);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_s);
			glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_t);
			glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexImage3D(texture_type, 0, internalFormat, width, height, num_layers, 0, format, type, nullptr);
			//glBindTexture(texture_type, 0);
		}
	}
protected:
	unsigned num_layers;
};