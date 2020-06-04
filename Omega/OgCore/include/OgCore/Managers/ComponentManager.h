#pragma once
#include <OgCore/Export.h>
#include <OgCore/Components/ComponentArray.h>
#include <memory>

namespace OgEngine
{
	/**
	 * @brief ComponentManager is a wrapper who manipulate the ComponentArray template object
	 */
	class CORE_API ComponentManager
	{
	public:
		/**
		 * @brief Register a component into a component array
		 * @note It is very important to register all the component you will use in the program before using it. Otherwise the component will not exist and will throw an exception. 
		 */
		template<typename T>
		void RegisterComponent();

		/**
		 * @brief Return the component's type of a component array
		 * @return The component's of a component array
		 */
		template<typename T>
		ComponentType GetComponentType();

		/**
		 * @brief Add a component to an entity
		 * @param p_entity The entity to which we add a component
		 * @param p_component The component to add
		 * @note The method will fail if the entity doesn't exist or if the component already exists for this entity
		 */
		template<typename T>
		void AddComponent(Entity p_entity, T p_component);

		/**
		 * @brief Remove a component of an entity
		 * @param p_entity The entity to which we remove a component
		 * @note The method will fail if the entity doesn't exist or if the component doesn't exist for this entity
		 */
		template<typename T>
		void RemoveComponent(Entity p_entity);

		/**
		 * @brief Get a component of an entity
		 * @param p_entity The entity to which we get a component
		 * @return The component of the entity
		 * @note The method will fail if the entity doesn't exist or if the component doesn't exist for this entity
		 */
		template<typename T>
		T& GetComponent(Entity p_entity);

		/**
		 * @brief Destroy all the component of an entity in all the existing component array where the entity is referred to.
		 * @param p_entity The entity you want to remove
		 */
		inline void EntityDestroyed(Entity p_entity);

	private:
		// Map from type string pointer to a component type
		std::unordered_map<std::string, ComponentType> m_componentTypes{};

		// Map from type string pointer to a component array
		std::unordered_map<std::string, std::shared_ptr<IComponentArray>> m_componentArrays{};

		// The component type to be assigned to the next registered component - starting at 0
		ComponentType m_nextComponentType{};

		/**
		 * @brief Convenience function to get the statically casted pointer to the ComponentArray of type T
		 */
		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray();
	};
}

#include <OgCore/Managers/ComponentManager.inl>
