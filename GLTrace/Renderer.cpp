#include "Renderer.h"
bool Renderer::mouseIsFree = false;
void Renderer::Render(Camera& activeCamera, const Scene& activeScene)
{
	glClear(GL_COLOR_BUFFER_BIT);
	SetupUI();
	RenderScene(activeCamera, activeScene);

	// Render UI
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(window);
	}

	scrollOffsetX = 0.0;
	scrollOffsetY = 0.0;
}

void Renderer::RenderScene(Camera& activeCamera, const Scene& activeScene)
{
	glViewport(SCR_X_POS, SCR_Y_POS, SCR_WIDTH, SCR_HEIGHT);

	if (SCR_WIDTH > 0 && SCR_HEIGHT > 0) {
		// Update camera
		if (activeCamera.HasCameraMoved()) {
			ResetAccumulation();
			activeCamera.SetCameraHasMoved(false);
		}
		activeCamera.Initialise(SCR_WIDTH, SCR_HEIGHT);
		activeCamera.SetUniforms(rtCompute);
		rtCompute.setInt("accumulation_frame_index", accumulation_frame_index);

		// Update scene information
		activeScene.SetUniforms(rtCompute);

		// Dispatch RT compute shader
		screenBuffers.BindImage(GL_WRITE_ONLY, 0);
		rtCompute.DispatchCompute(SCR_WIDTH / WORK_GROUP_SIZE, SCR_HEIGHT / WORK_GROUP_SIZE, 1, GL_ALL_BARRIER_BITS);

		// Render screen quad
		//screenQuadShader.Use();
		//screenBuffers.BindToSlot(0);
		//screenQuad.DrawMeshData();

		if (accumulate_frames) { accumulation_frame_index++; }
	}
}

void Renderer::SetupUI()
{
	// ImGui frame start
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Menu
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Close")) {
				glfwSetWindowShouldClose(window, true);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// Create docking environment
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("InvisibleWindow", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	ImGuiID dockSpaceID = ImGui::GetID("InvisibleWindowDockSpace");

	ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	// Scene view
	ImGui::Begin("Viewport");
	viewport_width = ImGui::GetContentRegionAvail().x;
	viewport_height = ImGui::GetContentRegionAvail().y;
	//ImGui::Image((ImTextureID)(intptr_t)screenBuffers.ID(), ImVec2(viewport_width, viewport_height));
	ImGui::End();

	if (viewport_width != SCR_WIDTH || viewport_height != SCR_HEIGHT) {
		unsigned int width = viewport_width;
		unsigned int height = viewport_height;
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
	}
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
	glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

	if (InitIMGUI()) { std::cout << "ImGui initialised" << std::endl; }
	else { std::cout << "Failed to initialise ImGui" << std::endl; }
	return true;
}

bool Renderer::InitIMGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		// Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;		// Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;			// Enable docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;			// Enable multi-viewport

	// Setup platform backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return true;
}
