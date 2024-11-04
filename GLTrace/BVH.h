#pragma once
#include <glm/ext/vector_float3.hpp>
#include <vector>
#include "Hittables.h"

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
		bvhSSBO->BufferData(nullptr, (sizeof(BVHNode) * nodesUsed) + (sizeof(unsigned int) * 4), GL_STATIC_DRAW);

		// Buffer data
		bvhSSBO->BufferSubData(&totalElements, sizeof(unsigned int), 0);
		bvhSSBO->BufferSubData(&nodesUsed, sizeof(unsigned int), sizeof(unsigned int));
		bvhSSBO->BufferSubData(&tree[0], sizeof(BVHNode) * nodesUsed, sizeof(unsigned int) * 4);

		// Sphere ID buffer
		// ----------------
		// Initialise buffer
		sphereSSBO->BufferData(nullptr, sizeof(unsigned int) * sphereIDs.size(), GL_STATIC_DRAW);

		// Buffer data
		sphereSSBO->BufferData(&sphereIDs[0], sizeof(unsigned int) * sphereIDs.size(), GL_STATIC_DRAW);

		// Quad ID buffer
		// --------------
		// Initialise buffer
		quadSSBO->BufferData(nullptr, sizeof(unsigned int) * quadIDs.size(), GL_STATIC_DRAW);

		// Buffer data
		quadSSBO->BufferData(&quadIDs[0], sizeof(unsigned int) * quadIDs.size(), GL_STATIC_DRAW);
	}

private:
	void UpdateNodeBounds(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		BVHNode& node = tree[nodeID];
		node.aabbMin = glm::vec4(1e30f, 1e30f, 1e30f, 1.0f);
		node.aabbMax = glm::vec4(-1e30f, -1e30f, -1e30f, 1.0f);
		for (unsigned int first = node.firstQuadPrimitive, i = 0; i < node.quadPrimitiveCount; i++) {
			const unsigned int quadID = quadIDs[first + i];
			const Quad& leafQuad = quads[quadID];

			const glm::vec3& Q = leafQuad.GetQ(), U = leafQuad.GetU(), V = leafQuad.GetV();
			const glm::vec4 QU = glm::vec4(Q + U, 1.0f);
			const glm::vec4 QV = glm::vec4(Q + V, 1.0f);

			node.aabbMin = glm::min(node.aabbMin, glm::vec4(Q, 1.0f));
			node.aabbMin = glm::min(node.aabbMin, QU);
			node.aabbMin = glm::min(node.aabbMin, QV);

			node.aabbMax = glm::max(node.aabbMax, glm::vec4(Q, 1.0f));
			node.aabbMax = glm::max(node.aabbMax, QU);
			node.aabbMax = glm::max(node.aabbMax, QV);
		}
		for (unsigned int first = node.firstSpherePrimitive, i = 0; i < node.spherePrimitiveCount; i++) {
			const unsigned int sphereID = sphereIDs[first + i];
			const Sphere& leafSphere = spheres[sphereID];

			const float sphereRadius = leafSphere.Radius;
			const glm::vec3 sphereMin = leafSphere.Center - glm::vec3(sphereRadius), sphereMax = leafSphere.Center + glm::vec3(sphereRadius);

			node.aabbMin = glm::min(node.aabbMin, glm::vec4(sphereMin, 1.0f));
			node.aabbMax = glm::max(node.aabbMax, glm::vec4(sphereMax, 1.0f));
		}
	}

	void Subdivide(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		BVHNode& node = tree[nodeID];
		if (node.quadPrimitiveCount + node.spherePrimitiveCount <= 2) { return; }

		// Find biggest extent
		const glm::vec3 extent = node.aabbMax - node.aabbMin;
		int axis = 0;
		if (extent.y > extent.x) { axis = 1; }
		if (extent.z > extent[axis]) { axis = 2; }
		const float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

		// Split node quads
		int quadI = node.firstQuadPrimitive;
		int quadJ = quadI + node.quadPrimitiveCount - 1;
		while (quadI <= quadJ) {
			if (quads[quadIDs[quadI]].GetCentre()[axis] < splitPos) { quadI++; }
			else { std::swap(quadIDs[quadI], quadIDs[quadJ--]); }
		}
		// Split node spheres
		int sphereI = node.firstSpherePrimitive;
		int sphereJ = sphereI + node.spherePrimitiveCount - 1;
		while (sphereI <= sphereJ) {
			if (spheres[sphereIDs[sphereI]].Center[axis] < splitPos) { sphereI++; }
			else { std::swap(sphereIDs[sphereI], sphereIDs[sphereJ--]); }
		}

		// Check if one side of split is empty
		int leftQuadCount = quadI - node.firstQuadPrimitive;
		int leftSphereCount = sphereI - node.firstSpherePrimitive;
		if (leftQuadCount + leftSphereCount == 0 || leftQuadCount + leftSphereCount == node.quadPrimitiveCount + node.spherePrimitiveCount) { return; }

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


	unsigned int rootNodeID, nodesUsed, totalElements;
	std::vector<BVHNode> tree;

	std::vector<unsigned int> quadIDs, sphereIDs;
};