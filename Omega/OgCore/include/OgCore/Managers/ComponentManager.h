#pragma once
#include <OgCore/Components/ComponentArray.h>
#include <memory>

namespace OgEngine
{
	class ComponentManager
	{
	public:
		template<typename T>
		void RegisterComponent();

		template<typename T>
		ComponentType GetComponentType();

		template<typename T>
		void AddComponent(Entity p_entity, T p_component);
		
		template<typename T>
		void RemoveComponent(Entity p_entity);

		template<typename T>
		T& GetComponent(Entity p_entity);

		inline void EntityDestroyed(Entity p_entity);

	private:
		// Map from type string pointer to a component type
		std::unordered_map<const char*, ComponentType> m_componentTypes{};

		// Map from type string pointer to a component array
		std::unordered_map<const char*, std::shared_ptr<IComponentArray>> m_componentArrays{};

		// The component type to be assigned to the next registered component - starting at 0
		ComponentType m_nextComponentType{};

		// Convenience function to get the statically casted pointer to the ComponentArray of type T
		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray();
	};
}

#include <OgCore/Managers/ComponentManager.inl>
