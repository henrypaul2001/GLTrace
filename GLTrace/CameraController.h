#pragma once
#include "Camera.h"
#include "InputManager.h"
#include "GLTMath.h"
class CameraController {
public:
	Camera* activeCamera;
	float movementSpeed = 6.5f;
	float mouseSensitivity = 0.1f;

	void Initialise() {
		zoom = activeCamera->vfov;
		glm::vec3 front = glm::normalize(activeCamera->lookfrom - activeCamera->lookat);
		pitch = glm::degrees(asin(front.y));
		yaw = glm::degrees(atan2(front.z, front.x));
	}

	void Update(const float dt) {
		if (activeCamera && !Renderer::IsMouseFree()) {

			// Update view vectors
			glm::vec3 front;
			front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			front.y = sin(glm::radians(pitch));
			front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			front = glm::normalize(front);

			glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
			glm::vec3 up = glm::normalize(glm::cross(right, front));

			// Process key inputs
			float velocity = movementSpeed * dt;
			if (InputManager::IsKeyDown(GLFW_KEY_W)) {
				activeCamera->lookfrom = activeCamera->lookfrom += front * velocity;
				activeCamera->SetCameraHasMoved(true);
			}
			if (InputManager::IsKeyDown(GLFW_KEY_S)) {
				activeCamera->lookfrom = activeCamera->lookfrom -= front * velocity;
				activeCamera->SetCameraHasMoved(true);
			}
			if (InputManager::IsKeyDown(GLFW_KEY_A)) {
				activeCamera->lookfrom = activeCamera->lookfrom -= right * velocity;
				activeCamera->SetCameraHasMoved(true);
			}
			if (InputManager::IsKeyDown(GLFW_KEY_D)) {
				activeCamera->lookfrom = activeCamera->lookfrom += right * velocity;
				activeCamera->SetCameraHasMoved(true);
			}
			if (InputManager::IsKeyDown(GLFW_KEY_SPACE)) {
				activeCamera->lookfrom = activeCamera->lookfrom += up * velocity;
				activeCamera->SetCameraHasMoved(true);
			}
			if (InputManager::IsKeyDown(GLFW_KEY_LEFT_SHIFT)) {
				activeCamera->lookfrom = activeCamera->lookfrom -= up * velocity;
				activeCamera->SetCameraHasMoved(true);
			}

			activeCamera->lookat = activeCamera->lookfrom + front;
			activeCamera->vup = up;

			activeCamera->vfov = zoom;
		}
	}

	void ProcessMouseMovement(const double xpos, const double ypos) {
		if (firstMouse) {
			lastMouseX = xpos;
			lastMouseY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastMouseX;
		float yoffset = lastMouseY - ypos;

		if (abs(xoffset) > 0.0f || abs(yoffset) > 0.0f) { activeCamera->SetCameraHasMoved(true); }

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

	void ProcessMouseScroll(const double yoffset) {
		zoom -= (float)yoffset;
		if (zoom < 1.0f) {
			zoom = 1.0f;
		}
		else if (zoom > 120.0f) {
			zoom = 120.0f;
		}

		if (abs(yoffset) > 0.0f) { activeCamera->SetCameraHasMoved(true); }
	}

private:
	float yaw = -90.0f;
	float pitch = 0.0f;
	float zoom = 90.0f;

	float lastMouseX, lastMouseY;
	bool firstMouse;
};