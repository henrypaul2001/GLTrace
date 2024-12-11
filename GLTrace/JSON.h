#pragma once
#include "json.hpp"
#include "Scene.h"
#include "EmptyScene.h"

#include "Windows.h"
#include "shobjidl.h"

using namespace nlohmann;
class JSON {
public:
	static bool changeSceneAtEndOfFrame;
	static std::string loadPath;

	// xCENTx answered the following StackOverflow question with this implementation: https://stackoverflow.com/questions/68601080/how-do-you-open-a-file-explorer-dialogue-in-c
	static bool WindowsFileSelect(std::string& sSelectedFile, std::string& sFilePath) {
		// Create file object instance
		HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(f_SysHr)) {
			return false;
		}

		// Create FileOpenDialog object
		IFileOpenDialog* f_FileSystem;
		f_SysHr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
		if (FAILED(f_SysHr)) {
			CoUninitialize();
			return false;
		}

		// Show file dialog window
		f_SysHr = f_FileSystem->Show(NULL);
		if (FAILED(f_SysHr)) {
			f_FileSystem->Release();
			CoUninitialize();
			return false;
		}

		// Retrieve file name from selected item
		IShellItem* f_Files;
		f_SysHr = f_FileSystem->GetResult(&f_Files);
		if (FAILED(f_SysHr)) {
			f_FileSystem->Release();
			CoUninitialize();
			return false;
		}

		// Store and convert file name
		PWSTR f_Path;
		f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
		if (FAILED(f_SysHr)) {
			f_Files->Release();
			f_FileSystem->Release();
			CoUninitialize();
			return false;
		}

		// Format and store file path
		std::wstring path(f_Path);
		std::string c(path.begin(), path.end());
		sFilePath = c;

		// Format string for executable name
		const size_t slash = sFilePath.find_last_of("/\\");
		sSelectedFile = sFilePath.substr(slash + 1);

		// Success, clean up
		CoTaskMemFree(f_Path);
		f_Files->Release();
		f_FileSystem->Release();
		CoUninitialize();
		return true;
	}

	static void WriteSceneToJSON(const char* filepath, const Scene& scene) {
		json j;
		j["scene"] = {
			{"name", scene.GetName()}
		};
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

			std::string scene_name = j.at("scene").at("name").get<std::string>();
			Scene* scene = new EmptyScene(scene_name);
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

			JSONToSpheres(j, scene);
			JSONToQuads(j, scene);

			std::vector<glm::mat4> transformBuffer;
			JSONToTransforms(j, transformBuffer);
			scene->transformBuffer = transformBuffer;

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
		j["material_sets"];
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
			j["spheres"][std::to_string(i)] = {
				{"center", {sphere.Center[0], sphere.Center[1], sphere.Center[2]}},
				{"radius", sphere.Radius},
				{"material_index", sphere.material_index},
				{"transform_ID", sphere.GetTransformID()},
				{"name", sphere_names[i]}
			};
		}
	}
	static void QuadsToJSON(nlohmann::json& j, const std::vector<Quad>& quads, const std::vector<std::string>& quad_names) {
		assert(quads.size() == quad_names.size());
		for (int i = 0; i < quads.size(); i++) {
			const Quad& quad = quads[i];
			j["quads"][std::to_string(i)] = {
				{"quad_type", quad.triangle_disk_id},
				{"Q", {quad.Q[0], quad.Q[1], quad.Q[2]}},
				{"U", {quad.U[0], quad.U[1], quad.U[2]}},
				{"V", {quad.V[0], quad.V[1], quad.V[2]}},
				{"material_index", quad.material_index},
				{"transform_ID", (int)quad.Normal.w},
				{"name", quad_names[i]}
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
	static void JSONToSpheres(const nlohmann::json& j, Scene* scene) {
		auto& jsonSpheres = j.at("spheres");
		const int num_spheres = jsonSpheres.size();
		scene->spheres.clear();
		scene->spheres.reserve(num_spheres);
		for (int i = 0; i < num_spheres; i++) {
			auto& jsonSphere = jsonSpheres.at(std::to_string(i));
			std::vector<float> readCenter = jsonSphere.at("center").get<std::vector<float>>();
			glm::vec3 sphereCenter = glm::vec3(readCenter[0], readCenter[1], readCenter[2]);
			std::string sphereName = jsonSphere.at("name").get<std::string>();
			unsigned int material_index = jsonSphere.at("material_index").get<unsigned int>();
			float sphereRadius = jsonSphere.at("radius").get<float>();
			unsigned int transformID = jsonSphere.at("transform_ID").get<unsigned int>();
			Sphere* new_sphere = scene->AddSphere(sphereName, sphereCenter, sphereRadius, material_index);
			new_sphere->transform_ID = transformID;
		}
	}
	static void JSONToQuads(const nlohmann::json& j, Scene* scene) {
		auto& jsonQuads = j.at("quads");
		const int num_quads = jsonQuads.size();
		scene->quads.clear();
		scene->quads.reserve(num_quads);
		for (int i = 0; i < num_quads; i++) {
			auto& jsonQuad = jsonQuads.at(std::to_string(i));
			
			std::vector<float> readQ = jsonQuad.at("Q").get<std::vector<float>>();
			glm::vec3 Q = glm::vec3(readQ[0], readQ[1], readQ[2]);

			std::vector<float> readU = jsonQuad.at("U").get<std::vector<float>>();
			glm::vec3 U = glm::vec3(readU[0], readU[1], readU[2]);

			std::vector<float> readV = jsonQuad.at("V").get<std::vector<float>>();
			glm::vec3 V = glm::vec3(readV[0], readV[1], readV[2]);

			unsigned int material_index = jsonQuad.at("material_index").get<unsigned int>();
			std::string quad_name = jsonQuad.at("name").get<std::string>();
			unsigned int quad_type = jsonQuad.at("quad_type").get<unsigned int>();
			unsigned int transform_ID = jsonQuad.at("transform_ID").get<unsigned int>();

			Quad* new_quad = nullptr;
			switch (quad_type) {
			case QUAD:
				new_quad = scene->AddQuad(quad_name, Q, U, V, material_index);
				break;
			case TRIANGLE:
				new_quad = scene->AddTriangle(quad_name, Q, U, V, material_index);
				break;
			case DISK:
				new_quad = scene->AddDisk(quad_name, Q, U, V, material_index);
				break;
			}
			if (new_quad) {
				new_quad->Normal.w = transform_ID;
			}
		}
	}
	static void JSONToTransforms(const nlohmann::json& j, std::vector<glm::mat4>& transforms) {
		auto& jsonTransforms = j.at("transformBuffer");
		const int num_transforms = jsonTransforms.size();
		transforms.clear();
		transforms.reserve(num_transforms);
		for (int i = 0; i < num_transforms; i++) {
			auto& jsonTransform = jsonTransforms.at(std::to_string(i));

			std::vector<float> readM0 = jsonTransform.at("m0").get<std::vector<float>>();
			std::vector<float> readM1 = jsonTransform.at("m1").get<std::vector<float>>();
			std::vector<float> readM2 = jsonTransform.at("m2").get<std::vector<float>>();
			std::vector<float> readM3 = jsonTransform.at("m3").get<std::vector<float>>();

			glm::mat4 new_mat4 = glm::mat4(1.0f);
			new_mat4[0][0] = readM0[0];
			new_mat4[0][1] = readM0[1];
			new_mat4[0][2] = readM0[2];
			new_mat4[0][3] = readM0[3];

			new_mat4[1][0] = readM1[0];
			new_mat4[1][1] = readM1[1];
			new_mat4[1][2] = readM1[2];
			new_mat4[1][3] = readM1[3];

			new_mat4[2][0] = readM2[0];
			new_mat4[2][1] = readM2[1];
			new_mat4[2][2] = readM2[2];
			new_mat4[2][3] = readM2[3];

			new_mat4[3][0] = readM3[0];
			new_mat4[3][1] = readM3[1];
			new_mat4[3][2] = readM3[2];
			new_mat4[3][3] = readM3[3];

			transforms.push_back(new_mat4);
		}
	}
};