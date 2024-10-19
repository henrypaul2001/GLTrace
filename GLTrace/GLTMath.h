#pragma once
#include <limits>

// Constant
const double infinity_d = std::numeric_limits<double>::infinity();
const float infinity = std::numeric_limits<float>::infinity();
const double pi = 3.1415926535897932385;

// Utility
inline float degrees_to_radians(const float degrees) {
	return degrees * pi / 180.0f;
}