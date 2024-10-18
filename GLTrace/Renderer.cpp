#include "Renderer.h"
void Renderer::Render()
{
	glViewport(SCR_X_POS, SCR_Y_POS, SCR_WIDTH, SCR_HEIGHT);

	if (SCR_WIDTH > 0 && SCR_HEIGHT > 0) {
		// Dispatch RT compute shader
		screenTexture.BindImage(GL_WRITE_ONLY);
		rtCompute.DispatchCompute(SCR_WIDTH, SCR_HEIGHT, 1, GL_ALL_BARRIER_BITS);

		// Render screen quad
		screenQuadShader.Use();
		screenTexture.Bind();
		screenQuad.DrawMeshData();
	}

	glfwSwapBuffers(window);

	glfwPollEvents();
}

bool Renderer::Initialise()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw: create window
	// -------------------
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GLTrace", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// Allow a C++ member function to be used by the C, OpenGl api callback for input callbacks
	glfwSetWindowUserPointer(glfwGetCurrentContext(), this);
	auto key = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->key_callback(window, key, scancode, action, mods);
	};

	auto scroll = [](GLFWwindow* window, double xoffset, double yoffset) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->scroll_callback(window, xoffset, yoffset);
	};

	auto mouse = [](GLFWwindow* window, double xpos, double ypos) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->mouse_callback(window, xpos, ypos);
	};

	auto mouse_button = [](GLFWwindow* window, int button, int action, int mods) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->mouse_button_callback(window, button, action, mods);
	};

	auto frame_size = [](GLFWwindow* window, int width, int height) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->framebuffer_size_callback(window, width, height);
	};

	glfwSetKeyCallback(window, key);
	glfwSetScrollCallback(window, scroll);
	glfwSetCursorPosCallback(window, mouse);
	glfwSetMouseButtonCallback(window, mouse_button);
	glfwSetFramebufferSizeCallback(window, frame_size);
	//glfwSwapInterval(0); // disables v sync

	// glad: load OpenGL function pointers
	// -----------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return false;
	}

	// Set up GL Debug output
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		//glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_MEDIUM, -1, "error message here"); // example of custom error message
	}

	ComputeShader::InitOpenGLConstants();

	std::cout << "OpenGL initialised" << std::endl;

	return true;
}