#pragma once
#include "Texture.h"
#include "Texture2DArray.h"
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

	static Texture2DArray* LoadTextureArrayFromFile(const std::vector<const char*>& filepaths, const bool srgb = false) {
		std::string filepathsCombined = "";
		for (const char* filepath : filepaths) {
			filepathsCombined += filepath;
		}
		std::unordered_map<const char*, Texture2DArray*>::iterator textureIt = loadedTextureArrays.find(filepathsCombined.c_str());

		if (textureIt == loadedTextureArrays.end()) {
			// Load texture

			Texture2DArray* textureArray = new Texture2DArray(filepaths.size());
			textureArray->GenerateTexture();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray->ID());
			for (int i = 0; i < filepaths.size(); i++) {
				int width, height, nrComponents;
				unsigned char* data = stbi_load(filepaths[i], &width, &height, &nrComponents, 0);
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

					if (i == 0) {
						textureArray->ResizeTexture(width, height);
						//glActiveTexture(GL_TEXTURE0);
						//glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray->ID());
					}

					glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, dataFormat, GL_UNSIGNED_BYTE, data);
				}
				else {
					std::clog << "Error loading texture at path: " << filepaths[i] << "\r\n" << std::flush;
				}
			}
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

			loadedTextureArrays[filepathsCombined.c_str()] = textureArray;
			return textureArray;
		}
		else {
			// Texture array already loaded
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

		std::unordered_map<const char*, Texture2DArray*>::iterator textureArraysIt = loadedTextureArrays.begin();
		while (textureArraysIt != loadedTextureArrays.end()) {
			delete textureArraysIt->second;
			textureArraysIt++;
		}
		loadedTextureArrays.clear();
	}
private:
	static std::unordered_map<const char*, Texture2D*> loadedTextures;
	static std::unordered_map<const char*, Texture2DArray*> loadedTextureArrays;
};