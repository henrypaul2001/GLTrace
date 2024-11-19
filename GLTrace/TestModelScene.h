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
		sceneCamera.samples_per_pixel = 5;
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

		Material light;
		light.Albedo = glm::vec3(1.0f);
		light.Roughness = 0.0f;
		light.Metal = 0.0f;
		light.EmissiveColour = light.Albedo;
		light.EmissivePower = 10.5f;
		AddMaterial(light); // 1

		Material mirror;
		mirror.Albedo = glm::vec3(1.0f);
		mirror.Roughness = 0.1f;
		mirror.Metal = 1.0f;
		AddMaterial(mirror); // 2

		// Spheres
		AddSphere("Light", glm::vec3(5.5f, 10.0f, -5.5f), 3.25f, 1);

		// Quads
		AddQuad("Floor", glm::vec3(-100.0f, -1.2f, -100.0f), glm::vec3(200.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 200.0f), 2);

		const int N = 12582;
		FILE* file;
		fopen_s(&file, "Models/unity.tri", "r");
		float a, b, c, d, e, f, g, h, i;
		for (int t = 0; t < N; t++) {
			fscanf_s(file, "%f %f %f %f %f %f %f %f %f\n", &a, &b, &c, &d, &e, &f, &g, &h, &i);
			const glm::vec3 origin = glm::vec3(a, b, c);
			const glm::vec3 u = glm::vec3(d, e, f) - origin;
			const glm::vec3 v = glm::vec3(g, h, i) - origin;
			std::string triName = std::string("Triangle") + std::to_string(t);
			AddTriangle(triName.c_str(), origin, u, v, 0);
		}
		fclose(file);
	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};