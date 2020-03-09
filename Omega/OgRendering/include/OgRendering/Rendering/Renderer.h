#pragma once
#include <OgRendering/Export.h>
#include <memory>
#include <OgRendering/Rendering/VulkanContext.h>

#define VULKAN_API

namespace OgEngine
{
	class RENDERING_API Renderer final
	{

#ifdef NO_API
	public:
		Renderer() = delete;
		~Renderer() = delete;
	private:
#endif

#ifdef OPENGL_API
	public:
		Renderer() = delete;
		~Renderer() = delete;
	private:
#endif

#ifdef VULKAN_API
	public:
		Renderer() = delete;
		~Renderer() = delete;
		static void Run();
		static void InitVkRenderer(const int p_width, const int p_height, const char* p_name);
		static const std::shared_ptr<OgEngine::VulkanContext>& GetVkContext();
	private:
		static std::shared_ptr<OgEngine::VulkanContext> m_context;
#endif
	};
}