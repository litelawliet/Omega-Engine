#include <OgCore/Managers/SceneManager.h>

std::unique_ptr<OgEngine::ComponentManager> OgEngine::SceneManager::m_componentManager = std::make_unique<ComponentManager>();
std::unique_ptr<OgEngine::EntityManager> OgEngine::SceneManager::m_entityManager = std::make_unique<EntityManager>();
std::unique_ptr<OgEngine::SystemManager> OgEngine::SceneManager::m_systemManager = std::make_unique<SystemManager>();

OgEngine::Entity OgEngine::SceneManager::CreateEntity()
{
	const Entity idEntity = m_entityManager->CreateEntity();

	SceneManager::AddComponent(idEntity, Transform{});

	return idEntity;
}

void OgEngine::SceneManager::DestroyEntity(const Entity p_entity)
{
	m_entityManager->DestroyEntity(p_entity);

	m_componentManager->EntityDestroyed(p_entity);

	m_systemManager->EntityDestroyed(p_entity);
}
