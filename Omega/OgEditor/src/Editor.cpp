#include <iostream>
#include <Editor.h>

#include "OgCore/Components/CustomScript.h"

int main()
{
	try
	{
		srand(static_cast<unsigned int>(time(NULL)));
		OgEngine::Editor editor(1920, 1080, "Omega Editor");
		editor.Run();

		return EXIT_SUCCESS;
	}
	catch (const std::exception& p_exception)
	{
		std::cerr << p_exception.what() << '\n';
		return EXIT_FAILURE;
	}
}

OgEngine::Editor::Editor(const uint64_t p_width, const uint64_t p_height, const char* p_title)
	: currentRotationEntity(UINT64_MAX), worldRotation(true)
{
	memset(currentEulers, 0x0, sizeof(currentEulers));
	OgEngine::ResourceManager::Add<OgEngine::Mesh>("Resources/models/cube.obj");
	m_modelNames.emplace_back("cube.obj");
	OgEngine::ResourceManager::Add<OgEngine::Mesh>("Resources/models/sphere.obj");
	m_modelNames.emplace_back("sphere.obj");
	OgEngine::ResourceManager::Add<OgEngine::Mesh>("Resources/models/plane.obj");
	m_modelNames.emplace_back("plane.obj");

	//OgEngine::ResourceManager::Add<OgEngine::Texture>("Resources/textures/default.png");
	//OgEngine::ResourceManager::Add<OgEngine::Texture>("Resources/textures/error.png");
	OgEngine::ResourceManager::WaitForAll();

	m_engine = std::make_unique<OgEngine::Core>(p_width, p_height, p_title);
	m_path = std::filesystem::current_path();
	PrepareIcons();
	fileDialog.SetTitle("Save scene");
	fileDialog.SetTypeFilters({ ".omega" });
}

OgEngine::Editor::~Editor()
{

}

void OgEngine::Editor::UpdateEditor(float p_dt)
{
	if (m_engine->m_vulkanContext->IsRaytracing())
	{
		ImGui::SetCurrentContext(m_engine->m_vulkanContext->GetRTPipeline()->GetUIContext());
	}
	else
	{
		ImGui::SetCurrentContext(m_engine->m_vulkanContext->GetRSPipeline()->GetUIContext());
	}

	PrepareImGUIFrame();
	DrawEditor();
	ImGui::BeginMainMenuBar();
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Reload Shaders"))
			{
				m_engine->m_vulkanContext->GetRTPipeline()->ReloadShaders();
			}

			if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
			{
				fileDialog.SetTitle("Save scene");
				fileDialog.EnableSaveMode(true);
				fileDialog.Open();
			}
			if (ImGui::MenuItem("Load Scene", nullptr, false, SceneManager::CurrentScene() == Scene::EDITOR_SCENE))
			{
				fileDialog.SetTitle("Load scene");
				fileDialog.EnableSaveMode(false);
				fileDialog.Open();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("GameObject"))
		{
			OpenAddMenu(m_engine->roots[static_cast<uint8_t>(SceneManager::CurrentScene())]);

			ImGui::EndMenu();
		}
	}
	ImGui::EndMainMenuBar();

	ImGui::Begin("Scene");
	{
		if (ImGui::ImageButton(m_localAxisButton, ImVec2(32, 32), ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (ImGui::IsItemHovered())
			{

			}
			worldRotation = false;
		}
		ImGui::SameLine(0, -1);

		if (ImGui::ImageButton(m_globalAxisButton, ImVec2(32, 32), ImVec2(0, 1), ImVec2(1, 0)))
		{
			worldRotation = true;
		}
		ImGui::SameLine(0, -1);

		ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 12, ImGui::GetCursorPosY()));

		if (ImGui::ImageButton(m_playIcon, ImVec2(32, 32)))
		{
			if (SceneManager::CurrentScene() == Scene::EDITOR_SCENE)
			{
				currentRotationEntity = UINT64_MAX;
				m_engine->PlayScene();
				m_engine->inspectorNode = nullptr;
			}
		}
		ImGui::SameLine(0, -1);

		if (ImGui::ImageButton(m_pauseIcon, ImVec2(32, 32)))
		{
			if (SceneManager::CurrentScene() == Scene::PLAY_SCENE)
			{
				currentRotationEntity = UINT64_MAX;
				m_engine->EditorScene();
				m_engine->inspectorNode = nullptr;
			}
		}

		const ImVec2 newSize = ImGui::GetContentRegionAvail();
		if (m_engine->m_vulkanContext->IsRaytracing())
			ImGui::Image(m_engine->m_vulkanContext->GetRTPipeline()->m_sceneID, newSize);
		else
			ImGui::Image(m_engine->m_vulkanContext->GetRSPipeline()->m_sceneID, newSize);
	}
	ImGui::End();

	ImGui::Begin("GameObjects");
	{
		LoopOnChild(m_engine->roots[static_cast<uint8_t>(SceneManager::CurrentScene())]);
	}
	ImGui::End();

	ImGui::Begin("Camera");
	{
		ImGui::Text("Position");
		if (m_engine->m_vulkanContext->IsRaytracing())
		{
			const glm::vec3 position = m_engine->m_vulkanContext->GetRTPipeline()->m_camera.position;
			float pos[3] = { position.x, position.y, position.z };
			ImGui::DragFloat3("CameraPos", pos);
			m_engine->m_vulkanContext->GetRTPipeline()->m_camera.SetPosition(glm::vec3(pos[0], pos[1], pos[2]));

			const glm::vec3 rotation = m_engine->m_vulkanContext->GetRTPipeline()->m_camera.rotation;
			float rot[3] = { rotation.x, rotation.y, rotation.z };
			ImGui::DragFloat3("CameraRot", rot);
			m_engine->m_vulkanContext->GetRTPipeline()->m_camera.SetRotation(glm::vec3(rot[0], rot[1], rot[2]));

		}
		else
		{
			const glm::vec3 position = m_engine->m_vulkanContext->GetRSPipeline()->GetCurrentCamera().position;
			float pos[3] = { position.x, position.y, position.z };
			ImGui::DragFloat3("CameraPos", pos);

			const glm::vec3 rotation = m_engine->m_vulkanContext->GetRSPipeline()->GetCurrentCamera().rotation;
			float rot[3] = { rotation.x, rotation.y, rotation.z };
			ImGui::DragFloat3("CameraRot", rot);

			m_engine->m_vulkanContext->GetRSPipeline()->UpdateCamera(glm::vec3(pos[0], pos[1], pos[2]), glm::vec3(rot[0], rot[1], rot[2]));
		}
	}
	ImGui::End();
	ImGui::Begin("Inspector");
	{
		if (m_engine->inspectorNode != nullptr)
			ShowInfo(m_engine->inspectorNode);
	}
	ImGui::End();
	DisplayFileBrowser();


	if (fileDialog.HasSelected())
	{
		ImGui::Text(fileDialog.GetSelected().string().c_str());
		std::cout << "Selected filename" << fileDialog.GetSelected().string() << std::endl;
		if (fileDialog.SavingMode())
		{
			m_engine->SaveScene(fileDialog.GetSelected().string());
		}
		else
		{
			m_engine->LoadScene(fileDialog.GetSelected().string());
		}
		fileDialog.ClearSelected();
	}
	fileDialog.Display();
	RenderUI();
}
bool OgEngine::Editor::LoopOnChild(OgEngine::SceneNode* p_node)
{
	const std::string name = m_engine->GetComponent<Transform>(p_node->GetEntity()).name + "##" + std::to_string(p_node->GetEntity());
	bool opened = true;
	if (p_node != m_engine->roots[static_cast<uint8_t>(SceneManager::CurrentScene())])
	{
		opened = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Selected);

		if (ImGui::IsItemClicked())
		{
			m_engine->inspectorNode = p_node;
		}

		//Popup for object menu
		if (ImGui::IsItemHovered() && ImGui::IsItemClicked(1))
		{
			ImGui::OpenPopup(std::string(name + "popup").c_str());
		}

		//Add Menu
		if (ImGui::BeginPopup(std::string(name + "popup").c_str()))
		{
			if (ImGui::Button("Destroy"))
			{
				DestroyObject(p_node);
			}

			if (ImGui::BeginMenu("Add Component"))
			{
				if (ImGui::BeginMenu("RigidBody"))
				{
					ImGui::Text("Collider Type");

					if (ImGui::MenuItem("PLANE"))
					{
						m_engine->AddComponent<RigidBody>(p_node->GetEntity(),
							RigidBody(OgEngine::RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_PLANE, false));
						m_engine->AddRigidBodyToPhysics(p_node->GetEntity());
					}

					if (ImGui::MenuItem("BOX"))
					{
						m_engine->AddComponent<RigidBody>(p_node->GetEntity(),
							RigidBody(OgEngine::RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_BOX, false));
						m_engine->AddRigidBodyToPhysics(p_node->GetEntity());
					}

					if (ImGui::MenuItem("SPHERE"))
					{
						m_engine->AddComponent<RigidBody>(p_node->GetEntity(),
							RigidBody(OgEngine::RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_SPHERE, false));
						m_engine->AddRigidBodyToPhysics(p_node->GetEntity());
					}

					if (ImGui::MenuItem("PLANE STATIC"))
					{
						m_engine->AddComponent<RigidBody>(p_node->GetEntity(),
							RigidBody(OgEngine::RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_PLANE, true));
						m_engine->AddRigidBodyToPhysics(p_node->GetEntity());
					}
					if (ImGui::MenuItem("BOX STATIC "))
					{
						m_engine->AddComponent<RigidBody>(p_node->GetEntity(),
							RigidBody(OgEngine::RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_BOX, true));
						m_engine->AddRigidBodyToPhysics(p_node->GetEntity());

					}
					if (ImGui::MenuItem("SPHERE STATIC "))
					{
						m_engine->AddComponent<RigidBody>(p_node->GetEntity(),
							RigidBody(OgEngine::RB_COLLIDER_TYPE::RB_COLLIDER_TYPE_SPHERE, true));
						m_engine->AddRigidBodyToPhysics(p_node->GetEntity());
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Light Source"))
				{
					m_engine->AddComponent(p_node->GetEntity(), LightSource());
				}

				if (ImGui::MenuItem("Script"))
				{
					m_engine->AddComponent(p_node->GetEntity(), AScript());
					// That's not the use we want, we really should have a real scripting with interpretor.
					m_engine->GetComponent<AScript>(p_node->GetEntity()).SetRunningScript(std::make_shared<CustomScript>());
				}
				ImGui::EndMenu();
			}

			OpenAddMenu(p_node);
			ImGui::EndPopup();
		};
	}

	if (opened)
	{
		for (auto& child : p_node->GetChildren())
			LoopOnChild(child);

		if (p_node != m_engine->roots[static_cast<uint8_t>(SceneManager::CurrentScene())])
			ImGui::TreePop();
	}
	return opened;
}

void OgEngine::Editor::OpenAddMenu(SceneNode* p_node)
{
	if (ImGui::BeginMenu("Add"))
	{
		if (ImGui::MenuItem("Empty"))
		{
			m_engine->AddEntity(p_node);
		}

		for (auto& mesh : m_modelNames)
		{
			if (ImGui::MenuItem(mesh.c_str()))
			{
				m_engine->AddEntity(p_node);
				auto* rsMesh = ResourceManager::Get<Mesh>(mesh);
				if (rsMesh != nullptr)
				{
					if (rsMesh->SubMeshes().empty())
					{
						m_engine->AddComponent(p_node->LastChild()->GetEntity(), ModelRS(mesh.c_str()));
						m_engine->GetComponent<ModelRS>(p_node->LastChild()->GetEntity()).Material().SetColor(glm::vec4(1, 1, 1, 1));
					}
					else
					{
						m_engine->AddEntity(p_node->LastChild());
						m_engine->AddComponent(p_node->LastChild()->LastChild()->GetEntity(), ModelRS(mesh.c_str()));
						m_engine->GetComponent<ModelRS>(p_node->LastChild()->LastChild()->GetEntity()).Material().SetColor(glm::vec4(1, 1, 1, 1));
						auto& model = m_engine->GetComponent<ModelRS>(p_node->LastChild()->LastChild()->GetEntity());
						{
							for (auto& subMesh : model.GetMesh()->SubMeshes())
							{
								// For each submesh, add a new entity with a new model RS for each mesh
								m_engine->AddEntity(p_node->LastChild());
								m_engine->AddComponent(p_node->LastChild()->LastChild()->GetEntity(), ModelRS(subMesh.get()));
								m_engine->GetComponent<ModelRS>(p_node->LastChild()->LastChild()->GetEntity()).Material().SetColor(glm::vec4(1, 1, 1, 1));
							}
						}
					}
				}
			}
		}
		ImGui::EndMenu();
	}
}

void OgEngine::Editor::ShowInfo(OgEngine::SceneNode* p_node)
{
	const Entity entity = p_node->GetEntity();
	std::string headerID = "Info " + std::to_string(entity);

	auto& trans = m_engine->GetComponent<Transform>(entity);
	ImGui::Begin("Inspector");
	{
#pragma region Transform Component
		ImGui::Image(m_transformIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::SameLine();
		if (ImGui::CollapsingHeader(std::string("Transform##" + std::to_string(entity)).c_str(), true), ImGuiTreeNodeFlags_DefaultOpen)
		{
			const std::string nameID = "##" + std::to_string(entity) + "n";
			std::string name;
			name.resize(32);
			name = trans.name;
			strcpy_s(input, name.data());
			if (ImGui::InputText(nameID.c_str(), input, sizeof(char) * 32, ImGuiInputTextFlags_AlwaysInsertMode))
			{
				name = std::string(input);
				if (strlen(name.data()) != 0)
					trans.SetName(name);
			}

			ImGui::Text("Position");
			const std::string positionID = "##" + std::to_string(entity) + "p";
			float pos[3] = { trans.localPosition.x, trans.localPosition.y, trans.localPosition.z };
			ImGui::DragFloat3(positionID.c_str(), pos, 0.05f);
			trans.SetPosition({ pos[0], pos[1], pos[2] });

			ImGui::Text("Rotation");
			const std::string rotationID = "##" + std::to_string(entity) + "r";
			if (currentRotationEntity != entity)
			{
				currentRotationEntity = entity;
			}
			float rot[3] = { trans.editorRotation.x, trans.editorRotation.y, trans.editorRotation.z };
			if (ImGui::DragFloat3(rotationID.c_str(), rot, 0.05f))
			{
				float x, y, z;
				x = rot[0] - trans.editorRotation.x;
				y = rot[1] - trans.editorRotation.y;
				z = rot[2] - trans.editorRotation.z;

				const glm::quat q1 = glm::angleAxis(glm::radians(x), glm::vec3(1.0, 0.0, 0.0));
				const glm::quat q2 = glm::angleAxis(glm::radians(y), glm::vec3(0.0, 1.0, 0.0));
				const glm::quat q3 = glm::angleAxis(glm::radians(z), glm::vec3(0.0, 0.0, 1.0));
				glm::quat newDirection = q1 * q2 * q3;
				trans.SetEditorRotation(glm::vec3(rot[0], rot[1], rot[2]));
				newDirection = glm::normalize(newDirection);

				//if (std::fabs(x) > 0 && std::fabs(y) > 0 && std::fabs(z) > 0)
				//{
				if(worldRotation)
					trans.SetRotation(newDirection * trans.localRotation);
				else
					trans.SetRotation(trans.localRotation * newDirection);
				//}

				//glm::quat finalRotation = worldRotation ? trans.rotation * newDirection : trans.localRotation * newDirection;
				//trans.SetRotation(finalRotation);
				memcpy_s(currentEulers, sizeof(currentEulers), rot, sizeof(rot));
			}

			ImGui::Text("Scale");
			const std::string scaleID = "##" + std::to_string(entity) + "s";
			float scale[3] = { trans.localScale.x, trans.localScale.y, trans.localScale.z };
			ImGui::DragFloat3(scaleID.c_str(), scale, 0.05f);
			trans.SetScale({ scale[0], scale[1], scale[2] });
		}
#pragma endregion

#pragma region Material Component
		if (p_node != m_engine->roots[static_cast<uint8_t>(SceneManager::CurrentScene())])
		{
			if (m_engine->HasComponent<ModelRS>(entity))
			{
				ImGui::Image(m_materialIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::SameLine();
				auto& mat = m_engine->GetComponent<ModelRS>(entity).Material();
				if (ImGui::CollapsingHeader(("Material##" + std::to_string(entity) + "material").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					float color[3] = { mat.color.x, mat.color.y, mat.color.z };
					ImGui::ColorEdit3(("Color##" + std::to_string(entity) + "material").c_str(), color);
					mat.SetColor(glm::vec4(color[0], color[1], color[2], 1));

					float specular[3] = { mat.specular.x, mat.specular.y, mat.specular.z };
					ImGui::ColorEdit3(("Specular##" + std::to_string(entity) + "material").c_str(), specular);
					mat.SetSpecular(glm::vec4(specular[0], specular[1], specular[2], 1));

					float roughness = mat.roughness;
					ImGui::DragFloat(("Roughness##" + std::to_string(entity) + "material").c_str(), &roughness, 0.05f, 0.0f, 1.0f);
					mat.SetRoughness(roughness);

					float ior = mat.ior;
					ImGui::DragFloat(("IOR##" + std::to_string(entity) + "material").c_str(), &ior, 0.05f, 0.0f, 2.0f);
					mat.SetIOR(ior);

					float emissive[4] = { mat.emissive.x, mat.emissive.y, mat.emissive.z };
					ImGui::ColorEdit3(("Emissive##" + std::to_string(entity) + "material").c_str(), emissive);
					mat.SetEmissive(glm::vec4(emissive[0], emissive[1], emissive[2], 1));

					int materialType = mat.type;
					std::vector<std::string> types{ "NONE", "BLINN PHONG", "SPECULAR", "REFRACTION", "EMISSIVE", "GGX" };

					std::string typeItem = types[materialType];
					int materialSelected = materialType;
					if (ImGui::BeginCombo(("Type ##" + std::to_string(entity) + "material").c_str(), typeItem.c_str()))
					{
						for (int n = 0; n < types.size(); n++)
						{
							const bool is_selected = (typeItem == types[n]);
							if (ImGui::Selectable(types[n].c_str(), is_selected))
							{
								typeItem = types[n];
								materialSelected = n;
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					mat.SetType(materialSelected);


					std::string textureItem = mat.texName;
					std::string texturePath = mat.texPath;
					ImGui::Text("Texture");
					std::vector<std::string>::iterator it = std::find(m_previewTexturesName.begin(), m_previewTexturesName.end(), textureItem);
					if (it != m_previewTexturesName.end())
					{
						auto index = std::distance(m_previewTexturesName.begin(), it);
						ImGui::Image(m_previewTextures[index], ImVec2(64, 64));
					}
					ImGui::SameLine();
					if (ImGui::BeginCombo(("##Texture" + std::to_string(entity) + "material").c_str(), textureItem.c_str()))
					{
						//if()
						for (auto& textureData : m_texturesData)
						{
							const bool is_selected = (textureItem == textureData.first);
							std::vector<std::string>::iterator tex = std::find(m_previewTexturesName.begin(), m_previewTexturesName.end(), textureData.first);
							if (tex != m_previewTexturesName.end())
							{
								auto index = std::distance(m_previewTexturesName.begin(), tex);
								ImGui::Image(m_previewTextures[index], ImVec2(16, 16));
							}
							ImGui::SameLine();
							if (ImGui::Selectable(textureData.first.c_str(), is_selected))
							{
								textureItem = textureData.first;
								texturePath = textureData.second;
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					mat.SetTextureID(textureItem, texturePath);

					std::string normalMapItem = mat.normName;
					std::string normalMapPath = mat.normPath;
					ImGui::Text("Normal Map");
					std::vector<std::string>::iterator itn = std::find(m_previewTexturesName.begin(), m_previewTexturesName.end(), normalMapItem);
					if (itn != m_previewTexturesName.end())
					{
						auto index = std::distance(m_previewTexturesName.begin(), itn);
						ImGui::Image(m_previewTextures[index], ImVec2(64, 64));
					}
					else
					{
						ImGui::Image(m_previewTextures[2], ImVec2(64, 64));
					}

					ImGui::SameLine();
					if (ImGui::BeginCombo(("##" + std::to_string(entity) + "material").c_str(), normalMapItem.c_str()))
					{
						for (auto& normalMapData : m_normalMapsData)
						{
							const bool is_selected = (normalMapItem == normalMapData.first);
							std::vector<std::string>::iterator norm = std::find(m_previewTexturesName.begin(), m_previewTexturesName.end(), normalMapData.first);
							if (norm != m_previewTexturesName.end())
							{
								auto index = std::distance(m_previewTexturesName.begin(), norm);
								ImGui::Image(m_previewTextures[index], ImVec2(16, 16));
							}
							ImGui::SameLine();
							if (ImGui::Selectable(normalMapData.first.c_str(), is_selected))
							{
								normalMapItem = normalMapData.first;
								normalMapPath = normalMapData.second;
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					mat.SetNormalMapID(normalMapItem, normalMapPath);
				}
			}
		}
#pragma endregion
#pragma region RigidBody Component
		if (m_engine->HasComponent<RigidBody>(entity))
		{
			if (p_node != m_engine->roots[static_cast<uint8_t>(SceneManager::CurrentScene())])
			{
				auto& rb = m_engine->GetComponent<RigidBody>(entity);

				ImGui::Image(m_rigidbodyIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::SameLine();
				if (ImGui::CollapsingHeader(("RigidBody##" + std::to_string(entity) + "rigidbody").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					bool useGravity = rb.UseGravity();
					ImGui::Checkbox(("UseGravity##" + std::to_string(entity) + "rb").c_str(), &useGravity);
					rb.EnableGravity(useGravity);

					float mass = rb.Mass();
					ImGui::DragFloat(("Mass##" + std::to_string(entity) + "rb").c_str(), &mass);
					rb.SetMass(mass);

					float size[3] = { rb.ShapeSizeX(), rb.ShapeSizeY(), rb.ShapeSizeZ() };
					ImGui::DragFloat3(("Collider Size##" + std::to_string(entity) + "rb").c_str(), size);
					rb.SetShapeSize(size[0], size[1], size[2]);


					if (ImGui::ImageButton(m_trashButton, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0)))
					{
						m_engine->m_physicsEngine.DeleteActor(&SceneManager::GetComponent<RigidBody>(entity).GetRigidBody());
						m_engine->RemoveComponent<RigidBody>(entity);
					}
				}
			}
		}
#pragma endregion
#pragma region LightSource Component
		if (m_engine->HasComponent<LightSource>(entity))
		{
			if (p_node != m_engine->roots[static_cast<uint8_t>(SceneManager::CurrentScene())])
			{
				ImGui::Image(m_lightIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::SameLine();
				auto& light = m_engine->GetComponent<LightSource>(entity);

				if (ImGui::CollapsingHeader(("Light##" + std::to_string(entity) + "light").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					float color[3] = { light.color.x , light.color.y, light.color.z };
					ImGui::ColorEdit3(("Color##" + std::to_string(entity) + "light").c_str(), color);
					light.color = glm::vec4(color[0], color[1], color[2], light.color.w);

					float intensity = light.color.w;
					ImGui::DragFloat(("Intensity##" + std::to_string(entity) + "light").c_str(), &intensity, 0.1f, 0.0f, 10000.0f);
					light.color.w = intensity;

					int type = light.lightType;
					std::vector<std::string> types{ "POINT LIGHT", "DIRECTIONNAL LIGHT" };

					if (light.lightType == 1)
					{
						float dir[3] = { light.direction.x , light.direction.y, light.direction.z };
						ImGui::DragFloat3(("Direction##" + std::to_string(entity) + "light").c_str(), dir, 0.02f, -1.0f, 1.0f);
						light.direction = glm::vec4(dir[0], dir[1], dir[2], light.direction.w);
					}

					std::string typeItem = types[type];
					int lightTypeSelected = type;
					if (ImGui::BeginCombo(("Type ##" + std::to_string(entity) + "light").c_str(), typeItem.c_str()))
					{
						for (int n = 0; n < types.size(); n++)
						{
							const bool is_selected = (typeItem == types[n]);
							if (ImGui::Selectable(types[n].c_str(), is_selected))
							{
								typeItem = types[n];
								lightTypeSelected = n;
							}
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					light.lightType = static_cast<LIGHT_TYPE>(lightTypeSelected);
				}
			}
		}
#pragma endregion
	}
	ImGui::End();
}

void OgEngine::Editor::PrepareIcons()
{
	m_playIcon = AddIcon("Resources/textures/internal/play_icon.png");
	m_pauseIcon = AddIcon("Resources/textures/internal/pause_icon.png");
	m_previewError = AddIcon("Resources/textures/internal/preview_error.png");
	m_previewDir = AddIcon("Resources/textures/internal/preview_dir.png");
	m_backButton = AddIcon("Resources/textures/internal/back.png");
	m_previewModel = AddIcon("Resources/textures/internal/preview_model.png");
	m_localAxisButton = AddIcon("Resources/textures/internal/local.png");
	m_globalAxisButton = AddIcon("Resources/textures/internal/global.png");
	m_trashButton = AddIcon("Resources/textures/internal/trash.png");
	m_transformIcon = AddIcon("Resources/textures/internal/transform.png");
	m_materialIcon = AddIcon("Resources/textures/internal/material.png");
	m_lightIcon = AddIcon("Resources/textures/internal/light.png");
	m_rigidbodyIcon = AddIcon("Resources/textures/internal/rigidbody.png");
}

void OgEngine::Editor::DestroyObject(SceneNode* p_node) const
{
	if (p_node == nullptr)
	{
		return;
	}

	if (SceneManager::HasComponent<RigidBody>(p_node->GetEntity()))
	{
		m_engine->m_physicsEngine.DeleteActor(&SceneManager::GetComponent<RigidBody>(p_node->GetEntity()).GetRigidBody());
	}

	m_engine->DestroyEntityNode(p_node);
	m_engine->inspectorNode = nullptr;
}

void OgEngine::Editor::PrepareImGUIFrame() const
{
	if (m_engine->m_vulkanContext->IsRaytracing())
		m_engine->m_vulkanContext->GetRTPipeline()->InitImGuiFrame();
	else
		m_engine->m_vulkanContext->GetRSPipeline()->PrepareIMGUIFrame();
}

void OgEngine::Editor::DrawEditor() const
{
	if (m_engine->m_vulkanContext->IsRaytracing())
		m_engine->m_vulkanContext->GetRTPipeline()->SetupEditor();
	else
		m_engine->m_vulkanContext->GetRSPipeline()->DrawEditor();
}

void OgEngine::Editor::RenderUI() const
{
	if (m_engine->m_vulkanContext->IsRaytracing())
		m_engine->m_vulkanContext->GetRTPipeline()->RenderEditor();
	else
		m_engine->m_vulkanContext->GetRSPipeline()->DrawUI();
}

void OgEngine::Editor::DisplayFileBrowser()
{
	ImGui::Begin("Browser");
	{
		if (ImGui::ImageButton(m_backButton, ImVec2(16, 16)))
		{
			m_path = m_path.parent_path();
		}
		ImGui::SameLine();
		ImGui::Text(m_path.string().c_str());
		ImGui::Checkbox("Show all files", &showAllFiles);

		int i = 0;
		int currentY = static_cast<int>(ImGui::GetCursorPosY());
		for (const auto& file : std::filesystem::directory_iterator(m_path))
		{
			const int spacing = (76 * i) + 10;
			std::string fileName = file.path().filename().string();
			ImGui::SetCursorPosX(static_cast<float>(spacing));

			if (std::filesystem::is_directory(file))
			{
				ImGui::PushID(file.path().string().c_str());

				if (ImGui::ImageButton(m_previewDir, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0), 0))
				{
					m_path = file;
				}
				ImGui::PopID();
				ShowFileName(spacing, currentY, fileName.c_str(), i);

			}
			else if (file.path().extension() == ".png" || file.path().extension() == ".jpg" ||
				file.path().extension() == ".tga")
			{
				auto it = std::find(m_previewTexturesName.begin(), m_previewTexturesName.end(), file.path().filename().string());
				if (it != m_previewTexturesName.end())
				{
					const std::uint64_t index = it - m_previewTexturesName.begin();

					if (m_selectedFile == file)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.000f, 1.000f, 1.000f, 0.356f));

						if (ImGui::ImageButton(m_previewTextures[index], ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
							m_selectedFile = file;


						ImGui::PopStyleColor();
					}
					else
					{
						if (ImGui::ImageButton(m_previewTextures[index], ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0)))
							m_selectedFile = file;
					}

					if (ImGui::IsItemHovered() && ImGui::IsItemClicked(1))
					{
						m_selectedFile = file;
						ImGui::OpenPopup("ButtonAddTexture##");
					}
				}
				else
				{
					std::cout << "Added Icon: " << file.path().string().c_str() << '\n';
					std::string filePath = file.path().string();
					std::replace(filePath.begin(), filePath.end(), '\\', '/');

					m_previewTextures.push_back(AddIcon(filePath.c_str()));
					m_previewTexturesPaths.push_back(file.path().string());
					m_previewTexturesName.push_back(file.path().filename().string());

					ImGui::Image(m_previewTextures.back(), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
				}

				ShowFileName(spacing, currentY, fileName.c_str(), i);
			}
			else if (file.path().extension() == ".obj" || file.path().extension() == ".fbx" || file.path().extension() == ".gltf")
			{
				ImGui::ImageButton(m_previewModel, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

				if (ImGui::IsItemHovered() && ImGui::IsItemClicked(1))
				{
					ImGui::OpenPopup("ButtonAddModel##");
					m_selectedFile = file;
				}
				ShowFileName(spacing, currentY, fileName.c_str(), i);
			}
			else if (showAllFiles)
			{
				ImGui::Image(m_previewError, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
				ShowFileName(spacing, currentY, fileName.c_str(), i);
			}


			if (static_cast<float>(spacing) + 128.0f >= ImGui::GetWindowSize().x)
			{
				currentY += 128;
				i = 0;
			}
			ImGui::SetCursorPosY(static_cast<float>(currentY));

		}

		if (ImGui::BeginPopup("ButtonAddModel##"))
		{
			if (ImGui::Button("Load model"))
			{
				std::string path = m_selectedFile.path().string();
				std::replace(path.begin(), path.end(), '\\', '/');
				const std::string filename = m_selectedFile.path().filename().string();

				if (ResourceManager::Get<Mesh>(filename) == nullptr)
				{
					ResourceManager::Add<Mesh>(path);
					ResourceManager::WaitForAll();

					if (ResourceManager::Get<Mesh>(filename) != nullptr)
						m_modelNames.push_back(filename);
					else
						std::cerr << "Failed to load model.\n";
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("ButtonAddTexture##"))
		{
			if (ImGui::Button("Load as texture"))
			{
				std::string path = m_selectedFile.path().string();
				std::replace(path.begin(), path.end(), '\\', '/');
				ResourceManager::Add<Texture>(path);
				ResourceManager::WaitForAll();

				if (ResourceManager::Get<Texture>(m_selectedFile.path().filename().string()) != nullptr)
				{
					m_engine->AddTexture(m_selectedFile.path().filename().string(), TEXTURE_TYPE::TEXTURE);

					m_texturesData.emplace_back(m_selectedFile.path().filename().string(), path);
				}
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Load as normal map"))
			{
				std::string path = m_selectedFile.path().string();
				std::replace(path.begin(), path.end(), '\\', '/');
				ResourceManager::Add<Texture>(path);
				ResourceManager::WaitForAll();

				if (ResourceManager::Get<Texture>(m_selectedFile.path().filename().string()) != nullptr)
				{
					m_engine->AddTexture(m_selectedFile.path().filename().string(), TEXTURE_TYPE::NORMAL);

					m_normalMapsData.emplace_back(m_selectedFile.path().filename().string(), path);
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

void OgEngine::Editor::ShowFileName(const int p_spacing, const int p_y, const char* p_fileName, int& p_iteration)
{
	ImGui::SetCursorPosX(static_cast<float>(p_spacing + 10));
	ImGui::SetCursorPosY(static_cast<float>(p_y + 76));

	ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 64);
	ImGui::Text(p_fileName, 64);
	ImGui::PopTextWrapPos();

	p_iteration++;
}


ImTextureID OgEngine::Editor::AddIcon(const char* p_path) const
{
	ResourceManager::Add<Texture>(p_path);
	ResourceManager::WaitForAll();

	if (m_engine->m_vulkanContext->IsRaytracing())
	{
		ImTextureID id = m_engine->m_vulkanContext->GetRTPipeline()->AddUITexture(p_path);
		return id;
	}
	else
	{
		ImTextureID id = m_engine->m_vulkanContext->GetRSPipeline()->AddUITexture(p_path);
		return id;
	}
}

void OgEngine::Editor::Run()
{
	double   previousTime = m_engine->m_vulkanContext->TimeOfContext();
	double   currentTime = 0.0;
	uint64_t frameCount = 0u;

	float dt = 0.0f;

	Vector2D pos;
	m_texturesData.emplace_back("default.png", "Resources/textures/default.png");
	m_texturesData.emplace_back("error.png", "Resources/textures/error.png");
	m_normalMapsData.emplace_back("NONE", "NONE");

	m_previewTextures.push_back(AddIcon("Resources/textures/default.png"));
	m_previewTexturesPaths.emplace_back("Resources/textures/default.png");
	m_previewTexturesName.emplace_back("default.png");
	m_previewTextures.push_back(AddIcon("Resources/textures/error.png"));
	m_previewTexturesPaths.emplace_back("Resources/textures/error.png");
	m_previewTexturesName.emplace_back("error.png");
	m_previewTextures.push_back(AddIcon("Resources/textures/errorNormal.png"));
	m_previewTexturesPaths.emplace_back("Resources/textures/errorNormal.png");
	m_previewTexturesName.emplace_back("errorNormal.png");

	while (!m_engine->m_vulkanContext->WindowShouldClose() || !m_engine->m_vulkanContext->IsRendering())
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		currentTime = m_engine->m_vulkanContext->TimeOfContext();
		m_engine->m_vulkanContext->PollEvents();

		pos = OgEngine::InputManager::CursorPosition();
		frameCount++;
		if (currentTime - previousTime >= 1.0)
		{
			if (SceneManager::CurrentScene() == Scene::PLAY_SCENE)
			{
				m_engine->m_vulkanContext->ChangeWindowTitle("Omega - Playing..., FPS:", frameCount);
			}
			else if (SceneManager::CurrentScene() == Scene::EDITOR_SCENE)
			{
				m_engine->m_vulkanContext->ChangeWindowTitle("Omega - Editor FPS:", frameCount);
			}
			frameCount = 0u;
			previousTime = currentTime;

		}
		m_engine->Run(dt);
		UpdateEditor(dt);
		m_engine->Display();

		auto stopTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
	}

	if (m_engine->m_vulkanContext)
		m_engine->m_vulkanContext->SetRenderingLoop(false);
}

std::string OgEngine::Editor::GetFileName(const std::string& p_filename)
{
	auto sep = '/';

#ifdef _WIN32
	sep = '\\';
#endif

	const size_t separatorIndex = p_filename.rfind(sep, p_filename.length());
	if (separatorIndex != std::string::npos) {
		return(p_filename.substr(separatorIndex + 1, p_filename.length() - separatorIndex));
	}

	return std::string("");
}

std::string OgEngine::Editor::TrimName(std::string p_name) const
{
	p_name.erase(p_name.begin(), std::find_if(p_name.begin(), p_name.end(), [](const int p_ch)
		{
			return !std::isspace(p_ch);
		}));

	return p_name;
}

bool OgEngine::Editor::HasSpecialChar(const std::string& p_stringToCheck) const
{
	return std::find_if(p_stringToCheck.begin(), p_stringToCheck.end(),
		[](const char p_character)
		{
			return !(isalnum(p_character) || p_character == '_' || p_character == '.' || p_character == '-');
		}) != p_stringToCheck.end();
}