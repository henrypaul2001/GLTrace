#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "ComputeShader.h"
#include "Shader.h"
#include "MeshData.h"
#include "Texture2DArray.h"
#include "Camera.h"
#include "InputManager.h"
#include "Scene.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

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

const unsigned int WORK_GROUP_SIZE = 32u;

class Renderer
{
public:
	Renderer(const unsigned int width = 600u, const unsigned int height = 600u, unsigned int xPos = 0u, unsigned int yPos = 0u) : SCR_WIDTH(width), SCR_HEIGHT(height), SCR_X_POS(xPos), SCR_Y_POS(yPos), accumulation_frame_index(1), accumulate_frames(true) {
		Initialise(); 

		// Load shaders
		screenQuadShader.LoadShader("Shaders/passthrough.vert", "Shaders/screenQuad.frag");
		rtCompute.LoadShader("Shaders/RTCompute.comp");

		rtCompute.Use();
		const int max_textures = 31;
		for (int i = 0; i <= max_textures; i++) {
			rtCompute.setInt("material_textures[" + std::to_string(i) + "]", i + 6);
		}
		rtCompute.AddNewSSBO(1); // BVH buffer
		rtCompute.AddNewSSBO(2); // Sphere ID buffer
		rtCompute.AddNewSSBO(3); // Quad ID buffer
		rtCompute.AddNewSSBO(4); // Sphere buffer
		rtCompute.AddNewSSBO(5); // Quad buffer

		// Set up screen quad
		std::vector<Vertex> vertices;
		vertices.reserve(4);

		Vertex vertex;
		vertex.Position = glm::vec3(-1.0f, 1.0f, 0.0f);
		vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		vertices.push_back(vertex); // top left

		vertex.Position = glm::vec3(-1.0f, -1.0f, 0.0f);
		vertex.TexCoords = glm::vec2(0.0f, 1.0f);
		vertices.push_back(vertex); // bottom left

		vertex.Position = glm::vec3(1.0f, 1.0f, 0.0f);
		vertex.TexCoords = glm::vec2(1.0f, 0.0f);
		vertices.push_back(vertex); // top right

		vertex.Position = glm::vec3(1.0f, -1.0f, 0.0f);
		vertex.TexCoords = glm::vec2(1.0f, 1.0f);
		vertices.push_back(vertex); // bottom right

		std::vector<unsigned int> indices = {
			0, 1, 3,
			0, 3, 2
		};

		screenQuad.SetupMesh(vertices, indices);

		screenBuffers = Texture2DArray(4);
		screenBuffers.GenerateTexture();
		ResizeWindow(SCR_WIDTH, SCR_HEIGHT);
		//screenTexture.ResizeTexture(SCR_WIDTH, SCR_HEIGHT);

		InputManager::ClearInputs();
	}

	~Renderer() {
		glfwTerminate();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Render(Camera& activeCamera, const Scene& activeScene);

	void ResizeWindow(const unsigned int width, const unsigned int height) {
		unsigned int widthR = width % WORK_GROUP_SIZE;
		unsigned int heightR = height % WORK_GROUP_SIZE;

		SCR_WIDTH = width;
		SCR_HEIGHT = height;
		if (widthR != 0) {
			SCR_WIDTH = width - widthR;
		}
		if (heightR != 0) {
			SCR_HEIGHT = height - heightR;
		}
		screenBuffers.ResizeTexture(SCR_WIDTH, SCR_HEIGHT);
		glfwSetWindowSize(window, SCR_WIDTH, SCR_HEIGHT);
	}

	void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
		ResizeWindow(width, height);
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); }
		if (action == GLFW_RELEASE) {
			InputManager::OnKeyUp(key);
			if (key == GLFW_KEY_C) {
				if (mouseIsFree) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					mouseIsFree = false;
				}
				else {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
					mouseIsFree = true;
				}
			}
		}
		else if (action == GLFW_PRESS) {
			InputManager::OnKeyDown(key);
		}
	}

	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
		scrollOffsetX = xoffset;
		scrollOffsetY = yoffset;
	}

	void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
		mousePos.x = xpos;
		mousePos.y = ypos;
	}

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {}

	void ResetAccumulation() { accumulation_frame_index = 1; }
	void ToggleAccumulation() { accumulate_frames = !accumulate_frames; ResetAccumulation(); }

	GLFWwindow* GetWindow() { return window; }
	const glm::vec2& MousePos() const { return mousePos; }
	const double MouseScrollOffsetX() const { return scrollOffsetX; }
	const double MouseScrollOffsetY() const { return scrollOffsetY; }
	static const bool IsMouseFree() { return mouseIsFree; }
	ComputeShader& GetRTCompute() { return rtCompute; }
private:
	bool Initialise();
	bool InitIMGUI();

	void RenderScene(Camera& activeCamera, const Scene& activeScene);
	void SetupUI();

	Texture2DArray screenBuffers;
	MeshData screenQuad;
	Shader screenQuadShader;
	ComputeShader rtCompute;

	GLFWwindow* window;
	unsigned int SCR_WIDTH, SCR_HEIGHT, SCR_X_POS, SCR_Y_POS, accumulation_frame_index;
	unsigned int viewport_width, viewport_height;
	glm::vec2 mousePos;
	double scrollOffsetX, scrollOffsetY;
	bool accumulate_frames;
	static bool mouseIsFree;
};