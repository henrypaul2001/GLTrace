#pragma once
#include <vector>
#include "Shader.h"
#include "TextureLoader.h"
#include "Hittables.h"
#include "BVH.h"
#include <unordered_map>

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
		//BuildBVH();
		RefitBVH();
	}

	Camera* GetSceneCamera() { return &sceneCamera; }
	const std::vector<Sphere>& GetSpheres() const { return spheres; }
	const std::vector<Quad>& GetQuads() const { return quads; }
	const std::string& GetSphereName(const unsigned int index) const { return sphere_names[index]; }
	const std::string& GetQuadName(const unsigned int index) const { return quad_names[index]; }
	Sphere* GetSphere(const unsigned int index) { if (index < spheres.size()) { return &spheres[index]; } else { Logger::LogError("Sphere index out of bounds"); return nullptr; } }
	Quad* GetQuad(const unsigned int index) { if (index < quads.size()) { return &quads[index]; } else { Logger::LogError("Quad index out of bounds"); return nullptr; } }

	const std::vector<Material>& GetMaterials() const { return materials; }

	const BVH& GetBVH() const { return bvh; }

	void BuildBVH() { bvh.BuildBVH(quads, spheres); }
	void RefitBVH() { bvh.RefitBVH(quads, spheres); }
	void BufferBVH(ComputeShader& computeShader) const { bvh.Buffer(computeShader); }
	void BufferSceneHittables(ComputeShader& computeShader) const {
		const ShaderStorageBuffer* sphereSSBO = computeShader.GetSSBO(4);
		const ShaderStorageBuffer* quadSSBO = computeShader.GetSSBO(5);
		unsigned int num_spheres = spheres.size();
		unsigned int num_quads = quads.size();

		if (num_spheres > 0) {
			// Buffer spheres
			// --------------
			// Initialise buffer
			sphereSSBO->BufferData(nullptr, (sizeof(unsigned int) * 4) + (sizeof(Sphere) * spheres.size()), GL_STATIC_DRAW);

			// Buffer data
			sphereSSBO->BufferSubData(&num_spheres, sizeof(unsigned int), 0);
			sphereSSBO->BufferSubData(&spheres[0], sizeof(Sphere) * num_spheres, sizeof(unsigned int) * 4);
		}

		if (num_quads > 0) {
			// Buffer quads
			// ------------
			// Initialise buffer
			quadSSBO->BufferData(nullptr, (sizeof(unsigned int) * 4) + (sizeof(Quad) * quads.size()), GL_STATIC_DRAW);

			// Buffer data
			quadSSBO->BufferSubData(&num_quads, sizeof(unsigned int), 0);
			quadSSBO->BufferSubData(&quads[0], sizeof(Quad) * num_quads, sizeof(unsigned int) * 4);
		}
	}

protected:
	Sphere& AddSphere(const std::string& name, const glm::vec3& position, const float radius, const unsigned int material_index) {
		if (sphere_map.find(name) == sphere_map.end()) {
			if (spheres.size() < MAX_SPHERES) {
				spheres.push_back(Sphere(glm::vec4(position, 1.0f), radius, material_index));
				sphere_names.push_back(name);
				sphere_map[name] = spheres.size() - 1;
			}
			else {
				Logger::LogWarning("Maximum sphere count reached");
			}
			return spheres.back();
		}
		else {
			Logger::LogError("Sphere name already exists");
		}
	}
	Quad& AddQuad(const std::string& name, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quad_map.find(name) == quad_map.end()) {
			if (quads.size() < MAX_QUADS) {
				quads.push_back(Quad(QUAD, Q, U, V, material_index));
				quad_names.push_back(name);
				quad_map[name] = quads.size() - 1;
			}
			else {
				Logger::LogWarning("Maximum quad count reached");
			}
			return quads.back();
		}
		else {
			Logger::LogError("Quad name already exists");
		}
	}
	Quad& AddTriangle(const std::string& name, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quad_map.find(name) == quad_map.end()) {
			if (quads.size() < MAX_QUADS) {
				quads.push_back(Quad(TRIANGLE, Q, U, V, material_index));
				quad_names.push_back(name);
				quad_map[name] = quads.size() - 1;
			}
			else {
				Logger::LogWarning("Maximum quad count reached");
			}
			return quads.back();
		}
		else {
			Logger::LogError("Quad name already exists");
		}
	}
	Quad& AddDisk(const std::string& name, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quad_map.find(name) == quad_map.end()) {
			if (quads.size() < MAX_QUADS) {
				quads.push_back(Quad(DISK, Q, U, V, material_index));
				quad_names.push_back(name);
				quad_map[name] = quads.size() - 1;
			}
			else {
				Logger::LogWarning("Maximum quad count reached");
			}
			return quads.back();
		}
		else {
			Logger::LogError("Quad name already exists");
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

		sides.push_back(&AddQuad(name + "_front", glm::vec3(min.x, min.y, max.z), dx, dy, material_index)); // front
		sides.push_back(&AddQuad(name + "_right", glm::vec3(max.x, min.y, max.z), -dz, dy, material_index)); // right
		sides.push_back(&AddQuad(name + "_back", glm::vec3(max.x, min.y, min.z), -dx, dy, material_index)); // back
		sides.push_back(&AddQuad(name + "_left", glm::vec3(min.x, min.y, min.z), dz, dy, material_index)); // left
		sides.push_back(&AddQuad(name + "_top", glm::vec3(min.x, max.y, max.z), dx, -dz, material_index)); // top
		sides.push_back(&AddQuad(name + "_bottom", glm::vec3(min.x, min.y, min.z), dx, dz, material_index)); // bottom

		return sides;
	}

	void AddMaterial(const Material& mat) {
		if (materials.size() < MAX_MATERIALS) {
			materials.push_back(mat);
		}
	}

	void AddMaterialSet(const MaterialSet& mat_set) {
		if (material_sets.size() < MAX_MATERIALS) {
			material_sets.push_back(mat_set);
		}
	}

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

	std::vector<Material> materials;
	std::vector<MaterialSet> material_sets;

	BVH bvh;
};