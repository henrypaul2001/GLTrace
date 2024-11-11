#pragma once
#include "Scene.h"

class StressTestScene : public Scene {
public:
	StressTestScene() {}
	~StressTestScene() {}

	void SetupScene() override {
		// Setup camera
		sceneCamera.vfov = 80.0f;
		sceneCamera.lookfrom = glm::vec3(0.0f, 25.0f, 0.0f);
		sceneCamera.lookat = glm::vec3(0.0f, 0.0f, -1.0f);
		sceneCamera.vup = glm::vec3(0.0f, 1.0f, 0.0f);
		sceneCamera.samples_per_pixel = 2;
		sceneCamera.max_bounces = 10;
		sceneCamera.focus_dist = 1.0f;
		sceneCamera.defocus_angle = 0.0f;
		//sceneCamera.sky_colour_min_y = glm::vec3(0.0f);
		//sceneCamera.sky_colour_max_y = glm::vec3(0.0f);

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

		// Spheres
		glm::vec3 origin = glm::vec3(0.0f, 0.0f, -10.0f);
		float offset = 7.5f;
		for (unsigned int i = 0; i < 20; i++) {
			for (unsigned int j = 0; j < 20; j++) {
				for (unsigned int k = 0; k < 20; k++) {
					glm::vec3 position = origin + glm::vec3(offset * i, offset * j, offset * k);
					AddSphere(position, 3.0f, 0);
				}
			}
		}

		// Quads
	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};