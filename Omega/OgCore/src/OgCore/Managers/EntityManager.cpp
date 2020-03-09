#include <OgCore/Managers/EntityManager.h>
#include <cassert>

OgEngine::EntityManager::EntityManager()
{
	for (Entity entity = 0u; entity < MAX_ENTITIES; ++entity)
	{
		m_availableEntities.push(entity);
	}
}

OgEngine::Entity OgEngine::EntityManager::CreateEntity()
{
	assert(m_livingEntityCount < MAX_ENTITIES && "Too many entitites in existence.");

	// Take an ID from the front of the queue
	const Entity id = m_availableEntities.front();
	m_availableEntities.pop();
	++m_livingEntityCount;

	return id;
}

void OgEngine::EntityManager::DestroyEntity(const Entity p_entity)
{
	assert(p_entity < MAX_ENTITIES && "Entity out of range.");

	// Invalidate the destroyed entity's signature
	m_signatures[p_entity].reset();

	// Put the destroyed ID at the back of the queue
	m_availableEntities.push(p_entity);
	--m_livingEntityCount;
}

void OgEngine::EntityManager::SetSignature(Entity p_entity, Signature p_signature)
{
	assert(p_entity < MAX_ENTITIES && "Entity ouf of range.");

	// Put this entity's signature into the array
	m_signatures[p_entity] = p_signature;
}

OgEngine::Signature OgEngine::EntityManager::GetSignature(Entity p_entity)
{
	assert(p_entity < MAX_ENTITIES && "Entity out of range.");

	// Get this entity's signature from the array
	return m_signatures[p_entity];
}
