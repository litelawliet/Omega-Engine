#include <OgCore/Managers/SceneManager.h>

std::array<std::unique_ptr<OgEngine::ComponentManager>, 2> OgEngine::SceneManager::m_componentManager = { std::make_unique<ComponentManager>(), std::make_unique<ComponentManager>() };
std::array < std::unique_ptr<OgEngine::EntityManager>, 2> OgEngine::SceneManager::m_entityManager = { std::make_unique<EntityManager>(), std::make_unique<EntityManager>() };
std::array < std::unique_ptr<OgEngine::SystemManager>, 2> OgEngine::SceneManager::m_systemManager = { std::make_unique<SystemManager>(), std::make_unique<SystemManager>() };
OgEngine::Scene OgEngine::SceneManager::m_currentScene = OgEngine::Scene::EDITOR_SCENE;

void OgEngine::SceneManager::ChangeScene(const Scene& p_newScene)
{
	if (p_newScene >= Scene::COUNT)
		return;
	
	m_currentScene = p_newScene;
}

OgEngine::Scene OgEngine::SceneManager::CurrentScene()
{
	return m_currentScene;
}

OgEngine::Entity OgEngine::SceneManager::CreateEntity()
{
	const Entity idEntity = m_entityManager[static_cast<uint8_t>(m_currentScene)]->CreateEntity();

	AddComponent(idEntity, Transform{});
	const std::string _name = "GameObject" + std::to_string(idEntity);
	GetComponent<Transform>(idEntity).SetName(_name);

	return idEntity;
}

void OgEngine::SceneManager::DestroyEntity(const Entity p_entity)
{
	m_entityManager[static_cast<uint8_t>(m_currentScene)]->DestroyEntity(p_entity);

	m_componentManager[static_cast<uint8_t>(m_currentScene)]->EntityDestroyed(p_entity);

	m_systemManager[static_cast<uint8_t>(m_currentScene)]->EntityDestroyed(p_entity);
}

OgEngine::Signature OgEngine::SceneManager::GetSignature(const Entity p_entity)
{
	return m_entityManager[static_cast<uint8_t>(m_currentScene)]->GetSignature(p_entity);
}
