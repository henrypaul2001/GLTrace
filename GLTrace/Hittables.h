#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_geometric.hpp>
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
class Sphere {
public:
	Sphere(const glm::vec4& center, const float radius, const unsigned int material_index = 0) : Center(center), Radius(radius), padding(0.0f, 0.0f), material_index(material_index) {}
	~Sphere() {}

	glm::vec4 Center;
	float Radius;
	unsigned int material_index;

private:
	glm::vec2 padding;
};
class Quad {
public:
	Quad(const QUAD_TYPE quad_type, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index = 0) : quad_type(quad_type), Q(Q), U(U), V(V), material_index(material_index) {
		Recalculate();
	}

	glm::vec3 GetCentre() const {
		glm::vec3 extent = Q + U + V;
		if (quad_type == TRIANGLE) { return extent * 0.3333f; }
		else { return extent * 0.5f; }
	}

	void Recalculate() {
		glm::vec3 n = glm::cross(U, V);
		Normal = glm::normalize(n);
		D = glm::dot(Normal, Q);
		W = n / glm::dot(n, n);
		Area = glm::length(n);
	}

	const glm::vec3& GetQ() const { return Q; }
	const glm::vec3& GetU() const { return U; }
	const glm::vec3& GetV() const { return V; }
	const glm::vec3& GetNormal() const { return Normal; }
	const glm::vec3& GetW() const { return W; }
	const float GetD() const { return D; }
	const float GetArea() const { return Area; }

	void SetQ(const glm::vec3& q) { Q = q; Recalculate(); }
	void SetU(const glm::vec3& u) { U = u; Recalculate(); }
	void SetV(const glm::vec3& v) { V = v; Recalculate(); }

	QUAD_TYPE quad_type;
	unsigned int material_index;

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
			s.setUInt("quad_hittables[" + i_string + "].triangle_disk_mask", 0u);
			break;
		case TRIANGLE:
			s.setUInt("quad_hittables[" + i_string + "].triangle_disk_mask", 1u << 1);
			break;
		case DISK:
			s.setUInt("quad_hittables[" + i_string + "].triangle_disk_mask", 1u);
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