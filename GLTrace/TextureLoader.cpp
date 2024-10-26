#define STB_IMAGE_IMPLEMENTATION
#include "TextureLoader.h"
std::unordered_map<const char*, Texture2D*> TextureLoader::loadedTextures;
std::unordered_map<const char*, Texture2DArray*> TextureLoader::loadedTextureArrays;