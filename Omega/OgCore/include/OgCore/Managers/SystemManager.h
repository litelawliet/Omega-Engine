#pragma once
#include <OgCore/Export.h>
#include <memory>
#include <unordered_map>
#include <OgCore/Entities/Types.h>
#include <OgCore/Systems/System.h>

namespace OgEngine
{
	class CORE_API SystemManager
	{
	public:
		/**
		 * @brief Register a system, gives the records and handle of entities automatically.
		 * @return Return a shared_ptr of a valid system class.
		 * @note You need to register a system before using it even if it is a valid class. Note that a valid system is a system who inherit from the System class.
		 */
		template<typename T>
		std::shared_ptr<T> RegisterSystem();

		/**
		 * @brief Set a signature to a system
		 * @param p_signature The new signature of a system
		 * @note A signature is a bitfield where each bit correspond to a specific component defining what component a system needs in order to work properly. The method will fail if the system is not registered yet.
		 */
		template<typename T>
		void SetSignature(Signature p_signature);

		/**
		 * @brief Remove the entity from a system
		 * @param p_entity The entity you want to remove from a system
		 */
		void EntityDestroyed(Entity p_entity);

		/**
		 * @brief Notify all systems that an entity's signature has been changed. If the signature of a system and the entity still matches then the entity stays in the set, otherwise it gets removed from the set.
		 * @param p_entity The entity who change its signature
		 * @param p_entitySignature The new signature of the entity
		 */
		void EntitySignatureChanged(Entity p_entity, Signature p_entitySignature);

	private:
		// Map from system type string pointer to a signature
		std::unordered_map<const char*, Signature> m_signatures{};

		// Map from system type string pointer to a system pointer
		std::unordered_map<const char*, std::shared_ptr<System>> m_systems{};
	};
}

#include <OgCore/Managers/SystemManager.inl>