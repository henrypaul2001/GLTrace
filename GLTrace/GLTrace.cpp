#include "Renderer.h"
#include "CameraController.h"
#include "TestScene.h"
#include "CornellBox.h"
int main()
{
	Renderer renderer = Renderer(1920, 1080);
	CameraController camControl;
	Scene* scene = new TestScene();
	scene->SetupScene();

	camControl.activeCamera = scene->GetSceneCamera();
	camControl.Initialise();

	GLFWwindow* window = renderer.GetWindow();

	float lastFrame = static_cast<float>(glfwGetTime());
	float currentFrame;
	float dt = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		currentFrame = static_cast<float>(glfwGetTime()) + 0.0001f;
		dt = currentFrame - lastFrame;
		lastFrame = currentFrame;

		std::clog << "Delta time: " << dt << " || FPS: " << 1.0 / dt << "										\r" << std::flush;

		// Process inputs
		const glm::vec2& mousePos = renderer.MousePos();
		camControl.ProcessMouseMovement(mousePos.x, mousePos.y);
		camControl.ProcessMouseScroll(renderer.MouseScrollOffsetY());

		if (InputManager::IsKeyDown(GLFW_KEY_R)) { renderer.ToggleAccumulation(); }

		// Update scene
		camControl.Update(dt);
		scene->UpdateScene(dt);

		renderer.Render(*scene->GetSceneCamera(), *scene);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	delete scene;
}