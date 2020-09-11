#pragma once
#include <string>
#include <OgCore/Export.h>
#include <glm/vec4.hpp>

namespace OgEngine
{
	struct Transform;
	enum LIGHT_TYPE
	{
		POINT_TYPE = 0,
		DIRECTIONNAL_TYPE = 1
	};

	struct CORE_API LightSource
	{
	public:
		LightSource() = default;
		LightSource(const LightSource& p_other);
		LightSource(LightSource&& p_other) noexcept;

		//color of the light, light intensity is the w component of this vector to pack data easily
		glm::vec4 color{};

		//Only if DIRECTIONNAL_TYPE
		glm::vec4 direction{};

		LIGHT_TYPE lightType{ LIGHT_TYPE::POINT_TYPE };

		void SetLocalTransform(Transform& p_transform);
		[[nodiscard]] std::string Serialize(const int p_depth) const;

		inline LightSource& operator=(const LightSource& p_other);
		inline LightSource& operator=(LightSource&& p_other) noexcept;

	private:
		Transform* m_lightTransform{};

		[[nodiscard]] static std::string DepthIndent(const int p_depth);
	};
}