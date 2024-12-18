#include "Renderer.h"
#include "CameraController.h"
#include "TestScene.h"
#include "CornellBox.h"
#include "CornellMirror.h"
#include "StressTestScene.h"
#include "TestModelScene.h"
#include "BigBen.h"
#include "CPURTDEBUG.h"

#include "JSON.h"
bool JSON::changeSceneAtEndOfFrame = false;
std::string JSON::loadPath = std::string("");


void TestBVH(const BVH_DEBUG_RAY ray, const Scene* scene) {
	BVH_DEBUG_HIT_RECORD rec;
	bool hit_anything = false;
	float closest_so_far = 1000000.0;
	hit_anything = CPURTDEBUG::DebugBVHTraversal(ray, BVH_DEBUG_INTERVAL(0.001f, 1000000.0f), rec, closest_so_far, scene->GetQuads(), scene->GetSpheres(), scene->GetBVH());
}

int main()
{
	Renderer renderer = Renderer(1920, 1080);
	CameraController camControl;

	Scene* scene = JSON::LoadSceneFromJSON(JSON::loadPath.c_str());
	//Scene* scene = new TestModelScene();
	if (!scene) {
		return 1;
	}
	scene->SetupScene();
	scene->BuildBVH();
	scene->BufferBVH(renderer.GetRTCompute());
	scene->BufferSceneHittables(renderer.GetRTCompute());

	//glm::vec3 origin = glm::vec3(-13.63268f, 15.22046f, 17.82981f);
	//glm::vec3 direction = glm::vec3(0.25227f, -0.69145f, -1.0188f);
	//BVH_DEBUG_RAY test_ray = BVH_DEBUG_RAY(origin, direction);
	//TestBVH(test_ray, scene);

	camControl.activeCamera = scene->GetSceneCamera();
	camControl.Initialise();

	GLFWwindow* window = renderer.GetWindow();

	float lastFrame = static_cast<float>(glfwGetTime());
	float currentFrame;
	float dt = 0.0f;

	// json test
	//JSON::WriteSceneToJSON("Scenes/TestModelScene.json", *scene);

	while (!glfwWindowShouldClose(window))
	{
		currentFrame = static_cast<float>(glfwGetTime()) + 0.0001f;
		dt = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//std::clog << "Delta time: " << dt << " || FPS: " << 1.0 / dt << "										\r" << std::flush;

		// Process inputs
		const glm::vec2& mousePos = renderer.MousePos();
		camControl.ProcessMouseMovement(mousePos.x, mousePos.y);
		camControl.ProcessMouseScroll(renderer.MouseScrollOffsetY());

		if (InputManager::IsKeyDown(GLFW_KEY_R)) { renderer.ToggleAccumulation(); }

		// Update scene
		camControl.Update(dt);
		scene->UpdateScene(dt);

		scene->BufferBVH(renderer.GetRTCompute());
		scene->BufferSceneHittables(renderer.GetRTCompute());

		// Render
		renderer.Render(*scene->GetSceneCamera(), *scene, dt);

		glfwSwapBuffers(window);

		glfwPollEvents();

		if (JSON::changeSceneAtEndOfFrame) {
			TextureLoader::ClearResources();
			Scene* new_scene = JSON::LoadSceneFromJSON(JSON::loadPath.c_str());

			if (new_scene) {
				scene->ClearBuffers(renderer.GetRTCompute());
				scene->GetBVH().ClearBuffer(renderer.GetRTCompute());
				delete scene;
				scene = new_scene;
				camControl.activeCamera = scene->GetSceneCamera();
				camControl.Initialise();
				renderer.ResetAccumulation();
				scene->SetupScene();
				scene->BuildBVH();
			}
			JSON::changeSceneAtEndOfFrame = false;
		}
	}

	delete scene;
}