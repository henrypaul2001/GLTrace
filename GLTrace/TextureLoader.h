#pragma once
#include "Texture.h"
#include "stb_image.h"
#include <unordered_map>
#include <iostream>
class TextureLoader {
public:
	static Texture2D* LoadTextureFromFile(const char* filepath, const bool srgb = false) {
		std::unordered_map<const char*, Texture2D*>::iterator textureIt = loadedTextures.find(filepath);

		if (textureIt == loadedTextures.end()) {
			// Load texture
			int width, height, nrComponents;
			unsigned char* data = stbi_load(filepath, &width, &height, &nrComponents, 0);
			if (data) {
				GLenum internalFormat = GL_RGB;
				GLenum dataFormat = GL_RGB;
				if (nrComponents == 1) {
					internalFormat = GL_RED;
					dataFormat = GL_RED;
				}
				else if (nrComponents == 2) {
					internalFormat = GL_RG;
					dataFormat = GL_RG;
				}
				else if (nrComponents == 3) {
					internalFormat = srgb ? GL_SRGB : GL_RGB;
					dataFormat = GL_RGB;
				}
				else if (nrComponents == 4) {
					internalFormat = srgb ? GL_SRGB_ALPHA : GL_RGBA;
					dataFormat = GL_RGBA;
				}

				Texture2D* texture = new Texture2D(width, height, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, internalFormat, dataFormat, GL_UNSIGNED_BYTE);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture->ID());
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
				glGenerateMipmap(GL_TEXTURE_2D);

				loadedTextures[filepath] = texture;
				return texture;
			}
			else {
				std::clog << "Error loading texture at path: " << filepath << "\r\n" << std::flush;
			}
		}
		else {
			// Texture already loaded
			return textureIt->second;
		}
	}

	static void ClearResources() {
		std::unordered_map<const char*, Texture2D*>::iterator textureIt = loadedTextures.begin();
		while (textureIt != loadedTextures.end()) {
			delete textureIt->second;
			textureIt++;
		}
		loadedTextures.clear();
	}
private:
	static std::unordered_map<const char*, Texture2D*> loadedTextures;
};