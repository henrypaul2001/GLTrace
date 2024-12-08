#pragma once
#include "json.hpp"
#include "Scene.h"

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
		nlohmann::json j;
		return nullptr;
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
		j["texture_sets"] = { texture_sets };
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
			j["materials"][material_names[i]] = {
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
};