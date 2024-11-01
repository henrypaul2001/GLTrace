#pragma once
#include <glm/ext/vector_float3.hpp>
#include <vector>
#include "Hittables.h"

struct BVHNode {
	glm::vec3 aabbMin, aabbMax;
	unsigned int leftChild; // rightChild == leftChild + 1
	unsigned int firstQuadPrimitive, quadPrimitiveCount;
	bool isLeaf() { return quadPrimitiveCount > 0; }
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
		UpdateNodeBounds(rootNodeID, quads, spheres);

		// Recursive build
		Subdivide(rootNodeID, quads, spheres);
	}

	void UpdateNodeBounds(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		BVHNode& node = tree[nodeID];
		node.aabbMin = glm::vec3(1e30f);
		node.aabbMax = glm::vec3(-1e30f);
		for (unsigned int first = node.firstQuadPrimitive, i = 0; i < node.quadPrimitiveCount; i++) {
			unsigned int quadID = quadIDs[first + i];
			const Quad& leafQuad = quads[quadID];

			const glm::vec3& Q = leafQuad.GetQ(), U = leafQuad.GetU(), V = leafQuad.GetV();

			node.aabbMin = glm::min(node.aabbMin, Q);
			node.aabbMin = glm::min(node.aabbMin, Q + U);
			node.aabbMin = glm::min(node.aabbMin, Q + V);

			node.aabbMax = glm::max(node.aabbMax, Q);
			node.aabbMax = glm::max(node.aabbMax, Q + U);
			node.aabbMax = glm::max(node.aabbMax, Q + V);
		}
	}

	void Subdivide(const unsigned int nodeID, const std::vector<Quad>& quads, const std::vector<Sphere>& spheres) {
		BVHNode& node = tree[nodeID];
		if (node.quadPrimitiveCount <= 2) { return; }

		// Find biggest extent
		const glm::vec3 extent = node.aabbMax - node.aabbMin;
		int axis = 0;
		if (extent.y > extent.x) { axis = 1; }
		if (extent.z > extent[axis]) { axis = 2; }
		const float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

		// Split node
		int i = node.firstQuadPrimitive;
		int j = i + node.quadPrimitiveCount - 1;
		while (i <= j) {
			if (quads[quadIDs[i]].GetCentre()[axis] < splitPos) { i++; }
			else { std::swap(quadIDs[i], quadIDs[j--]); }
		}

		// Check if one side of split is empty
		int leftCount = i - node.firstQuadPrimitive;
		if (leftCount == 0 || leftCount == node.quadPrimitiveCount) { return; }

		// Create child nodes
		int leftChildID = ++nodesUsed;
		int rightChildID = ++nodesUsed;
		node.leftChild = leftChildID;
		
		tree[leftChildID].firstQuadPrimitive = node.firstQuadPrimitive;
		tree[leftChildID].quadPrimitiveCount = leftCount;
		tree[rightChildID].firstQuadPrimitive = i;
		tree[rightChildID].quadPrimitiveCount = node.quadPrimitiveCount - leftCount;
		node.quadPrimitiveCount = 0;

		UpdateNodeBounds(leftChildID, quads, spheres);
		UpdateNodeBounds(rightChildID, quads, spheres);

		// Recursive split
		Subdivide(leftChildID, quads, spheres);
		Subdivide(rightChildID, quads, spheres);
	}

private:
	unsigned int rootNodeID, nodesUsed, totalElements;
	std::vector<BVHNode> tree;

	std::vector<unsigned int> quadIDs, sphereIDs;
};