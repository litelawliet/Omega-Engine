#pragma once
#include <OgCore/Components/Transform.h>
#include <OgCore/Components/ModelRS.h>
#include <OgCore/Components/LightSource.h>
#include <OgCore/Components/Material.h>
#include <OgCore/Components/RigidBody.h>
#include <OgCore/Components/AScript.h>

template <typename T>
inline void OgEngine::SceneManager::RegisterComponent()
{
	m_componentManager[static_cast<uint8_t>(m_currentScene)]->RegisterComponent<T>();
}

template <typename T>
void OgEngine::SceneManager::AddComponent(Entity p_entity, T p_component)
{
	const auto indexScene = static_cast<uint8_t>(m_currentScene);
	m_componentManager[indexScene]->AddComponent<T>(p_entity, p_component);

	if (typeid(T) == typeid(OgEngine::ModelRS))
	{
		GetComponent<ModelRS>(p_entity).SetLocalTransform(GetComponent<Transform>(p_entity));
	}
	else if (typeid(T) == typeid(OgEngine::LightSource))
	{
		GetComponent<LightSource>(p_entity).SetLocalTransform(GetComponent<Transform>(p_entity));
	}
	else if (typeid(T) == typeid(OgEngine::Material))
	{
		GetComponent<Material>(p_entity).SetLocalTransform(GetComponent<Transform>(p_entity));
	}
	else if (typeid(T) == typeid(OgEngine::RigidBody))
	{
		GetComponent<RigidBody>(p_entity).SetLocalTransform(GetComponent<Transform>(p_entity));
	}
	else if (typeid(T) == typeid(OgEngine::AScript))
	{
		GetComponent<AScript>(p_entity).SetLocalTransform(GetComponent<Transform>(p_entity));
		GetComponent<AScript>(p_entity).Start();
	}

	auto signature = m_entityManager[indexScene]->GetSignature(p_entity);
	signature.set(m_componentManager[indexScene]->GetComponentType<T>(), true);
	m_entityManager[indexScene]->SetSignature(p_entity, signature);

	m_systemManager[indexScene]->EntitySignatureChanged(p_entity, signature);
}

template <typename T>
void OgEngine::SceneManager::RemoveComponent(Entity p_entity)
{
	const auto indexScene = static_cast<uint8_t>(m_currentScene);

	m_componentManager[indexScene]->RemoveComponent<T>(p_entity);

	auto signature = m_entityManager[indexScene]->GetSignature(p_entity);
	signature.set(m_componentManager[indexScene]->GetComponentType<T>(), false);
	m_entityManager[indexScene]->SetSignature(p_entity, signature);

	m_systemManager[indexScene]->EntitySignatureChanged(p_entity, signature);
}

template <typename T>
inline T& OgEngine::SceneManager::GetComponent(Entity p_entity)
{
	return m_componentManager[static_cast<uint8_t>(m_currentScene)]->GetComponent<T>(p_entity);
}

template <typename T>
OgEngine::ComponentType OgEngine::SceneManager::GetComponentType()
{
	return m_componentManager[static_cast<uint8_t>(m_currentScene)]->GetComponentType<T>();
}

template <typename T>
bool OgEngine::SceneManager::HasComponent(const OgEngine::Entity p_entity)
{
	const auto signature = m_entityManager[static_cast<uint8_t>(m_currentScene)]->GetSignature(p_entity);
	return signature.test(m_componentManager[static_cast<uint8_t>(m_currentScene)]->GetComponentType<T>());
}

template <typename T>
std::shared_ptr<T> OgEngine::SceneManager::RegisterSystem()
{
	return m_systemManager[static_cast<uint8_t>(m_currentScene)]->RegisterSystem<T>();
}

template <typename T>
void OgEngine::SceneManager::SetSystemSignature(Signature p_signature)
{
	m_systemManager[static_cast<uint8_t>(m_currentScene)]->SetSignature<T>(p_signature);
}
