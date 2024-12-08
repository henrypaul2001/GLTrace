#pragma once
#include "json.hpp"
#include "Scene.h"

class JSON {
public:
	static Scene* LoadSceneFromJSON(const char* filepath) {
		nlohmann::json j;
		return nullptr;
	}
};