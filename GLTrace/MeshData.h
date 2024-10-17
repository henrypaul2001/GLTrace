#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <vector>
#include <glad/glad.h>

struct Vertex {
	glm::vec3 Position;
	glm::vec2 TexCoords;
};

class MeshData
{
public:
	MeshData() {
		VAO = 0;
		VBO = 0;
		EBO = 0;
		generated = false;
	}
	MeshData(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const GLenum bufferUsage = GL_STATIC_DRAW) {
		VAO = 0;
		VBO = 0;
		EBO = 0;
		
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		SetupMesh(vertices, indices, bufferUsage);
	}
	~MeshData() {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	}

	void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const GLenum bufferUsage = GL_STATIC_DRAW) {
		this->vertices = vertices;
		this->indices = indices;

		if (!generated) {
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			generated = true;
		}

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], bufferUsage);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], bufferUsage);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		// texture coords
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	}

	void DrawMeshData(const GLenum drawPrimitive = GL_TRIANGLES) {
		glBindVertexArray(VAO);
		glDrawElements(drawPrimitive, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	const std::vector<Vertex>& GetVertices() const { return vertices; }
	const unsigned int GetVAO() const { return VAO; }
	const unsigned int GetVBO() const { return VBO; }
	const unsigned int GetEBO() const { return EBO; }
private:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	unsigned int VAO, VBO, EBO;
	bool generated;
};