#pragma once
#include <memory>
#include <OgCore/Entities/Entity.h>
#include <OgCore/Managers/ComponentManager.h>
#include <OgCore/Managers/EntityManager.h>
#include <OgCore/Managers/SystemManager.h>

namespace OgEngine
{
	struct Transform;

	class SceneManager
	{
	public:
#pragma region EntitiesMethods
		[[nodiscard]] static Entity CreateEntity();
		static void                 DestroyEntity(const Entity p_entity);
#pragma endregion
#pragma region ComponentsMethods
		template <typename T>
		static void RegisterComponent();

		template <typename T>
		static void AddComponent(const Entity p_entity, T p_component);

		template <typename T>
		static void RemoveComponent(const Entity p_entity);

		template <typename T>
		static T& GetComponent(const Entity p_entity);

		template <typename T>
		static ComponentType GetComponentType();
#pragma endregion
#pragma region Systems methods
		template <typename T>
		static std::shared_ptr<T> RegisterSystem();

		template <typename T>
		static void SetSystemSignature(const Signature p_signature);
#pragma endregion
	private:
		static std::unique_ptr<ComponentManager> m_componentManager;
		static std::unique_ptr<EntityManager>    m_entityManager;
		static std::unique_ptr<SystemManager>    m_systemManager;
	};
}

#include <OgCore/Managers/SceneManager.inl>
