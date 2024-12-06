#pragma once
#include "Hittables.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Mesh {
	std::string name;
	std::vector<glm::vec4> vertices;
	std::vector<unsigned int> indices;
};

class ModelLoader
{
public:
	static bool LoadModelFromFile(std::vector<Mesh>& out_meshes, const char* filepath) {
		int numMeshes = ReadFile(out_meshes, filepath);
		Logger::Log(std::string(std::to_string(numMeshes) + " meshes loaded from '" + filepath + "'").c_str());
		return (numMeshes > 0);
	}

	static bool LoadModelFromFile(std::vector<Sphere>& sceneSpheres, const char* filepath) {
		std::vector<Mesh> loadedMeshes;
		int numMeshes = ReadFile(loadedMeshes, filepath);
		Logger::Log(std::string(std::to_string(numMeshes) + " meshes loaded from '" + filepath + "'").c_str());
	}

	static bool LoadModelFromFile(std::vector<Quad>& sceneQuads, const char* filepath) {
		std::vector<Mesh> loadedMeshes;
		int numMeshes = ReadFile(loadedMeshes, filepath);
		Logger::Log(std::string(std::to_string(numMeshes) + " meshes loaded from '" + filepath + "'").c_str());
	}

private:
	// returns number of meshes
	static int ReadFile(std::vector<Mesh>& out_meshes, const char* filepath) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::string errorString = "Failed to load file: " + std::string(filepath) + " assimp: '" + importer.GetErrorString() + "'";
			Logger::LogError(errorString.c_str());
			return 0;
		}

		ProcessNode(scene, scene->mRootNode, out_meshes);

		return out_meshes.size();
	}

	static void ProcessNode(const aiScene* scene, const aiNode* node, std::vector<Mesh>& out_meshes) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			out_meshes.push_back(ProcessMesh(mesh));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ProcessNode(scene, node->mChildren[i], out_meshes);
		}
	}

	static Mesh ProcessMesh(const aiMesh* mesh) {
		Mesh meshReturn;
		meshReturn.name = mesh->mName.C_Str();
		Logger::Log(std::string("Processing mesh '" + meshReturn.name + "'").c_str());

		std::vector<glm::vec4> vertices;
		std::vector<unsigned int> indices;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			vertices.push_back(glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f));
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}

		meshReturn.vertices = vertices;
		meshReturn.indices = indices;
		return meshReturn;
	}
};