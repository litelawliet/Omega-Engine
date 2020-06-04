#pragma once
#include <OgCore/Export.h>
#include <OgCore/Entities/Types.h>
#include <queue>
#include <array>

namespace OgEngine
{
	class CORE_API EntityManager
	{
	public:
		/**
		 * @brief Constructor. This constructor is important since he will allocate and handle up to 5000 entities.
		 * @note There is one EntityManager by SceneManager object. You might want to have several SceneManager if you have multiple scenes in your game.
		 */
		EntityManager();

		/**
		 * @brief Create an entity
		 * @note The method will fail if you try to create more entity than the maximum supported AND alive entities (5000)
		 */
		Entity CreateEntity();

		/**
		 * @brief Destroy an entity
		 * @param p_entity The entity to destroy
		 * @note The method will fail if the entity doesn't exist
		 */
		void DestroyEntity(Entity p_entity);

		/**
		 * @brief Set a signature to an entity
		 * @param p_entity The entity to change a signature
		 * @param p_signature The new signature of an entity
		 * @note A signature is a bitfield where each bit correspond to a specific component, therefore we can keep track of existing components of an entity using a bitfield  The method will fail if you exceeded the maxinum number of entity supported (5000)
		 */
		void SetSignature(Entity p_entity, Signature p_signature);

		/**
		 * @brief Return the signature of
		 */
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
