#pragma once
#include "Scene.h"

class CornellBoxScene : public Scene {
public:
	CornellBoxScene() {}
	~CornellBoxScene() {}

	void SetupScene() override {
		// Setup camera
		sceneCamera.vfov = 80.0f;
		sceneCamera.lookfrom = glm::vec3(-27.75f, 27.75f, -1.0f);
		sceneCamera.lookat = glm::vec3(0.0f, 0.0f, -1.0f);
		sceneCamera.vup = glm::vec3(0.0f, 1.0f, 0.0f);
		sceneCamera.samples_per_pixel = 10;
		sceneCamera.max_bounces = 10;
		sceneCamera.focus_dist = 1.0f;
		sceneCamera.defocus_angle = 0.0f;
		sceneCamera.sky_colour_min_y = glm::vec3(0.0f);
		sceneCamera.sky_colour_max_y = glm::vec3(0.0f);

		// Load material sets

		// Construct scene
		// ---------------
		// Material sets
		
		// Materals
		Material white;
		white.Albedo = glm::vec3(0.73f);
		white.Roughness = 1.0f;
		white.Metal = 0.0f;
		AddMaterial(white); // 0

		Material red;
		red.Albedo = glm::vec3(0.65f, 0.05f, 0.05f);
		red.Roughness = 1.0f;
		red.Metal = 0.0f;
		AddMaterial(red); // 1

		Material green;
		green.Albedo = glm::vec3(0.12f, 0.45f, 0.15f);
		green.Roughness = 1.0f;
		green.Metal = 0.0f;
		AddMaterial(green); // 2

		Material light;
		light.Albedo = glm::vec3(1.0f);
		light.Roughness = 1.0f;
		light.Metal = 0.0f;
		light.EmissivePower = 7.0f;
		light.EmissiveColour = glm::vec3(1.0f);
		AddMaterial(light); // 3

		// Spheres

		// Quads
		AddQuad("Left", glm::vec3(-55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 2); // left
		AddQuad("Right", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 1); // Right
		AddQuad("Light", glm::vec3(-55.5f + 18.5f, 55.4f, -18.5f), glm::vec3(0.0f, 0.0f, -18.5f), glm::vec3(18.5f, 0.0f, 0.0f), 3); // Light
		AddQuad("Top", glm::vec3(-55.5f, 55.5f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 0); // Top
		AddQuad("Bottom", glm::vec3(-55.5f, 0.0f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 0); // Bottom
		AddQuad("Back", glm::vec3(-55.5f, 0.0f, -55.5f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), 0); // Back
		AddQuad("Front", glm::vec3(-55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), 0); // Front

		// Boxes
		AddBox("Tall Box", glm::vec3(-40.0f, 0.0f, -40.0f), glm::vec3(-25.0f, 35.0f, -30.0f), 0);
		AddBox("Small Box", glm::vec3(-15.0f, 0.0f, -15.0f), glm::vec3(-20.0f, 15.0f, -20.0f), 0);
	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};