#include "Renderer.h"

int main()
{
	Renderer renderer;
	Camera cam;
	cam.vfov = 90.0;
	cam.lookfrom = glm::vec3(0.0);
	cam.lookat = glm::vec3(0.0, 0.0, -1.0);
	cam.vup = glm::vec3(0.0, 1.0, 0.0);
	cam.samples_per_pixel = 10;

	GLFWwindow* window = renderer.GetWindow();

	float lastFrame = static_cast<float>(glfwGetTime());
	float currentFrame;
	float dt = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		currentFrame = static_cast<float>(glfwGetTime()) + 0.0001f;
		dt = currentFrame - lastFrame;
		lastFrame = currentFrame;

		std::clog << "Delta time: " << dt << "										\r" << std::flush;

		//cam.lookfrom = cam.lookfrom + (glm::vec3(1.0f, 0.0f, 0.0f) * dt);
		//cam.lookat = cam.lookfrom + glm::vec3(0.0f, 0.0f, -1.0f);

		// Process inputs
		//ProcessInputs();

		// Update scene
		//OnUpdateFrame();

		renderer.Render(cam);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}
}