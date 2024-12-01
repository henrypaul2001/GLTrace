#pragma once
#include <limits>
#include <cmath>

// Constant
const double infinity_d = std::numeric_limits<double>::infinity();
const float infinity = std::numeric_limits<float>::infinity();
const double pi = 3.1415926535897932385;

// Utility
inline float degrees_to_radians(const float degrees) {
	return degrees * pi / 180.0f;
}
inline const glm::mat4 create_transform(const glm::vec3& translation, const glm::vec3& euler_degrees, const glm::vec3& scale) {
	glm::mat4 transform = glm::mat4(1.0f);
	transform = glm::scale(transform, scale);

	glm::quat rotation = glm::quat(glm::vec3(
		glm::radians(euler_degrees.x),
		glm::radians(euler_degrees.y),
		glm::radians(euler_degrees.z)));

	transform = glm::mat4_cast(rotation) * transform;
	transform = glm::translate(transform, translation);
	return transform;
}
inline void decompose_transform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale) {
	// Extract translation
	translation = glm::vec3(transform[3]);

	// Extract scale
	scale = glm::vec3(
		glm::length(glm::vec3(transform[0])),
		glm::length(glm::vec3(transform[1])),
		glm::length(glm::vec3(transform[2]))
	);

	// Remove scale from the matrix to isolate rotation
	glm::mat4 rotationMatrix = transform;
	rotationMatrix[0] /= scale.x;
	rotationMatrix[1] /= scale.y;
	rotationMatrix[2] /= scale.z;

	// Extract Euler angles (rotation) from the rotation matrix
	rotation = glm::eulerAngles(glm::quat_cast(rotationMatrix));
}
inline glm::mat4 compose_transform(const glm::vec3& translation, const glm::vec3& euler_rotation, const glm::vec3& scale) {
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation);
	transform *= glm::mat4_cast(glm::quat(euler_rotation));
	transform = glm::scale(transform, scale);
	return transform;
}
inline glm::mat4 compose_transform(const glm::vec3& translation, const glm::quat& rotation_quat, const glm::vec3& scale) {
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation);
	transform *= glm::mat4_cast(rotation_quat);
	transform = glm::scale(transform, scale);
	return transform;
}