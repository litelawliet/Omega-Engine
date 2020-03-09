#pragma once

#include <GPM/GPM.h>

namespace OgEngine
{
	struct Transform;

	struct LightSource
	{
		alignas(16) GPM::Vector3F position{};
		alignas(16) GPM::Vector4F ambient;
		alignas(16) GPM::Vector4F diffuse;
		alignas(16) GPM::Vector4F specular;

		LightSource();

		void SetLocalTransform(Transform& p_transform);

	private:
		Transform* m_lightTransform{};
	};
}