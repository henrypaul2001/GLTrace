#pragma once
#include "Scene.h"

class CornellBoxMirrorScene : public Scene {
public:
	CornellBoxMirrorScene() {}
	~CornellBoxMirrorScene() {}

	void SetupScene() override {
		// Setup camera
		sceneCamera.vfov = 80.0f;
		sceneCamera.lookfrom = glm::vec3(-27.75f, 27.75f, -1.0f);
		sceneCamera.lookat = glm::vec3(0.0f, 0.0f, -1.0f);
		sceneCamera.vup = glm::vec3(0.0f, 1.0f, 0.0f);
		sceneCamera.samples_per_pixel = 2;
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
		light.Albedo = glm::vec3(3.0f, 0.85f, 0.1f);
		light.Roughness = 1.0f;
		light.Metal = 0.0f;
		light.EmissivePower = 2.5f;
		light.EmissiveColour = glm::vec3(3.0f, 0.85f, 0.1f);
		AddMaterial(light); // 3

		Material mirror;
		mirror.Albedo = glm::vec3(1.0f);
		mirror.Roughness = 0.0f;
		mirror.Metal = 1.0f;
		AddMaterial(mirror); // 4

		Material glass;
		glass.Albedo = glm::vec3(1.0f);
		glass.Roughness = 1.0f;
		glass.Metal = 0.0f;
		glass.is_transparent = true;
		glass.refractive_index = 1.5f;
		AddMaterial(glass); // 5

		// Spheres
		AddSphere(glm::vec3(-16.5f, 35.0f, -26.5f), 7.5f, 3);

		glm::vec3 spherePosition = glm::vec3(-27.75f, 4.0f, -11.0f);
		AddSphere(spherePosition, 4.0f, 5);
		AddSphere(spherePosition + glm::vec3(12.5f, 0.0f, 10.0f), 4.0f, 5);
		AddSphere(spherePosition + glm::vec3(-10.0f, 0.0f, 11.0f), 4.0f, 5);

		// Quads
		// Cornell box
		AddQuad(glm::vec3(-55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 2); // left
		AddQuad(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 1); // Right
		AddQuad(glm::vec3(-55.5f, 55.5f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 0); // Top
		AddQuad(glm::vec3(-55.5f, 0.0f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -55.5f), 0); // Bottom
		AddQuad(glm::vec3(-55.5f, 0.0f, -55.5f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), 0); // Back
		//AddQuad(glm::vec3(-55.5f, 0.0f, 0.0f), glm::vec3(0.0f, 55.5f, 0.0f), glm::vec3(55.5f, 0.0f, 0.0f), 0); // Front

		// Geometry
		AddQuad(glm::vec3(-36.5f, 27.5f, -25.0f), glm::vec3(25.0f, 0.0f, 30.0f), glm::vec3(10.0f, -35.0f, 0.0f), 0); // Light blocker
		AddQuad(glm::vec3(-40.0f, 10.0f, -45.0f), glm::vec3(-5.5f, 0.0f, 15.0f) * 2.5f, glm::vec3(10.0f, 40.0f, 0.0f), 4); // mirror
	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};