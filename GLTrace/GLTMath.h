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