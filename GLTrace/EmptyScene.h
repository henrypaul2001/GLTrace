#pragma once
#include "Scene.h"
class EmptyScene : public Scene {
public:
	EmptyScene() : Scene("Empty Scene") {}
	~EmptyScene() {}

	void SetupScene() override {

	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};