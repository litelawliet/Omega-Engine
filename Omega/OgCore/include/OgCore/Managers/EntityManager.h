#pragma once
#include <OgCore/Entities/Entity.h>
#include <queue>
#include <array>

namespace OgEngine
{
	class EntityManager
	{
	public:
		EntityManager();

		Entity CreateEntity();

		void DestroyEntity(Entity p_entity);

		void SetSignature(Entity p_entity, Signature p_signature);

		Signature GetSignature(Entity p_entity);

	private:
		// Queue of unused entity IDs
		std::queue<Entity> m_availableEntities{};

		// Array of signatures where the index corresponds to the entity ID
		std::array<Signature, MAX_ENTITIES> m_signatures{};

		// Total living entities - used to keep limits on how many exist
		std::uint64_t m_livingEntityCount{};
	};
}
