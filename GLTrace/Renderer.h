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

#include "Logging.h"

static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
	// ignore warning codes or insignificant errors
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::string stringMessage = "---------------\n";
	stringMessage += std::string("Debug message (" + std::to_string(id) + "): " + message + "\r\n");

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		stringMessage += "source: API\r\n"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		stringMessage += "Source: Window System\r\n"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		stringMessage += "Source: Shader Compiler\r\n"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		stringMessage += "Source: Third Party\r\n"; break;
	case GL_DEBUG_SOURCE_APPLICATION:
		stringMessage += "Source: Application\r\n"; break;
	case GL_DEBUG_SOURCE_OTHER:
		stringMessage += "Source: Other\r\n"; break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		stringMessage += "Type: Error\r\n"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		stringMessage += "Type: Deprecated Behaviour\r\n"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		stringMessage += "Type: Undefined Behaviour\r\n"; break;
	case GL_DEBUG_TYPE_PORTABILITY:
		stringMessage += "Type: Portability\r\n"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		stringMessage += "Type: Performance\r\n"; break;
	case GL_DEBUG_TYPE_MARKER:
		stringMessage += "Type: Marker\r\n"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		stringMessage += "Type: Push Group\r\n"; break;
	case GL_DEBUG_TYPE_POP_GROUP:
		stringMessage += "Type: Pop Group\r\n"; break;
	case GL_DEBUG_TYPE_OTHER:
		stringMessage += "Type: Other\r\n"; break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		stringMessage += "Severity: high\r\n"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		stringMessage += "Severity: medium\r\n"; break;
	case GL_DEBUG_SEVERITY_LOW:
		stringMessage += "Severity: low\r\n"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		stringMessage += "Severity: notification\r\n"; break;
	}

	Logger::LogError(stringMessage.c_str());
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
		finalImage = Texture2D();
		finalImage.GenerateTexture();
		ResizeWindow(SCR_WIDTH, SCR_HEIGHT);
		//screenTexture.ResizeTexture(SCR_WIDTH, SCR_HEIGHT);

		// Set up framebuffer
		finalImageFBO = 0;
		glGenFramebuffers(1, &finalImageFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, finalImageFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finalImage.ID(), 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		InputManager::ClearInputs();
	}

	~Renderer() {
		glfwTerminate();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Render(Camera& activeCamera, const Scene& activeScene, const float dt);

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
		finalImage.ResizeTexture(SCR_WIDTH, SCR_HEIGHT);
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
	void SetupUI(const float dt);

	Texture2DArray screenBuffers;
	Texture2D finalImage;
	MeshData screenQuad;
	Shader screenQuadShader;
	ComputeShader rtCompute;

	GLFWwindow* window;
	unsigned int SCR_WIDTH, SCR_HEIGHT, SCR_X_POS, SCR_Y_POS, accumulation_frame_index;
	unsigned int viewport_width, viewport_height;
	unsigned int finalImageFBO;
	glm::vec2 mousePos;
	double scrollOffsetX, scrollOffsetY;
	bool accumulate_frames;
	static bool mouseIsFree;
};