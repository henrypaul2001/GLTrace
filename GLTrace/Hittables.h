#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include "Shader.h"

struct HittableTransform {
	glm::vec3 translation = glm::vec3(0.0f);
	glm::vec3 euler_rotation = glm::vec3(0.0f); // Degrees
	glm::vec3 scale = glm::vec3(1.0f);
};

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
	Sphere(const glm::vec4& center, const float radius, const unsigned int material_index = 0) : Center(center), Radius(radius), padding(0.0f), material_index(material_index) {}
	~Sphere() {}

	void Transform(const glm::mat4& transform) {
		Center = transform * Center;
	}

	glm::vec4 Center;
	float Radius;
	unsigned int material_index;
	unsigned int transformID;

private:
	float padding;
};
class Quad {
public:
	Quad(const QUAD_TYPE quad_type, const glm::vec3& Q, const glm::vec3& U, const glm::vec3& V, const unsigned int material_index = 0) : Q(Q, 1.0f), U(U, 1.0f), V(V, 1.0f), material_index(material_index) {
		switch (quad_type) {
		case QUAD:
			triangle_disk_id = 0u;
			break;
		case TRIANGLE:
			triangle_disk_id = 1u;
			break;
		case DISK:
			triangle_disk_id = 2u;
			break;
		}
		Recalculate();
	}

	glm::vec3 GetCentre() const {
		glm::vec3 extent = U + V;
		if (triangle_disk_id == 1) { return glm::vec3(Q) + (extent * 0.3333f); }
		if (triangle_disk_id == 2) { return glm::vec3(Q); }
		else { return glm::vec3(Q) + (extent * 0.5f); }
	}

	void Transform(const glm::mat4& transform) {
		// Get world space vertices
		glm::vec4 worldQ = Q;
		glm::vec4 worldU = glm::vec4(glm::vec3(worldQ + U), 1.0f);
		glm::vec4 worldV = glm::vec4(glm::vec3(worldQ + V), 1.0f);

		// Transform vertices
		glm::vec4 transformedWorldQ = transform * worldQ;
		glm::vec4 transformedWorldU = transform * worldU;
		glm::vec4 transformedWorldV = transform * worldV;
	
		Q = transformedWorldQ;
		U = transformedWorldU - transformedWorldQ;
		V = transformedWorldV - transformedWorldQ;

		Recalculate();
	}

	void Recalculate() {
		glm::vec4 n = glm::vec4(glm::cross(glm::vec3(U), glm::vec3(V)), 0.0f);
		Normal = glm::normalize(n);
		D = glm::dot(Normal, Q);
		W = n / glm::dot(n, n);
		Area = glm::length(n);
	}

	const int GetTransformID() const { return Normal.w; }

	const glm::vec4& GetQ() const { return Q; }
	const glm::vec4& GetU() const { return U; }
	const glm::vec4& GetV() const { return V; }
	const glm::vec3& GetNormal() const { return Normal; }
	const glm::vec3& GetW() const { return W; }
	const float GetD() const { return D; }
	const float GetArea() const { return Area; }

	void SetQ(const glm::vec3& q) { Q = glm::vec4(q, 1.0f); Recalculate(); }
	void SetU(const glm::vec3& u) { U = glm::vec4(u, 1.0f); Recalculate(); }
	void SetV(const glm::vec3& v) { V = glm::vec4(v, 1.0f); Recalculate(); }

	glm::vec4 Q;
	glm::vec4 U, V;

	glm::vec4 W;
	glm::vec4 Normal;
	float D;
	float Area;
	unsigned int material_index;
	unsigned int triangle_disk_id;
};