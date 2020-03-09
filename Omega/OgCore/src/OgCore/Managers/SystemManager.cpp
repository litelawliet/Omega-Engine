#include <OgCore/Managers/SystemManager.h>

void OgEngine::SystemManager::EntityDestroyed(Entity p_entity)
{
	// Erase a destroyed entity from all system lists
	// m_entitites is a set so no check needed
	for (auto const& pair : m_systems)
	{
		auto const& system = pair.second;

		system->m_entities.erase(p_entity);
	}
}

void OgEngine::SystemManager::EntitySignatureChanged(Entity p_entity, Signature p_entitySignature)
{
	// Notify each system that an entity's signature changed
	for (auto const& pair : m_systems)
	{
		auto const& type = pair.first;
		auto const& system = pair.second;
		auto const& systemSignature = m_signatures[type];

		// Entity signature matches system signatures - insert into set
		if ((p_entitySignature & systemSignature) == systemSignature)
		{
			system->m_entities.insert(p_entity);
		}
		// Entity signature does not match system signatures - erase from set
		else
		{
			system->m_entities.erase(p_entity);
		}
	}
}
