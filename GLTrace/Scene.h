#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <vector>
#include "Shader.h"

struct MaterialSet {
	int albedo_index = -1;
	int normal_index = -1;
	int roughness_index = -1;
	int metal_index = -1;
	int emission_index = -1;
	int opacity_index = -1;
};

struct Material {
	glm::vec3 Albedo = glm::vec3(1.0f);
	float Roughness = 0.0f;
	float Metal = 0.0f;
	glm::vec3 EmissiveColour = glm::vec3(0.0f);
	float EmissivePower = 0.0f;
	
	bool is_transparent = false;
	float refractive_index = 1.5f;

	int material_set_index = -1;

	// Volumetric material
	bool is_constant_medium = false;
	float neg_inv_density = 0.0f;
};

enum QUAD_TYPE {
	QUAD,
	TRIANGLE,
	DISK
};
class Hittable {
public:
	Hittable(const unsigned int material_index = 0) : material_index(material_index) {}
	virtual ~Hittable() {}

	unsigned int material_index;
};
class Sphere : public Hittable {
public:
	Sphere(const glm::vec3& center, const float radius, const unsigned int material_index = 0) : Hittable(material_index), Center(center), Radius(radius) {}
	~Sphere() {}

	glm::vec3 Center;
	float Radius;
};
class Quad : public Hittable {
public:
	Quad(const QUAD_TYPE quad_type, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index = 0) : Hittable(material_index), quad_type(quad_type), Q(Q), U(U), V(V) {
		Recalculate();
	}

	void Recalculate() {
		glm::vec3 n = glm::normalize(glm::cross(U, V));
		Normal = glm::normalize(n);
		D = glm::dot(Normal, Q);
		W = n / glm::dot(n, n);
		Area = glm::length(n);
	}

	void SetQ(const glm::vec3& q) { Q = q; Recalculate(); }
	void SetU(const glm::vec3& u) { U = u; Recalculate(); }
	void SetV(const glm::vec3& v) { V = v; Recalculate(); }

	QUAD_TYPE quad_type;

	void SetUniforms(const Shader& s, const int i) {
		std::string i_string = std::to_string(i);
		s.setVec3("quads[" + i_string + "].Q", Q);
		s.setVec3("quads[" + i_string + "].u", U);
		s.setVec3("quads[" + i_string + "].v", V);
		s.setVec3("quads[" + i_string + "].w", W);
		s.setVec3("quads[" + i_string + "].normal", Normal);
		s.setFloat("quads[" + i_string + "].D", D);
		s.setFloat("quads[" + i_string + "].area", Area);
		s.setUInt("quads[" + i_string + "].material_index", material_index);

		switch (quad_type) {
		case QUAD:
			s.setBool("quads[" + i_string + "].is_triangle", false);
			s.setBool("quads[" + i_string + "].is_disk", false);
			break;
		case TRIANGLE:
			s.setBool("quads[" + i_string + "].is_triangle", true);
			s.setBool("quads[" + i_string + "].is_disk", false);
			break;
		case DISK:
			s.setBool("quads[" + i_string + "].is_triangle", false);
			s.setBool("quads[" + i_string + "].is_disk", true);
			break;
		}
	}

protected:
	glm::vec3 Q;
	glm::vec3 U, V;

	glm::vec3 W;
	glm::vec3 Normal;
	float D;
	float Area;
};

static const int MAX_SPHERES = 100;
static const int MAX_QUADS = 100;
static const int MAX_MATERIALS = 100;

class Scene {
public:
	Scene() {
		spheres.reserve(MAX_SPHERES);
		quads.reserve(MAX_QUADS);
		materials.reserve(MAX_MATERIALS);
		material_sets.reserve(MAX_MATERIALS);
	}
	virtual ~Scene() {}

	void SetUniforms(const Shader& shader) {
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
		for (int i = 0; i < quads.size(); i++) {
			quads[i].SetUniforms(shader, i);
		}

		// Set materials
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
	virtual void UpdateScene() {}

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

private:
	std::vector<Sphere> spheres;
	std::vector<Quad> quads;
	std::vector<Material> materials;
	std::vector<MaterialSet> material_sets;
};