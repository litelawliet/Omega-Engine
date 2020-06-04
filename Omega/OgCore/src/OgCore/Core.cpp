#include <stack>
#include <OgCore/Core.h>
#include <OgRendering/Rendering/Renderer.h>
#include <OgCore/Managers/SceneManager.h>
#include <OgRendering/Managers/InputManager.h>
#include <OgCore/SceneLoader/SceneLoader.h>

OgEngine::Core::Core(const uint64_t p_width, const uint64_t p_height, const char* p_title)
{
	OgEngine::Renderer::InitVkRenderer(static_cast<int>(p_width), static_cast<int>(p_height), p_title);
	m_vulkanContext = Renderer::GetVkContext();

	RegisterComponentsAndSystems(OgEngine::Scene::EDITOR_SCENE);
	RegisterComponentsAndSystems(OgEngine::Scene::PLAY_SCENE);
	SceneManager::ChangeScene(Scene::EDITOR_SCENE);
	const auto indexEditorScene = static_cast<uint8_t>(Scene::EDITOR_SCENE);
	roots[indexEditorScene] = new SceneNode(SceneManager::CreateEntity());
}

OgEngine::Core::~Core()
{
	try
	{
		delete roots[static_cast<uint8_t>(Scene::EDITOR_SCENE)];
		delete roots[static_cast<uint8_t>(Scene::PLAY_SCENE)];
		roots = { nullptr, nullptr };
		Renderer::DestroyVkRenderer();
		m_vulkanContext = nullptr;
	}
	catch (const std::exception& p_exception)
	{
		std::cerr << p_exception.what();
	}
}

void OgEngine::Core::Run(float p_dt)
{
	if (m_vulkanContext->IsRaytracing())
	{
		if (InputManager::IsKeyPressed(KeyCode::R))
		{
			m_vulkanContext->GetRTPipeline()->isRefreshing = !m_vulkanContext->GetRTPipeline()->isRefreshing;
		}

		if (InputManager::IsKeyPressed(KeyCode::W))
			m_vulkanContext->GetRTPipeline()->m_camera.Translate(
				m_vulkanContext->GetRTPipeline()->m_camera.forward * p_dt * 10);

		if (InputManager::IsKeyPressed(KeyCode::A))
			m_vulkanContext->GetRTPipeline()->m_camera.Translate(
				m_vulkanContext->GetRTPipeline()->m_camera.right * -p_dt * 10);

		if (InputManager::IsKeyPressed(KeyCode::S))
			m_vulkanContext->GetRTPipeline()->m_camera.Translate(
				m_vulkanContext->GetRTPipeline()->m_camera.forward * -p_dt * 10);

		if (InputManager::IsKeyPressed(KeyCode::D))
			m_vulkanContext->GetRTPipeline()->m_camera.Translate(
				m_vulkanContext->GetRTPipeline()->m_camera.right * p_dt * 10);
	}

	const auto indexScene = static_cast<uint8_t>(SceneManager::CurrentScene());
	m_physicsSystem[indexScene]->Update(p_dt, m_physicsEngine);
	roots[indexScene]->Update(p_dt);
	m_renderSystem[indexScene]->Update(p_dt, m_vulkanContext);
	m_lightSystem[indexScene]->Update(p_dt, m_vulkanContext);
	m_scriptSystem[indexScene]->Update(p_dt, m_vulkanContext);
}

void OgEngine::Core::Display() const
{
	if (m_vulkanContext->IsRaytracing())
	{
		m_vulkanContext->GetRTPipeline()->UpdateTLAS();
		m_vulkanContext->GetRTPipeline()->RenderFrame();
	}
	else
	{
		m_vulkanContext->GetRSPipeline()->RenderFrame();
	}
}

void OgEngine::Core::AddEntity(SceneNode* p_parent) const
{
	if (p_parent)
	{
		p_parent->AddChild(new SceneNode(SceneManager::CreateEntity()));
	}
}

void OgEngine::Core::DestroyEntityNode(SceneNode* p_entity) const
{
	if (p_entity)
	{
		RemoveRenderedObjects(p_entity);
		// Removing the node from the scene graph
		p_entity->GetParent()->RemoveChild(p_entity);
	}
}

void OgEngine::Core::PlayScene()
{
	// Copy all the current scene (which is EDITOR_SCENE) into the second scene (PLAY_SCENE)
	if (m_vulkanContext->IsRaytracing())
	{
		m_vulkanContext->GetRTPipeline()->DestroyAllObjects();
	}
	else
	{
		m_vulkanContext->GetRSPipeline()->CleanAllObjectInstance();
	}
	// Since we normally always have a clean PLAY_SCENE after deletion, we need to add ourself a root node to the scene graph as baseline
	SceneManager::ChangeScene(Scene::PLAY_SCENE);
	const auto indexPlayScene = static_cast<uint8_t>(OgEngine::Scene::PLAY_SCENE);
	const unsigned char indexPauseScene = static_cast<uint8_t>(OgEngine::Scene::EDITOR_SCENE);
	roots[indexPlayScene] = new SceneNode(SceneManager::CreateEntity());

	// Recreate all the scene graph
	CreateChildrenOf(roots[indexPlayScene], roots[indexPauseScene]);
	// I don't need to delete anything because what we have right now in EDITOR_SCENE is exactly what we will have as PLAY_SCENE,
	// but with all systems updating. It's only when returning from PLAY_SCENE to EDITOR_SCENE(aka EditorScene
	// that we need to destroy all the PLAY_SCENE
	SceneManager::ChangeScene(Scene::PLAY_SCENE);
}

void OgEngine::Core::EditorScene()
{
	// We were in play scene before, so we just need to clear the current scene (which is PLAY_SCENE)
	// and fall back to EDITOR_SCENE using the change scene method from SceneManager

	// Remove all PLAY_SCENE objects from the rendering pipeline
	SceneManager::ChangeScene(Scene::PLAY_SCENE);
	//DestroyEntityNode(roots[PLAY_SCENE]);
	// delete the PLAY_SCENE root node (will be created when switching again to PLAY_SCENE)
	const uint8_t indexPlayScene = static_cast<uint8_t>(OgEngine::Scene::PLAY_SCENE);
	delete roots[indexPlayScene];
	roots[indexPlayScene] = nullptr;

	if (m_vulkanContext->IsRaytracing())
	{
		m_vulkanContext->GetRTPipeline()->DestroyAllObjects();
	}
	else
	{
		// Not needed anymore. Need to run further tests
		m_vulkanContext->GetRSPipeline()->CleanAllObjectInstance();
	}

	SceneManager::ChangeScene(Scene::EDITOR_SCENE);
}

void OgEngine::Core::SaveScene(const std::string& p_sceneName)
{
	std::ofstream file;
	file.open(p_sceneName, std::ios::out | std::ios::trunc);
	if (file.is_open())
	{
		const uint8_t indexScene = static_cast<uint8_t>(Scene::EDITOR_SCENE);
		int depth = 1;
		file << "<SceneNode>\n";
		file << SceneManager::GetComponent<Transform>(roots[indexScene]->GetEntity()).Serialize(depth);
		SerializeChildren(file, roots[indexScene], depth);

		file << "</SceneNode>";
	}

	file.close();
}

void OgEngine::Core::LoadScene(const std::string& p_file)
{
	if (!OgEngine::SceneLoader::SceneFileIntegrityCheck(p_file))
	{
		std::cerr << "File: " << p_file << " is not a valid Omega scene file or may be corrupted.\n";
		return;
	}
	std::ifstream file;
	file.open(p_file, std::ios::in);

	int nbLines = 0;
	std::string line;
	if (file.is_open())
	{
		SceneManager::ChangeScene(Scene::EDITOR_SCENE);
		const auto sceneIndexEditor = static_cast<uint8_t>(OgEngine::Scene::EDITOR_SCENE);
		RemoveRenderedObjects(roots[sceneIndexEditor]);
		inspectorNode = nullptr;
		delete roots[sceneIndexEditor];
		roots[sceneIndexEditor] = nullptr;
		
		const auto sceneIndexPlay = static_cast<uint8_t>(OgEngine::Scene::EDITOR_SCENE);
		RemoveRenderedObjects(roots[sceneIndexPlay]);
		delete roots[sceneIndexPlay];
		roots[sceneIndexPlay] = nullptr;

		std::stack<SceneNode*> latestNodes;

		try
		{
			while (!file.eof())
			{
				std::getline(file, line);
				// Add a scene to the scenegraph
				if (line.find("<SceneNode>") != std::string::npos)
				{
					// Starting point, always there
					if (roots[sceneIndexEditor] == nullptr)
					{
						roots[sceneIndexEditor] = new SceneNode(SceneManager::CreateEntity());
						latestNodes.push(roots[sceneIndexEditor]);
					}
					else
					{
						AddEntity(latestNodes.top());
						latestNodes.push(latestNodes.top()->LastChild());
						auto& trans = SceneManager::GetComponent<Transform>(latestNodes.top()->GetEntity());
					}
				}
				else if (line.find("<Transform>") != std::string::npos)
				{
					std::getline(file, line);
					const std::string name = SceneLoader::ExtractNameFromAttribute(line);
					std::getline(file, line);
					const Vector3F position = SceneLoader::ExtractVector3FromAttribute(line);
					std::getline(file, line);
					const Vector4F rotation = SceneLoader::ExtractVector4FromAttribute(line);
					std::getline(file, line);
					const Vector3F scale = SceneLoader::ExtractVector3FromAttribute(line);

					if (SceneManager::HasComponent<Transform>(latestNodes.top()->GetEntity()))
					{
						auto* transform = &SceneManager::GetComponent<Transform>(latestNodes.top()->GetEntity());
						transform->SetName(name);
						transform->SetPosition(position);
						transform->SetRotation(Quaternion(rotation.x, rotation.y, rotation.z, rotation.w));
						transform->SetScale(scale);
					}
				}
				else if (line.find("<Model>") != std::string::npos)
				{
					std::getline(file, line);
					const std::string parentMeshName = SceneLoader::ExtractNameFromAttribute(line);
					std::getline(file, line);
					const std::string meshName = SceneLoader::ExtractNameFromAttribute(line);
					std::getline(file, line);
					const std::string meshFilepath = SceneLoader::ExtractNameFromAttribute(line);
					std::getline(file, line);
					const bool isSubMesh = SceneLoader::ExtractIntegerFromAttribute(line);
					std::getline(file, line);
					const uint32_t subMeshIndex = SceneLoader::ExtractIntegerFromAttribute(line);

					if (!SceneManager::HasComponent<ModelRS>(latestNodes.top()->GetEntity()))
					{
						// Try to get the mesh:
						// 1 - Check if it exists in resource manager
						Mesh* meshToLink = ResourceManager::Get<Mesh>(parentMeshName);
						if (meshToLink)
						{
							// Mesh exists we can add directly
							if (!isSubMesh)
							{
								SceneManager::AddComponent(latestNodes.top()->GetEntity(), ModelRS(meshToLink));
							}
							else
							{
								meshToLink = meshToLink->SubMeshes()[subMeshIndex].get();
								SceneManager::AddComponent(latestNodes.top()->GetEntity(), ModelRS(meshToLink));
							}
						}
						else
						{
							// Mesh not in resource manager yet, try to add it
							ResourceManager::Add<Mesh>(meshFilepath);
							ResourceManager::WaitForResource<Mesh>(parentMeshName);
							meshToLink = ResourceManager::Get<Mesh>(parentMeshName);
							if (meshToLink)
							{
								if (!isSubMesh)
								{
									SceneManager::AddComponent(latestNodes.top()->GetEntity(), ModelRS(meshToLink));
								}
								else
								{
									meshToLink = meshToLink->SubMeshes()[subMeshIndex].get();
									SceneManager::AddComponent(latestNodes.top()->GetEntity(), ModelRS(meshToLink));
								}
							}
							else
							{
								// meshFilepath points to a faulty path or corrupted 3D mesh. We replace it by a cube as default
								SceneManager::AddComponent(latestNodes.top()->GetEntity(), ModelRS("cube.obj"));
							}
						}
					}
				}
				else if (line.find("<Material>") != std::string::npos)
				{
					std::getline(file, line);
					const Vector3F color = SceneLoader::ExtractVector3FromAttribute(line);

					std::getline(file, line);
					const Vector4F specular = SceneLoader::ExtractVector4FromAttribute(line);

					std::getline(file, line);
					const Vector4F emissive = SceneLoader::ExtractVector4FromAttribute(line);

					std::getline(file, line);
					const float ior = SceneLoader::ExtractFloatFromAttribute(line);

					std::getline(file, line);
					const float roughness = SceneLoader::ExtractFloatFromAttribute(line);

					std::getline(file, line);
					const int type = SceneLoader::ExtractIntegerFromAttribute(line);

					std::getline(file, line);
					std::string textureName = SceneLoader::ExtractNameFromAttribute(line);

					std::getline(file, line);
					std::string texturePath = SceneLoader::ExtractNameFromAttribute(line);

					std::getline(file, line);
					std::string normalName = SceneLoader::ExtractNameFromAttribute(line);

					std::getline(file, line);
					std::string normalPath = SceneLoader::ExtractNameFromAttribute(line);

					if (SceneManager::HasComponent<ModelRS>(latestNodes.top()->GetEntity()))
					{
						auto& material = SceneManager::GetComponent<ModelRS>(latestNodes.top()->GetEntity());

						// Check if both texture and normal exist
						Texture* tex = ResourceManager::Get<Texture>(textureName);
						if (!tex)
						{
							ResourceManager::Add<Texture>(texturePath);
							ResourceManager::WaitForResource<Texture>(textureName);
							tex = ResourceManager::Get<Texture>(textureName);
							if (!tex)
							{
								textureName = "error.png";
								texturePath = "Resources/textures/error.png";
								AddTexture(textureName, TEXTURE);
							}
							else
							{
								AddTexture(textureName, TEXTURE);
							}
						}
						if (normalName != "NONE")
						{
							Texture* norm = ResourceManager::Get<Texture>(normalName);
							if (!norm)
							{
								ResourceManager::Add<Texture>(normalPath);
								ResourceManager::WaitForResource<Texture>(normalName);
								norm = ResourceManager::Get<Texture>(normalName);
								if (!norm)
								{
									normalName = "error.png";
									normalPath = "Resources/textures/error.png";
									AddTexture(normalName, TEXTURE);
								}
								else
								{
									AddTexture(normalName, TEXTURE);
								}
							}
						}

						Material mat;
						mat.SetColor(Vector4F(color));
						mat.SetSpecular(specular);
						mat.SetEmissive(emissive);
						mat.SetIOR(ior);
						mat.SetRoughness(roughness);
						mat.SetType(type);
						mat.SetTextureID(textureName, texturePath);
						mat.SetNormalMapID(normalName, normalPath);

						material.SetMaterial(mat);
					}
				}
				else if (line.find("<RigidBody>") != std::string::npos)
				{
					std::getline(file, line);
					const float shapeSizeX = SceneLoader::ExtractFloatFromAttribute(line);

					std::getline(file, line);
					const float shapeSizeY = SceneLoader::ExtractFloatFromAttribute(line);

					std::getline(file, line);
					const float shapeSizeZ = SceneLoader::ExtractFloatFromAttribute(line);
					
					std::getline(file, line);
					const float mass = SceneLoader::ExtractFloatFromAttribute(line);

					std::getline(file, line);
					const int type = SceneLoader::ExtractIntegerFromAttribute(line);

					std::getline(file, line);
					const bool useGravity = SceneLoader::ExtractBooleanFromAttribute(line);

					std::getline(file, line);
					const bool isStatic = SceneLoader::ExtractBooleanFromAttribute(line);

					if (!SceneManager::HasComponent<RigidBody>(latestNodes.top()->GetEntity()))
					{
						SceneManager::AddComponent(latestNodes.top()->GetEntity(), RigidBody(static_cast<RB_COLLIDER_TYPE>(type), isStatic));
						auto& rigidBody = SceneManager::GetComponent<RigidBody>(latestNodes.top()->GetEntity());
						rigidBody.Initialize(m_physicsEngine, shapeSizeX, shapeSizeY, shapeSizeZ);
						m_physicsEngine.AddRigidBodyToScene(&rigidBody.GetRigidBody(), RigidBody::ConvertGPMtoPhysics(rigidBody.Transform()), isStatic);
						// give the new values
						rigidBody.SetMass(mass);
						rigidBody.SetShapeSize(shapeSizeX, shapeSizeY, shapeSizeZ);
						rigidBody.EnableGravity(useGravity);
					}
				}
				else if (line.find("<LightSource>") != std::string::npos)
				{
					std::getline(file, line);
					const Vector4F color = SceneLoader::ExtractVector4FromAttribute(line);

					std::getline(file, line);
					const Vector4F direction = SceneLoader::ExtractVector4FromAttribute(line);

					std::getline(file, line);
					const LIGHT_TYPE lightType = static_cast<LIGHT_TYPE>(SceneLoader::ExtractIntegerFromAttribute(line));

					if (!SceneManager::HasComponent<LightSource>(latestNodes.top()->GetEntity()))
					{
						SceneManager::AddComponent(latestNodes.top()->GetEntity(), LightSource());

						auto& lightSource = SceneManager::GetComponent<LightSource>(
							latestNodes.top()->GetEntity());

						lightSource.color = color;
						lightSource.direction = direction;
						lightSource.lightType = lightType;
					}
				}
				else if (line.find("</SceneNode>") != std::string::npos)
				{
					latestNodes.pop();
				}
				nbLines++;
			}
		}
		catch (const std::exception& p_exception)
		{
			// In case of thrown exception at reading time, we will remove everything we added previously
			// and make the root node again
			std::cerr << "File corrupted: " << p_exception.what() << '\n';
			RemoveRenderedObjects(roots[sceneIndexEditor]);
			delete roots[sceneIndexEditor];
			roots[sceneIndexEditor] = nullptr;
		}
	}
	file.close();

	// Happenned in case of empty scene, should not happen but if a user remove everything, it will crash if no roots exists
	const auto sceneIndex = static_cast<uint8_t>(OgEngine::Scene::EDITOR_SCENE);
	if (roots[sceneIndex] == nullptr)
	{
		roots[sceneIndex] = new SceneNode(SceneManager::CreateEntity());
	}
}

void OgEngine::Core::RemoveRenderedObjects(SceneNode* p_parent) const
{
	if (p_parent)
	{
		// Children of p_entity : Removing them from the rendering pipeline
		for (auto* node : p_parent->GetChildren())
		{
			if (m_vulkanContext->IsRaytracing())
			{
				m_vulkanContext->GetRTPipeline()->DestroyObject(node->GetEntity());
				if (SceneManager::HasComponent<LightSource>(node->GetEntity()))
				{
					m_vulkanContext->GetRTPipeline()->DestroyLight(node->GetEntity());
				}
			}
			else
			{
				RemoveRenderedObjects(node);
				m_vulkanContext->GetRSPipeline()->DestroyObject(node->GetEntity());
			}
		}

		// Removing p_entity itself from the rendering pipeline
		if (m_vulkanContext->IsRaytracing())
		{
			m_vulkanContext->GetRTPipeline()->DestroyObject(p_parent->GetEntity());
			if (SceneManager::HasComponent<LightSource>(p_parent->GetEntity()))
			{
				m_vulkanContext->GetRTPipeline()->DestroyLight(p_parent->GetEntity());
			}
		}
		else
		{
			m_vulkanContext->GetRSPipeline()->DestroyObject(p_parent->GetEntity());
		}
	}
}

void OgEngine::Core::SerializeChildren(std::ostream& p_file, SceneNode* p_parent, int& p_depth)
{
	if (p_parent)
	{
		for (auto* node : p_parent->GetChildren())
		{
			p_file << DepthIndent(p_depth) << "<SceneNode>\n";
			++p_depth;
			p_file << SceneManager::GetComponent<Transform>(node->GetEntity()).Serialize(p_depth);
			if (SceneManager::HasComponent<ModelRS>(node->GetEntity()))
			{
				// Call ModelRS Serialize
				p_file << SceneManager::GetComponent<ModelRS>(node->GetEntity()).Serialize(p_depth);
			}
			if (SceneManager::HasComponent<RigidBody>(node->GetEntity()))
			{
				// Call RigidBody Serialize
				p_file << SceneManager::GetComponent<RigidBody>(node->GetEntity()).Serialize(p_depth);
			}
			// TODO: Material component is under discussion as getting removed from being a component
			if (SceneManager::HasComponent<Material>(node->GetEntity()))
			{
				// Call Mateial Serialize
				p_file << SceneManager::GetComponent<Material>(node->GetEntity()).Serialize(p_depth);
			}
			if (SceneManager::HasComponent<LightSource>(node->GetEntity()))
			{
				// Call LightSource Serialize
				p_file << SceneManager::GetComponent<LightSource>(node->GetEntity()).Serialize(p_depth);
			}

			SerializeChildren(p_file, node, p_depth);

			--p_depth;
			p_file << DepthIndent(p_depth) << "</SceneNode>\n";
		}
	}
}

void OgEngine::Core::AddTexture(const std::string& p_texture, const TEXTURE_TYPE p_textureType) const
{
	if (m_vulkanContext->IsRaytracing())
		m_vulkanContext->GetRTPipeline()->AddTexture(p_texture, p_textureType);
	else
		m_vulkanContext->GetRSPipeline()->CreateTexture(p_texture, p_textureType);
}

void OgEngine::Core::AddRigidBodyToPhysics(const Entity p_entity)
{
	auto& rigidBody = GetComponent<RigidBody>(p_entity);
	auto& tr = GetComponent<Transform>(p_entity);
	rigidBody.SetShapeSize(tr.scale.x, tr.scale.y, tr.scale.z);
	rigidBody.Initialize(m_physicsEngine, tr.scale.x, tr.scale.y, tr.scale.z);
	m_physicsEngine.AddRigidBodyToScene(&rigidBody.GetRigidBody(), RigidBody::ConvertGPMtoPhysics(rigidBody.Transform()), rigidBody.IsStatic());
}

std::string OgEngine::Core::DepthIndent(const int p_depth)
{
	std::string depthCode;
	for (auto i = 0; i < p_depth; ++i)
	{
		depthCode += "\t";
	}

	return depthCode;
}

void OgEngine::Core::RegisterComponentsAndSystems(const Scene& p_scene)
{
	SceneManager::ChangeScene(p_scene);

	SceneManager::RegisterComponent<Transform>();
	SceneManager::RegisterComponent<ModelRS>();
	SceneManager::RegisterComponent<LightSource>();
	SceneManager::RegisterComponent<RigidBody>();
	SceneManager::RegisterComponent<Material>();
	SceneManager::RegisterComponent<AScript>();

	const auto sceneIndex = static_cast<uint8_t>(p_scene);

	m_renderSystem[sceneIndex] = SceneManager::RegisterSystem<RenderingSystem>();
	{
		Signature signature;
		signature.set(SceneManager::GetComponentType<Transform>());
		signature.set(SceneManager::GetComponentType<ModelRS>());
		SceneManager::SetSystemSignature<RenderingSystem>(signature);
	}

	m_physicsSystem[sceneIndex] = SceneManager::RegisterSystem<PhysicsSystem>();
	{
		Signature signature;
		signature.set(SceneManager::GetComponentType<Transform>());
		signature.set(SceneManager::GetComponentType<RigidBody>());
		SceneManager::SetSystemSignature<PhysicsSystem>(signature);
	}

	m_lightSystem[sceneIndex] = SceneManager::RegisterSystem<LightSystem>();
	{
		Signature signature;
		signature.set(SceneManager::GetComponentType<Transform>());
		signature.set(SceneManager::GetComponentType<LightSource>());
		SceneManager::SetSystemSignature<LightSystem>(signature);
	}

	m_scriptSystem[sceneIndex] = SceneManager::RegisterSystem<ScriptSystem>();
	{
		Signature signature;
		signature.set(SceneManager::GetComponentType<Transform>());
		signature.set(SceneManager::GetComponentType<AScript>());
		SceneManager::SetSystemSignature<ScriptSystem>(signature);
	}

	m_renderSystem[sceneIndex]->Init();
	m_physicsSystem[sceneIndex]->Init();
	m_lightSystem[sceneIndex]->Init();
	m_scriptSystem[sceneIndex]->Init();
}

void OgEngine::Core::CreateChildrenOf(SceneNode* p_parent, SceneNode* p_parentToReproduce) const
{
	if (p_parent && p_parentToReproduce)
	{
		for (auto* child : p_parentToReproduce->GetChildren())
		{
			SceneManager::ChangeScene(Scene::PLAY_SCENE);
			// We have children, so we add them one by one
			AddEntity(p_parent);
			// Go to EDITOR_SCENE, get the willing component
			// Stores a reference to the component
			// Go to PLAY_SCENE, add a component
			// Copy the EditorComponent into the PlayComponent
			SceneManager::ChangeScene(Scene::EDITOR_SCENE);
			if (HasComponent<Transform>(child->GetEntity()))
			{
				const auto& componentEditor = SceneManager::GetComponent<Transform>(child->GetEntity());
				SceneManager::ChangeScene(Scene::PLAY_SCENE);
				auto& componentPlay = SceneManager::GetComponent<Transform>(
					p_parent->LastChild()->GetEntity());
				componentPlay = componentEditor;
			}
			SceneManager::ChangeScene(Scene::EDITOR_SCENE);
			if (HasComponent<ModelRS>(child->GetEntity()))
			{
				const auto componentEditor = SceneManager::GetComponent<ModelRS>(child->GetEntity());
				SceneManager::ChangeScene(Scene::PLAY_SCENE);
				SceneManager::AddComponent(p_parent->LastChild()->GetEntity(), componentEditor);
			}
			SceneManager::ChangeScene(Scene::EDITOR_SCENE);
			if (HasComponent<RigidBody>(child->GetEntity()))
			{
				const auto componentEditor = SceneManager::GetComponent<RigidBody>(child->GetEntity());
				SceneManager::ChangeScene(Scene::PLAY_SCENE);
				SceneManager::AddComponent(p_parent->LastChild()->GetEntity(), componentEditor);
			}
			SceneManager::ChangeScene(Scene::EDITOR_SCENE);
			if (HasComponent<Material>(child->GetEntity()))
			{
				const auto componentEditor = SceneManager::GetComponent<Material>(child->GetEntity());
				SceneManager::ChangeScene(Scene::PLAY_SCENE);
				SceneManager::AddComponent(p_parent->LastChild()->GetEntity(), componentEditor);
			}
			SceneManager::ChangeScene(Scene::EDITOR_SCENE);
			if (HasComponent<LightSource>(child->GetEntity()))
			{
				const auto componentEditor = SceneManager::GetComponent<LightSource>(child->GetEntity());
				SceneManager::ChangeScene(Scene::PLAY_SCENE);
				SceneManager::AddComponent(p_parent->LastChild()->GetEntity(), componentEditor);
			}
		}

		// Call back the CreateAllChildrenOf on children of the children (yeah... I know)
		for (uint64_t index = 0u; index < p_parent->ChildCount(); ++index)
		{
			SceneManager::ChangeScene(Scene::PLAY_SCENE);
			CreateChildrenOf(p_parent->GetChildren()[index], p_parentToReproduce->GetChildren()[index]);
		}
	}
}
