#pragma once
#include "Scene.h"

class BigBenScene : public Scene {
public:
	BigBenScene() {}
	~BigBenScene() {}

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

		FILE* file;
		fopen_s(&file, "Models/bigben.tri", "r");
		float a, b, c, d, e, f, g, h, i;
		for (int t = 0; t < N; t++) {
			fscanf_s(file, "%f %f %f %f %f %f %f %f %f\n", &a, &b, &c, &d, &e, &f, &g, &h, &i);
			const glm::vec3 origin = glm::vec3(a, b, c);
			const glm::vec3 u = glm::vec3(d, e, f) - origin;
			const glm::vec3 v = glm::vec3(g, h, i) - origin;
			//AddTriangle(origin, u, v, 0);
			originalQuads.push_back(Quad(TRIANGLE, origin, u, v, 0));
		}
		fclose(file);

		SetQuadList(originalQuads);

		r = 0.0f;
	}
	void UpdateScene(const float dt) override {
		std::vector<Quad> animatedQuads = originalQuads;
		if ((r += 0.05f) > 2 * pi) { r -= 2 * pi; }
		float a = sinf(r) * 0.5f;
		for (int i = 0; i < N; i++) {
			glm::vec4& originalVertex = originalQuads[i].Q;
			float s = a * (originalVertex.y - 0.2f) * 0.2f;
			float x = originalVertex.x * cosf(s) - originalVertex.y * sinf(s);
			float y = originalVertex.x * sinf(s) + originalVertex.y * cosf(s);
			glm::vec4 newVertex = glm::vec4(x, y, originalVertex.z, 1.0f);
			animatedQuads[i].Q = newVertex;
		}

		SetQuadList(animatedQuads);

		Scene::UpdateScene(dt);
	}

private:
	const int N = 20944;
	float r;
	std::vector<Quad> originalQuads;
};