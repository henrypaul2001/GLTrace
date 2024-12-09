#pragma once
#include "Scene.h"
class EmptyScene : public Scene {
public:
	EmptyScene() {}
	~EmptyScene() {}

	void SetupScene() override {

	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};