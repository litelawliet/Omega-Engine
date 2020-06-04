#pragma once


template<typename T>
void OgEngine::ComponentArray<T>::InsertData(const OgEngine::Entity p_entity, T p_component)
{
	assert(m_entityToIndexMap.find(p_entity) == m_entityToIndexMap.end() && "Component added to same p_entity more than once.");

	// Put new entry at end and update the maps
	size_t newIndex = m_size;
	m_entityToIndexMap[p_entity] = newIndex;
	m_indexToEntityMap[newIndex] = p_entity;
	m_componentArray[newIndex] = p_component;
	++m_size;
}

template <typename T>
void OgEngine::ComponentArray<T>::RemoveData(const OgEngine::Entity p_entity)
{
	assert(m_entityToIndexMap.find(p_entity) != m_entityToIndexMap.end() && "Removing non-existent component.");

	// Copy element at the end into deleted element's place to maintain density
	size_t indexOfRemovedEntity = m_entityToIndexMap[p_entity];
	size_t indexOfLastElement = m_size - 1;
	m_componentArray[indexOfRemovedEntity] = m_componentArray[indexOfLastElement];

	// Update map to point to moved spot
	const Entity entityOfLastElement = m_indexToEntityMap[indexOfLastElement];
	m_entityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
	m_indexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

	m_entityToIndexMap.erase(p_entity);
	m_indexToEntityMap.erase(indexOfLastElement);

	--m_size;
}

template <typename T>
T& OgEngine::ComponentArray<T>::GetData(const Entity p_entity)
{
	assert(m_entityToIndexMap.find(p_entity) != m_entityToIndexMap.end() && "Retrieving non-existant component");

	// Return a reference to the entity's component
	return m_componentArray[m_entityToIndexMap[p_entity]];
}

template <typename T>
void OgEngine::ComponentArray<T>::EntityDestroyed(const Entity p_entity)
{
	if (m_entityToIndexMap.find(p_entity) != m_entityToIndexMap.end())
	{
		// remove the entity's component if it existed
		RemoveData(p_entity);
	}
}
