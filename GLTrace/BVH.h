#pragma once
#include <glm/ext/vector_float3.hpp>
#include <vector>
#include "Hittables.h"
#include "ComputeShader.h"
#include <algorithm>
#include <chrono>
struct aabb {
	glm::vec4 aabbMin = glm::vec4(glm::vec3(1e30f), 1.0f), aabbMax = glm::vec4(glm::vec3(-1e30f), 1.0f); // vec4 for 16 byte padding
	void grow(const glm::vec3& p) {
		aabbMin = glm::min(aabbMin, glm::vec4(p, 1.0f));
		aabbMax = glm::max(aabbMax, glm::vec4(p, 1.0f));
	}
	void grow(const aabb& bounds) {
		if (bounds.aabbMin.x != 1e30f) {
			grow(bounds.aabbMin);
			grow(bounds.aabbMax);
		}
	}
	float area() const {
		const glm::vec3 e = aabbMax - aabbMin;
		return e.x * e.y + e.y * e.z + e.z * e.x;
	}
};

struct Bin {
	aabb bounds;
	int quadCount = 0;
	int sphereCount = 0;
};

struct BVHNode {
	aabb bbox;
	unsigned int leftChild; // rightChild == leftChild + 1
	unsigned int firstQuadPrimitive, quadPrimitiveCount;
	unsigned int firstSpherePrimitive, spherePrimitiveCount;
	unsigned int padding1, padding2, padding3;
	bool isLeaf() { return (quadPrimitiveCount > 0 || spherePrimitiveCount > 0); }
};

class BVH {
public:
	BVH() {}
	~BVH() {}

	void RefitBVH(const std::vector<Quad>& quads, const std::vector<Sphere>& spheres, const std::vector<glm::mat4>& transformBuffer) {
		for (int i = nodesUsed - 1; i >= 0; i--) {
			if (i != 1) {
				BVHNode& node = tree[i];
				if (node.isLeaf()) {
					UpdateNodeBounds(i, quads, spheres, transformBuffer);
					continue;
				}
				// not leaf node, adjust to child node bounds
				BVHNode& leftChild = tree[node.leftChild];
				BVHNode& rightChild = tree[node.leftChild + 1];
				node.bbox.aabbMin = glm::min(leftChild.bbox.aabbMin, rightChild.bbox.aabbMin);
				node.bbox.aabbMax = glm::max(leftChild.bbox.aabbMax, rightChild.bbox.aabbMax);
			}
		}
	}

	void BuildBVH(const std::vector<Quad>& quads, const std::vector<Sphere>& spheres, const std::vector<glm::mat4>& transformBuffer) {
		auto start = std::chrono::high_resolution_clock::now();
		totalElements = quads.size() + spheres.size();
		nodesUsed = 2;

		if (totalElements <= 0) {
			return;
		}

		// Initialise hittable IDs
		quadIDs.clear();
		sphereIDs.clear();
		quadIDs.reserve(quads.size());
		sphereIDs.reserve(spheres.size());
		for (unsigned int i = 0; i < quads.size(); i++) {
			quadIDs.push_back(i);
		}
		for (unsigned int i = 0; i < spheres.size(); i++) {
			sphereIDs.push_back(i);
		}

		// Create root node
		tree.clear();
		tree.reserve(totalElements * 2 + 2);
		for (unsigned int i = 0; i < totalElements * 2 + 2; i++) {
			tree.push_back(BVHNode());
		}
		BVHNode& root = tree[0];
		root.leftChild = 0;
		root.quadPrimitiveCount = quads.size();
		root.spherePrimitiveCount = spheres.size();
		root.padding1 = 0;
		root.padding2 = 0;
		root.padding3 = 0;
		UpdateNodeBounds(rootNodeID, quads, spheres, transformBuffer);

		// Recursive build
		Subdivide(rootNodeID, quads, spheres, transformBuffer);
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		// Extract hours, minutes, and seconds from the total duration
		auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
		duration -= hours;
		auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
		duration -= minutes;
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
		duration -= seconds;
		auto milliseconds = duration;  // Remaining milliseconds

		// Display the time elapsed
		//std::clog << "BVH constructed in: "
		//	<< hours.count() << " hours, "
		//	<< minutes.count() << " minutes, "
		//	<< seconds.count() << " seconds, "
		//	<< milliseconds.count() << " milliseconds\r\n";
	}

	void Buffer(ComputeShader& computeShader) const {
		const ShaderStorageBuffer* bvhSSBO = computeShader.GetSSBO(1);
		const ShaderStorageBuffer* sphereSSBO = computeShader.GetSSBO(2);
		const ShaderStorageBuffer* quadSSBO = computeShader.GetSSBO(3);

		// BVH buffer
		// ----------
		// Initialise buffer
		bvhSSBO->BufferData(nullptr, (sizeof(BVHNode) * (nodesUsed + 1)) + (sizeof(unsigned int) * 4), GL_STREAM_COPY);

		// Buffer data
		bvhSSBO->BufferSubData(&totalElements, sizeof(unsigned int), 0);
		bvhSSBO->BufferSubData(&nodesUsed, sizeof(unsigned int), sizeof(unsigned int));
		bvhSSBO->BufferSubData(&tree[0], sizeof(BVHNode) * (nodesUsed + 1), sizeof(unsigned int) * 4);

		// Sphere ID buffer
		// ----------------
		if (sphereIDs.size() > 0) {
			// Initialise buffer
			sphereSSBO->BufferData(nullptr, sizeof(unsigned int) * sphereIDs.size(), GL_STREAM_COPY);

			// Buffer data
			sphereSSBO->BufferData(&sphereIDs[0], sizeof(unsigned int) * sphereIDs.size(), GL_STREAM_COPY);
		}

		// Quad ID buffer
		// --------------
		if (quadIDs.size() > 0) {
			// Initialise buffer
			quadSSBO->BufferData(nullptr, sizeof(unsigned int) * quadIDs.size(), GL_STREAM_COPY);

			// Buffer data
			quadSSBO->BufferData(&quadIDs[0], sizeof(unsigned int) * quadIDs.size(), GL_STATIC_DRAW);
		}
	}

	const std::vector<BVHNode>& GetTree() const { return tree; }
	const std::vector<unsigned int>& GetQuadIDs() const { return quadIDs; }
	const std::vector<unsigned int>& GetSphereIDs() const { return sphereIDs; }

private:
	void UpdateNodeBounds(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres, const std::vector<glm::mat4>& transformBuffer) {
		BVHNode& node = tree[nodeID];
		for (unsigned int first = node.firstQuadPrimitive, i = 0; i < node.quadPrimitiveCount; i++) {
			const unsigned int quadID = quadIDs[first + i];
			const Quad& leafQuad = quads[quadID];
			const glm::mat4& transform = transformBuffer[spheres.size() + quadID];

			const glm::vec4& rawQ = leafQuad.GetQ(), rawU = leafQuad.GetU(), rawV = leafQuad.GetV();

			// Get world space vertices
			glm::vec4 worldQ = rawQ;
			glm::vec4 worldU = glm::vec4(glm::vec3(worldQ + rawU), 1.0f);
			glm::vec4 worldV = glm::vec4(glm::vec3(worldQ + rawV), 1.0f);

			// Transform vertices
			glm::vec4 transformedWorldQ = transform * worldQ;
			glm::vec4 transformedWorldU = transform * worldU;
			glm::vec4 transformedWorldV = transform * worldV;

			const glm::vec4 Q = transformedWorldQ;
			const glm::vec4 U = transformedWorldU - transformedWorldQ;
			const glm::vec4 V = transformedWorldV - transformedWorldQ;

			const glm::vec4 QU = Q + U;
			const glm::vec4 QV = Q + V;
			const glm::vec4 QUV = Q + U + V;

			node.bbox.grow(Q);
			node.bbox.grow(QU);
			node.bbox.grow(QV);

			if (leafQuad.triangle_disk_id != 1u) { // if not a triangleh
				node.bbox.grow(QUV);
			}

			if (leafQuad.triangle_disk_id == 2u) {
				node.bbox.grow(Q - U);
				node.bbox.grow(Q - V);
				node.bbox.grow(Q - (U + V));
				node.bbox.grow(Q - (U - V));
			}
		}
		for (unsigned int first = node.firstSpherePrimitive, i = 0; i < node.spherePrimitiveCount; i++) {
			const unsigned int sphereID = sphereIDs[first + i];
			const Sphere& leafSphere = spheres[sphereID];
			const glm::mat4& transform = transformBuffer[sphereID];

			const glm::vec4& center = transform * leafSphere.Center;
			const float sphereRadius = leafSphere.Radius;

			const glm::vec3 sphereMin = glm::vec3(center) - glm::vec3(sphereRadius), sphereMax = glm::vec3(center) + glm::vec3(sphereRadius);

			node.bbox.grow(glm::vec4(sphereMin, 1.0f));
			node.bbox.grow(glm::vec4(sphereMax, 1.0f));
		}
	}

	float EvaluateSAH(const BVHNode& node, const int axis, const float pos, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres, const std::vector<glm::mat4>& transformBuffer) {
		aabb leftAABB, rightAABB;
		int leftCount = 0, rightCount = 0;
		for (unsigned int i = 0; i < node.quadPrimitiveCount; i++) {
			const Quad& quad = quads[quadIDs[node.firstQuadPrimitive + i]];
			const glm::mat4& transform = transformBuffer[quadIDs[node.firstQuadPrimitive + i] + spheres.size()];

			const glm::vec4& rawQ = quad.GetQ(), rawU = quad.GetU(), rawV = quad.GetV();
			// Get world space vertices
			glm::vec4 worldQ = rawQ;
			glm::vec4 worldU = glm::vec4(glm::vec3(worldQ + rawU), 1.0f);
			glm::vec4 worldV = glm::vec4(glm::vec3(worldQ + rawV), 1.0f);

			// Transform vertices
			glm::vec4 transformedWorldQ = transform * worldQ;
			glm::vec4 transformedWorldU = transform * worldU;
			glm::vec4 transformedWorldV = transform * worldV;

			const glm::vec4 Q = transformedWorldQ;
			const glm::vec4 U = transformedWorldU - transformedWorldQ;
			const glm::vec4 V = transformedWorldV - transformedWorldQ;

			const glm::vec4 QU = Q + U;
			const glm::vec4 QV = Q + V;
			const glm::vec4 QUV = Q + U + V;

			if ((transform * quad.GetCentre())[axis] < pos) {
				leftCount++;

				leftAABB.grow(Q);
				leftAABB.grow(QU);
				leftAABB.grow(QV);

				if (quad.triangle_disk_id != 1u) { // if not a triangleh
					leftAABB.grow(QUV);
				}

				if (quad.triangle_disk_id == 2u) {
					leftAABB.grow(Q - U);
					leftAABB.grow(Q - V);
					leftAABB.grow(Q - (U + V));
					leftAABB.grow(Q - (U - V));
				}
			}
			else {
				rightCount++;

				rightAABB.grow(Q);
				rightAABB.grow(QU);
				rightAABB.grow(QV);

				if (quad.triangle_disk_id != 1u) { // if not a triangleh
					rightAABB.grow(QUV);
				}

				if (quad.triangle_disk_id == 2u) {
					rightAABB.grow(Q - U);
					rightAABB.grow(Q - V);
					rightAABB.grow(Q - (U + V));
					rightAABB.grow(Q - (U - V));
				}
			}
		}
		for (unsigned int i = 0; i < node.spherePrimitiveCount; i++) {
			const Sphere& sphere = spheres[sphereIDs[node.firstSpherePrimitive + i]];
			const glm::mat4& transform = transformBuffer[sphereIDs[node.firstSpherePrimitive + i]];

			const glm::vec4& center = transform * sphere.Center;
			const float sphereRadius = sphere.Radius;

			const glm::vec3 sphereMin = center - glm::vec4(sphereRadius, sphereRadius, sphereRadius, 0.0f);
			const glm::vec3 sphereMax = center + glm::vec4(sphereRadius, sphereRadius, sphereRadius, 0.0f);

			if (sphere.Center[axis] < pos) {
				leftCount++;
				
				leftAABB.grow(sphereMin);
				leftAABB.grow(sphereMax);
			}
			else {
				rightCount++;

				rightAABB.grow(sphereMin);
				rightAABB.grow(sphereMax);
			}
		}
		float leftBoxArea = leftAABB.area();
		float rightBoxArea = rightAABB.area();
		float cost = leftCount * leftBoxArea + rightCount * rightBoxArea;
		return cost > 0 ? cost : 1e30f;
	}

	/*
	float FindBestSplitPlane(const BVHNode& node, int& axis, float& splitPos, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		float bestCost = 1e30f;
		for (int a = 0; a < 3; a++) {
			for (unsigned int i = 0; i < node.quadPrimitiveCount; i++) {
				const Quad& quad = quads[quadIDs[node.firstQuadPrimitive + i]];
				float candidatePos = quad.GetCentre()[a];
				float cost = EvaluateSAH(node, a, candidatePos, quads, spheres);
				if (cost < bestCost) {
					splitPos = candidatePos;
					axis = a;
					bestCost = cost;
				}
			}
			for (unsigned int i = 0; i < node.spherePrimitiveCount; i++) {
				const Sphere& sphere = spheres[sphereIDs[node.firstSpherePrimitive + i]];
				float candidatePos = sphere.Center[a];
				float cost = EvaluateSAH(node, a, candidatePos, quads, spheres);
				if (cost < bestCost) {
					splitPos = candidatePos;
					axis = a;
					bestCost = cost;
				}
			}
		}
		return bestCost;
	}
	*/

	/*
	float FindBestSplitPlane(const BVHNode& node, int& axis, float& splitPos, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		float bestCost = 1e30f;
		const int interval = 4;
		for (int a = 0; a < 3; a++) {
			float boundsMin = node.bbox.aabbMin[a];
			float boundsMax = node.bbox.aabbMax[a];
			if (boundsMin == boundsMax) { continue; }
			float scale = (boundsMax - boundsMin) / interval;
			for (unsigned int i = 1; i < interval; i++) {
				float candidatePos = boundsMin + i * scale;
				float cost = EvaluateSAH(node, a, candidatePos, quads, spheres);
				if (cost < bestCost) {
					splitPos = candidatePos;
					axis = a;
					bestCost = cost;
				}
			}
		}
		return bestCost;
	}
	*/
	
	float FindBestSplitPlane(const BVHNode& node, int& axis, float& splitPos, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres, const std::vector<glm::mat4>& transformBuffer) {
		float bestCost = 1e30f;
		for (int a = 0; a < 3; a++) {
			float boundsMin = 1e30f, boundsMax = -1e30f;
			for (int i = 0; i < node.quadPrimitiveCount; i++) {
				const Quad& quad = quads[quadIDs[node.firstQuadPrimitive + i]];
				const glm::mat4& transform = transformBuffer[quadIDs[node.firstQuadPrimitive + i] + spheres.size()];
				boundsMin = std::min(boundsMin, (transform * quad.GetCentre())[a]);
				boundsMax = std::max(boundsMax, (transform * quad.GetCentre())[a]);
			}
			for (int i = 0; i < node.spherePrimitiveCount; i++) {
				const Sphere& sphere = spheres[sphereIDs[node.firstSpherePrimitive + i]];
				const glm::mat4& transform = transformBuffer[sphereIDs[node.firstSpherePrimitive + i]];
				boundsMin = std::min(boundsMin, (transform * sphere.Center)[a]);
				boundsMax = std::max(boundsMax, (transform * sphere.Center)[a]);
			}
			if (boundsMin == boundsMax) { continue; }
			// populate bins
			const int BINS = 8;
			Bin bin[BINS];
			float scale = BINS / (boundsMax - boundsMin);
			// Quads
			for (unsigned int i = 0; i < node.quadPrimitiveCount; i++) {
				const Quad& quad = quads[quadIDs[node.firstQuadPrimitive + i]];
				const glm::mat4& transform = transformBuffer[quadIDs[node.firstQuadPrimitive + i] + spheres.size()];

				const int binID = std::min(BINS - 1, (int)(((transform * quad.GetCentre())[a] - boundsMin) * scale));
				bin[binID].quadCount++;

				const glm::vec4& rawQ = quad.GetQ(), rawU = quad.GetU(), rawV = quad.GetV();
				// Get world space vertices
				glm::vec4 worldQ = rawQ;
				glm::vec4 worldU = glm::vec4(glm::vec3(worldQ + rawU), 1.0f);
				glm::vec4 worldV = glm::vec4(glm::vec3(worldQ + rawV), 1.0f);

				// Transform vertices
				glm::vec4 transformedWorldQ = transform * worldQ;
				glm::vec4 transformedWorldU = transform * worldU;
				glm::vec4 transformedWorldV = transform * worldV;

				const glm::vec4 Q = transformedWorldQ;
				const glm::vec4 U = transformedWorldU - transformedWorldQ;
				const glm::vec4 V = transformedWorldV - transformedWorldQ;

				const glm::vec4 QU = Q + U;
				const glm::vec4 QV = Q + V;
				const glm::vec4 QUV = Q + U + V;

				bin[binID].bounds.grow(Q);
				bin[binID].bounds.grow(QU);
				bin[binID].bounds.grow(QV);

				if (quad.triangle_disk_id != 1u) { // if not a triangleh
					bin[binID].bounds.grow(QUV);
				}

				if (quad.triangle_disk_id == 2u) {
					bin[binID].bounds.grow(Q - U);
					bin[binID].bounds.grow(Q - V);
					bin[binID].bounds.grow(Q - (U + V));
					bin[binID].bounds.grow(Q - (U - V));
				}
			}
			// Spheres
			for (unsigned int i = 0; i < node.spherePrimitiveCount; i++) {
				const Sphere& sphere = spheres[sphereIDs[node.firstSpherePrimitive + i]];
				const glm::mat4& transform = transformBuffer[sphereIDs[node.firstSpherePrimitive + i]];

				const int binID = std::min(BINS - 1, (int)(((transform * sphere.Center)[a] - boundsMin) * scale));
				bin[binID].sphereCount++;

				const glm::vec4& center = transform * sphere.Center;
				const float sphereRadius = sphere.Radius;

				const glm::vec3 sphereMin = center - glm::vec4(sphereRadius, sphereRadius, sphereRadius, 0.0f);
				const glm::vec3 sphereMax = center + glm::vec4(sphereRadius, sphereRadius, sphereRadius, 0.0f);

				bin[binID].bounds.grow(sphereMin);
				bin[binID].bounds.grow(sphereMax);
			}
			// gather data for planes between bins
			float leftArea[BINS - 1], rightArea[BINS - 1];
			int leftCount[BINS - 1], rightCount[BINS - 1];
			aabb leftBox, rightBox;
			int leftSum = 0, rightSum = 0;
			for (int i = 0; i < BINS - 1; i++) {
				leftSum += bin[i].quadCount + bin[i].sphereCount;
				leftCount[i] = leftSum;
				leftBox.grow(bin[i].bounds);
				leftArea[i] = leftBox.area();

				rightSum += bin[BINS - 1 - i].quadCount + bin[BINS - 1 - i].sphereCount;
				rightCount[BINS - 2 - i] = rightSum;
				rightBox.grow(bin[BINS - 1 - i].bounds);
				rightArea[BINS - 2 - i] = rightBox.area();
			}
			// calculate SAH cost for planes
			scale = (boundsMax - boundsMin) / BINS;
			for (int i = 0; i < BINS - 1; i++) {
				float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
				if (planeCost < bestCost) {
					axis = a;
					splitPos = boundsMin + scale * (i + 1);
					bestCost = planeCost;
				}
			}
		}
		return bestCost;
	}
	
	float CalculateNodeCost(const BVHNode& node) {
		float parentArea = node.bbox.area();
		return (node.quadPrimitiveCount + node.spherePrimitiveCount) * parentArea;
	}

	void Subdivide(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres, const std::vector<glm::mat4>& transformBuffer) {
		BVHNode& node = tree[nodeID];

		// Determine split axis using SAH
		int axis;
		float splitPos;
		float splitCost = FindBestSplitPlane(node, axis, splitPos, quads, spheres, transformBuffer);

		// Get parent area
		float parentCost = CalculateNodeCost(node);
		if (splitCost >= parentCost) { return; } // Further splits will be detrimental. Return

		// Split node quads
		//const int BINS = 8;
		//const float scale = BINS / (node.bbox.aabbMax[axis] - node.bbox.aabbMin[axis]);

		int quadI = node.firstQuadPrimitive;
		int quadJ = quadI + node.quadPrimitiveCount - 1;
		while (quadI <= quadJ) {
			//const int binID = std::min(BINS - 1, (int)((quads[quadIDs[quadI]].GetCentre()[axis] - node.bbox.aabbMin[axis]) * scale));
			//if (binID < splitPos) { quadI++; }
			//else { std::swap(quadIDs[quadI], quadIDs[quadJ--]); }
			const glm::mat4& transform = transformBuffer[quadIDs[quadI] + spheres.size()];
			if ((transform * quads[quadIDs[quadI]].GetCentre())[axis] < splitPos) { quadI++; }
			else { std::swap(quadIDs[quadI], quadIDs[quadJ--]); }
		}
		// Split node spheres
		int sphereI = node.firstSpherePrimitive;
		int sphereJ = sphereI + node.spherePrimitiveCount - 1;
		while (sphereI <= sphereJ) {
			//const int binID = std::min(BINS - 1, (int)((spheres[sphereIDs[sphereI]].Center[axis] - node.bbox.aabbMin[axis]) * scale));
			//if (binID < splitPos) { sphereI++; }
			//else { std::swap(sphereIDs[sphereI], sphereIDs[sphereJ--]); }
			const glm::mat4& transform = transformBuffer[sphereIDs[sphereI]];
			if ((transform * spheres[sphereIDs[sphereI]].Center)[axis] < splitPos) { sphereI++; }
			else { std::swap(sphereIDs[sphereI], sphereIDs[sphereJ--]); }
		}

		int leftQuadCount = quadI - node.firstQuadPrimitive;
		int leftSphereCount = sphereI - node.firstSpherePrimitive;

		if (leftQuadCount + leftSphereCount == 0) { return; }

		// Create child nodes
		int leftChildID = ++nodesUsed;
		int rightChildID = ++nodesUsed;
		node.leftChild = leftChildID;

		tree[leftChildID].firstQuadPrimitive = node.firstQuadPrimitive;
		tree[leftChildID].quadPrimitiveCount = leftQuadCount;
		tree[leftChildID].firstSpherePrimitive = node.firstSpherePrimitive;
		tree[leftChildID].spherePrimitiveCount = leftSphereCount;
		tree[leftChildID].padding1 = 0;
		tree[leftChildID].padding2 = 0;
		tree[leftChildID].padding3 = 0;
		tree[rightChildID].firstQuadPrimitive = quadI;
		tree[rightChildID].quadPrimitiveCount = node.quadPrimitiveCount - leftQuadCount;
		tree[rightChildID].firstSpherePrimitive = sphereI;
		tree[rightChildID].spherePrimitiveCount = node.spherePrimitiveCount - leftSphereCount;
		tree[rightChildID].padding1 = 0;
		tree[rightChildID].padding2 = 0;
		tree[rightChildID].padding3 = 0;
		node.quadPrimitiveCount = 0;
		node.spherePrimitiveCount = 0;

		UpdateNodeBounds(leftChildID, quads, spheres, transformBuffer);
		UpdateNodeBounds(rightChildID, quads, spheres, transformBuffer);

		// Recursive split
		Subdivide(leftChildID, quads, spheres, transformBuffer);
		Subdivide(rightChildID, quads, spheres, transformBuffer);
	}

	/*
	void Subdivide(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		BVHNode& node = tree[nodeID];
		if (node.quadPrimitiveCount + node.spherePrimitiveCount <= 2) { return; }

		// Find biggest extent
		const glm::vec3 extent = node.aabbMax - node.aabbMin;
		int biggestAxis[3] = { 0, 1, 2 };
		std::sort(biggestAxis, biggestAxis + 3, [&](int a, int b) { return extent[a] > extent[b]; });

		int quadI;
		int sphereI;
		int bestAxis = -1;
		for (int i = 0; i < 3; i++) {
			// Test split without swapping
			const int axis = biggestAxis[i];
			const float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

			// Split node quads
			quadI = node.firstQuadPrimitive;
			int quadJ = quadI + node.quadPrimitiveCount - 1;
			while (quadI <= quadJ) {
				if (quads[quadIDs[quadI]].GetCentre()[axis] < splitPos) { quadI++; }
				else { quadJ--; }
			}
			// Split node spheres
			sphereI = node.firstSpherePrimitive;
			int sphereJ = sphereI + node.spherePrimitiveCount - 1;
			while (sphereI <= sphereJ) {
				if (spheres[sphereIDs[sphereI]].Center[axis] < splitPos) { sphereI++; }
				else { sphereJ--; }
			}

			// Check if one side of split is empty
			int leftQuadCount = quadI - node.firstQuadPrimitive;
			int leftSphereCount = sphereI - node.firstSpherePrimitive;
			int totalLeftCount = leftQuadCount + leftSphereCount;
			if (totalLeftCount > 0 && totalLeftCount < node.quadPrimitiveCount + node.spherePrimitiveCount) {
				// valid split found
				bestAxis = axis;
				break;
			}
		}
		// If no axis found, return. Node cannot be split
		if (bestAxis == -1) { return; }

		const float splitPos = node.aabbMin[bestAxis] + extent[bestAxis] * 0.5f;

		// Split node quads
		quadI = node.firstQuadPrimitive;
		int quadJ = quadI + node.quadPrimitiveCount - 1;
		while (quadI <= quadJ) {
			if (quads[quadIDs[quadI]].GetCentre()[bestAxis] < splitPos) { quadI++; }
			else { std::swap(quadIDs[quadI], quadIDs[quadJ--]); }
		}
		// Split node spheres
		sphereI = node.firstSpherePrimitive;
		int sphereJ = sphereI + node.spherePrimitiveCount - 1;
		while (sphereI <= sphereJ) {
			if (spheres[sphereIDs[sphereI]].Center[bestAxis] < splitPos) { sphereI++; }
			else { std::swap(sphereIDs[sphereI], sphereIDs[sphereJ--]); }
		}

		int leftQuadCount = quadI - node.firstQuadPrimitive;
		int leftSphereCount = sphereI - node.firstSpherePrimitive;

		// Create child nodes
		int leftChildID = ++nodesUsed;
		int rightChildID = ++nodesUsed;
		node.leftChild = leftChildID;

		tree[leftChildID].firstQuadPrimitive = node.firstQuadPrimitive;
		tree[leftChildID].quadPrimitiveCount = leftQuadCount;
		tree[leftChildID].firstSpherePrimitive = node.firstSpherePrimitive;
		tree[leftChildID].spherePrimitiveCount = leftSphereCount;
		tree[leftChildID].padding1 = 0;
		tree[leftChildID].padding2 = 0;
		tree[leftChildID].padding3 = 0;
		tree[rightChildID].firstQuadPrimitive = quadI;
		tree[rightChildID].quadPrimitiveCount = node.quadPrimitiveCount - leftQuadCount;
		tree[rightChildID].firstSpherePrimitive = sphereI;
		tree[rightChildID].spherePrimitiveCount = node.spherePrimitiveCount - leftSphereCount;
		tree[rightChildID].padding1 = 0;
		tree[rightChildID].padding2 = 0;
		tree[rightChildID].padding3 = 0;
		node.quadPrimitiveCount = 0;
		node.spherePrimitiveCount = 0;

		UpdateNodeBounds(leftChildID, quads, spheres);
		UpdateNodeBounds(rightChildID, quads, spheres);

		// Recursive split
		Subdivide(leftChildID, quads, spheres);
		Subdivide(rightChildID, quads, spheres);
	}
	*/

	unsigned int rootNodeID, nodesUsed, totalElements;
	std::vector<BVHNode> tree;

	std::vector<unsigned int> quadIDs, sphereIDs;
};