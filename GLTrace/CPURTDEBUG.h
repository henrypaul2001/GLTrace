#pragma once
#include "BVH.h"
struct BVH_DEBUG_RAY {
	BVH_DEBUG_RAY(const glm::vec3& origin, const glm::vec3& direction) : origin(origin), direction(direction) {}
	glm::vec3 at(const float t) const {
		return origin + (t * direction);
	}

	glm::vec3 origin;
	glm::vec3 direction;
};

struct BVH_DEBUG_INTERVAL {
	BVH_DEBUG_INTERVAL(const float min, const float max) : tmin(min), tmax(max) {}
	bool contains(const float x) const {
		return tmin <= x && x <= tmax;
	}
	bool surrounds(const float x) const {
		return tmin < x && x < tmax;
	}

	float tmin;
	float tmax;
};

struct BVH_DEBUG_HIT_RECORD {
	glm::vec3 p;
	glm::vec3 normal;
	float t;
	float u;
	float v;
	bool front_face;
	unsigned int material_index;
};

class CPURTDEBUG
{
public:
	static bool DebugBVHTraversal(const BVH_DEBUG_RAY& r, const BVH_DEBUG_INTERVAL& ray_t, BVH_DEBUG_HIT_RECORD& rec, float& closest_so_far, std::vector<Quad> quads, std::vector<Sphere> spheres, BVH bvh) {
		CPURTDEBUG::quads = quads;
		CPURTDEBUG::spheres = spheres;
		CPURTDEBUG::quadIDs = bvh.GetQuadIDs();
		CPURTDEBUG::sphereIDs = bvh.GetSphereIDs();
		CPURTDEBUG::tree = bvh.GetTree();

		bool hit_anything = false;
		unsigned int nodeID = 0;
		unsigned int stackIDs[64];
		unsigned int stackPTR = 0;

		while (true) {
			if (tree[nodeID].isLeaf()) {
				// Test primitives
				hit_anything = hit_bvh_primitives(nodeID, r, ray_t, rec, closest_so_far) || hit_anything;

				// Pop stack
				if (stackPTR == 0) { return hit_anything; }
				else {
					nodeID = stackIDs[--stackPTR];
				}
				continue;
			}

			// Test child nodes
			unsigned int childID1 = tree[nodeID].leftChild;
			unsigned int childID2 = childID1 + 1;
			float dist1;
			float dist2;

			BVH_DEBUG_INTERVAL ray_interval = BVH_DEBUG_INTERVAL(ray_t.tmin, closest_so_far);
			bool hit1 = hit_aabb(r, ray_interval, tree[childID1].aabbMin, tree[childID1].aabbMax, dist1);
			bool hit2 = hit_aabb(r, ray_interval, tree[childID2].aabbMin, tree[childID2].aabbMax, dist2);

			// Get closest hit
			if (hit1 && hit2 && dist1 > dist2) {
				float temp = dist1;
				dist1 = dist2;
				dist2 = temp;

				unsigned int tempChild = childID1;
				childID1 = childID2;
				childID2 = tempChild;
			}

			if (hit1) {
				// Set current node to hit1 node, add hit2 to stack for later testing
				nodeID = childID1;
				if (hit2 && stackPTR < 63) {
					stackIDs[stackPTR++] = childID2;
				}
			}
			else if (hit2) {
				nodeID = childID2;
			}
			else {
				// Pop stack
				if (stackPTR == 0) { return hit_anything; }
				else {
					stackPTR -= 1;
					nodeID = stackIDs[stackPTR];
				}
			}
		}
		return false;
	}
protected:
	static float length_squared(const glm::vec3 vec) {
		return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	}
	static bool hit_sphere(const unsigned int sphere_index, const BVH_DEBUG_RAY& r, BVH_DEBUG_INTERVAL ray_t, BVH_DEBUG_HIT_RECORD& rec) {
		if (sphere_index < spheres.size()) {
			glm::vec3 Center = spheres[sphere_index].Center;
			float Radius = spheres[sphere_index].Radius;
			unsigned int material_index = spheres[sphere_index].material_index;

			glm::vec3 oc = Center - r.origin;

			float a = length_squared(r.direction);
			float h = dot(r.direction, oc);
			float c = length_squared(oc) - Radius * Radius;
			float discriminant = h * h - a * c;

			if (discriminant < 0) {
				return false;
			}

			float sqrtd = sqrt(discriminant);

			// Find nearest root that lies in acceptable range
			float root = (h - sqrtd) / a;
			if (!ray_t.surrounds(root)) {
				root = (h + sqrtd) / a;
				if (!ray_t.surrounds(root)) {
					return false;
				}
			}

			rec.t = root;
			rec.p = r.at(rec.t);
			glm::vec3 outward_normal = (rec.p - Center) / Radius;
			//set_face_normal(rec, r, outward_normal);
			//get_sphere_uv(outward_normal, rec.u, rec.v);
			//rec.material_index = material_index;

			// Check for normal map
			//int mat_set_index = materials[material_index].material_set_index;
			//if (mat_set_index > -1) {
			//	if (material_sets[mat_set_index].normal_index > -1) {
			//		vec3 tangent_normal = texture(material_textures[mat_set_index], vec3(rec.u, rec.v, material_sets[mat_set_index].normal_index)).xyz * 2.0 - 1.0;

			//		rec.normal = tangent_normal_to_local(tangent_normal, rec.normal);

			//		if (!rec.front_face) {
			//			rec.normal = -rec.normal;
			//		}
			//	}
			//}
			return true;
		}
		// index out of bounds
		return false;
	}

	static bool quad_is_interior(const Quad quad_hittable, const float a, const float b, BVH_DEBUG_HIT_RECORD& rec) {
		BVH_DEBUG_INTERVAL unit_interval = BVH_DEBUG_INTERVAL(0.0f, 1.0f);

		if (!unit_interval.contains(a) || !unit_interval.contains(b)) {
			return false;
		}

		rec.u = a;
		rec.v = b;
		return true;
	}
	static bool hit_quad(const unsigned int quad_index, const BVH_DEBUG_RAY& r, const BVH_DEBUG_INTERVAL& ray_t, BVH_DEBUG_HIT_RECORD& rec) {
		if (quad_index < quads.size()) {
			glm::vec3 Normal = quads[quad_index].GetNormal();
			glm::vec3 Q = quads[quad_index].GetQ();
			glm::vec3 U = quads[quad_index].GetU();
			glm::vec3 V = quads[quad_index].GetV();
			glm::vec3 W = quads[quad_index].GetW();
			float D = quads[quad_index].GetD();
			unsigned int material_index = quads[quad_index].material_index;
			//bool is_triangle = quads[quad_index].is_triangle;
			//bool is_disk = quads[quad_index].is_disk;

			float denom = dot(Normal, r.direction);

			// Not hit if ray is parallel to plane
			if (abs(denom) < 1e-8) {
				return false;
			}

			// Interval check
			float t = (D - dot(Normal, r.origin)) / denom;
			if (!ray_t.contains(t)) {
				return false;
			}

			// Determine if hit point lies within planar shape using plane coordinates
			glm::vec3 intersection = r.at(t);
			glm::vec3 planar_hitpt_vector = intersection - Q;
			float alpha = glm::dot(W, glm::cross(planar_hitpt_vector, V));
			float beta = glm::dot(W, glm::cross(U, planar_hitpt_vector));

			if (!quad_is_interior(quads[quad_index], alpha, beta, rec)) {
				return false;
			}
			/*if (!is_triangle && !is_disk && !quad_is_interior(quad_hittables[quad_index], alpha, beta, rec)) {
				return false;
			}
			else if (is_triangle && !triangle_is_interior(quad_hittables[quad_index], alpha, beta, rec)) {
				return false;
			}
			else if (is_disk && !disk_is_interior(quad_hittables[quad_index], alpha, beta, rec)) {
				return false;
			}*/

			rec.t = t;
			rec.p = intersection;
			rec.material_index = material_index;
			//set_face_normal(rec, r, Normal);

			// Check for normal map
			//int mat_set_index = materials[material_index].material_set_index;
			//if (mat_set_index > -1) {
			//	if (material_sets[mat_set_index].normal_index > -1) {
			//		vec3 tangent_normal = texture(material_textures[mat_set_index], vec3(rec.u, rec.v, material_sets[mat_set_index].normal_index)).xyz * 2.0 - 1.0;

			//		rec.normal = tangent_normal_to_local(tangent_normal, rec.normal);

			//		if (!rec.front_face) {
			//			rec.normal = -rec.normal;
			//		}
			//	}
			//}

			return true;
		}
		// index out of bounds
		return false;
	}

	static bool hit_aabb(const BVH_DEBUG_RAY& r, const BVH_DEBUG_INTERVAL& t, const glm::vec3& aabbMin, const glm::vec3& aabbMax, float& hit_distance) {
		float tx1 = (aabbMin.x - r.origin.x) / (r.direction.x + 1e-8);
		float tx2 = (aabbMax.x - r.origin.x) / (r.direction.x + 1e-8);
		float tmin = std::min(tx1, tx2);
		float tmax = std::max(tx1, tx2);
		std::cout << "tx1: " << tx1 << ", tx2: " << tx2 << ", tmin (x): " << tmin << ", tmax (x): " << tmax << std::endl;

		float ty1 = (aabbMin.y - r.origin.y) / (r.direction.y + 1e-8);
		float ty2 = (aabbMax.y - r.origin.y) / (r.direction.y + 1e-8);
		tmin = std::max(tmin, std::min(ty1, ty2));
		tmax = std::min(tmax, std::max(ty1, ty2));
		std::cout << "ty1: " << ty1 << ", ty2: " << ty2 << ", tmin (y): " << tmin << ", tmax (y): " << tmax << std::endl;

		float tz1 = (aabbMin.z - r.origin.z) / (r.direction.z + 1e-8);
		float tz2 = (aabbMax.z - r.origin.z) / (r.direction.z + 1e-8);
		std::cout << "tz1: " << tz1 << ", tz2: " << tz2 << std::endl;

		tmin = std::max(tmin, std::min(tz1, tz2));
		tmax = std::min(tmax, std::max(tz1, tz2));
		std::cout << "tmin (z): " << tmin << ", tmax (z): " << tmax << std::endl;

		// Clamp tmin to prevent negative values if the ray starts inside the AABB
		tmin = std::max(tmin, 0.0f);
		std::cout << "Clamped tmin: " << tmin << std::endl;

		// Determine if there’s a hit based on the interval limits
		bool hit = (tmax >= tmin && tmin < t.tmax && tmax >= t.tmin && tmax > 0);
		std::cout << "Hit test: " << (hit ? "Hit" : "Miss") << ", tmin: " << tmin << ", tmax: " << tmax << std::endl;

		// If there is a hit, set hit_distance to tmin
		if (hit) {
			hit_distance = tmin > 0.0 ? tmin : tmax;
		}
		else {
			hit_distance = 1e30f;
		}

		return hit;
	}

	static bool hit_bvh_primitives(const unsigned int nodeID, const BVH_DEBUG_RAY& r, const BVH_DEBUG_INTERVAL& ray_t, BVH_DEBUG_HIT_RECORD& rec, float& closest_so_far) {
		bool hit_anything = false;
		unsigned int firstSphereIndex = tree[nodeID].firstSpherePrimitive;
		unsigned int firstQuadIndex = tree[nodeID].firstQuadPrimitive;
		unsigned int totalQuads = tree[nodeID].quadPrimitiveCount;
		unsigned int totalSpheres = tree[nodeID].spherePrimitiveCount;

		BVH_DEBUG_HIT_RECORD temp_hit;

		// Test spheres
		for (int i = 0; i < totalSpheres; i++) {
			unsigned int sphereID = sphereIDs[firstSphereIndex + i];
			//if (materials[spheres[sphereID].material_index].is_constant_medium) {
			//	if (hit_sphere_volume(sphereID, r, new_interval(ray_t.tmin, closest_so_far), temp_hit)) {
			//		hit_anything = true;
			//		closest_so_far = temp_hit.t;
			//		rec = temp_hit;
			//	}
			//}
			//else {
			if (hit_sphere(sphereID, r, BVH_DEBUG_INTERVAL(ray_t.tmin, closest_so_far), temp_hit)) {
				hit_anything = true;
				closest_so_far = temp_hit.t;
				rec = temp_hit;
			}
			//}
		}

		// Test quads
		for (int i = 0; i < totalQuads; i++) {
			unsigned int quadID = quadIDs[firstQuadIndex + i];
			if (hit_quad(quadID, r, BVH_DEBUG_INTERVAL(ray_t.tmin, closest_so_far), temp_hit)) {
				hit_anything = true;
				closest_so_far = temp_hit.t;
				rec = temp_hit;
			}
		}

		return hit_anything;
	}

	static std::vector<Sphere> spheres;
	static std::vector<Quad> quads;

	static std::vector<BVHNode> tree;
	static std::vector<unsigned int> quadIDs, sphereIDs;
};