#include "Renderer.h"
int main()
{
	Renderer renderer;
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

		// Process inputs
		//ProcessInputs();

		// Update scene
		//OnUpdateFrame();

		renderer.Render();
	}
}