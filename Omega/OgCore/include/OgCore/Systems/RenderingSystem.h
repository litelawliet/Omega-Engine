#pragma once
#include <OgCore/Systems/System.h>
#include <memory>

namespace OgEngine
{
	class VulkanContext;

	class RenderingSystem : public System
	{
	public:
		void Init();

		void Update(const float p_dt, const std::shared_ptr<VulkanContext>& p_context);
	};
}
