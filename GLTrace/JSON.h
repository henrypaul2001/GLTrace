#pragma once
#include "json.hpp"
#include "Scene.h"
#include "EmptyScene.h"

using namespace nlohmann;
class JSON {
public:
	static void WriteSceneToJSON(const char* filepath, const Scene& scene) {
		json j;
		const Camera& camera = scene.sceneCamera;
		CameraToJSON(j, camera);
		TextureSetsToJSON(j, scene.texture_sets);
		MaterialSetsToJSON(j, scene.material_sets);
		MaterialsToJSON(j, scene.materials, scene.material_names);
		SpheresToJSON(j, scene.spheres, scene.sphere_names);
		QuadsToJSON(j, scene.quads, scene.quad_names);
		TransformBufferToJSON(j, scene.transformBuffer);

		std::ofstream out_file(filepath);
		out_file << j.dump(4);
		out_file.close();
	}

	static Scene* LoadSceneFromJSON(const char* filepath) {
		std::ifstream in_file = std::ifstream(filepath);
		if (in_file.is_open()) {
			json j;
			in_file >> j;

			Scene* scene = new EmptyScene();
			JSONToCamera(j, scene->sceneCamera);

			std::vector<std::pair<std::vector<std::string>, unsigned int>> texture_sets;
			JSONToTextureSets(j, texture_sets);
			for (int i = 0; i < texture_sets.size(); i++) {
				std::vector<const char*> textures;
				textures.reserve(texture_sets[i].first.size());
				for (int texture = 0; texture < texture_sets[i].first.size(); texture++) {
					textures.push_back(texture_sets[i].first[texture].c_str());
				}
				scene->LoadTextureSet(textures, texture_sets[i].second);
			}

			std::vector<MaterialSet> mat_sets;
			JSONToMaterialSets(j, mat_sets);
			for (int i = 0; i < mat_sets.size(); i++) {
				scene->AddMaterialSet(mat_sets[i]);
			}

			std::vector<Material> materials;
			std::vector<std::string> material_names;
			JSONToMaterials(j, materials, material_names);
			assert(materials.size() == material_names.size());
			for (int i = 0; i < materials.size(); i++) {
				scene->AddMaterial(material_names[i], materials[i]);
			}
			return scene;
		}
		else {
			Logger::LogError(std::string("'" + std::string(filepath) + "' could not be opened").c_str());
			return nullptr;
		}
	}

private:
	static void CameraToJSON(nlohmann::json& j, const Camera& camera) {
		const glm::vec3& skyMin = camera.sky_colour_min_y;
		const glm::vec3& skyMax = camera.sky_colour_max_y;
		const glm::vec3& lookfrom = camera.lookfrom;
		const glm::vec3& lookat = camera.lookat;
		const glm::vec3& vup = camera.vup;

		j["camera"] = {
			{"samples_per_pixel", camera.samples_per_pixel},
			{"max_bounces", camera.max_bounces},
			{"sky_colour_min_y", {skyMin[0], skyMin[1], skyMin[2]}},
			{"sky_colour_max_y", {skyMax[0], skyMax[1], skyMax[2]}},
			{"vfov", camera.vfov},
			{"lookfrom", {lookfrom[0], lookfrom[1], lookfrom[2]}},
			{"lookat", {lookat[0], lookat[1], lookat[2]}},
			{"vup", {vup[0], vup[1], vup[2]}},
			{"defocus_angle", camera.defocus_angle},
			{"focus_dist", camera.focus_dist}
		};
	}
	static void TextureSetsToJSON(nlohmann::json& j, const std::vector<std::pair<std::vector<std::string>, unsigned int>>& texture_sets) {
		j["texture_sets"] = json::array();
		for (const auto& texture_set : texture_sets) {
			j["texture_sets"].push_back({
				texture_set.first,
				texture_set.second
				});
		}
	}
	static void MaterialSetsToJSON(nlohmann::json& j, const std::vector<MaterialSet>& material_sets) {
		for (int i = 0; i < material_sets.size(); i++) {
			const MaterialSet& mat = material_sets[i];
			j["material_sets"][std::to_string(i)] = {
				{"albedo_index", mat.albedo_index},
				{"normal_index", mat.normal_index},
				{"roughness_index", mat.roughness_index},
				{"metal_index", mat.metal_index},
				{"emission_index", mat.emission_index},
				{"opacity_index", mat.opacity_index}
			};
		}
	}
	static void MaterialsToJSON(nlohmann::json& j, const std::vector<Material>& materials, const std::vector<std::string>& material_names) {
		assert(materials.size() == material_names.size());
		for (int i = 0; i < materials.size(); i++) {
			const Material& mat = materials[i];
			j["materials"][std::to_string(i)] = {
				{"name", material_names[i]},
				{"albedo", {mat.Albedo[0], mat.Albedo[1], mat.Albedo[2]}},
				{"roughness", mat.Roughness},
				{"metal", mat.Metal},
				{"emissive_colour", {mat.EmissiveColour[0], mat.EmissiveColour[1], mat.EmissiveColour[2]}},
				{"emissive_power", mat.EmissivePower},
				{"is_transparent", mat.is_transparent},
				{"refractive_index", mat.refractive_index},
				{"material_set_index", mat.material_set_index},
				{"is_constant_medium", mat.is_constant_medium},
				{"neg_inv_density", mat.neg_inv_density}
			};
		}
	}
	static void SpheresToJSON(nlohmann::json& j, const std::vector<Sphere>& spheres, const std::vector<std::string>& sphere_names) {
		assert(spheres.size() == sphere_names.size());
		for (int i = 0; i < spheres.size(); i++) {
			const Sphere& sphere = spheres[i];
			j["spheres"][sphere_names[i]] = {
				{"center", {sphere.Center[0], sphere.Center[1], sphere.Center[2]}},
				{"radius", sphere.Radius},
				{"material_index", sphere.material_index},
				{"transform_ID", sphere.GetTransformID()}
			};
		}
	}
	static void QuadsToJSON(nlohmann::json& j, const std::vector<Quad>& quads, const std::vector<std::string>& quad_names) {
		assert(quads.size() == quad_names.size());
		for (int i = 0; i < quads.size(); i++) {
			const Quad& quad = quads[i];
			j["quads"][quad_names[i]] = {
				{"quad_type", quad.triangle_disk_id},
				{"Q", {quad.Q[0], quad.Q[1], quad.Q[2]}},
				{"U", {quad.U[0], quad.U[1], quad.U[2]}},
				{"V", {quad.V[0], quad.V[1], quad.V[2]}},
				{"material_index", quad.material_index},
				{"transform_ID", (int)quad.Normal.w}
			};
		}
	}
	static void TransformBufferToJSON(nlohmann::json& j, const std::vector<glm::mat4>& transform_buffer) {
		for (int i = 0; i < transform_buffer.size(); i++) {
			const glm::mat4& transform = transform_buffer[i];
			j["transformBuffer"][std::to_string(i)] = {
				{"m0", {transform[0][0], transform[0][1], transform[0][2], transform[0][3]}},
				{"m1", {transform[1][0], transform[1][1], transform[1][2], transform[1][3]}},
				{"m2", {transform[2][0], transform[2][1], transform[2][2], transform[2][3]}},
				{"m3", {transform[3][0], transform[3][1], transform[3][2], transform[3][3]}},
			};
		}
	}

	static void JSONToCamera(const nlohmann::json& j, Camera& camera) {
		glm::vec3& skyMin = camera.sky_colour_min_y;
		glm::vec3& skyMax = camera.sky_colour_max_y;
		glm::vec3& lookfrom = camera.lookfrom;
		glm::vec3& lookat = camera.lookat;
		glm::vec3& vup = camera.vup;

		auto jsonCamera = j.at("camera");
		camera.samples_per_pixel = jsonCamera.at("samples_per_pixel").get<int>();
		camera.max_bounces = jsonCamera.at("max_bounces").get<int>();

		std::vector<float> readSkyMin = jsonCamera.at("sky_colour_min_y").get<std::vector<float>>();
		skyMin = glm::vec3(readSkyMin[0], readSkyMin[1], readSkyMin[2]);
		std::vector<float> readSkyMax = jsonCamera.at("sky_colour_max_y").get<std::vector<float>>();
		skyMax = glm::vec3(readSkyMax[0], readSkyMax[1], readSkyMax[2]);

		camera.vfov = jsonCamera.at("vfov").get<float>();
		
		std::vector<float> readLookFrom = jsonCamera.at("lookfrom").get<std::vector<float>>();
		lookfrom = glm::vec3(readLookFrom[0], readLookFrom[1], readLookFrom[2]);
		std::vector<float> readLookAt = jsonCamera.at("lookat").get<std::vector<float>>();
		lookat = glm::vec3(readLookAt[0], readLookAt[1], readLookAt[2]);
		std::vector<float> readVup = jsonCamera.at("vup").get<std::vector<float>>();
		vup = glm::vec3(readVup[0], readVup[1], readVup[2]);

		camera.defocus_angle = jsonCamera.at("defocus_angle").get<float>();
		camera.focus_dist = jsonCamera.at("focus_dist").get<float>();
	}
	static void JSONToTextureSets(const nlohmann::json& j, std::vector<std::pair<std::vector<std::string>, unsigned int>>& texture_sets) {
		auto jsonTextureSets = j.at("texture_sets");
		for (const auto& entry : jsonTextureSets) {
			std::vector<std::string> textures = entry.at(0).get<std::vector<std::string>>();
			unsigned int bindSlot = entry.at(1).get<unsigned int>();
			texture_sets.emplace_back(textures, bindSlot);
		}
	}
	static void JSONToMaterialSets(const nlohmann::json& j, std::vector<MaterialSet>& material_sets) {
		auto jsonMatSets = j.at("material_sets");
		const int numSets = jsonMatSets.size();
		material_sets.reserve(numSets);
		for (int i = 0; i < numSets; i++) {
			auto jsonMat = jsonMatSets.at(std::to_string(i));
			MaterialSet newSet;
			newSet.albedo_index = jsonMat.at("albedo_index").get<int>();
			newSet.normal_index = jsonMat.at("normal_index").get<int>();
			newSet.roughness_index = jsonMat.at("roughness_index").get<int>();
			newSet.metal_index = jsonMat.at("metal_index").get<int>();
			newSet.emission_index = jsonMat.at("emission_index").get<int>();
			newSet.opacity_index = jsonMat.at("opacity_index").get<int>();
			material_sets.push_back(newSet);
		}
	}
	static void JSONToMaterials(const nlohmann::json& j, std::vector<Material>& materials, std::vector<std::string>& material_names) {
		auto jsonMaterials = j.at("materials");
		const int num_materials = jsonMaterials.size();
		materials.reserve(num_materials);
		material_names.reserve(num_materials);
		for (int i = 0; i < num_materials; i++) {
			auto jsonMat = jsonMaterials.at(std::to_string(i));
			Material newMat;

			std::vector<float> readAlbedo = jsonMat.at("albedo").get<std::vector<float>>();
			newMat.Albedo = glm::vec3(readAlbedo[0], readAlbedo[1], readAlbedo[2]);

			newMat.Roughness = jsonMat.at("roughness").get<float>();
			newMat.Metal = jsonMat.at("metal").get<float>();
			
			std::vector<float> readEmissiveColour = jsonMat.at("emissive_colour").get<std::vector<float>>();
			newMat.EmissiveColour = glm::vec3(readEmissiveColour[0], readEmissiveColour[1], readEmissiveColour[2]);

			newMat.EmissivePower = jsonMat.at("emissive_power").get<float>();
			newMat.is_transparent = jsonMat.at("is_transparent").get<bool>();
			newMat.refractive_index = jsonMat.at("refractive_index").get<float>();
			newMat.material_set_index = jsonMat.at("material_set_index").get<int>();
			newMat.is_constant_medium = jsonMat.at("is_constant_medium").get<bool>();
			newMat.neg_inv_density = jsonMat.at("neg_inv_density").get<float>();

			materials.push_back(newMat);
			material_names.push_back(jsonMat.at("name").get<std::string>());
		}
	}
};