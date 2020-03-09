#pragma once
#include <memory>
#include <unordered_map>
#include <OgCore/Entities/Entity.h>
#include <OgCore/Systems/System.h>

namespace OgEngine
{
	class SystemManager
	{
	public:
		template<typename T>
		std::shared_ptr<T> RegisterSystem();

		template<typename T>
		void SetSignature(Signature p_signature);

		void EntityDestroyed(Entity p_entity);

		void EntitySignatureChanged(Entity p_entity, Signature p_entitySignature);

	private:
		// Map from system type string poitner to a signature
		std::unordered_map<const char*, Signature> m_signatures{};

		// Map from system type string pointer to a system pointer
		std::unordered_map<const char*, std::shared_ptr<System>> m_systems{};
	}; 
}

#include <OgCore/Managers/SystemManager.inl>