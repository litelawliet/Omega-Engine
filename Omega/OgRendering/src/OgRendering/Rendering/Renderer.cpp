#include <OgRendering/Rendering/Renderer.h>

#include <cassert>

#ifdef VULKAN_API
OgEngine::VulkanContext* OgEngine::Renderer::m_context = nullptr;
#endif

void OgEngine::Renderer::Run()
{
    m_context->MainLoop();
}

void OgEngine::Renderer::InitVkRenderer(const int p_width, const int p_height, const char* p_name)
{
#ifdef VULKAN_API
    assert(m_context == nullptr);

    m_context = new OgEngine::VulkanContext();

    assert(m_context != nullptr);
    
    m_context->InitWindow(p_width, p_height, p_name, true);
    m_context->InitAPI();
    //Print context creation successful with Debug Tool

#endif
}

void OgEngine::Renderer::DestroyVkRenderer()
{
	if (m_context)
	{
        m_context->DestroyContext();
        delete m_context;
        m_context = nullptr;
	}
}

OgEngine::VulkanContext* OgEngine::Renderer::GetVkContext()
{
    return m_context;
}
