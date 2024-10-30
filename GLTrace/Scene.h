#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <vector>

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
		glm::vec3 n = glm::normalize(glm::cross(U, V));
		Normal = glm::normalize(n);
		D = glm::dot(Normal, Q);
		W = n / glm::dot(n, n);
		Area = glm::length(n);
	}

	glm::vec3 Q;
	glm::vec3 U, V;
	QUAD_TYPE quad_type;

protected:
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

	void SetUniforms() {

	}

	virtual void SetupScene() = 0;

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

	std::vector<Sphere> spheres;
	std::vector<Quad> quads;
	std::vector<Material> materials;
	std::vector<MaterialSet> material_sets;
};