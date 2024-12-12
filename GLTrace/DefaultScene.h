#pragma once
#include "Scene.h"

class DefaultScene : public Scene {
public:
	DefaultScene() : Scene("New Scene") {}
	~DefaultScene() {}

	void SetupScene() override {
		sceneCamera.lookfrom = glm::vec3(0.0f, 0.0f, -15.0f);
		sceneCamera.lookat = glm::vec3(0.0f, 0.0f, -1.0f);

		Material defaultMat;
		defaultMat.Albedo = glm::vec3(0.75f);
		defaultMat.Metal = 0.0f;
		defaultMat.Roughness = 1.0f;
		AddMaterial("Default", defaultMat);

		AddSphere("New Sphere", glm::vec3(0.0f), 5.0f, 0);
		//AddQuad("New Quad", glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(0.0, 5.0f, 0.0f), 0);
	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};