#include <OgRendering/Rendering/Renderer.h>

#include <cassert>
#include <iostream>

#ifdef VULKAN_API
std::shared_ptr<OgEngine::VulkanContext> OgEngine::Renderer::m_context;
#endif

void OgEngine::Renderer::Run()
{
    m_context->MainLoop();
    m_context->DestroyContext();
}

void OgEngine::Renderer::InitVkRenderer(const int p_width, const int p_height, const char* p_name)
{
#ifdef VULKAN_API
    assert(m_context == nullptr);

    m_context = std::make_shared<OgEngine::VulkanContext>();

    assert(m_context != nullptr);
    
    m_context->InitWindow(p_width, p_height, p_name, true);
    m_context->InitAPI();
#endif
}

const std::shared_ptr<OgEngine::VulkanContext>& OgEngine::Renderer::GetVkContext()
{
    return m_context;
}
