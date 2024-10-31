#include "Renderer.h"
#include "CameraController.h"
#include "TestScene.h"
int main()
{
	Renderer renderer = Renderer(1920, 1080);
	CameraController camControl;
	Scene* scene = new TestScene();
	scene->SetupScene();

	camControl.activeCamera = scene->GetSceneCamera();

	//cam.lookfrom = glm::vec3(278.0f, 278.0f, -800.0f);
	//cam.lookat = glm::vec3(0.0f, 0.0f, -1.0f);

	//cam.sky_colour_min_y = glm::vec3(0.2f, 0.05f, 0.05f);
	//cam.sky_colour_max_y = glm::vec3(0.8f, 0.5f, 0.25f);
	//cam.sky_colour_min_y = glm::vec3(0.0f);
	//cam.sky_colour_max_y = glm::vec3(0.0f);

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

		//cam.lookfrom = cam.lookfrom + (glm::vec3(0.05f, 0.0f, 0.0f) * dt);
		//cam.lookat = cam.lookfrom + glm::vec3(0.0f, 0.0f, -1.0f);

		// Process inputs
		const glm::vec2& mousePos = renderer.MousePos();
		camControl.ProcessMouseMovement(mousePos.x, mousePos.y);
		camControl.ProcessMouseScroll(renderer.MouseScrollOffsetY());

		// Update scene
		camControl.Update(dt);
		scene->UpdateScene(dt);

		renderer.Render(*scene->GetSceneCamera(), *scene);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	delete scene;
}