#include "Renderer.h"
#include "CameraController.h"
#include "TextureLoader.h"
int main()
{
	Renderer renderer = Renderer(1920, 1080);
	Camera cam;
	CameraController camControl;

	camControl.activeCamera = &cam;
	cam.vfov = 90.0;
	cam.lookfrom = glm::vec3(0.0);
	cam.lookat = glm::vec3(0.0, 0.0, -1.0);
	cam.vup = glm::vec3(0.0, 1.0, 0.0);
	cam.samples_per_pixel = 1;
	cam.max_bounces = 100;
	cam.focus_dist = 1.0f;
	cam.defocus_angle = 0.0f;
	cam.sky_colour_min_y = glm::vec3(0.0f);

	GLFWwindow* window = renderer.GetWindow();

	float lastFrame = static_cast<float>(glfwGetTime());
	float currentFrame;
	float dt = 0.0f;

	stbi_set_flip_vertically_on_load(true);
	Texture2D* earth = TextureLoader::LoadTextureFromFile("Textures/earth/albedo.jpg");
	Texture2D* earth_specular = TextureLoader::LoadTextureFromFile("Textures/earth/specular.jpg");
	Texture2D* earth_displacement = TextureLoader::LoadTextureFromFile("Textures/earth/displacement.jpg");

	Texture2D* windowTexture = TextureLoader::LoadTextureFromFile("Textures/window.png");

	//Texture2D* marble_tile = TextureLoader::LoadTextureFromFile("Textures/marbleTile/albedo.png");
	//Texture2D* marble_tile_normal = TextureLoader::LoadTextureFromFile("Textures/marbleTile/normal.png");
	//Texture2D* marble_tile_metal = TextureLoader::LoadTextureFromFile("Textures/marbleTile/metal.png");
	//Texture2D* marble_tile_rough = TextureLoader::LoadTextureFromFile("Textures/marbleTile/roughness.png");
	//marble_tile->BindToSlot(1);
	//marble_tile_normal->BindToSlot(2);
	//marble_tile_metal->BindToSlot(3);
	//marble_tile_rough->BindToSlot(4);

	earth->BindToSlot(2);
	earth_specular->BindToSlot(4);
	earth_displacement->BindToSlot(5);
	windowTexture->BindToSlot(6);

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

		renderer.Render(cam);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}
}