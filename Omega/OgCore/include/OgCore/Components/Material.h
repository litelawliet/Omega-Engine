#pragma once

#include <GPM/GPM.h>

#include <OgCore/Components/Transform.h>

namespace OgEngine
{
	struct Material
	{
		alignas(16) GPM::Vector3F color;
		alignas(16) float rough;
		alignas(16) bool metal;

		Material();
		
		void SetLocalTransform(Transform& p_transform);

	private:
		Transform* m_lightTransform{};
	};
}
