#pragma once
#include "Camera.h"
#include "GLTMath.h"
class CameraController {
public:
	Camera* activeCamera;
	float movementSpeed = 6.5f;
	float mouseSensitivity = 0.1f;

	void Update() {
		if (activeCamera) {
			glm::vec3 front;
			front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			front.y = sin(glm::radians(pitch));
			front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			front = glm::normalize(front);

			glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
			glm::vec3 up = glm::normalize(glm::cross(right, front));

			activeCamera->lookat = activeCamera->lookfrom + front;
			activeCamera->vup = up;
		}
	}

	void ProcessMouseMovement(double xpos, double ypos) {
		if (firstMouse) {
			lastMouseX = xpos;
			lastMouseY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastMouseX;
		float yoffset = lastMouseY - ypos;
		lastMouseX = xpos;
		lastMouseY = ypos;

		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;

		yaw += xoffset;
		pitch += yoffset;

		if (pitch > 89.0f) {
			pitch = 89.0f;
		}
		if (pitch < -89.0f) {
			pitch = -89.0f;
		}
	}

private:
	float yaw = -90.0f;
	float pitch = 0.0f;

	float lastMouseX, lastMouseY;
	bool firstMouse;
};