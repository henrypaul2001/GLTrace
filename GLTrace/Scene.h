#pragma once
#include <vector>
#include "Shader.h"
#include "TextureLoader.h"
#include "Hittables.h"
#include "BVH.h"
#include <unordered_map>
#include "ModelLoader.h"
static const int MAX_SPHERES = 1000000;
static const int MAX_QUADS = 1000000;
static const int MAX_MATERIALS = 10;

class Scene {
public:
	Scene() {
		spheres.reserve(MAX_SPHERES);
		quads.reserve(MAX_QUADS);
		materials.reserve(MAX_MATERIALS);
		material_sets.reserve(MAX_MATERIALS);
	}
	virtual ~Scene() {}

	void SetUniforms(const AbstractShader& shader) const {
		shader.Use();

		// Set materials
		shader.setInt("num_materials", materials.size());
		for (int i = 0; i < materials.size(); i++) {
			std::string i_string = std::to_string(i);
			shader.setVec3("materials[" + i_string + "].albedo", materials[i].Albedo);
			shader.setFloat("materials[" + i_string + "].roughness", materials[i].Roughness);
			shader.setFloat("materials[" + i_string + "].metal", materials[i].Metal);
			shader.setVec3("materials[" + i_string + "].emissive_colour", materials[i].EmissiveColour);
			shader.setFloat("materials[" + i_string + "].emissive_power", materials[i].EmissivePower);
			shader.setBool("materials[" + i_string + "].is_transparent", materials[i].is_transparent);
			shader.setFloat("materials[" + i_string + "].refractive_index", materials[i].refractive_index);
			shader.setInt("materials[" + i_string + "].material_set_index", materials[i].material_set_index);
			shader.setBool("materials[" + i_string + "].is_constant_medium", materials[i].is_constant_medium);
			shader.setFloat("materials[" + i_string + "].neg_inv_density", materials[i].neg_inv_density);
		}

		// Set material sets
		for (int i = 0; i < material_sets.size(); i++) {
			std::string i_string = std::to_string(i);
			shader.setInt("material_sets[" + i_string + "].albedo_index", material_sets[i].albedo_index);
			shader.setInt("material_sets[" + i_string + "].normal_index", material_sets[i].normal_index);
			shader.setInt("material_sets[" + i_string + "].roughness_index", material_sets[i].roughness_index);
			shader.setInt("material_sets[" + i_string + "].metal_index", material_sets[i].metal_index);
			shader.setInt("material_sets[" + i_string + "].emission_index", material_sets[i].emission_index);
			shader.setInt("material_sets[" + i_string + "].opacity_index", material_sets[i].opacity_index);
		}
	}

	virtual void SetupScene() = 0;

	virtual void UpdateScene(const float dt) {
		for (Quad& quad : quads) {
			quad.Recalculate(transformBuffer[quad.Normal.w]);
		}
		BuildBVH();
		//RefitBVH();
	}

	Camera* GetSceneCamera() { return &sceneCamera; }
	const std::vector<Sphere>& GetSpheres() const { return spheres; }
	const std::vector<Quad>& GetQuads() const { return quads; }
	const std::vector<glm::mat4>& GetTransforms() const { return transformBuffer; }
	const std::string& GetSphereName(const unsigned int index) const { return sphere_names[index]; }
	const std::string& GetQuadName(const unsigned int index) const { return quad_names[index]; }
	Sphere* GetSphere(const unsigned int index) { if (index < spheres.size()) { return &spheres[index]; } else { Logger::LogError("Sphere index out of bounds"); return nullptr; } }
	Quad* GetQuad(const unsigned int index) { if (index < quads.size()) { return &quads[index]; } else { Logger::LogError("Quad index out of bounds"); return nullptr; } }
	glm::mat4* GetTransform(const unsigned int index) { if (index < transformBuffer.size()) { return &transformBuffer[index]; } else { Logger::LogError("Transform index out of bounds"); return nullptr; } }

	const std::vector<Material>& GetMaterials() const { return materials; }
	const std::string& GetMaterialName(const unsigned int index) const { return material_names[index]; }
	Material& GetMaterial(const unsigned int index) { return materials[index]; }
	const std::vector<MaterialSet>& GetMaterialSets() const { return material_sets; }

	const BVH& GetBVH() const { return bvh; }

	glm::mat4* GetSphereTransform(const unsigned int sphereID) {
		if (sphereID < spheres.size()) {
			const unsigned int transformID = spheres[sphereID].GetTransformID();
			if (transformID < transformBuffer.size()) {
				return &transformBuffer[transformID];
			}
		}
		return nullptr;
	}

	glm::mat4* GetQuadTransform(const unsigned int quadID) {
		if (quadID < quads.size()) {
			const unsigned int transformID = quads[quadID].Normal.a;
			if (transformID < transformBuffer.size()) {
				return &transformBuffer[transformID];
			}
		}
		return nullptr;
	}

	void BuildBVH() { bvh.BuildBVH(quads, spheres, transformBuffer); }
	void RefitBVH() { bvh.RefitBVH(quads, spheres, transformBuffer); }
	void BufferBVH(ComputeShader& computeShader) const { bvh.Buffer(computeShader); }
	void BufferSceneHittables(ComputeShader& computeShader) const {
		const ShaderStorageBuffer* sphereSSBO = computeShader.GetSSBO(4);
		const ShaderStorageBuffer* quadSSBO = computeShader.GetSSBO(5);
		const ShaderStorageBuffer* transformSSBO = computeShader.GetSSBO(6);
		const unsigned int num_spheres = spheres.size();
		const unsigned int num_quads = quads.size();
		const unsigned int num_transforms = transformBuffer.size();

		// Buffer spheres
		// --------------
		// Initialise buffer
		sphereSSBO->BufferData(nullptr, (sizeof(unsigned int) * 4) + (sizeof(Sphere) * spheres.size()), GL_STATIC_DRAW);
		// Buffer data
		sphereSSBO->BufferSubData(&num_spheres, sizeof(unsigned int), 0);
		if (num_spheres > 0) {
			sphereSSBO->BufferSubData(&spheres[0], sizeof(Sphere) * num_spheres, sizeof(unsigned int) * 4);
		}

		// Buffer quads
		// ------------
		// Initialise buffer
		quadSSBO->BufferData(nullptr, (sizeof(unsigned int) * 4) + (sizeof(Quad) * quads.size()), GL_STATIC_DRAW);
		// Buffer data
		quadSSBO->BufferSubData(&num_quads, sizeof(unsigned int), 0);
		if (num_quads > 0) {
			quadSSBO->BufferSubData(&quads[0], sizeof(Quad) * num_quads, sizeof(unsigned int) * 4);
		}

		// Buffer transforms
		// -----------------
		// Initialise buffer
		transformSSBO->BufferData(nullptr, (sizeof(glm::mat4) * num_transforms), GL_STATIC_COPY);
		// Buffer data
		if (num_transforms > 0) {
			transformSSBO->BufferData(&transformBuffer[0], sizeof(glm::mat4) * num_transforms, GL_STATIC_COPY);
		}
	}

	Sphere* AddSphere(const std::string& name, const glm::vec3& position, const float radius, const unsigned int material_index) {
		if (sphere_map.find(name) == sphere_map.end()) {
			if (spheres.size() < MAX_SPHERES) {
				const int num_spheres = spheres.size();
				spheres.push_back(Sphere(glm::vec4(position, 1.0f), radius, num_spheres, material_index));
				sphere_names.push_back(name);
				sphere_map[name] = num_spheres;
				transformBuffer.insert(transformBuffer.begin() + num_spheres, glm::mat4(1.0f));

				// Increment quad transform pointers
				for (Quad& quad : quads) {
					quad.Normal.w++;
				}
			}
			else {
				Logger::LogWarning("Maximum sphere count reached");
				return nullptr;
			}
			return &spheres.back();
		}
		else {
			Logger::LogError("Sphere name already exists");
			return nullptr;
		}
	}
	Quad* AddQuad(const std::string& name, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quad_map.find(name) == quad_map.end()) {
			if (quads.size() < MAX_QUADS) {
				quads.push_back(Quad(QUAD, Q, U, V, transformBuffer.size(), material_index));
				quad_names.push_back(name);
				quad_map[name] = quads.size() - 1;
				transformBuffer.push_back(glm::mat4(1.0f));
			}
			else {
				Logger::LogWarning("Maximum quad count reached");
				return nullptr;
			}
			return &quads.back();
		}
		else {
			Logger::LogError("Quad name already exists");
			return nullptr;
		}
	}
	Quad* AddTriangle(const std::string& name, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quad_map.find(name) == quad_map.end()) {
			if (quads.size() < MAX_QUADS) {
				quads.push_back(Quad(TRIANGLE, Q, U, V, transformBuffer.size(), material_index));
				quad_names.push_back(name);
				quad_map[name] = quads.size() - 1;
				transformBuffer.push_back(glm::mat4(1.0f));
			}
			else {
				Logger::LogWarning("Maximum quad count reached");
				return nullptr;
			}
			return &quads.back();
		}
		else {
			Logger::LogError("Quad name already exists");
			return nullptr;
		}
	}
	Quad* AddDisk(const std::string& name, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quad_map.find(name) == quad_map.end()) {
			if (quads.size() < MAX_QUADS) {
				quads.push_back(Quad(DISK, Q, U, V, transformBuffer.size(), material_index));
				quad_names.push_back(name);
				quad_map[name] = quads.size() - 1;
				transformBuffer.push_back(glm::mat4(1.0f));
			}
			else {
				Logger::LogWarning("Maximum quad count reached");
				return nullptr;
			}
			return &quads.back();
		}
		else {
			Logger::LogError("Quad name already exists");
			return nullptr;
		}
	}
	std::vector<Quad*> AddBox(const std::string& name, const glm::vec3& a, const glm::vec3& b, const unsigned int material_index) {
		std::vector<Quad*> sides;
		sides.reserve(6);

		glm::vec3 min = glm::vec3(std::fmin(a.x, b.x), std::fmin(a.y, b.y), std::fmin(a.z, b.z));
		glm::vec3 max = glm::vec3(std::fmax(a.x, b.x), std::fmax(a.y, b.y), std::fmax(a.z, b.z));

		glm::vec3 dx = glm::vec3(max.x - min.x, 0.0f, 0.0f);
		glm::vec3 dy = glm::vec3(0.0f, max.y - min.y, 0.0f);
		glm::vec3 dz = glm::vec3(0.0f, 0.0f, max.z - min.z);

		sides.push_back(AddQuad(name + "_front", glm::vec3(min.x, min.y, max.z), dx, dy, material_index)); // front
		sides.push_back(AddQuad(name + "_right", glm::vec3(max.x, min.y, max.z), -dz, dy, material_index)); // right
		sides.push_back(AddQuad(name + "_back", glm::vec3(max.x, min.y, min.z), -dx, dy, material_index)); // back
		sides.push_back(AddQuad(name + "_left", glm::vec3(min.x, min.y, min.z), dz, dy, material_index)); // left
		sides.push_back(AddQuad(name + "_top", glm::vec3(min.x, max.y, max.z), dx, -dz, material_index)); // top
		sides.push_back(AddQuad(name + "_bottom", glm::vec3(min.x, min.y, min.z), dx, dz, material_index)); // bottom

		return sides;
	}

	const bool LoadModelAsTriangles(const char* filepath) {
		std::vector<Mesh> meshes;
		if (ModelLoader::LoadModelFromFile(meshes, filepath)) {
			unsigned int totalVertices = 0;
			for (const Mesh& mesh : meshes) {
				const std::vector<glm::vec4>& vertices = mesh.vertices;
				const std::vector<unsigned int>& indices = mesh.indices;
				totalVertices += indices.size();

				transformBuffer.push_back(glm::mat4(1.0f));

				unsigned triangle_index = 0;
				for (int i = 0; i < indices.size() - 3; i += 3) {
					triangle_index++;
					const glm::vec4 Q = vertices[indices[i]];
					const glm::vec4 U = vertices[indices[i + 1]] - Q;
					const glm::vec4 V = vertices[indices[i + 2]] - Q;

					std::string name = mesh.name + "/triangle" + std::to_string(triangle_index);
					if (quad_map.find(name) == quad_map.end()) {
						if (quads.size() < MAX_QUADS) {
							quads.push_back(Quad(TRIANGLE, Q, U, V, transformBuffer.size() - 1, 0));
							quad_names.push_back(name);
							quad_map[name] = quads.size() - 1;
						}
						else {
							Logger::LogWarning("Maximum quad count reached");
							return false;
						}
					}
					else {
						Logger::LogError("Quad name already exists");
						return false;
					}
				}
			}
			Logger::Log(std::string("Vertex count = " + std::to_string(totalVertices)).c_str());
			Logger::Log(std::string("Triangle count = " + std::to_string(totalVertices / 3)).c_str());
			return true;
		}
		return false;
	}

	void RemoveSphere(const unsigned int sphereIndex) {
		if (sphereIndex < spheres.size()) {
			const unsigned int transformID = spheres[sphereIndex].GetTransformID();
			const std::string sphereName = sphere_names[sphereIndex];
			sphere_names.erase(sphere_names.begin() + sphereIndex);
			spheres.erase(spheres.begin() + sphereIndex);
			sphere_map.erase(sphereName);
			transformBuffer.erase(transformBuffer.begin() + transformID);

			// decrement all quad transform pointers and following sphere transform pointers
			for (int i = sphereIndex; i < spheres.size(); i++) {
				spheres[i].DecrementTransformID();
			}
			for (Quad& quad : quads) {
				quad.Normal.w--;
			}
		}
	}
	void RemoveQuad(const unsigned int quadIndex) {
		if (quadIndex < quads.size()) {
			const unsigned int transformID = quads[quadIndex].Normal.w;
			const std::string quadName = quad_names[quadIndex];
			quad_names.erase(quad_names.begin() + quadIndex);
			quads.erase(quads.begin() + quadIndex);
			quad_map.erase(quadName);
			transformBuffer.erase(transformBuffer.begin() + transformID);

			// decrement all following quad transform pointers
			for (int i = quadIndex; i < quads.size(); i++) {
				quads[i].Normal.w--;
			}
		}
	}

	void AddMaterial(const std::string& name, const Material& mat) {
		if (material_map.find(name) == material_map.end()) {
			if (materials.size() < MAX_MATERIALS) {
				materials.push_back(mat);
				material_names.push_back(name);
				material_map[name] = materials.size() - 1;
			}
			else {
				Logger::LogWarning("Maximum quad count reached");
			}
		}
		else {
			Logger::LogError("Quad name already exists");
		}
	}
	void AddMaterialSet(const MaterialSet& mat_set) {
		if (material_sets.size() < MAX_MATERIALS) {
			material_sets.push_back(mat_set);
		}
	}
protected:

	void ClearQuadList() { quads.clear(); }
	void SetQuadList(const std::vector<Quad>& newQuads) { this->quads = newQuads; }

	Camera sceneCamera;
private:
	std::unordered_map<std::string, unsigned int> sphere_map;
	std::vector<std::string> sphere_names;
	std::vector<Sphere> spheres;

	std::unordered_map<std::string, unsigned int> quad_map;
	std::vector<std::string> quad_names;
	std::vector<Quad> quads;

	std::unordered_map<std::string, unsigned int> material_map;
	std::vector<std::string> material_names;
	std::vector<Material> materials;

	std::vector<MaterialSet> material_sets;

	std::vector<glm::mat4> transformBuffer;

	BVH bvh;
};