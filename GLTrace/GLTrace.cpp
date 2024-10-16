#include "Renderer.h"
int main()
{
	Renderer renderer;
	GLFWwindow* window = renderer.GetWindow();

	while (!glfwWindowShouldClose(window))
	{
		renderer.Render();
	}
}