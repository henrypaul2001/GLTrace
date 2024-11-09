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
	Quad(const QUAD_TYPE quad_type, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index = 0) : Q(Q, 0.0f), U(U, 0.0f), V(V, 0.0f), material_index(material_index) {
		switch (quad_type) {
		case QUAD:
			triangle_disk_mask = 0u;
			break;
		case TRIANGLE:
			triangle_disk_mask = 1u << 1;
			break;
		case DISK:
			triangle_disk_mask = 1u;
			break;
		}
		Recalculate();
	}

	glm::vec3 GetCentre() const {
		glm::vec3 extent = Q + U + V;
		if ((triangle_disk_mask & 1u << 1) == 1u << 1) { return extent * 0.3333f; }
		else { return extent * 0.5f; }
	}

	void Recalculate() {
		glm::vec4 n = glm::vec4(glm::cross(glm::vec3(U), glm::vec3(V)), 0.0f);
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

	void SetQ(const glm::vec3& q) { Q = glm::vec4(q, 0.0f); Recalculate(); }
	void SetU(const glm::vec3& u) { U = glm::vec4(u, 0.0f); Recalculate(); }
	void SetV(const glm::vec3& v) { V = glm::vec4(v, 0.0f); Recalculate(); }

	glm::vec4 Q;
	glm::vec4 U, V;

	glm::vec4 W;
	glm::vec4 Normal;
	float D;
	float Area;
	unsigned int material_index;
	unsigned int triangle_disk_mask; // 0010 = triangle, 0001 = disk
};