#include <OgCore/Core.h>
#include <OgRendering/Rendering/Renderer.h>
#include <OgCore/Managers/SceneManager.h>

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

	m_renderSystem = SceneManager::RegisterSystem<RenderingSystem>();
	{
		Signature signature;
		signature.set(SceneManager::GetComponentType<Transform>());
		signature.set(SceneManager::GetComponentType<ModelRS>());
		SceneManager::SetSystemSignature<RenderingSystem>(signature);
	}

	m_renderSystem->Init();
	ResourceManager::WaitForAll();
	
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
			root->AddChild(new SceneNode(SceneManager::CreateEntity()));
			SceneManager::AddComponent(root->GetChild(root->ChildCount() - 1)->GetEntity(), ModelRS("lucy.obj"));
            SceneManager::GetComponent<ModelRS>(root->GetChild(root->ChildCount() - 1)->GetEntity()).Material().SetColor(GPM::Vector4F(1, 1, 1, 1));
		}
        if (OgEngine::InputManager::IsKeyPressed(KeyCode::R))
        {
            m_vulkanContext->GetRTPipeline()->m_cameraData.data.z = 1;
        }
		pos = OgEngine::InputManager::CursorPosition();
		std::cout << pos << '\n';

		// TODO: GAME LOGIC
		frameCount++;
		if (currentTime - previousTime >= 1.0)
		{
			m_vulkanContext->ChangeWindowTitle("Editor FPS:", frameCount);

			frameCount   = 0u;
			previousTime = currentTime;
		}

		root->Update(dt);
		m_renderSystem->Update(dt, m_vulkanContext);
		
		UpdateEditor(dt);

		// Rendering using the selected pipeline
		if (m_vulkanContext->IsRaytracing())
		{
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

void OgEngine::Core::UpdateEditor(float p_dt)
{
	if (m_vulkanContext->IsRaytracing())
	{
		ImGui::SetCurrentContext(m_vulkanContext->GetRTPipeline()->GetUIContext());
	}
	else
	{
		ImGui::SetCurrentContext(m_vulkanContext->GetRSPipeline()->GetUIContext());
	}
    PrepareImGUIFrame();
	DrawEditor();
	ImGui::BeginMainMenuBar();
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("GameObject"))
		{
			if (ImGui::BeginMenu("Add"))
			{
				if (ImGui::MenuItem("Cube"))
				{
					root->AddChild(new SceneNode(SceneManager::CreateEntity()));
					SceneManager::AddComponent(root->GetChild(root->ChildCount() - 1)->GetEntity(), ModelRS("cube.obj"));
					SceneManager::GetComponent<ModelRS>(root->GetChild(root->ChildCount() - 1)->GetEntity()).Material().SetColor(GPM::Vector4F(1, 1, 1, 1));
				}
				if (ImGui::MenuItem("Sphere"))
				{
					root->AddChild(new SceneNode(SceneManager::CreateEntity()));
					SceneManager::AddComponent(root->GetChild(root->ChildCount() - 1)->GetEntity(), ModelRS("sphere.obj"));
					SceneManager::GetComponent<ModelRS>(root->GetChild(root->ChildCount() - 1)->GetEntity()).Material().SetColor(GPM::Vector4F(1, 1, 1, 1));
				}
				if (ImGui::MenuItem("Plane"))
				{
					root->AddChild(new SceneNode(SceneManager::CreateEntity()));
					SceneManager::AddComponent(root->GetChild(root->ChildCount() - 1)->GetEntity(), ModelRS("plane.obj"));
					SceneManager::GetComponent<ModelRS>(root->GetChild(root->ChildCount() - 1)->GetEntity()).Material().SetColor(GPM::Vector4F(1, 1, 1, 1));
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
	}
	ImGui::EndMainMenuBar();

	ImGui::Begin("GameObjects");
	{
		LoopOnChild(root);
	}
	ImGui::End();

	ImGui::Begin("Inspector");
	{
		if(inspectorNode != nullptr)
			ShowInfo(inspectorNode);
	}
	ImGui::End();

    DrawUI();
}
bool OgEngine::Core::LoopOnChild(OgEngine::SceneNode* node)
{
	std::string name = "Object " + std::to_string(node->GetEntity());
	bool opened = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_OpenOnArrow);

	if (ImGui::IsItemClicked())
	{
		inspectorNode = node;
	}

	//Popup for object menu
	if (ImGui::IsItemHovered() && ImGui::IsItemClicked(1))
	{
		ImGui::OpenPopup((name + "popup").c_str());
	}

	//Add Menu
	if (ImGui::BeginPopup((name + "popup").c_str()))
	{
		OpenAddMenu(node);
		ImGui::EndPopup();
	};

	if(opened)
	{
		for (auto& child : node->GetChildren())
			LoopOnChild(child);

		ImGui::TreePop();
	}
	return opened;
}

void OgEngine::Core::OpenAddMenu(SceneNode* node)
{
	if(ImGui::BeginMenu("Add Object"))
	{
		if (ImGui::MenuItem("Cube"))
		{
			node->AddChild(new SceneNode(SceneManager::CreateEntity()));
			SceneManager::AddComponent(node->GetChild(node->ChildCount() - 1)->GetEntity(), ModelRS("cube.obj"));
			SceneManager::GetComponent<ModelRS>(node->GetChild(node->ChildCount() - 1)->GetEntity()).Material().SetColor(GPM::Vector4F(1, 1, 1, 1));
		}

		ImGui::EndMenu();
	}
}

void OgEngine::Core::ShowInfo(OgEngine::SceneNode* node)
{
	uint32_t entity = node->GetEntity();
	std::string headerID = "Info##" + std::to_string(entity);

	auto& trans = SceneManager::GetComponent<Transform>(entity);
	ImGui::Begin("Inspector");
	{

		if (ImGui::CollapsingHeader(("Transform##" + std::to_string(entity) + "transform").c_str()))
		{
			ImGui::Text("Position");
			std::string positionID = "##" + std::to_string(entity) + "p";
			float pos[3] = { trans.localPosition.x, trans.localPosition.y, trans.localPosition.z };
			ImGui::DragFloat3(positionID.c_str(), pos, 0.05f);
			trans.SetPosition({ pos[0], pos[1], pos[2] });

			ImGui::Text("Rotation");
			std::string rotationID = "##" + std::to_string(entity) + "r";
			float rot[3] = { trans.localRotation.ToEuler().x, trans.localRotation.ToEuler().y, trans.localRotation.ToEuler().z };
			ImGui::DragFloat3(rotationID.c_str(), rot, 0.05f);
			trans.SetRotation(Quaternion::MakeFromEuler(rot[0], rot[1], rot[2]));
		}

		if (node != root)
		{
			auto& mat = SceneManager::GetComponent<ModelRS>(entity).Material();
			if(ImGui::CollapsingHeader(("Material##" + std::to_string(entity) + "material").c_str()))
			{
				float color[3] = { mat.Color().x, mat.Color().y, mat.Color().z };
				ImGui::ColorEdit3(("Color##" + std::to_string(entity) + "material").c_str(), color);
				mat.SetColor(GPM::Vector4F(color[0], color[1], color[2], 1));

				float roughness = mat.Roughness();
				ImGui::DragFloat(("Roughness##" + std::to_string(entity) + "material").c_str(), &roughness, 0.05f, 0.0f, 1.0f);
				mat.SetRoughness(roughness);

				float metallic = mat.Metallic();
				ImGui::DragFloat(("Metallic##" + std::to_string(entity) + "material").c_str(), &metallic, 0.05f, 0.0f, 1.0f);
				mat.SetMetallic(metallic);

				float reflectance = mat.Reflectance();
				ImGui::DragFloat(("Reflectance##" + std::to_string(entity) + "material").c_str(), &reflectance, 0.05f, 0.0f, 1.0f);
				mat.SetReflectance(reflectance);
			}
		}
	}
	ImGui::End();
}
void OgEngine::Core::PrepareImGUIFrame()
{
	if (m_vulkanContext->IsRaytracing())
		m_vulkanContext->GetRTPipeline()->PrepareIMGUIFrame();
	else
		m_vulkanContext->GetRSPipeline()->PrepareIMGUIFrame();
}

void OgEngine::Core::DrawEditor()
{
	if (m_vulkanContext->IsRaytracing())
		m_vulkanContext->GetRTPipeline()->DrawEditor();
	else
		m_vulkanContext->GetRSPipeline()->DrawEditor();
}

void OgEngine::Core::DrawUI()
{
    if (m_vulkanContext->IsRaytracing())
        m_vulkanContext->GetRTPipeline()->DrawUI();
	else
		m_vulkanContext->GetRSPipeline()->DrawUI();
}