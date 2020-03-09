#include <OgCore/Core.h>
#include <OgRendering/Rendering/Renderer.h>
#include <OgCore/Managers/SceneManager.h>
#include <OgCore/Systems/RenderingSystem.h>

#include <OgCore/Components/LightSource.h>

#include <OgRendering/Managers/InputManager.h>


OgEngine::Core::Core(const uint64_t p_width, const uint64_t p_height, const char* p_title)
{
	OgEngine::Renderer::InitVkRenderer(static_cast<int>(p_width), static_cast<int>(p_height), p_title);
	m_vulkanContext = Renderer::GetVkContext();
	SceneManager::RegisterComponent<Transform>();
	SceneManager::RegisterComponent<ModelRS>();
	SceneManager::RegisterComponent<LightSource>();
	root = new SceneNode(SceneManager::CreateEntity());
}

OgEngine::Core::~Core()
{
	delete root;
	std::cout << "Everyone died\n";
	//delete root;
}

void OgEngine::Core::Run()
{
	auto renderSystem = SceneManager::RegisterSystem<RenderingSystem>();
	{
		Signature signature;
		signature.set(SceneManager::GetComponentType<Transform>());
		signature.set(SceneManager::GetComponentType<ModelRS>());
		SceneManager::SetSystemSignature<RenderingSystem>(signature);
	}

	renderSystem->Init();
	
	//ResourceManager::WaitForAll();

	//const Entity entity = SceneManager::CreateEntity();
	//SceneManager::AddComponent(entity, Transform{});
	//std::cout << SceneManager::GetComponent<Transform>(entity);

	//auto& trans = SceneManager::GetComponent<Transform>(entity);
	//trans.Translate(Vector3F(0.0f, 0.0, -2.0f));
	//std::cout << SceneManager::GetComponent<Transform>(entity);
	//const Entity entity2 = SceneManager::CreateEntity();
	//SceneManager::AddComponent(entity, ModelRS{ "lucy.obj" });
	

	//SceneManager::AddComponent(entity2, ModelRS{"lucy.obj"});
	//SceneManager::AddComponent(entity, LightSource{}); 
	//SceneManager::GetComponent<Transform>(entity2).SetScale(Vector3F{ 1.0f, 1.0f, 1.0f });
	 
	//std::cout << SceneManager::GetComponent<Transform>(entity);
	//auto& light = SceneManager::GetComponent<LightSource>(entity);
	//light.ambient = GPM::Vector4F{ 1.0f, 1.0f, 1.0f, 1.0f };
	//light.diffuse = GPM::Vector4F{0.0f, 0.0f, 1.0f, 1.0f};
	//light.specular= GPM::Vector4F{1.0f, 1.0f, 1.0f, 1.0f};
	
	double   previousTime = m_vulkanContext->TimeOfContext();
	double   currentTime  = 0.0;
	uint64_t frameCount   = 0u;

	float dt = 0.0f;

	Vector2D pos;

	while (!m_vulkanContext->WindowShouldClose() || !m_vulkanContext->IsRendering())
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		currentTime    = m_vulkanContext->TimeOfContext();
		// TODO: GET EVENT, AND UPDATE THEM
		m_vulkanContext->PollEvents();

		if (OgEngine::InputManager::IsKeyPressed(KeyCode::B))
		{
			/*Entity newEntity = SceneManager::CreateEntity();
			SceneManager::AddComponent(newEntity, ModelRS("lucy.obj"));*/
			root->AddChild(new SceneNode(SceneManager::CreateEntity()));
			SceneManager::AddComponent(root->GetChild(root->ChildCount()-1)->GetEntity(), ModelRS("lucy.obj"));
		}

		
		if (OgEngine::InputManager::IsKeyPressed(KeyCode::C))
		{
			/*Entity newEntity = SceneManager::CreateEntity();
			SceneManager::AddComponent(newEntity, ModelRS("lucy.obj"));*/
			auto& trans = SceneManager::GetComponent<Transform>(root->GetEntity());
			trans.SetScale(Vector3F(0.03f, 0.03f, 0.03f));
 			std::cout << "changing scale to 3\n";
		}
		 
		pos = OgEngine::InputManager::CursorPosition();
		std::cout << pos << '\n';

		// TODO: GAME LOGIC
		frameCount++;
		if (currentTime - previousTime >= 1.0)
		{
			m_vulkanContext->ChangeWindowTitle("Vulkan, FPS:", frameCount);

			frameCount   = 0u;
			previousTime = currentTime;
		}

		root->Update(dt);
		renderSystem->Update(dt, m_vulkanContext);

		// Rendering using the selected pipeline
		if (m_vulkanContext->IsRaytracing())
		{
            //SceneManager::GetComponent<Transform>(lucy).Rotate(10 * dt, { 0, 1, 0 });
			//m_vulkanContext->GetRTPipeline()->m_objects[0].Rotate({ 0, 0.002f, 0 });
			m_vulkanContext->GetRTPipeline()->UpdateTLAS();
			m_vulkanContext->GetRTPipeline()->Draw();
		}
		else
		{
			m_vulkanContext->GetRSPipeline()->DrawFrame();
		}

		auto stopTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
	}
	//m_renderSystem->Update(m_vulkanContext, dt);
	if (m_vulkanContext)
		m_vulkanContext->SetRenderingLoop(false);
}
