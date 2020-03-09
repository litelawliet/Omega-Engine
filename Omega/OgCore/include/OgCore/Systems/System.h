#pragma once
#include <OgCore/Entities/Entity.h>
#include <set>


namespace OgEngine
{
	class System
	{
	public:
		std::set<Entity> m_entities;
	};
}
