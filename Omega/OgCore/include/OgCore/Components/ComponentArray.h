#pragma once
#include <OgCore/Export.h>
#include <OgCore/Entities/Types.h>
#include <cassert>
#include <unordered_map>


namespace OgEngine
{
	class CORE_API IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity p_entity) = 0;
	};

	template <typename T>
	class ComponentArray : public IComponentArray
	{
	public:
		/**
		 * @brief Insert a component to an entity
		 * @param p_entity The entity to add a component
		 * @param p_component The component to add
		 * @note The method will fail if a component already exists on the entity or if the entity doesn't exist
		 */
		void InsertData(Entity p_entity, T p_component);

		/**
		 * @brief Remove a component to an entity
		 * @param p_entity The entity to remove from a component
		 * @note The method will fail if a component doesn't exist on the entity or if the entity doesn't exist
		 */
		void RemoveData(Entity p_entity);

		/**
		 * @brief Give a reference to a component of an entity
		 * @param p_entity The entity to look at
		 * @return The component of the entity in parameter
		 * @note The method will fail if a component doesn't exist on the entity or if the entity doesn't exist
		 */
		T& GetData(Entity p_entity);

		/**
		 * @brief Will destroy each component of the entity in parameter
		 * @param p_entity The entity removed
		 */
		void EntityDestroyed(Entity p_entity) override;

	private:
		// The packed array of components (of generic type T),
		// set to a specified maximum amount, matching the maximum number
		// // of entities allowed to exists simultaneously, so that each entity
		// // has a unique spot
		std::array<T, MAX_ENTITIES> m_componentArray;

		// Map from an entity ID to an array index
		std::unordered_map<Entity, size_t> m_entityToIndexMap;

		// Map from an array index to an entity ID
		std::unordered_map<size_t, Entity> m_indexToEntityMap;

		// Total size of valid entries in the array.
		size_t m_size{ 0u };
	};
}

#include <OgCore/Components/ComponentArray.inl>
