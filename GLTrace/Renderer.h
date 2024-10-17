#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "ComputeShader.h"
#include "Shader.h"

static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
	// ignore warning codes or insignificant errors
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:
		std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:
		std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:
		std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:
		std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:
		std::cout << "Type: Other"; break;
	}	std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:
		std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
}

class Renderer
{
public:
	Renderer(const unsigned int width = 600u, const unsigned int height = 600u) : SCR_WIDTH(width), SCR_HEIGHT(height) { Initialise(); }

	~Renderer() {
		glfwTerminate();
	}

	void Render();

	GLFWwindow* GetWindow() { return window; }

private:
	bool Initialise();

	Shader screenQuadShader;
	ComputeShader rtCompute;

	GLFWwindow* window;
	unsigned int SCR_WIDTH, SCR_HEIGHT, frameCount;
	float lastFrame, currentFrame;
	float dt;
};