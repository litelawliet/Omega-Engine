#pragma once
#include <OgCore/Entities/Entity.h>
#include <cassert>
#include <unordered_map>


namespace OgEngine
{
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity p_entity) = 0;
	};

	template <typename T>
	class ComponentArray : public IComponentArray
	{
	public:
		void InsertData(Entity p_entity, T p_component);
		void RemoveData(Entity p_entity);

		T& GetData(Entity p_entity);

		void EntityDestroyed(Entity p_entity) override;

	private:
		// The packed array of components (of generic type T),
		// set to a specified maximum amount, matching the maximum mumber
		// // of entities allowed to exists simultaneously, so that each entity
		// // has a unique spot
		std::array<T, MAX_ENTITIES> m_componentArray;

		// Map from an entity ID to an array index
		std::unordered_map<Entity, size_t> m_entityToIndexMap;

		// Map from an array index to an entity ID
		std::unordered_map<size_t, Entity> m_indexToEntityMap;

		// Total size of valid entires in the array.
		size_t m_size{ 0u };
	};
}

#include <OgCore/Components/ComponentArray.inl>
