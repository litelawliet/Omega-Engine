#pragma once

#include <iostream>
template<typename T>
void OgEngine::ComponentManager::RegisterComponent()
{
	const char* typeName = typeid(T).name();

	assert(m_componentTypes.find(typeName) == m_componentTypes.end() && "Registering component type more than once.");

	// Add this component type to the component type map
	m_componentTypes.insert({ typeName, m_nextComponentType });

	// Create a ComponentArray pointer and add it to the component arrays map
	m_componentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });

	// Increment the value so that the next componenet registered will be different
	++m_nextComponentType;
}

template <typename T>
OgEngine::ComponentType OgEngine::ComponentManager::GetComponentType()
{
	const char* typeName = typeid(T).name();

	assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "Component not registered before use.");

	// Return this component's type - used for creating signatures
	return m_componentTypes[typeName];
}

template <typename T>
void OgEngine::ComponentManager::AddComponent(Entity p_entity, T p_component)
{
	// Add a component to the array for an entity
	GetComponentArray<T>()->InsertData(p_entity, p_component);
}

template <typename T>
void OgEngine::ComponentManager::RemoveComponent(Entity p_entity)
{
	// Remove a component from the array for an entity
	GetComponentArray<T>()->RemoveData(p_entity);
}

template <typename T>
T& OgEngine::ComponentManager::GetComponent(Entity p_entity)
{
	// Get a reference to a component from the array for an entity
	return GetComponentArray<T>()->GetData(p_entity);
}

template <typename T>
std::shared_ptr<OgEngine::ComponentArray<T>> OgEngine::ComponentManager::GetComponentArray()
{
	const std::string typeName = typeid(T).name();

	assert(m_componentTypes.find(typeName) != m_componentTypes.end() && "Component not registered before use.");

	return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays[typeName]);
}

inline void OgEngine::ComponentManager::EntityDestroyed(Entity p_entity)
{
	// Notify each component array that an entity has been destroyed
	// If it has a component for that entity, it will remove it
	for (auto const& pair : m_componentArrays)
	{
		auto const& component = pair.second;

		component->EntityDestroyed(p_entity);
	}
}
