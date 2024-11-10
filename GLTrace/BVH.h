#pragma once
#include <glm/ext/vector_float3.hpp>
#include <vector>
#include "Hittables.h"
#include "ComputeShader.h"
#include <algorithm>
struct BVHNode {
	glm::vec4 aabbMin, aabbMax; // vec4 for 16 byte padding
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

	void BuildBVH(const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		totalElements = quads.size() + spheres.size();

		// Initialise hittable IDs
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
		tree.reserve(totalElements * 2 - 1);
		for (unsigned int i = 0; i < totalElements * 2 - 1; i++) {
			tree.push_back(BVHNode());
		}
		BVHNode& root = tree[0];
		root.leftChild = 0;
		root.quadPrimitiveCount = quads.size();
		root.spherePrimitiveCount = spheres.size();
		root.padding1 = 0;
		root.padding2 = 0;
		root.padding3 = 0;
		UpdateNodeBounds(rootNodeID, quads, spheres);

		// Recursive build
		Subdivide(rootNodeID, quads, spheres);
	}

	void Buffer(ComputeShader& computeShader) const {
		const ShaderStorageBuffer* bvhSSBO = computeShader.GetSSBO(1);
		const ShaderStorageBuffer* sphereSSBO = computeShader.GetSSBO(2);
		const ShaderStorageBuffer* quadSSBO = computeShader.GetSSBO(3);

		// BVH buffer
		// ----------
		// Initialise buffer
		bvhSSBO->BufferData(nullptr, (sizeof(BVHNode) * (nodesUsed + 1)) + (sizeof(unsigned int) * 4), GL_STATIC_DRAW);

		// Buffer data
		bvhSSBO->BufferSubData(&totalElements, sizeof(unsigned int), 0);
		bvhSSBO->BufferSubData(&nodesUsed, sizeof(unsigned int), sizeof(unsigned int));
		bvhSSBO->BufferSubData(&tree[0], sizeof(BVHNode) * (nodesUsed + 1), sizeof(unsigned int) * 4);

		// Sphere ID buffer
		// ----------------
		if (sphereIDs.size() > 0) {
			// Initialise buffer
			sphereSSBO->BufferData(nullptr, sizeof(unsigned int) * sphereIDs.size(), GL_STATIC_DRAW);

			// Buffer data
			sphereSSBO->BufferData(&sphereIDs[0], sizeof(unsigned int) * sphereIDs.size(), GL_STATIC_DRAW);
		}

		// Quad ID buffer
		// --------------
		if (quadIDs.size() > 0) {
			// Initialise buffer
			quadSSBO->BufferData(nullptr, sizeof(unsigned int) * quadIDs.size(), GL_STATIC_DRAW);

			// Buffer data
			quadSSBO->BufferData(&quadIDs[0], sizeof(unsigned int) * quadIDs.size(), GL_STATIC_DRAW);
		}
	}

	const std::vector<BVHNode>& GetTree() const { return tree; }
	const std::vector<unsigned int>& GetQuadIDs() const { return quadIDs; }
	const std::vector<unsigned int>& GetSphereIDs() const { return sphereIDs; }

private:
	void UpdateNodeBounds(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		BVHNode& node = tree[nodeID];
		node.aabbMin = glm::vec4(1e30f, 1e30f, 1e30f, 1.0f);
		node.aabbMax = glm::vec4(-1e30f, -1e30f, -1e30f, 1.0f);
		for (unsigned int first = node.firstQuadPrimitive, i = 0; i < node.quadPrimitiveCount; i++) {
			const unsigned int quadID = quadIDs[first + i];
			const Quad& leafQuad = quads[quadID];

			const glm::vec4& Q = leafQuad.GetQ(), U = leafQuad.GetU(), V = leafQuad.GetV();
			const glm::vec4 QU = Q + U;
			const glm::vec4 QV = Q + V;
			const glm::vec4 QUV = Q + U + V;

			node.aabbMin = glm::min(node.aabbMin, Q);
			node.aabbMin = glm::min(node.aabbMin, QU);
			node.aabbMin = glm::min(node.aabbMin, QV);

			node.aabbMax = glm::max(node.aabbMax, Q);
			node.aabbMax = glm::max(node.aabbMax, QU);
			node.aabbMax = glm::max(node.aabbMax, QV);

			if (leafQuad.triangle_disk_id != 1u) { // if not a triangle
				node.aabbMin = glm::min(node.aabbMin, QUV);
				node.aabbMax = glm::max(node.aabbMax, QUV);
			}
		}
		for (unsigned int first = node.firstSpherePrimitive, i = 0; i < node.spherePrimitiveCount; i++) {
			const unsigned int sphereID = sphereIDs[first + i];
			const Sphere& leafSphere = spheres[sphereID];

			const float sphereRadius = leafSphere.Radius;
			const glm::vec3 sphereMin = glm::vec3(leafSphere.Center) - glm::vec3(sphereRadius), sphereMax = glm::vec3(leafSphere.Center) + glm::vec3(sphereRadius);

			node.aabbMin = glm::min(node.aabbMin, glm::vec4(sphereMin, 1.0f));
			node.aabbMax = glm::max(node.aabbMax, glm::vec4(sphereMax, 1.0f));
		}
	}

	float EvaluateSAH(const BVHNode& node, const int axis, const float pos, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		glm::vec3 leftAABBMin = glm::vec3(1e30f), leftAABBMax = glm::vec3(-1e30f);
		glm::vec3 rightAABBMin = glm::vec3(1e30f), rightAABBMax = glm::vec3(-1e30f);
		int leftCount = 0, rightCount = 0;
		for (unsigned int i = 0; i < node.quadPrimitiveCount; i++) {
			const Quad& quad = quads[quadIDs[node.firstQuadPrimitive + i]];
			const glm::vec3& Q = quad.GetQ(), U = quad.GetU(), V = quad.GetV();
			const glm::vec3 QU = Q + U;
			const glm::vec3 QV = Q + V;
			const glm::vec3 QUV = Q + U + V;

			if (quad.GetCentre()[axis] < pos) {
				leftCount++;

				leftAABBMin = glm::min(leftAABBMin, Q);
				leftAABBMin = glm::min(leftAABBMin, QU);
				leftAABBMin = glm::min(leftAABBMin, QV);

				leftAABBMax = glm::max(leftAABBMax, Q);
				leftAABBMax = glm::max(leftAABBMax, QU);
				leftAABBMax = glm::max(leftAABBMax, QV);

				if (quad.triangle_disk_id != 1u) { // if not a triangle
					leftAABBMin = glm::min(leftAABBMin, QUV);
					leftAABBMax = glm::max(leftAABBMax, QUV);
				}
			}
			else {
				rightCount++;

				rightAABBMin = glm::min(rightAABBMin, Q);
				rightAABBMin = glm::min(rightAABBMin, QU);
				rightAABBMin = glm::min(rightAABBMin, QV);

				rightAABBMax = glm::max(rightAABBMax, Q);
				rightAABBMax = glm::max(rightAABBMax, QU);
				rightAABBMax = glm::max(rightAABBMax, QV);

				if (quad.triangle_disk_id != 1u) { // if not a triangle
					rightAABBMin = glm::min(rightAABBMin, QUV);
					rightAABBMax = glm::max(rightAABBMax, QUV);
				}
			}
		}
		for (unsigned int i = 0; i < node.spherePrimitiveCount; i++) {
			const Sphere& sphere = spheres[sphereIDs[node.firstSpherePrimitive + i]];
			const float sphereRadius = sphere.Radius;
			const glm::vec3 sphereMin = glm::vec3(sphere.Center) - glm::vec3(sphereRadius);
			const glm::vec3 sphereMax = glm::vec3(sphere.Center) + glm::vec3(sphereRadius);

			if (sphere.Center[axis] < pos) {
				leftCount++;

				leftAABBMin = glm::min(leftAABBMin, sphereMin);
				leftAABBMax = glm::max(leftAABBMax, sphereMax);
			}
			else {
				rightCount++;

				rightAABBMin = glm::min(rightAABBMin, sphereMin);
				rightAABBMax = glm::max(rightAABBMax, sphereMax);
			}
		}
		const glm::vec3 leftExtent = leftAABBMax - leftAABBMin;
		float leftBoxArea = leftExtent.x * leftExtent.y + leftExtent.y * leftExtent.z + leftExtent.z * leftExtent.x;
		const glm::vec3 rightExtent = rightAABBMax - rightAABBMin;
		float rightBoxArea = rightExtent.x * rightExtent.y + rightExtent.y * rightExtent.z + rightExtent.z * rightExtent.x;
		float cost = leftCount * leftBoxArea + rightCount * rightBoxArea;
		return cost > 0 ? cost : 1e30f;
	}

	void Subdivide(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		BVHNode& node = tree[nodeID];

		// Determine split axis using SAH
		int bestAxis = -1;
		float bestPos = 0.0f, bestCost = 1e30f;
		for (int axis = 0; axis < 3; axis++) {
			for (unsigned int i = 0; i < node.quadPrimitiveCount; i++) {
				const Quad& quad = quads[quadIDs[node.firstQuadPrimitive + i]];
				float candidatePos = quad.GetCentre()[axis];
				float cost = EvaluateSAH(node, axis, candidatePos, quads, spheres);
				if (cost < bestCost) {
					bestPos = candidatePos;
					bestAxis = axis;
					bestCost = cost;
				}
			}
			for (unsigned int i = 0; i < node.spherePrimitiveCount; i++) {
				const Sphere& sphere = spheres[sphereIDs[node.firstSpherePrimitive + i]];
				float candidatePos = sphere.Center[axis];
				float cost = EvaluateSAH(node, axis, candidatePos, quads, spheres);
				if (cost < bestCost) {
					bestPos = candidatePos;
					bestAxis = axis;
					bestCost = cost;
				}
			}
		}
		int axis = bestAxis;
		float splitPos = bestPos;

		// Get parent area
		const glm::vec3 e = node.aabbMax - node.aabbMin;
		float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
		float parentCost = (node.quadPrimitiveCount + node.spherePrimitiveCount) * parentArea;

		if (bestCost >= parentCost) { return; } // Further splits will be detrimental. Return

		// Split node quads
		int quadI = node.firstQuadPrimitive;
		int quadJ = quadI + node.quadPrimitiveCount - 1;
		while (quadI <= quadJ) {
			if (quads[quadIDs[quadI]].GetCentre()[bestAxis] < splitPos) { quadI++; }
			else { std::swap(quadIDs[quadI], quadIDs[quadJ--]); }
		}
		// Split node spheres
		int sphereI = node.firstSpherePrimitive;
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