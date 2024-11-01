#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_geometric.hpp>
struct MaterialSet {
	int albedo_index = -1;
	int normal_index = -1;
	int roughness_index = -1;
	int metal_index = -1;
	int emission_index = -1;
	int opacity_index = -1;
};
struct Material {
	Material() {}
	// Constructor for volumetric materials
	Material(const float density, const glm::vec3& colour) {
		Albedo = colour;
		is_constant_medium = true;
		neg_inv_density = (-1.0f / density);
	}

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
		glm::vec3 n = glm::cross(U, V);
		Normal = glm::normalize(n);
		D = glm::dot(Normal, Q);
		W = n / glm::dot(n, n);
		Area = glm::length(n);
	}

	void SetQ(const glm::vec3& q) { Q = q; Recalculate(); }
	void SetU(const glm::vec3& u) { U = u; Recalculate(); }
	void SetV(const glm::vec3& v) { V = v; Recalculate(); }

	QUAD_TYPE quad_type;

	void SetUniforms(const AbstractShader& s, const int i) const {
		std::string i_string = std::to_string(i);
		s.setVec3("quad_hittables[" + i_string + "].Q", Q);
		s.setVec3("quad_hittables[" + i_string + "].u", U);
		s.setVec3("quad_hittables[" + i_string + "].v", V);
		s.setVec3("quad_hittables[" + i_string + "].w", W);
		s.setVec3("quad_hittables[" + i_string + "].normal", Normal);
		s.setFloat("quad_hittables[" + i_string + "].D", D);
		s.setFloat("quad_hittables[" + i_string + "].area", Area);
		s.setUInt("quad_hittables[" + i_string + "].material_index", material_index);

		switch (quad_type) {
		case QUAD:
			s.setBool("quad_hittables[" + i_string + "].is_triangle", false);
			s.setBool("quad_hittables[" + i_string + "].is_disk", false);
			break;
		case TRIANGLE:
			s.setBool("quad_hittables[" + i_string + "].is_triangle", true);
			s.setBool("quad_hittables[" + i_string + "].is_disk", false);
			break;
		case DISK:
			s.setBool("quad_hittables[" + i_string + "].is_triangle", false);
			s.setBool("quad_hittables[" + i_string + "].is_disk", true);
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