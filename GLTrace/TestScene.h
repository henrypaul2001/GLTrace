#pragma once
#include "Scene.h"
class TestScene : public Scene {
public:
	TestScene() {}
	~TestScene() {}

	void SetupScene() override {
		// Setup camera
		sceneCamera.vfov = 90.0;
		sceneCamera.lookfrom = glm::vec3(0.0);
		sceneCamera.lookat = glm::vec3(0.0, 0.0, -1.0);
		sceneCamera.vup = glm::vec3(0.0, 1.0, 0.0);
		sceneCamera.samples_per_pixel = 5;
		sceneCamera.max_bounces = 10;
		sceneCamera.focus_dist = 1.0f;
		sceneCamera.defocus_angle = 0.0f;
		sceneCamera.sky_colour_min_y = glm::vec3(0.0f);
		sceneCamera.sky_colour_max_y = glm::vec3(0.0f);

		//sceneCamera.sky_colour_min_y = glm::vec3(0.2f, 0.05f, 0.05f);
		//sceneCamera.sky_colour_max_y = glm::vec3(0.8f, 0.5f, 0.25f);

		// Load material sets
		stbi_set_flip_vertically_on_load(true);
		Texture2DArray* earth_maps = TextureLoader::LoadTextureArrayFromFile({ "Textures/earth/albedo.jpg", "Textures/earth/specular.jpg", "Textures/earth/displacement.jpg" });
		earth_maps->BindToSlot(7);

		//Texture2D* windowTexture = TextureLoader::LoadTextureFromFile("Textures/window.png");

		//Texture2D* marble_tile = TextureLoader::LoadTextureFromFile("Textures/marbleTile/albedo.png");
		//Texture2D* marble_tile_normal = TextureLoader::LoadTextureFromFile("Textures/marbleTile/normal.png");
		//Texture2D* marble_tile_metal = TextureLoader::LoadTextureFromFile("Textures/marbleTile/metal.png");
		//Texture2D* marble_tile_rough = TextureLoader::LoadTextureFromFile("Textures/marbleTile/roughness.png");
		//marble_tile->BindToSlot(1);
		//marble_tile_normal->BindToSlot(2);
		//marble_tile_metal->BindToSlot(3);
		//marble_tile_rough->BindToSlot(4);

		Texture2DArray* space_blanket_maps = TextureLoader::LoadTextureArrayFromFile({ "Textures/space_blanket/albedo.png", "Textures/space_blanket/metallic.png", "Textures/space_blanket/normal.png", "Textures/space_blanket/roughness.png" });
		space_blanket_maps->BindToSlot(8);

		//windowTexture->BindToSlot(6);

		// Construct scene
		// ---------------
		// Material sets
		MaterialSet earth_set;
		earth_set.albedo_index = 0;
		earth_set.normal_index = -1;
		earth_set.metal_index = 1;
		earth_set.roughness_index = 2;
		earth_set.emission_index = 0;
		AddMaterialSet(earth_set); // 0

		MaterialSet space_blanket_set;
		space_blanket_set.albedo_index = 0;
		space_blanket_set.normal_index = 2;
		space_blanket_set.metal_index = 1;
		space_blanket_set.roughness_index = 3;
		space_blanket_set.emission_index = -1;
		AddMaterialSet(space_blanket_set); // 1

		// Materals
		Material sun;
		sun.Albedo = glm::vec3(0.5f, 0.0f, 0.3f);
		sun.Albedo = glm::vec3(244.0f / 255.0f, 128.0f / 255.0f, 55.0f / 255.0f);
		sun.Metal = 0.0f;
		sun.Roughness = 1.0f;
		sun.EmissiveColour = glm::vec3(4.0f);
		sun.EmissiveColour = sun.Albedo;
		sun.EmissivePower = 1.0f;
		sun.is_transparent = false;
		sun.refractive_index = 1.5f;
		AddMaterial("Sun", sun); // 0

		Material ground;
		ground.Albedo = glm::vec3(1.0f);
		ground.Metal = 0.1f;
		ground.Roughness = 1.0f;
		ground.EmissiveColour = glm::vec3(0.0f);
		ground.EmissivePower = 0.0f;
		ground.is_transparent = false;
		ground.refractive_index = 1.5f;
		AddMaterial("Ground", ground); // 1

		Material emissive_earth;
		emissive_earth.Albedo = glm::vec3(1.0f);
		emissive_earth.Metal = 0.0f;
		emissive_earth.Roughness = 0.0f;
		emissive_earth.EmissiveColour = glm::vec3(0.0f);
		emissive_earth.EmissivePower = 1.0f;
		emissive_earth.is_transparent = false;
		emissive_earth.refractive_index = 1.5f;
		emissive_earth.material_set_index = 0;
		AddMaterial("Emissive Earth", emissive_earth); // 2

		Material space_blanket;
		space_blanket.Albedo = glm::vec3(1.0f);
		space_blanket.Metal = 0.0f;
		space_blanket.Roughness = 0.0f;
		space_blanket.EmissiveColour = glm::vec3(0.0f);
		space_blanket.EmissivePower = 1.0f;
		space_blanket.is_transparent = false;
		space_blanket.refractive_index = 1.5f;
		space_blanket.material_set_index = 1;
		AddMaterial("Space Blanket", space_blanket); // 3

		Material glass;
		glass.Albedo = glm::vec3(1.0f, 0.0f, 0.0f);
		glass.Metal = 1.0f;
		glass.Roughness = 0.0f;
		glass.EmissiveColour = glm::vec3(0.0f);
		glass.EmissivePower = 0.0f;
		glass.is_transparent = true;
		glass.refractive_index = 1.5f;
		AddMaterial("Glass", glass); // 4

		Material bubble;
		bubble.Albedo = glm::vec3(1.0f);
		bubble.Metal = 1.0f;
		bubble.Roughness = 0.0f;
		bubble.EmissiveColour = glm::vec3(0.0f);
		bubble.EmissivePower = 0.0f;
		bubble.is_transparent = true;
		bubble.refractive_index = 1.0f / 1.5f;
		AddMaterial("Glass Bubble", bubble); // 5

		Material volume = Material(0.5, glm::vec3(0.15f));
		AddMaterial("Volume", volume); // 6

		// Spheres
		AddSphere("Sun", glm::vec3(150.0f, 30.0f, -55.0f), 50.0f, 0);
		AddSphere("Ground", glm::vec3(0.0f, -100.5f, -1.0f), 100.0f, 1);
		AddSphere("Smoke", glm::vec3(0.0f, 1.5f, 0.0f), 2.0f, 6);
		AddSphere("Glass Ball 1", glm::vec3(6.0f, 1.5f, 0.0f), 2.0f, 4);
		AddSphere("Glass Ball 2", glm::vec3(6.0f, 1.5f, 0.0f), 1.9f, 5);

		// Quads
		AddQuad("Test Quad", glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(2.5f, 0.0f, 0.0f), glm::vec3(0.0f, 2.5f, 0.0f), 3);
	
		// Test model load
		bool success = LoadModelAsTriangles("Models/rock/rock.obj");
	}

	void UpdateScene(const float dt) override {
		Scene::UpdateScene(dt);
	}
};