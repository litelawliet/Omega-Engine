#pragma once
#include <OgCore/Managers/SceneManager.h>

template<typename T>
inline void OgEngine::Core::AddComponent(const Entity p_entity, T p_component) const
{
	SceneManager::AddComponent(p_entity, p_component);
}

template<typename T>
inline T& OgEngine::Core::GetComponent(const Entity p_entity)
{
	return SceneManager::GetComponent<T>(p_entity);
}

template<typename T>
inline void OgEngine::Core::RemoveComponent(const Entity p_entity) const
{
	SceneManager::RemoveComponent<T>(p_entity);
}

template<typename T>
inline bool OgEngine::Core::HasComponent(const Entity p_entity) const
{
	return SceneManager::HasComponent<T>(p_entity);
}
