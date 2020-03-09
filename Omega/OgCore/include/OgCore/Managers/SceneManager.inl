#pragma once
#include <OgCore/Components/Transform.h>
#include <OgCore/Components/ModelRS.h>
#include <OgCore/Components/LightSource.h>

template <typename T>
void OgEngine::SceneManager::RegisterComponent()
{
	m_componentManager->RegisterComponent<T>();
}

template <typename T>
void OgEngine::SceneManager::AddComponent(Entity p_entity, T p_component)
{
	m_componentManager->AddComponent<T>(p_entity, p_component);

	if (typeid(T) == typeid(OgEngine::ModelRS))
	{
		GetComponent<ModelRS>(p_entity).SetLocalTransform(GetComponent<Transform>(p_entity));
	}
	else if (typeid(T) == typeid(OgEngine::LightSource))
	{
		GetComponent<LightSource>(p_entity).SetLocalTransform(GetComponent<Transform>(p_entity));
	}
	
	auto signature = m_entityManager->GetSignature(p_entity);
	signature.set(m_componentManager->GetComponentType<T>(), true);
	m_entityManager->SetSignature(p_entity, signature);

	m_systemManager->EntitySignatureChanged(p_entity, signature);
}

template <typename T>
void OgEngine::SceneManager::RemoveComponent(Entity p_entity)
{
	m_componentManager->RemoveComponent<T>(p_entity);

	auto signature = m_entityManager->GetSignature(p_entity);
	signature.set(m_componentManager->GetComponentType<T>(), false);
	m_entityManager->SetSignature(p_entity, signature);

	m_systemManager->EntitySignatureChanged(p_entity, signature);
}

template <typename T>
T& OgEngine::SceneManager::GetComponent(Entity p_entity)
{
	return m_componentManager->GetComponent<T>(p_entity);
}

template <typename T>
OgEngine::ComponentType OgEngine::SceneManager::GetComponentType()
{
	return m_componentManager->GetComponentType<T>();
}

template <typename T>
std::shared_ptr<T> OgEngine::SceneManager::RegisterSystem()
{
	return m_systemManager->RegisterSystem<T>();
}

template <typename T>
void OgEngine::SceneManager::SetSystemSignature(Signature p_signature)
{
	m_systemManager->SetSignature<T>(p_signature);
}


