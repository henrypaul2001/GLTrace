#pragma once
#include "Scene.h"
class EmptyScene : public Scene {
public:
	EmptyScene(const std::string& name = "Empty Scene") : Scene(name) {}
	~EmptyScene() {}

	void SetupScene() override {

	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};