#pragma once
#include <vector>
#include "Shader.h"
#include "TextureLoader.h"
#include "Hittables.h"

static const int MAX_SPHERES = 23;
static const int MAX_QUADS = 23;
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
		
		// Set sphere uniforms
		shader.setInt("num_spheres", spheres.size());
		for (int i = 0; i < spheres.size(); i++) {
			std::string i_string = std::to_string(i);
			shader.setVec3("spheres[" + std::to_string(i) + "].center", spheres[i].Center);
			shader.setFloat("spheres[" + std::to_string(i) + "].radius", spheres[i].Radius);
			shader.setUInt("spheres[" + std::to_string(i) + "].material_index", spheres[i].material_index);
		}

		// Set quads
		shader.setInt("num_quads", quads.size());
		for (int i = 0; i < quads.size(); i++) {
			quads[i].SetUniforms(shader, i);
		}

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
	virtual void UpdateScene(const float dt) {}

	Camera* GetSceneCamera() { return &sceneCamera; }
protected:
	Sphere& AddSphere(const glm::vec3& position, const float radius, const unsigned int material_index) {
		if (spheres.size() < MAX_SPHERES) {
			spheres.push_back(Sphere(position, radius, material_index));
		}
		return spheres.back();
	}
	Quad& AddQuad(const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quads.size() < MAX_QUADS) {
			quads.push_back(Quad(QUAD, Q, U, V, material_index));
		}
		return quads.back();
	}
	Quad& AddTriangle(const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quads.size() < MAX_QUADS) {
			quads.push_back(Quad(TRIANGLE, Q, U, V, material_index));
		}
		return quads.back();
	}
	Quad& AddDisk(const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index) {
		if (quads.size() < MAX_QUADS) {
			quads.push_back(Quad(DISK, Q, U, V, material_index));
		}
		return quads.back();
	}
	std::vector<Quad*> AddBox(const glm::vec3& a, const glm::vec3& b, const unsigned int material_index) {
		std::vector<Quad*> sides;
		sides.reserve(6);

		glm::vec3 min = glm::vec3(std::fmin(a.x, b.x), std::fmin(a.y, b.y), std::fmin(a.z, b.z));
		glm::vec3 max = glm::vec3(std::fmax(a.x, b.x), std::fmax(a.y, b.y), std::fmax(a.z, b.z));

		glm::vec3 dx = glm::vec3(max.x - min.x, 0.0f, 0.0f);
		glm::vec3 dy = glm::vec3(0.0f, max.y - min.y, 0.0f);
		glm::vec3 dz = glm::vec3(0.0f, 0.0f, max.z - min.z);

		sides.push_back(&AddQuad(glm::vec3(min.x, min.y, max.z), dx, dy, material_index)); // front
		sides.push_back(&AddQuad(glm::vec3(max.x, min.y, max.z), -dz, dy, material_index)); // right
		sides.push_back(&AddQuad(glm::vec3(max.x, min.y, min.z), -dx, dy, material_index)); // back
		sides.push_back(&AddQuad(glm::vec3(min.x, min.y, min.z), dz, dy, material_index)); // left
		sides.push_back(&AddQuad(glm::vec3(min.x, max.y, max.z), dx, -dz, material_index)); // top
		sides.push_back(&AddQuad(glm::vec3(min.x, min.y, min.z), dx, dz, material_index)); // bottom

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

	Camera sceneCamera;
private:
	std::vector<Sphere> spheres;
	std::vector<Quad> quads;
	std::vector<Material> materials;
	std::vector<MaterialSet> material_sets;
};