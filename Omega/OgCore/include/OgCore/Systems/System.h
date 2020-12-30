#pragma once
#include <OgCore/Export.h>

#include <OgCore/Entities/Types.h>
#include <set>

namespace OgEngine
{
	class CORE_API System
	{
	public:
		/**
		 * @brief The entities that each systems holds and can manipulate through the SceneManager directives.
		 * @note Do NOT modify this field directly since it can create undefined behaviour when systems will loop over the entities. Always use a SceneManager to access and modify an entity and it's component.
		 */
		std::set<Entity> m_entities;
	};
}
