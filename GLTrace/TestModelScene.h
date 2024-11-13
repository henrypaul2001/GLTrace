#pragma once
#include "Scene.h"

class TestModelScene : public Scene {
public:
	TestModelScene() {}
	~TestModelScene() {}

	void SetupScene() override {
		// Setup camera
		sceneCamera.vfov = 80.0f;
		sceneCamera.lookfrom = glm::vec3(0.0f, 0.0f, 0.0f);
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

		// Quads
		const int N = 12582;
		FILE* file;
		fopen_s(&file, "Models/unity.tri", "r");
		float a, b, c, d, e, f, g, h, i;
		for (int t = 0; t < N; t++) {
			fscanf_s(file, "%f %f %f %f %f %f %f %f %f\n", &a, &b, &c, &d, &e, &f, &g, &h, &i);
			const glm::vec3 origin = glm::vec3(a, b, c);
			const glm::vec3 u = glm::vec3(d, e, f) - origin;
			const glm::vec3 v = glm::vec3(g, h, i) - origin;
			AddTriangle(origin, u, v, 0);
		}
		fclose(file);
	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};