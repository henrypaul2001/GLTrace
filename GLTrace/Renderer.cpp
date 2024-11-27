#include "Renderer.h"
bool Renderer::mouseIsFree = false;
void Renderer::Render(Camera& activeCamera, Scene& activeScene, const float dt)
{
	glClear(GL_COLOR_BUFFER_BIT);
	SetupUI(activeCamera, activeScene, dt);
	RenderScene(activeCamera, activeScene);

	// Render UI
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(window);
	}

	scrollOffsetX = 0.0;
	scrollOffsetY = 0.0;
}

void Renderer::RenderScene(Camera& activeCamera, const Scene& activeScene)
{
	glViewport(SCR_X_POS, SCR_Y_POS, SCR_WIDTH, SCR_HEIGHT);

	if (SCR_WIDTH > 0 && SCR_HEIGHT > 0) {
		// Update camera
		if (activeCamera.HasCameraMoved() && auto_reset_accumulation) {
			ResetAccumulation();
			activeCamera.SetCameraHasMoved(false);
		}
		activeCamera.Initialise(SCR_WIDTH, SCR_HEIGHT);
		activeCamera.SetUniforms(rtCompute);
		rtCompute.setInt("accumulation_frame_index", accumulation_frame_index);

		// Update scene information
		activeScene.SetUniforms(rtCompute);

		// Dispatch RT compute shader
		screenBuffers.BindImage(GL_WRITE_ONLY, 0);
		rtCompute.DispatchCompute(SCR_WIDTH / WORK_GROUP_SIZE, SCR_HEIGHT / WORK_GROUP_SIZE, 1, GL_ALL_BARRIER_BITS);

		// Render screen quad
		glBindFramebuffer(GL_FRAMEBUFFER, finalImageFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		screenQuadShader.Use();
		screenBuffers.BindToSlot(0);
		screenQuad.DrawMeshData();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (accumulate_frames) { accumulation_frame_index++; }
	}
}

void Renderer::SetupUI(Camera& activeCamera, Scene& activeScene, const float dt)
{
	// ImGui frame start
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// - Menu -
	// --------
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Close")) {
				glfwSetWindowShouldClose(window, true);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// Create docking environment
	// --------------------------
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("InvisibleWindow", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	ImGuiID dockSpaceID = ImGui::GetID("InvisibleWindowDockSpace");

	ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	// Scene view
	// ----------
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse);
	viewport_width = ImGui::GetContentRegionAvail().x;
	viewport_height = ImGui::GetContentRegionAvail().y;
	ImGui::Image((ImTextureID)(intptr_t)finalImage.ID(), ImVec2(viewport_width, viewport_height), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
	ImGui::End();

	// - Log -
	// -------
	ImGui::Begin("Log");
	ImVec2 logDimensions = ImGui::GetContentRegionAvail();
	std::string dt_string = "Delta time: " + std::to_string(dt);
	std::string fps_string = "FPS: " + std::to_string(1.0 / dt);
	ImGui::Text(dt_string.c_str());
	ImGui::Text(fps_string.c_str());
	ImGui::Separator();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
		if (ImGui::BeginPopupContextWindow()) {
			if (ImGui::Selectable("Clear")) {
				// Clear log
				Logger::ClearLog();
			}
			ImGui::EndPopup();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
		const std::vector<LogEntry>& entries = Logger::GetEntries();
		for (const LogEntry& entry : entries) {
			const glm::vec4& typeCol = entry.GetTypeColour();
			const glm::vec4& logCol = entry.GetLogColour();
			ImVec4 typeColour = ImVec4(typeCol.r, typeCol.g, typeCol.b, typeCol.a);
			ImVec4 logColour = ImVec4(logCol.r, logCol.g, logCol.b, logCol.a);

			ImGui::PushStyleColor(ImGuiCol_Text, typeColour);
			ImGui::Text(entry.GetTypeName());
			ImGui::PopStyleColor(1);

			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Text, logColour);
			ImGui::Text(entry.GetLog());
			ImGui::PopStyleColor(1);
		}
		
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::PopStyleVar(1);
	}
	ImGui::EndChild();
	ImGui::End();

	static int selected = 0;
	static int selected_material = 0;
	static int selected_quad_type = 0;
	static int selected_edit_material = 0;
	static int selected_material_set = 0;
	const int num_spheres = activeScene.GetSpheres().size();
	const int num_quads = activeScene.GetQuads().size();
	const int num_materials = activeScene.GetMaterials().size();
	const int num_material_sets = activeScene.GetMaterialSets().size();

	// Scene details
	// -------------
	ImGui::Begin("Scene");
	if (ImGui::Button("Add Sphere")) {
		ImGui::OpenPopup("Create Sphere");
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Quad")) {
		ImGui::OpenPopup("Create Quad");
	}

	// Create sphere menu
	// ------------------
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Create Sphere", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::SeparatorText("Sphere properties");

		static char sphereName;
		static glm::vec3 spherePos = glm::vec3(0.0f);
		static float radius = 5.0f;
		static unsigned int material_index = 0;

		ImGui::Spacing();
		ImGui::InputText("Sphere name", &sphereName, sizeof(char) * 100);
		ImGui::DragFloat3("Position", &spherePos[0]);
		ImGui::DragFloat("Radius", &radius);
		
		ImGui::Spacing();
		if (ImGui::BeginCombo("Material", activeScene.GetMaterialName(material_index).c_str())) {
			for (int i = 0; i < num_materials; i++) {
				if (ImGui::Selectable(activeScene.GetMaterialName(i).c_str(), material_index == i)) {
					material_index = i;
				}

				if (material_index == i) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Confirm")) {
			Sphere* new_sphere = activeScene.AddSphere(std::string(&sphereName), spherePos, radius, material_index);
			if (new_sphere != nullptr) {
				ResetAccumulation();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Create quad menu
	// ----------------
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Create Quad", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::SeparatorText("Quad properties");

		static char quadName;
		static glm::vec3 q = glm::vec3(0.0f);
		static glm::vec3 u = glm::vec3(1.0f, 0.0f, 0.0f);
		static glm::vec3 v = glm::vec3(0.0f, 1.0f, 0.0f);
		static unsigned int material_index = 0;
		static unsigned int quad_type = 0;

		ImGui::Spacing();
		ImGui::InputText("Quad name", &quadName, sizeof(char) * 100);
		ImGui::DragFloat3("Origin", &q[0]);
		ImGui::DragFloat3("U extent", &u[0]);
		ImGui::DragFloat3("V extent", &v[0]);

		ImGui::Spacing();
		const char* quad_types[3] = { "Quad", "Triangle", "Disk" };
		if (ImGui::BeginCombo("Type", quad_types[quad_type])) {
			for (unsigned int i = 0; i < 3; i++) {
				if (ImGui::Selectable(quad_types[i], quad_type == i)) {
					quad_type = i;
				}

				if (quad_type == i) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Spacing();
		if (ImGui::BeginCombo("Material", activeScene.GetMaterialName(material_index).c_str())) {
			for (int i = 0; i < num_materials; i++) {
				if (ImGui::Selectable(activeScene.GetMaterialName(i).c_str(), material_index == i)) {
					material_index = i;
				}

				if (material_index == i) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Confirm")) {
			Quad* new_quad = nullptr;
			switch (quad_type) {
			case 0:
				new_quad = activeScene.AddQuad(std::string(&quadName), q, u, v, material_index);
				break;
			case 1:
				new_quad = activeScene.AddTriangle(std::string(&quadName), q, u, v, material_index);
				break;
			case 2:
				new_quad = activeScene.AddDisk(std::string(&quadName), q, u, v, material_index);
				break;
			}
			if (new_quad != nullptr) {
				ResetAccumulation();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SeparatorText("SceneName");
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 8));

		if (ImGui::Selectable("Camera", selected == 0)) {
			selected = 0;
		}

		// Spheres
		// -------
		if (num_spheres > 0) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Sphere Primitives")) {

				ImGuiListClipper clipper;
				clipper.Begin(num_spheres);
				while (clipper.Step()) {
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						if (ImGui::Selectable(activeScene.GetSphereName(i).c_str(), selected == i + 1)) {
							selected = i + 1;
						}
					}
				}

				ImGui::Separator();
				ImGui::TreePop();
			}
		}

		// Quads
		// -----
		if (num_quads > 0) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Quad Primitives")) {

				ImGuiListClipper clipper;
				clipper.Begin(num_quads);
				while (clipper.Step()) {
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						if (ImGui::Selectable(activeScene.GetQuadName(i).c_str(), selected == i + num_spheres + 1)) {
							selected = i + num_spheres + 1;
						}
					}
				}
				ImGui::Separator();
				ImGui::TreePop();
			}
		}

		ImGui::PopStyleVar(1);
	}
	ImGui::EndChild();
	ImGui::End();

	// Properties
	// ----------
	ImGui::Begin("Properties");
	
	if (selected == 0) {
		// Camera
		// ------
		ImGui::Text("Active Camera");
		ImGui::Separator();
		if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Physical Properties")) {

				ImGui::Text("Space");
				glm::vec3 position = activeCamera.lookfrom;
				glm::vec3 lookat = activeCamera.lookat;
				glm::vec3 vup = activeCamera.vup;
				ImGui::InputFloat3("Position", &position[0]);
				ImGui::InputFloat3("Look at", &lookat[0]);
				ImGui::InputFloat3("Up", &vup[0]);
				ImGui::SetItemTooltip("Up direction relative to camera.");

				if (position != activeCamera.lookfrom || lookat != activeCamera.lookat || vup != activeCamera.vup) {
					activeCamera.lookfrom = position;
					activeCamera.lookat = lookat;
					activeCamera.vup = vup;
					activeCamera.SetCameraHasMoved(true);
				}

				ImGui::Text("Lens");
				if (ImGui::SliderFloat("FOV", &activeCamera.vfov, 1.0f, 120.0f)) {
					ResetAccumulation();
				}

				if (ImGui::InputFloat("De-focus Angle", &activeCamera.defocus_angle, 0.25f, 1.0f)) {
					ResetAccumulation();
				}
				ImGui::SetItemTooltip("Variation angle of rays through each pixel. Higher value produces blurrier results for out of focus objects.");
				if (ImGui::InputFloat("Focus Distance", &activeCamera.focus_dist, 0.25f, 1.0f)) {
					ResetAccumulation();
				}
				ImGui::SetItemTooltip("Distance from camera lookfrom point to plane of perfect focus");

				ImGui::TreePop();
				ImGui::Separator();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Dimensions")) {

				int width = activeCamera.GetImageWidth();
				int height = activeCamera.GetImageHeight();
				float aspect = activeCamera.GetAspectRatio();

				ImGui::InputInt("Image width", &width, 0, 0, ImGuiInputTextFlags_ReadOnly);
				ImGui::InputInt("Image height", &height, 0, 0, ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat("Aspect ratio", &aspect, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

				ImGui::TreePop();
				ImGui::Separator();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Ray Properties")) {

				ImGui::InputInt("Samples per pixel", &activeCamera.samples_per_pixel, 1, 2);
				ImGui::SetItemTooltip("The number of rays fired from each pixel in a frame.\r\nEach ray will contribute to the final colour of that pixel by calculating an average.\r\nHigh values can significantly impact performance.");

				ImGui::InputInt("Max bounces", &activeCamera.max_bounces);
				ImGui::SetItemTooltip("Maximum times a ray can bounce off of scene geometry.\r\nHigher values will increase visual accuracy at expense of performance.");

				ImGui::Checkbox("Accumulation", &accumulate_frames);
				ImGui::SetItemTooltip("When enabled, final render will use an accumulation of previous frames, effectively gathering samples over multiple frames.\r\nWorks best with static scenes.");
				if (accumulate_frames) {
					ImGui::SameLine();
					ImGui::Checkbox("Auto-Reset", &auto_reset_accumulation);
					ImGui::SetItemTooltip("When enabled, camera movement will reset accumulation");
				}
				else {
					ResetAccumulation();
				}

				if (ImGui::Button("Reset Accumulation")) {
					ResetAccumulation();
				}

				ImGui::Text("Sky Colour Gradient");

				if (ImGui::ColorEdit3("Min-y colour", &activeCamera.sky_colour_min_y[0])) {
					ResetAccumulation();
				}
				ImGui::SetItemTooltip("When a ray misses the scene (hits the sky) and ray Y = 0, this colour will be used.");

				if (ImGui::ColorEdit3("Max-y colour", &activeCamera.sky_colour_max_y[0])) {
					ResetAccumulation();
				}
				ImGui::SetItemTooltip("When a ray misses the scene (hits the sky) and ray Y = 1, this colour will be used.");

				ImGui::TreePop();
				ImGui::Separator();
			}

			ImGui::PopStyleVar(1);
		}
		ImGui::EndChild();
	}
	else if (selected > 0) {
		int sphereID = selected - 1;
		int quadID = selected - (num_spheres + 1);

		if (sphereID < num_spheres) {
			// Sphere selected
			Sphere* sphere = activeScene.GetSphere(sphereID);
			const std::string& sphereName = activeScene.GetSphereName(sphereID);

			ImGui::Text(sphereName.c_str());
			ImGui::SameLine();

			float buttonWidth = ImGui::CalcTextSize("Delete sphere").x + ImGui::GetStyle().FramePadding.x * 2;
			float spaceAvailable = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceAvailable - buttonWidth);

			if (ImGui::Button("Delete sphere")) {
				activeScene.RemoveSphere(sphereID);
				selected--;
				ResetAccumulation();
			}

			ImGui::Separator();
			if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::TreeNode("Transform")) {
					HittableTransform* transform = activeScene.GetSphereTransform(sphereID);
					
					if (transform) {
						glm::vec3 translation = transform->translation;
						glm::vec3 euler_rotation = transform->euler_rotation;
						glm::vec3 scale = transform->scale;

						// Find the difference between previous transform state and new one
						if (ImGui::DragFloat3("Translation", &transform->translation[0])) {
							translation = transform->translation - translation;
							ResetAccumulation();
							sphere->Transform(create_transform(translation, glm::vec3(0.0f), glm::vec3(1.0f)));
						}
						if (ImGui::DragFloat3("Rotation", &transform->euler_rotation[0])) {
							euler_rotation = transform->euler_rotation - euler_rotation;
							ResetAccumulation();
							sphere->Transform(create_transform(glm::vec3(0.0f), euler_rotation, glm::vec3(1.0f)));
						}
						ImGui::BeginDisabled();
						if (ImGui::InputFloat3("Scale", &transform->scale[0], "%.3f", ImGuiInputTextFlags_ReadOnly)) {
							scale = (transform->scale - scale) + scale;
							ResetAccumulation();
							sphere->Transform(create_transform(glm::vec3(0.0f), glm::vec3(0.0f), scale));
						}
						ImGui::EndDisabled();
					}

					ImGui::TreePop();
					ImGui::Separator();
				}
				if (ImGui::TreeNode("Physical Properties")) {
					ImGui::Text("Position");
					if (ImGui::DragFloat3("Center", &sphere->Center[0])) {
						ResetAccumulation();
					}
					ImGui::Spacing();
					if (ImGui::DragFloat("Radius", &sphere->Radius)) {
						ResetAccumulation();
					}

					selected_material = sphere->material_index;
					ImGui::Spacing();
					if (ImGui::BeginCombo("Material", activeScene.GetMaterialName(selected_material).c_str())) {
						for (int i = 0; i < num_materials; i++) {
							if (ImGui::Selectable(activeScene.GetMaterialName(i).c_str(), selected_material == i)) {
								selected_material = i;
							}

							if (selected_material == i) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					if (sphere->material_index != selected_material) {
						sphere->material_index = selected_material;
						ResetAccumulation();
					}
					ImGui::TreePop();
					ImGui::Separator();
				}

				ImGui::PopStyleVar(1);
			}
			ImGui::EndChild();
		}
		else if (quadID < num_quads) {
			// Quad selected
			Quad* quad = activeScene.GetQuad(quadID);
			const std::string& quadName = activeScene.GetQuadName(quadID);
			bool quad_has_changed = false;
			ImGui::Text(quadName.c_str());
			ImGui::SameLine();

			float buttonWidth = ImGui::CalcTextSize("Delete quad").x + ImGui::GetStyle().FramePadding.x * 2;
			float spaceAvailable = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceAvailable - buttonWidth);

			if (ImGui::Button("Delete quad")) {
				activeScene.RemoveQuad(quadID);
				selected--;
				ResetAccumulation();
			}

			ImGui::Separator();
			if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::TreeNode("Physical Properties")) {
					if (ImGui::DragFloat3("Position", &quad->Q[0])) { quad_has_changed = true; }
					ImGui::Spacing();
					if (ImGui::DragFloat3("Horizontal extent", &quad->U[0])) { quad_has_changed = true; }
					ImGui::Spacing();
					if (ImGui::DragFloat3("Vertical extent", &quad->V[0])) { quad_has_changed = true; }

					if (quad_has_changed) { quad->Recalculate(); ResetAccumulation(); }

					selected_quad_type = quad->triangle_disk_id;
					const char* quad_types[3] = { "Quad", "Triangle", "Disk" };
					ImGui::Spacing();
					if (ImGui::BeginCombo("Quad Type", quad_types[selected_quad_type])) {
						for (unsigned int i = 0; i < 3; i++) {
							if (ImGui::Selectable(quad_types[i], selected_quad_type == i)) {
								selected_quad_type = i;
							}

							if (selected_quad_type == i) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					if (quad->triangle_disk_id != selected_quad_type) {
						quad->triangle_disk_id = selected_quad_type;
						ResetAccumulation();
					}

					selected_material = quad->material_index;
					ImGui::Spacing();
					if (ImGui::BeginCombo("Material", activeScene.GetMaterialName(selected_material).c_str())) {
						for (int i = 0; i < num_materials; i++) {
							if (ImGui::Selectable(activeScene.GetMaterialName(i).c_str(), selected_material == i)) {
								selected_material = i;
							}

							if (selected_material == i) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					if (quad->material_index != selected_material) {
						quad->material_index = selected_material;
						ResetAccumulation();
					}

					ImGui::TreePop();
					ImGui::Separator();
				}

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::TreeNode("Physical Constants")) {

					ImGuiInputTextFlags readonlyFlag = ImGuiInputTextFlags_ReadOnly;

					ImGui::InputFloat3("W", &quad->W[0], "%.3f", readonlyFlag);
					ImGui::InputFloat3("Normal", &quad->Normal[0], "%.3f", readonlyFlag);
					ImGui::InputFloat("D", &quad->D, 0.0f, 0.0f, "%.3f", readonlyFlag);
					ImGui::InputFloat("Area", &quad->Area, 0.0f, 0.0f, "%.3f", readonlyFlag);

					ImGui::TreePop();
					ImGui::Separator();
				}

				ImGui::PopStyleVar(1);
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();

	// Material browser
	// ----------------
	ImGui::Begin("Materials");
	ImGui::Text("Material count: %d", num_materials);
	ImGui::Text("Maximum materials: %d", MAX_MATERIALS);
	ImGui::Separator();
	if (num_materials > 0) {
		if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 8));

			ImGuiListClipper clipper;
			clipper.Begin(num_materials);
			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					if (ImGui::Selectable(activeScene.GetMaterialName(i).c_str(), selected_edit_material == i)) {
						selected_edit_material = i;
					}
				}
			}

			ImGui::PopStyleVar(1);
		}
		ImGui::EndChild();
		ImGui::Separator();
	}
	ImGui::End();

	ImGui::Begin("Material Properties");
	if (num_materials > 0) {
		const char* mat_name = activeScene.GetMaterialName(selected_edit_material).c_str();
		ImGui::Text(mat_name);
		ImGui::Separator();
		Material& mat = activeScene.GetMaterial(selected_edit_material);
		if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 8));

			// Surface
			ImGui::SeparatorText("Surface");
			if (ImGui::ColorEdit3("Albedo", &mat.Albedo[0])) {
				ResetAccumulation();
			}
			if (ImGui::DragFloat("Roughness", &mat.Roughness, 0.01f, 0.0f, 1.0f)) {
				ResetAccumulation();
			}
			if (ImGui::DragFloat("Metalness", &mat.Metal, 0.01f, 0.0f, 1.0f)) {
				ResetAccumulation();
			}

			selected_material_set = mat.material_set_index;
			ImGui::Spacing();
			if (ImGui::BeginCombo("Texture set", std::to_string(selected_material_set).c_str())) {
				for (int i = -1; i < num_material_sets; i++) {
					if (ImGui::Selectable(std::to_string(i).c_str(), selected_material_set == i)) {
						selected_material_set = i;
					}

					if (selected_material_set == i) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			if (mat.material_set_index != selected_material_set) {
				mat.material_set_index = selected_material_set;
				ResetAccumulation();
			}

			// Transparency
			ImGui::SeparatorText("Transparency");
			bool isTransparent = mat.is_transparent;
			ImGui::Checkbox("Transparent", &isTransparent);
			if (isTransparent != mat.is_transparent) {
				mat.is_transparent = isTransparent;
				ResetAccumulation();
			}
			if (isTransparent) {
				if (ImGui::DragFloat("Refractive index", &mat.refractive_index, 0.01f)) {
					ResetAccumulation();
				}
			}

			// Emission
			ImGui::SeparatorText("Emission");
			if (ImGui::ColorEdit3("Emissive Colour", &mat.EmissiveColour[0])) {
				ResetAccumulation();
			}
			if (ImGui::DragFloat("Emissive power", &mat.EmissivePower, 0.01f)) {
				ResetAccumulation();
			}

			// Volumetric
			ImGui::SeparatorText("Volumetric");
			bool isVolume = mat.is_constant_medium;
			ImGui::Checkbox("Volumetric", &isVolume);
			if (isVolume != mat.is_constant_medium) {
				mat.is_constant_medium = isVolume;
				ResetAccumulation();
			}
			if (isVolume) {
				float density = -1.0f / mat.neg_inv_density;
				if (ImGui::DragFloat("Density", &density, 0.001f, 0.00001f)) {
					mat.neg_inv_density = -1.0f / density;
					ResetAccumulation();
				}
			}

			ImGui::PopStyleVar(1);
		}
		ImGui::EndChild();
	}
	ImGui::End();

	viewport_width -= viewport_width % WORK_GROUP_SIZE;
	viewport_height -= viewport_height % WORK_GROUP_SIZE;

	if (viewport_width != SCR_WIDTH || viewport_height != SCR_HEIGHT) {
		SCR_WIDTH = viewport_width;
		SCR_HEIGHT = viewport_height;

		screenBuffers.ResizeTexture(SCR_WIDTH, SCR_HEIGHT);
		finalImage.ResizeTexture(SCR_WIDTH, SCR_HEIGHT);
		activeCamera.SetCameraHasMoved(true);
	}
}

bool Renderer::Initialise()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw: create window
	// -------------------
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GLTrace", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL) {
		Logger::LogError("Failed to create GLFW window");
		glfwTerminate();
		return false;
	}

	// Allow a C++ member function to be used by the C, OpenGl api callback for input callbacks
	glfwSetWindowUserPointer(glfwGetCurrentContext(), this);
	auto key = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->key_callback(window, key, scancode, action, mods);
	};

	auto scroll = [](GLFWwindow* window, double xoffset, double yoffset) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->scroll_callback(window, xoffset, yoffset);
	};

	auto mouse = [](GLFWwindow* window, double xpos, double ypos) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->mouse_callback(window, xpos, ypos);
	};

	auto mouse_button = [](GLFWwindow* window, int button, int action, int mods) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->mouse_button_callback(window, button, action, mods);
	};

	auto frame_size = [](GLFWwindow* window, int width, int height) {
		static_cast<Renderer*>(glfwGetWindowUserPointer(window))->framebuffer_size_callback(window, width, height);
	};

	glfwSetKeyCallback(window, key);
	glfwSetScrollCallback(window, scroll);
	glfwSetCursorPosCallback(window, mouse);
	glfwSetMouseButtonCallback(window, mouse_button);
	glfwSetFramebufferSizeCallback(window, frame_size);
	glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSwapInterval(0); // disables v sync

	// glad: load OpenGL function pointers
	// -----------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::LogError("Failed to initialize GLAD");
		glfwTerminate();
		return false;
	}

	// Set up GL Debug output
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		//glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_MEDIUM, -1, "error message here"); // example of custom error message
	}

	ComputeShader::InitOpenGLConstants();

	Logger::Log("OpenGL initialised");

	if (InitIMGUI()) { Logger::Log("ImGui initialised"); }
	else { Logger::LogError("Failed to initialise ImGui"); }
	return true;
}

bool Renderer::InitIMGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		// Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;		// Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;			// Enable docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;			// Enable multi-viewport

	// Setup platform backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return true;
}
