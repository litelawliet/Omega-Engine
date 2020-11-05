#include <iostream>
#include <algorithm>
#include <array>
#include <set>

#include <OgRendering/Rendering/RaytracingPipeline.h>
#include <OgRendering/Utils/Initializers.h>
#include <vulkan/vulkan.h>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#define INDEX_RAYGEN 0
#define INDEX_MISS 1
#define INDEX_SHADOW_MISS 2
#define INDEX_CLOSEST_HIT 3
#define INDEX_SHADOW_HIT 4
#define SHADER_COUNT 5

using namespace OgEngine;


void RaytracingPipeline::CHECK_ERROR(VkResult p_result)
{
    if (p_result != VK_SUCCESS)
    {
        std::cout << p_result << '\n';
        throw std::runtime_error(("Error with Vulkan function"));
    }
}

SwapChainSupportDetails RaytracingPipeline::QuerySwapChainSupport(VkPhysicalDevice& p_gpu)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_gpu, m_vulkanDevice.surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(p_gpu, m_vulkanDevice.surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(p_gpu, m_vulkanDevice.surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(p_gpu, m_vulkanDevice.surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(p_gpu, m_vulkanDevice.surface, &presentModeCount,
            details.presentModes.data());
    }

    return details;
}
VkSurfaceFormatKHR RaytracingPipeline::ChooseSwapSurfaceFormat(
    const std::vector<struct VkSurfaceFormatKHR>& p_availableFormats)
{
    for (const auto& availableFormat : p_availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return p_availableFormats[0];
}

VkPresentModeKHR RaytracingPipeline::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& p_availablePresentModes)
{
    for (const auto& availablePresentMode : p_availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D RaytracingPipeline::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& p_capabilities) const
{
    if (p_capabilities.currentExtent.width != UINT32_MAX)
    {
        return p_capabilities.currentExtent;
    }
    else
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(p_capabilities.minImageExtent.width,
            std::min(p_capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(p_capabilities.minImageExtent.height,
            std::min(p_capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}
VkImageView RaytracingPipeline::CreateImageView(VkImage            p_image, VkFormat       p_format,
    VkImageAspectFlags p_aspectFlags, uint32_t p_mipLevels) const
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = p_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = p_format;
    viewInfo.subresourceRange.aspectMask = p_aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = p_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_vulkanDevice.logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void RaytracingPipeline::SetupSwapchain(uint32_t p_width, uint32_t p_height, bool p_vsync)
{
    const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_vulkanDevice.gpu);

    const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR   presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D         extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0u && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    m_minImageCount = imageCount;


    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_vulkanDevice.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1u;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    m_width = extent.width;
    m_height = extent.height;
    uint32_t queueFamilyIndices[] = {
        m_vulkanDevice.presentFamily.value(), m_vulkanDevice.graphicFamily.value()
    };

    if (m_vulkanDevice.graphicFamily != m_vulkanDevice.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_vulkanDevice.logicalDevice, &createInfo, nullptr, &m_swapChain.swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, &imageCount, nullptr);
    m_swapChain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, &imageCount, m_swapChain.images.data());

    m_swapChain.colorFormat = surfaceFormat.format;
    m_swapChain.extent = extent;
    m_swapChain.imageCount = imageCount;
    m_swapChain.views.resize(m_swapChain.images.size());

    for (uint32_t i = 0; i < m_swapChain.images.size(); ++i)
    {
        m_swapChain.views[i] = CreateImageView(m_swapChain.images[i], m_swapChain.colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1u);
    }
}
VkCommandBuffer RaytracingPipeline::CreateCommandBuffer(VkCommandBufferLevel p_level, bool p_begin) const
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = Initializers::commandBufferAllocateInfo(m_commandPool, p_level, 1);

    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(m_vulkanDevice.logicalDevice, &cmdBufAllocateInfo, &cmdBuffer);


    if (p_begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = Initializers::commandBufferBeginInfo();
        vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
    }

    return cmdBuffer;
}


void RaytracingPipeline::CleanPipeline()
{
    if (m_swapChain.swapChain != nullptr)
    {
        for (uint32_t i = 0; i < m_swapChain.imageCount; i++)
        {
            vkDestroyImageView(m_vulkanDevice.logicalDevice, m_swapChain.views[i], nullptr);
        }
    }
    if (m_vulkanDevice.surface != nullptr)
    {
        vkDestroySwapchainKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, nullptr);
    }

    m_swapChain.swapChain = nullptr;

    for (auto frame_buffer : m_swapchainFrameBuffers)
    {
        vkDestroyFramebuffer(m_vulkanDevice.logicalDevice, frame_buffer, nullptr);
    }

    vkDestroyPipeline(m_vulkanDevice.logicalDevice, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanDevice.logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_vulkanDevice.logicalDevice, m_renderpass, nullptr);
    vkDestroySwapchainKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, nullptr);
    vkDestroyDescriptorPool(m_vulkanDevice.logicalDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanDevice.logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyImage(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilImage, nullptr);
    vkDestroyImageView(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilView, nullptr);
    vkFreeMemory(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilMemory, nullptr);

    vkQueueWaitIdle(m_graphicsQueue);
    /*for (auto accel : m_BLAS)
        vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, accel.accelerationStructure, nullptr);*/

    vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, m_TLAS.accelerationStructure, nullptr);

    vkDestroyPipelineCache(m_vulkanDevice.logicalDevice, m_pipelineCache, nullptr);

    for (auto fence : m_waitFences)
        vkDestroyFence(m_vulkanDevice.logicalDevice, fence, nullptr);

    vkDestroyCommandPool(m_vulkanDevice.logicalDevice, m_commandPool, nullptr);
    DestroyShaderBuffers(false);
    vkQueueWaitIdle(m_graphicsQueue);
}
void OgEngine::RaytracingPipeline::ResizeCleanup()
{
    if (m_swapChain.swapChain != nullptr)
    {
        for (uint32_t i = 0; i < m_swapChain.imageCount; i++)
        {
            vkDestroyImageView(m_vulkanDevice.logicalDevice, m_swapChain.views[i], nullptr);
        }
    }
    if (m_vulkanDevice.surface != nullptr)
    {
        vkDestroySwapchainKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, nullptr);
    }

    m_swapChain.swapChain = VK_NULL_HANDLE;
    for (auto frame_buffer : m_swapchainFrameBuffers)
    {
        vkDestroyFramebuffer(m_vulkanDevice.logicalDevice, frame_buffer, nullptr);
    }

    vkFreeCommandBuffers(m_vulkanDevice.logicalDevice, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());
    vkDestroyPipelineLayout(m_vulkanDevice.logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_vulkanDevice.logicalDevice, m_pipeline, nullptr);
    vkDestroyRenderPass(m_vulkanDevice.logicalDevice, m_renderpass, nullptr);

    vkDestroySwapchainKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, nullptr);
    vkDestroyDescriptorPool(m_vulkanDevice.logicalDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanDevice.logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyImage(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilImage, nullptr);
    vkDestroyImageView(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilView, nullptr);
    vkDestroyImage(m_vulkanDevice.logicalDevice, m_storageImage.image, nullptr);
    vkDestroyImageView(m_vulkanDevice.logicalDevice, m_storageImage.view, nullptr);

    vkFreeMemory(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilMemory, nullptr);

    vkDestroyPipelineCache(m_vulkanDevice.logicalDevice, m_pipelineCache, nullptr);

    vkDestroyCommandPool(m_vulkanDevice.logicalDevice, m_commandPool, nullptr);

    DestroyShaderBuffers(true);
}

void OgEngine::RaytracingPipeline::InitImGuiFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void OgEngine::RaytracingPipeline::RenderEditor()
{
    ImGui::Render();
}

void OgEngine::RaytracingPipeline::SetupEditor()
{
    ImGuiIO& io = ImGui::GetIO();

    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;


    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    bool open = true;
    ImGui::Begin("Editor", &open, window_flags);
    {

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
    }
    ImGui::End();

    ImGui::Begin("Camera Settings");
    {
        ImGui::Checkbox("Depth Of Field", &m_camera.DOF);
        ImGui::DragFloat("Focus", &m_cameraData.data.x, 0.01f, 0.1f, 500.0f);
        ImGui::DragFloat("Aperture", &m_cameraData.data.y, 0.01f);
    }
    ImGui::End();

    ImGui::Begin("Raytracing Settings");
    {
        ImGui::Checkbox("Use Global Illumination", &m_camera.useGI);
        ImGui::DragInt("Bounce count", &m_camera.bounceCount);
    }
    ImGui::End();
}

void OgEngine::RaytracingPipeline::SetupOffScreenPass()
{
    m_offScreenPass.width = m_width;
    m_offScreenPass.height = m_height;


    VkFormat fbDepthFormat;
    VkBool32 validDepthFormat = GetSupportedDepthFormat(m_vulkanDevice.gpu, &fbDepthFormat);
    assert(validDepthFormat);


    VkImageCreateInfo image = Initializers::imageCreateInfo();
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = VK_FORMAT_B8G8R8A8_UNORM;
    image.extent.width = m_offScreenPass.width;
    image.extent.height = m_offScreenPass.height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
    VkMemoryRequirements memReqs;

    CHECK_ERROR(vkCreateImage(m_vulkanDevice.logicalDevice, &image, nullptr, &m_offScreenPass.color.image));
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, m_offScreenPass.color.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memAlloc, nullptr, &m_offScreenPass.color.mem));
    CHECK_ERROR(vkBindImageMemory(m_vulkanDevice.logicalDevice, m_offScreenPass.color.image, m_offScreenPass.color.mem, 0));

    VkImageViewCreateInfo colorImageView = Initializers::imageViewCreateInfo();
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;
    colorImageView.image = m_offScreenPass.color.image;
    CHECK_ERROR(vkCreateImageView(m_vulkanDevice.logicalDevice, &colorImageView, nullptr, &m_offScreenPass.color.view));

    VkSamplerCreateInfo samplerInfo = Initializers::samplerCreateInfo();
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    CHECK_ERROR(vkCreateSampler(m_vulkanDevice.logicalDevice, &samplerInfo, nullptr, &m_offScreenPass.sampler));

    image.format = fbDepthFormat;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    CHECK_ERROR(vkCreateImage(m_vulkanDevice.logicalDevice, &image, nullptr, &m_offScreenPass.depth.image));
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, m_offScreenPass.depth.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memAlloc, nullptr, &m_offScreenPass.depth.mem));
    CHECK_ERROR(vkBindImageMemory(m_vulkanDevice.logicalDevice, m_offScreenPass.depth.image, m_offScreenPass.depth.mem, 0));

    VkImageViewCreateInfo depthStencilView = Initializers::imageViewCreateInfo();
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = fbDepthFormat;
    depthStencilView.flags = 0;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 1;
    depthStencilView.image = m_offScreenPass.depth.image;
    CHECK_ERROR(vkCreateImageView(m_vulkanDevice.logicalDevice, &depthStencilView, nullptr, &m_offScreenPass.depth.view));


    std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
    attchmentDescriptions[0].format = VK_FORMAT_B8G8R8A8_UNORM;
    attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attchmentDescriptions[1].format = fbDepthFormat;
    attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
    renderPassInfo.pAttachments = attchmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    CHECK_ERROR(vkCreateRenderPass(m_vulkanDevice.logicalDevice, &renderPassInfo, nullptr, &m_offScreenPass.renderPass));

    VkImageView attachments[2];
    attachments[0] = m_offScreenPass.color.view;
    attachments[1] = m_offScreenPass.depth.view;

    VkFramebufferCreateInfo fbufCreateInfo = Initializers::framebufferCreateInfo();
    fbufCreateInfo.renderPass = m_offScreenPass.renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = m_offScreenPass.width;
    fbufCreateInfo.height = m_offScreenPass.height;
    fbufCreateInfo.layers = 1;

    CHECK_ERROR(vkCreateFramebuffer(m_vulkanDevice.logicalDevice, &fbufCreateInfo, nullptr, &m_offScreenPass.frameBuffer));

    m_offScreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    m_offScreenPass.descriptor.imageView = m_offScreenPass.color.view;
    m_offScreenPass.descriptor.sampler = m_offScreenPass.sampler;
}

void OgEngine::RaytracingPipeline::DestroyShaderBuffers(bool p_resizedWindow)
{
    m_shaderBindingTable.Unmap();
    m_shaderBindingTable.Destroy();

    if (!p_resizedWindow)
    {
        m_cameraBuffer.Unmap();
        m_cameraBuffer.Destroy();
        for (auto buf : m_shaderData.vertexBuffer)
        {
            buf.Unmap();
            buf.Destroy();
        }
        for (auto buf : m_shaderData.indexBuffer)
        {
            buf.Unmap();
            buf.Destroy();
        }
        for (auto buf : m_shaderData.materialBuffer)
        {
            buf.Unmap();
            buf.Destroy();
        }
        for (auto buf : m_shaderData.lightBuffer)
        {
            buf.Unmap();
            buf.Destroy();
        }
        m_shaderData.normalMapIDBuffer.Destroy();
        m_shaderData.objectBLASbuffer.Destroy();
        m_shaderData.textureIDBuffer.Destroy();
    }
}
void OgEngine::RaytracingPipeline::CreateTextureMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) const
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.gpu, p_imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = p_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = p_texWidth;
    int32_t mipHeight = p_texHeight;

    for (uint32_t i = 1; i < p_mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            p_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = p_mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    QueueCmdBufferAndFlush(commandBuffer, m_graphicsQueue);
}
VkDescriptorImageInfo OgEngine::RaytracingPipeline::CreateTextureDescriptor(const VkDevice& p_device, TextureData& p_image, const VkSamplerCreateInfo& p_samplerCreateInfo, const VkFormat& p_format, const VkImageLayout& p_layout)
{
    VkImageViewCreateInfo viewInfo = Initializers::imageViewCreateInfo();
    viewInfo.image = p_image.img;
    viewInfo.format = p_format;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, ~0u, 0, 1 };

    VkImageView view;
    vkCreateImageView(m_vulkanDevice.logicalDevice, &viewInfo, nullptr, &view);
    p_image.view = view;

    VkSampler sampler;
    vkCreateSampler(m_vulkanDevice.logicalDevice, &p_samplerCreateInfo, nullptr, &sampler);
    p_image.sampler = sampler;

    VkDescriptorImageInfo info = Initializers::descriptorImageInfo(sampler, view, p_layout);
    return info;
}
void OgEngine::RaytracingPipeline::SetupRaytracingPipeline()
{
    ConfigureRaytracingCommands();
    FindQueueFamilies();
    CreateCommandPool();
    SetupSwapchain(m_width, m_height, false);
    SetupOffScreenPass();
    CreateCommandBuffers();
    CreateSynchronizationPrimitives();
    SetupDepthStencil();
    SetupRenderPass();
    CreatePipelineCache();
    SetupFramebuffer();

    SetupPipelineAndBind(false);
}

void OgEngine::RaytracingPipeline::DestroyObject(uint64_t p_id)
{
    /*std::vector<uint64_t>::iterator it = std::find(m_objectIDs.begin(), m_objectIDs.end(), p_id);

    if (it != m_objectIDs.end())
    {

        int id = std::distance(m_objectIDs.begin(), it);

        m_objectIDs.erase(it);
        m_objectAccIDs.erase(m_objectAccIDs.begin() + id);
        m_textureIDs.erase(m_textureIDs.begin() + id);
        m_normalMapIDs.erase(m_normalMapIDs.begin() + id);
        m_geometryInstances.erase(m_geometryInstances.begin() + id);
        m_shaderData.materialBuffer.erase(m_shaderData.materialBuffer.begin() + id);
        m_objects[id].m_geometryBuffer.Destroy();
        
        //vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, m_BLAS[id].accelerationStructure, nullptr);
        std::vector<std::pair<Mesh*, int>>::iterator it;
        int meshID = 0;

        m_objects.erase(m_objects.begin() + id);

        return;
    }
    UpdateDescriptorSets();*/
}

void OgEngine::RaytracingPipeline::DestroyLight(uint64_t p_id)
{
    std::vector<uint64_t>::iterator lightiterator = std::find(m_lightsIDs.begin(), m_lightsIDs.end(), p_id);
    if (lightiterator != m_lightsIDs.end())
    {
        int lightID = std::distance(m_lightsIDs.begin(), lightiterator);
        m_lights.erase(m_lights.begin() + lightID);
        m_lightsIDs.erase(m_lightsIDs.begin() + lightID);
        m_shaderData.lightBuffer.erase(m_shaderData.lightBuffer.begin() + lightID);
    }
}

void OgEngine::RaytracingPipeline::DestroyAllObjects()
{

    m_objectIDs.erase(m_objectIDs.begin() + 1, m_objectIDs.end());
    m_objectAccIDs.erase(m_objectAccIDs.begin() + 1, m_objectAccIDs.end());
    m_textureIDs.erase(m_textureIDs.begin() + 1, m_textureIDs.end());
    m_normalMapIDs.erase(m_normalMapIDs.begin() + 1, m_normalMapIDs.end());
    m_geometryInstances.erase(m_geometryInstances.begin() + 1, m_geometryInstances.end());
    m_objects.erase(m_objects.begin() + 1, m_objects.end());

    m_shaderData.materialBuffer.erase(m_shaderData.materialBuffer.begin() + 1, m_shaderData.materialBuffer.end());

    m_lights.erase(m_lights.begin() + 1, m_lights.end());
    m_lightsIDs.erase(m_lightsIDs.begin() + 1, m_lightsIDs.end());
    m_shaderData.lightBuffer.erase(m_shaderData.lightBuffer.begin() + 1, m_shaderData.lightBuffer.end());
    
    vkQueueWaitIdle(m_graphicsQueue);

    //CRASH WHEN TRYING TO FREE THE ACCELERATION STRUCTURES FROM PREVIOUS OBJECTS, TO BE FIXED, CAUSES MEMORY LEAKS IN GPU
    /*for (int i = 0; i < m_BLAS.size(); ++i)
    {
        vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, m_BLAS[i].accelerationStructure, nullptr);
    }*/

    m_BLAS.erase(m_BLAS.begin() + 1, m_BLAS.end());
    m_shaderData.indexBuffer.erase(m_shaderData.indexBuffer.begin() + 1, m_shaderData.indexBuffer.end());
    m_shaderData.vertexBuffer.erase(m_shaderData.vertexBuffer.begin() + 1, m_shaderData.vertexBuffer.end());

    vkQueueWaitIdle(m_graphicsQueue);
}

void OgEngine::RaytracingPipeline::UpdateTLAS()
{

    /*const VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    m_geometryInstances.clear();

    for (auto object : m_objects)
    {
        m_geometryInstances.push_back(object.m_geometry);
    }

    Buffer instanceBuffer;
    CreateBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &instanceBuffer,
        sizeof(GeometryInstance) * m_geometryInstances.size(),
        m_geometryInstances.data());

    AccelerationStructure newDataAS;
    CreateTopLevelAccelerationStructure(newDataAS, m_geometryInstances.size());

    VkMemoryRequirements2 memReqTopLevelAS;
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo2{};
    memoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo2.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    memoryRequirementsInfo2.accelerationStructure = newDataAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo2, &memReqTopLevelAS);

    const VkDeviceSize scratchBufferSize = memReqTopLevelAS.memoryRequirements.size;

    Buffer scratchBuffer;
    CreateBuffer(
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &scratchBuffer,
        scratchBufferSize);

    VkAccelerationStructureInfoNV buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
    buildInfo.pGeometries = nullptr;
    buildInfo.geometryCount = 0;
    buildInfo.instanceCount = m_geometryInstances.size();

    vkCmdBuildAccelerationStructureNV(
        cmdBuffer,
        &buildInfo,
        instanceBuffer.buffer,
        0,
        VK_FALSE,
        newDataAS.accelerationStructure,
        nullptr,
        scratchBuffer.buffer,
        0);

    VkMemoryBarrier memoryBarrier = Initializers::memoryBarrier();
    memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

    vkCmdCopyAccelerationStructureNV(cmdBuffer, m_TLAS.accelerationStructure, newDataAS.accelerationStructure, VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_NV);
    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue, true);
    vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, newDataAS.accelerationStructure, nullptr);
    vkFreeMemory(m_vulkanDevice.logicalDevice, newDataAS.memory, nullptr);

    scratchBuffer.Destroy();
    instanceBuffer.Destroy();*/
}

void OgEngine::RaytracingPipeline::UpdateDescriptorSets()
{
    if (m_shaderData.textureIDBuffer.buffer == nullptr)
        return;

    VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures = &m_TLAS.accelerationStructure;

    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
    accelerationStructureWrite.dstSet = m_descriptorSet;
    accelerationStructureWrite.dstBinding = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

    VkDescriptorImageInfo storageImageDescriptor{};
    storageImageDescriptor.imageView = m_storageImage.view;
    storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    std::vector<VkDescriptorImageInfo> textureInfos;
    for (auto tex : m_textures)
        textureInfos.push_back(tex.info);

    std::vector<VkDescriptorImageInfo> normalMapInfos;
    for (auto norm : m_normalMaps)
        normalMapInfos.push_back(norm.info);

    std::vector<VkDescriptorBufferInfo> vertexDescriptor;
    std::vector<VkDescriptorBufferInfo> indexDescriptor;
    std::vector<VkDescriptorBufferInfo> materialDescriptor;
    std::vector<VkDescriptorBufferInfo> lightDescriptor;

    for (auto buf : m_shaderData.vertexBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        vertexDescriptor.push_back(buf.descriptor);
    }

    for (auto buf : m_shaderData.indexBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        indexDescriptor.push_back(buf.descriptor);
    }

    for (auto buf : m_shaderData.materialBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        materialDescriptor.push_back(buf.descriptor);
    }

    for (auto buf : m_shaderData.lightBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        lightDescriptor.push_back(buf.descriptor);
    }

    m_shaderData.textureIDBuffer.Map();
    memcpy(m_shaderData.textureIDBuffer.mapped, m_textureIDs.data(), m_textureIDs.size() * sizeof(uint32_t));
    m_shaderData.textureIDBuffer.Unmap();

    m_shaderData.normalMapIDBuffer.Map();
    memcpy(m_shaderData.normalMapIDBuffer.mapped, m_normalMapIDs.data(), m_normalMapIDs.size() * sizeof(uint32_t));
    m_shaderData.normalMapIDBuffer.Unmap();

    m_shaderData.objectBLASbuffer.Map();
    memcpy(m_shaderData.objectBLASbuffer.mapped, m_objectAccIDs.data(), m_objectAccIDs.size() * sizeof(uint32_t));
    m_shaderData.objectBLASbuffer.Unmap();

    VkWriteDescriptorSet textureDescriptor{};
    textureDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptor.dstSet = m_descriptorSet;
    textureDescriptor.dstBinding = 5;
    textureDescriptor.dstArrayElement = 0;
    textureDescriptor.descriptorCount = static_cast<uint32_t>(m_textures.size());
    textureDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptor.pImageInfo = textureInfos.data();

    VkWriteDescriptorSet normalMapDescriptor{};
    normalMapDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    normalMapDescriptor.dstSet = m_descriptorSet;
    normalMapDescriptor.dstBinding = 11;
    normalMapDescriptor.dstArrayElement = 0;
    normalMapDescriptor.descriptorCount = static_cast<uint32_t>(m_normalMaps.size());
    normalMapDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalMapDescriptor.pImageInfo = normalMapInfos.data();

    const VkWriteDescriptorSet resultImageWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
    const VkWriteDescriptorSet uniformBufferWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &m_cameraBuffer.descriptor);
    const VkWriteDescriptorSet vertexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, vertexDescriptor.data(), static_cast<uint32_t>(m_shaderData.vertexBuffer.size()));
    const VkWriteDescriptorSet indexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, indexDescriptor.data(), static_cast<uint32_t>(m_shaderData.indexBuffer.size()));
    const VkWriteDescriptorSet textureIDWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, &m_shaderData.textureIDBuffer.descriptor);
    const VkWriteDescriptorSet materialWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7, materialDescriptor.data(), static_cast<uint32_t>(m_shaderData.materialBuffer.size()));
    const VkWriteDescriptorSet objectBLASWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 9, &m_shaderData.objectBLASbuffer.descriptor);
    const VkWriteDescriptorSet normalMapIDWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10, &m_shaderData.normalMapIDBuffer.descriptor);
    const VkWriteDescriptorSet lightWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 12, lightDescriptor.data(), static_cast<uint32_t>(m_shaderData.lightBuffer.size()));
    
    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    {
        accelerationStructureWrite,
        resultImageWrite,
        uniformBufferWrite,
        vertexWrite,
        indexWrite,
        textureDescriptor,
        textureIDWrite,
        materialWrite,
        objectBLASWrite,
        normalMapDescriptor,
        normalMapIDWrite,
        lightWrite
    };

    vkQueueWaitIdle(m_graphicsQueue);
    vkUpdateDescriptorSets(m_vulkanDevice.logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    vkFreeCommandBuffers(m_vulkanDevice.logicalDevice, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());
    CreateCommandBuffers();
    StartCastingRays();

}

void OgEngine::RaytracingPipeline::InitImGUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    SetupImGUIStyle();
    io.Fonts->AddFontFromFileTTF("Resources/textures/internal/fonts/font_regular.ttf", 16);
    ImGui_ImplGlfw_InitForVulkan(m_window, true);

    VkAttachmentDescription attachment = {};
    attachment.format = m_swapChain.colorFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    if (vkCreateRenderPass(m_vulkanDevice.logicalDevice, &info, nullptr, &m_ImGUIrenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create Dear ImGui's render pass");
    }

    const std::vector<VkDescriptorPoolSize> ImGUIpoolSizes =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1500 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfoIMGUI = Initializers::descriptorPoolCreateInfo(ImGUIpoolSizes, 15000);
    vkCreateDescriptorPool(m_vulkanDevice.logicalDevice, &descriptorPoolCreateInfoIMGUI, nullptr, &m_ImGUIdescriptorPool);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_vulkanDevice.instance;
    init_info.PhysicalDevice = m_vulkanDevice.gpu;
    init_info.Device = m_vulkanDevice.logicalDevice;
    init_info.QueueFamily = m_vulkanDevice.graphicFamily.value();
    init_info.Queue = m_graphicsQueue;
    init_info.PipelineCache = m_pipelineCache;
    init_info.DescriptorPool = m_ImGUIdescriptorPool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = m_minImageCount;
    init_info.ImageCount = m_swapChain.imageCount;
    init_info.CheckVkResultFn = CHECK_ERROR;
    ImGui_ImplVulkan_Init(&init_info, m_ImGUIrenderPass);

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);


    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);
}

void OgEngine::RaytracingPipeline::SetupImGUI()
{

    m_ImGUIcommandBuffers.resize(m_swapChain.views.size());
    for (size_t i = 0; i < m_swapChain.views.size(); i++)
    {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = m_vulkanDevice.graphicFamily.value();
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        CHECK_ERROR(vkCreateCommandPool(m_vulkanDevice.logicalDevice, &cmdPoolInfo, nullptr, &m_ImGUIcommandPool));

        m_ImGUIcommandBuffers.resize(1);

        VkCommandBufferAllocateInfo cmdBufAllocateInfo =
            Initializers::commandBufferAllocateInfo(
                m_ImGUIcommandPool,
                VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                static_cast<uint32_t>(m_ImGUIcommandBuffers.size()));

        CHECK_ERROR(vkAllocateCommandBuffers(m_vulkanDevice.logicalDevice, &cmdBufAllocateInfo, m_ImGUIcommandBuffers.data()));
    }
    m_sceneID = ImGui_ImplVulkan_AddTexture(m_offScreenPass.sampler, m_offScreenPass.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void OgEngine::RaytracingPipeline::SetupImGUIStyle()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.2f, 0.000f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
    colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
    colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

    style->ChildRounding = 4.0f;
    style->FrameBorderSize = 1.0f;
    style->FrameRounding = 2.0f;
    style->GrabMinSize = 7.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->ScrollbarSize = 13.0f;
    style->TabBorderSize = 1.0f;
    style->TabRounding = 0.0f;
    style->WindowRounding = 4.0f;
}

void OgEngine::RaytracingPipeline::SetupImGUIFrameBuffers()
{
    VkImageView attachment[1];

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = nullptr;
    frameBufferCreateInfo.renderPass = m_ImGUIrenderPass;
    frameBufferCreateInfo.attachmentCount = 1;
    frameBufferCreateInfo.pAttachments = attachment;
    frameBufferCreateInfo.width = m_width;
    frameBufferCreateInfo.height = m_height;
    frameBufferCreateInfo.layers = 1;

    m_ImGUIframeBuffers.resize(m_swapChain.imageCount);
    for (uint32_t i = 0; i < m_ImGUIframeBuffers.size(); i++)
    {
        attachment[0] = m_swapChain.views[i];
        CHECK_ERROR(vkCreateFramebuffer(m_vulkanDevice.logicalDevice, &frameBufferCreateInfo, nullptr, &m_ImGUIframeBuffers[i]));
    }
}

void OgEngine::RaytracingPipeline::RescaleImGUI()
{
    ImGui::SetNextWindowSize({ (float)m_width, (float)m_height });
    for (int i = 0; i < m_ImGUIframeBuffers.size(); ++i)
        vkDestroyFramebuffer(m_vulkanDevice.logicalDevice, m_ImGUIframeBuffers[i], nullptr);
}

void OgEngine::RaytracingPipeline::RenderUI(uint32_t p_id)
{
    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = m_ImGUIrenderPass;
    info.framebuffer = m_ImGUIframeBuffers[p_id];
    info.renderArea.extent.width = m_width;
    info.renderArea.extent.height = m_height;
    info.clearValueCount = 1;

    VkClearValue clear;
    clear.color = { 0.0, 0.0, 0.0, 1.0f };
    clear.depthStencil = { 1,0 };
    info.pClearValues = &clear;

    const VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    for (int i = 0; i < m_swapChain.images.size(); ++i)
    {
        SetImageLayout(cmdBuffer, m_swapChain.images[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    }
    vkCmdBeginRenderPass(cmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
    vkCmdEndRenderPass(cmdBuffer);

    for (int i = 0; i < m_swapChain.images.size(); ++i)
    {
        SetImageLayout(cmdBuffer, m_swapChain.images[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    }

    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);

}

void OgEngine::RaytracingPipeline::ResizeWindow()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    m_width = width;
    m_height = height;

    vkDeviceWaitIdle(m_vulkanDevice.logicalDevice);
    ResizeCleanup();
    vkQueueWaitIdle(m_graphicsQueue);
    vkQueueWaitIdle(m_presentQueue);

    FindQueueFamilies();
    CreateCommandPool();
    SetupSwapchain(m_width, m_height, false);
    SetupOffScreenPass();
    CreateCommandBuffers();
    CreateSynchronizationPrimitives();
    SetupDepthStencil();
    SetupRenderPass();
    CreatePipelineCache();
    SetupFramebuffer();

    SetupPipelineAndBind(true);
    
}
void OgEngine::RaytracingPipeline::UpdateObject(uint64_t p_id, const glm::mat4& p_transform, Mesh* p_mesh, std::string p_texID,
    const char* p_normID, glm::vec4 p_albedo, float p_roughness, float p_ior, glm::vec4 p_specular, glm::vec4 p_emissive, int p_type)
{
    int texID = GetTexture(p_texID.c_str());

    int normalMapID = GetNormalMap(p_normID);

    int id = FindObjectID(p_id);

    if (id != -1)
    {
        m_objects[id].m_geometry.instanceId = id;
        m_objects[id].SetTransform(p_transform);
        UpdateMaterial(id, p_albedo, p_roughness, p_ior, p_specular, p_emissive, p_type, texID, normalMapID);
        return;
    }

    RTMaterial newMaterial;
    newMaterial.albedo = p_albedo;
    newMaterial.data.x = p_roughness;
    newMaterial.specular = p_specular;
    newMaterial.data.y = p_ior;
    newMaterial.emissive = p_emissive;
    newMaterial.data.z = p_type;

    AddEntity(p_id, p_mesh, texID, newMaterial, 12345);
    m_objects.back().SetTransform(p_transform);
}

void OgEngine::RaytracingPipeline::UpdateLight(uint64_t p_id, glm::vec4 p_position, glm::vec4 p_color, glm::vec4 p_direction, int p_type)
{
    /*std::vector<uint64_t>::iterator it = std::find(m_lightsIDs.begin(), m_lightsIDs.end(), p_id);
    if (it != m_lightsIDs.end())
    {
        int id = std::distance(m_lightsIDs.begin(), it);
        m_lights[id].color = p_color;
        m_lights[id].pos = glm::vec4({ p_position.x, p_position.y, p_position.z, 1 });
        m_lights[id].dir = glm::vec4({ p_direction.x, p_direction.y, p_direction.z, (float)p_type });

        m_shaderData.lightBuffer[id].Map();
        memcpy(m_shaderData.lightBuffer[id].mapped, &m_lights[id], sizeof(m_lights[id]));
        m_shaderData.lightBuffer[id].Unmap();
    }
    else
    {
        m_lightsIDs.push_back(p_id);
        RTLight newLight;
        newLight.color = p_color;
        newLight.pos = glm::vec4({ p_position.x, p_position.y, p_position.z, 1 });
        newLight.dir = glm::vec4({ p_direction.x, p_direction.y, p_direction.z, (float)p_type });
        m_lights.push_back(newLight);

        Buffer lightBuffer;
        CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &lightBuffer, sizeof(RTLight),
            &newLight));
        m_shaderData.lightBuffer.push_back(lightBuffer);

    }*/
}

int OgEngine::RaytracingPipeline::GetTexture(const char* p_tex)
{
    std::vector<std::string>::iterator it = std::find(m_textureCtr.begin(), m_textureCtr.end(), p_tex);
    if (it != m_textureCtr.end())
    {
        return std::distance(m_textureCtr.begin(), it);
    }
    else
    {
        std::cout << "Texture not found! \n";
        return 0;
    }
}

int OgEngine::RaytracingPipeline::GetNormalMap(const char* p_norm)
{
    std::vector<std::string>::iterator it = std::find(m_normalMapsCtr.begin(), m_normalMapsCtr.end(), p_norm);
    if (it != m_normalMapsCtr.end())
    {
        return std::distance(m_normalMapsCtr.begin(), it);
    }
    else
    {
        return 12345;
    }
}

void OgEngine::RaytracingPipeline::UpdateMaterial(uint64_t p_id, glm::vec4 p_albedo, float p_roughness, float p_ior, glm::vec4 p_specular, glm::vec4 p_emissive, int p_type, int p_texID, int p_normID)
{

    RTMaterial mat;
    mat.albedo = p_albedo;
    mat.data.x = p_roughness;
    mat.specular = p_specular;
    mat.data.y = p_ior;
    mat.emissive = p_emissive;
    mat.data.z = p_type;
    m_textureIDs[p_id] = p_texID;
    m_normalMapIDs[p_id] = p_normID;

    m_shaderData.materialBuffer[p_id].Map();
    memcpy(m_shaderData.materialBuffer[p_id].mapped, &mat, sizeof(mat));
    m_shaderData.materialBuffer[p_id].Unmap();

    UpdateDescriptorSets();

}



VkResult RaytracingPipeline::AcquireNextImage( uint32_t* p_imageIndex) const
{
    if (m_swapChain.swapChain == VK_NULL_HANDLE)
        std::runtime_error("No swapchain available, swapchain -> NULL");

    VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
    VkFence fence;
    CHECK_ERROR(vkCreateFence(m_vulkanDevice.logicalDevice, &fenceInfo, nullptr, &fence));
    CHECK_ERROR(vkAcquireNextImageKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, UINT64_MAX, nullptr, fence, p_imageIndex));
    CHECK_ERROR(vkWaitForFences(m_vulkanDevice.logicalDevice, 1, &fence, VK_TRUE, 100000000000));
    vkDestroyFence(m_vulkanDevice.logicalDevice, fence, nullptr);
    return vkQueueWaitIdle(m_graphicsQueue);
}

VkResult RaytracingPipeline::QueuePresent(VkQueue p_queue, uint32_t p_imageIndex, VkSemaphore p_waitSemaphore = nullptr)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapChain.swapChain;
    presentInfo.pImageIndices = &p_imageIndex;

    /* if (p_waitSemaphore != nullptr)
     {
         presentInfo.pWaitSemaphores = &p_waitSemaphore;
         presentInfo.waitSemaphoreCount = 1;
     }*/

    return vkQueuePresentKHR(p_queue, &presentInfo);
}

void OgEngine::RaytracingPipeline::ConfigureRaytracingCommands()
{
    /*vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCreateAccelerationStructureNV"));
    vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkDestroyAccelerationStructureNV"));
    vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkBindAccelerationStructureMemoryNV"));
    vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkGetAccelerationStructureHandleNV"));
    vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkGetAccelerationStructureMemoryRequirementsNV"));
    vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCmdBuildAccelerationStructureNV"));
    vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCreateRayTracingPipelinesNV"));
    vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkGetRayTracingShaderGroupHandlesNV"));
    vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCmdTraceRaysNV"));
    vkCmdSetCheckpointNV = reinterpret_cast<PFN_vkCmdSetCheckpointNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCmdSetCheckpointNV"));
    vkGetQueueCheckpointDataNV = reinterpret_cast<PFN_vkGetQueueCheckpointDataNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkGetQueueCheckpointDataNV"));
    vkCmdCopyAccelerationStructureNV = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCmdCopyAccelerationStructureNV"));*/
}

void RaytracingPipeline::FindQueueFamilies()
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanDevice.gpu, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanDevice.gpu, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_vulkanDevice.graphicFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_vulkanDevice.gpu, i, m_vulkanDevice.surface, &presentSupport);

        if (presentSupport)
        {
            m_vulkanDevice.presentFamily = i;
        }

        if (m_vulkanDevice.graphicFamily.has_value() && m_vulkanDevice.presentFamily.has_value())
        {
            break;
        }

        i++;
    }
}

void RaytracingPipeline::CreateCommandPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = m_vulkanDevice.graphicFamily.value();
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CHECK_ERROR(vkCreateCommandPool(m_vulkanDevice.logicalDevice, &cmdPoolInfo, nullptr, &m_commandPool));
}

void RaytracingPipeline::CreateCommandBuffers()
{
    m_commandBuffers.resize(m_swapChain.imageCount);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        Initializers::commandBufferAllocateInfo(
            m_commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<uint32_t>(m_commandBuffers.size()));

    CHECK_ERROR(vkAllocateCommandBuffers(m_vulkanDevice.logicalDevice, &cmdBufAllocateInfo, m_commandBuffers.data()));
}

int RaytracingPipeline::IsAccelerationInMemory(const Mesh* p_mesh)
{
    for (int i = 0; i < m_BLAS.size(); ++i)
    {
        if (strcmp(p_mesh->MeshName().c_str(), m_BLAS[i].first) == 0)
            return i;
    }
    return -1;
}

VkResult RaytracingPipeline::CreateVkBuffer(Device& p_device, VkBufferUsageFlags p_usageFlags, Buffer* p_buffer, VkDeviceSize p_size, void * p_data = nullptr) const
{
    p_buffer->device = p_device.logicalDevice;


    VkBufferCreateInfo bufferCreateInfo = Initializers::bufferCreateInfo(p_usageFlags, p_size);
    vkCreateBuffer(p_device.logicalDevice, &bufferCreateInfo, nullptr, &p_buffer->buffer);

    VkMemoryRequirements memory_requierements;
    VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(p_device.logicalDevice, p_buffer->buffer, &memory_requierements);
    memAlloc.allocationSize = memory_requierements.size;

    memAlloc.memoryTypeIndex = GetMemoryType(p_device, memory_requierements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(p_device.logicalDevice, &memAlloc, nullptr, &p_buffer->memory);

    p_buffer->alignment = memory_requierements.alignment;
    p_buffer->size = memAlloc.allocationSize;
    p_buffer->usageFlags = p_usageFlags;
    p_buffer->memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


    if (p_data != nullptr)
    {
        p_buffer->Map();
        memcpy(p_buffer->mapped, p_data, p_size);
        if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            p_buffer->Flush();

        p_buffer->Unmap();
    }


    p_buffer->SetupDescriptor();


    return p_buffer->Bind();
}

AccelerationStructure RaytracingPipeline::CreateBottomLevelAccelerationStructure(VkAccelerationStructureGeometryKHR& geometry, 
VkAccelerationStructureBuildOffsetInfoKHR& offset, VkAccelerationStructureCreateGeometryTypeInfoKHR& createInfo, GeometryBuffer* p_buffer)
{
    AccelerationStructure acc;
    acc.isBuilt = false;
    acc.info = new GeometryInfo(geometry, offset, createInfo, p_buffer);
    return acc;
}
VkDeviceMemory RaytracingPipeline::AllocateMemory(VkMemoryAllocateInfo& allocateInfo)
{
    VkMemoryAllocateFlagsInfo flags{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    flags.deviceMask = 1u;
    flags.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT;
    

    allocateInfo.pNext = &flags;  // <-- Enabling Export
    VkDeviceMemory mem;
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &allocateInfo, nullptr, &mem));
    return mem;
}

void RaytracingPipeline::AllocateAccelerationStructure(AccelerationStructure& acc, VkAccelerationStructureCreateInfoKHR* asCreateInfo)
{
    // 1. Create the acceleration structure
    CHECK_ERROR(vkCreateAccelerationStructureKHR(m_vulkanDevice.logicalDevice, asCreateInfo, nullptr, &acc.accelerationStructure));

    // 2. Find memory requirements
    VkAccelerationStructureMemoryRequirementsInfoKHR memInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR };
    memInfo.accelerationStructure = acc.accelerationStructure;
    memInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
    memInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
    VkMemoryRequirements2 memReqs{ VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    vkGetAccelerationStructureMemoryRequirementsKHR(m_vulkanDevice.logicalDevice, &memInfo, &memReqs);


    VkMemoryAllocateFlagsInfo memFlagInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memFlagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    // 3. Allocate memory
    VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memAlloc.allocationSize = memReqs.memoryRequirements.size;
    memAlloc.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memReqs.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    acc.memory = AllocateMemory(memAlloc);

    // 4. Bind memory with acceleration structure
    VkBindAccelerationStructureMemoryInfoKHR bind{ VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR };
    bind.accelerationStructure = acc.accelerationStructure;
    bind.memory = acc.memory;
    bind.memoryOffset = 0;
    CHECK_ERROR(vkBindAccelerationStructureMemoryKHR(m_vulkanDevice.logicalDevice, 1, &bind));
}
void RaytracingPipeline::BuildBottomLevelAccelerationStructure(std::pair<const char*, AccelerationStructure>& bottomAccel)
{
    VkDeviceSize maxScratch{ 0 };

    VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    asCreateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    //subMeshes
    //asCreateInfo.maxGeometryCount = (uint32_t)bottomAccel.second.info->geometryInfo.size();
    asCreateInfo.maxGeometryCount = 1u;
    asCreateInfo.pGeometryInfos = &bottomAccel.second.info->geometryInfo;
    AllocateAccelerationStructure(bottomAccel.second, &asCreateInfo);

    VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo{
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR };
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
    memoryRequirementsInfo.accelerationStructure = bottomAccel.second.accelerationStructure;
    memoryRequirementsInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

    VkMemoryRequirements2 reqMem{ VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    vkGetAccelerationStructureMemoryRequirementsKHR(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo, &reqMem);
    VkDeviceSize scratchSize = reqMem.memoryRequirements.size;


    //blas.flags = flags;
    maxScratch = std::max(maxScratch, scratchSize);
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
    vkGetAccelerationStructureMemoryRequirementsKHR(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo, &reqMem);
    bottomAccel.second.memSize = reqMem.memoryRequirements.size;

    Buffer scratchBuffer{};
    CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, &scratchBuffer, maxScratch);
    
    VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    bufferInfo.buffer = scratchBuffer.buffer;
    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(m_vulkanDevice.logicalDevice, &bufferInfo);

    VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    qpci.queryCount = 1u;
    qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    VkQueryPool queryPool;
    vkCreateQueryPool(m_vulkanDevice.logicalDevice, &qpci, nullptr, &queryPool);

    VkCommandBuffer cmdBuf = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    const VkAccelerationStructureGeometryKHR* pGeometry = &bottomAccel.second.info->geometry;
    VkAccelerationStructureBuildGeometryInfoKHR bottomASInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    bottomASInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    bottomASInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    bottomASInfo.update = VK_FALSE;
    bottomASInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    bottomASInfo.dstAccelerationStructure = bottomAccel.second.accelerationStructure;
    bottomASInfo.geometryArrayOfPointers = VK_FALSE;
    bottomASInfo.geometryCount = 1u;
    bottomASInfo.ppGeometries = &pGeometry;
    bottomASInfo.scratchData.deviceAddress = scratchAddress;

    // Pointers of offset
    const VkAccelerationStructureBuildOffsetInfoKHR* pBuildOffset;
    pBuildOffset = &bottomAccel.second.info->offset;

    // Building the AS
    vkCmdBuildAccelerationStructureKHR(cmdBuf, 1, &bottomASInfo, &pBuildOffset);

    // Since the scratch buffer is reused across builds, we need a barrier to ensure one build
    // is finished before starting the next one
    VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    QueueCmdBufferAndFlush(cmdBuf, m_graphicsQueue);
    vkDestroyQueryPool(m_vulkanDevice.logicalDevice, queryPool, VK_NULL_HANDLE);
    scratchBuffer.Destroy();

}

std::pair<const char*, AccelerationStructure>* RaytracingPipeline::ObjectToGeometry(const Mesh* p_mesh)
{
    //WILL NEED TO SUPPORT SUBMESHES ASAP

    if (IsAccelerationInMemory(p_mesh) == -1)
    {
        VkAccelerationStructureCreateGeometryTypeInfoKHR accCreate;
        accCreate.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accCreate.indexType = VK_INDEX_TYPE_UINT32;
        accCreate.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        accCreate.maxPrimitiveCount = static_cast<uint32_t>(p_mesh->Indices().size() / 3);
        accCreate.maxVertexCount = static_cast<uint32_t>(p_mesh->Vertices().size());
        accCreate.allowsTransforms = VK_FALSE;

        GeometryBuffer* modelBuffer = new GeometryBuffer();
        CHECK_ERROR(CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &modelBuffer->m_vertBuffer,
            p_mesh->Vertices().size() * sizeof(Vertex), (void*)p_mesh->Vertices().data()));
        CHECK_ERROR(CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &modelBuffer->m_indexBuffer,
            p_mesh->Indices().size() * sizeof(uint32_t), (void*)p_mesh->Indices().data()));

        VkDeviceAddress vertexAddress = modelBuffer->m_vertBuffer.GetBufferAdress();
        VkDeviceAddress indexAddress = modelBuffer->m_indexBuffer.GetBufferAdress();

        VkAccelerationStructureGeometryTrianglesDataKHR triangles;
        triangles.vertexFormat = accCreate.vertexFormat;
        triangles.vertexData = { vertexAddress };
        triangles.vertexStride = sizeof(Vertex);
        triangles.indexType = accCreate.indexType;
        triangles.indexData = { indexAddress };
        triangles.transformData = {};

        VkAccelerationStructureGeometryKHR asGeom;
        asGeom.geometryType = accCreate.geometryType;
        asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        asGeom.geometry.triangles = triangles;

        // The primitive itself
        VkAccelerationStructureBuildOffsetInfoKHR offset;
        offset.firstVertex = 0;
        offset.primitiveCount = accCreate.maxPrimitiveCount;
        offset.primitiveOffset = 0;
        offset.transformOffset = 0;

        AccelerationStructure acc = CreateBottomLevelAccelerationStructure(asGeom, offset, accCreate, modelBuffer);
        std::pair<const char*, AccelerationStructure> blas = std::pair< const char*, AccelerationStructure >(p_mesh->MeshName().c_str(), acc);
        BuildBottomLevelAccelerationStructure(blas);
        return &blas;
        // Our blas is only one geometry, but could be made of many geometries
    }
    return nullptr;
    /*else
    {
            
    }*/
}

void RaytracingPipeline::CreateTopLevelAccelerationStructure(AccelerationStructure& p_accelerationStruct, uint32_t p_instanceCount)
{
    VkAccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    accelerationStructureInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
    accelerationStructureInfo.instanceCount = p_instanceCount;
    accelerationStructureInfo.geometryCount = 0;

    VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    accelerationStructureCI.info = accelerationStructureInfo;
    CHECK_ERROR(vkCreateAccelerationStructureNV(m_vulkanDevice.logicalDevice, &accelerationStructureCI, nullptr, &p_accelerationStruct.accelerationStructure));

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = p_accelerationStruct.accelerationStructure;

    VkMemoryRequirements2 memoryRequirements2{};
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo, &memoryRequirements2);

    VkMemoryAllocateInfo memoryAllocateInfo = Initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memoryAllocateInfo, nullptr, &p_accelerationStruct.memory));
    
    VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    accelerationStructureMemoryInfo.accelerationStructure = p_accelerationStruct.accelerationStructure;
    accelerationStructureMemoryInfo.memory = p_accelerationStruct.memory;
    
    CHECK_ERROR(vkBindAccelerationStructureMemoryNV(m_vulkanDevice.logicalDevice, 1, &accelerationStructureMemoryInfo));

    CHECK_ERROR(vkGetAccelerationStructureHandleNV(m_vulkanDevice.logicalDevice, p_accelerationStruct.accelerationStructure, sizeof(uint64_t), &p_accelerationStruct.handle));
}

void RaytracingPipeline::CreateStorageImage(StorageImage& p_image)
{
    VkImageCreateInfo image = Initializers::imageCreateInfo();
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = m_swapChain.colorFormat;
    image.extent.width = m_sceneReswidth;
    image.extent.height = m_sceneResheight;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    CHECK_ERROR(vkCreateImage(m_vulkanDevice.logicalDevice, &image, nullptr, &p_image.image));

    VkMemoryRequirements memory_requierements;
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, p_image.image, &memory_requierements);
    VkMemoryAllocateInfo memoryAllocateInfo = Initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memory_requierements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memory_requierements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memoryAllocateInfo, nullptr, &p_image.memory));
    CHECK_ERROR(vkBindImageMemory(m_vulkanDevice.logicalDevice, p_image.image, p_image.memory, 0));

    VkImageViewCreateInfo colorImageView = Initializers::imageViewCreateInfo();
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = m_swapChain.colorFormat;
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;
    colorImageView.image = p_image.image;
    CHECK_ERROR(vkCreateImageView(m_vulkanDevice.logicalDevice, &colorImageView, nullptr, &p_image.view));

    VkSamplerCreateInfo samplerInfo = Initializers::samplerCreateInfo();
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = FLT_MAX;
    CHECK_ERROR(vkCreateSampler(m_vulkanDevice.logicalDevice, &samplerInfo, nullptr, &p_image.imgSampler));

    const VkCommandBuffer cmd_buffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    SetImageLayout(cmd_buffer, p_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    QueueCmdBufferAndFlush(cmd_buffer, m_graphicsQueue);
}

VkBool32 RaytracingPipeline::GetSupportedDepthFormat(VkPhysicalDevice p_physicalDevice, VkFormat* p_depthFormat)
{
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(p_physicalDevice, format, &formatProps);

        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *p_depthFormat = format;
            return true;
        }
    }

    return false;
}
void RaytracingPipeline::AddEntity(uint64_t p_id, Mesh* p_mesh, uint32_t p_textureID, RTMaterial p_material, uint32_t p_normID)
{
    /*vkQueueWaitIdle(m_graphicsQueue);
    Model object = Model(p_mesh, true);

    //Geometry Instance
    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &object.m_geometryBuffer,
        sizeof(GeometryInstance),
        &object.m_geometry));

    Buffer materialBuffer;
    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &materialBuffer, sizeof(RTMaterial),
        &p_material));

    m_shaderData.materialBuffer.push_back(materialBuffer);
    
    int meshBuffersID = CheckForExistingMesh(p_mesh);

    object.m_id = p_id;
    object.m_geometry.instanceId = FindObjectID(p_id);
    object.m_geometry.accelerationStructureHandle = m_BLAS[meshBuffersID].handle;
    
    m_objectIDs.push_back(p_id);
    m_objectAccIDs.push_back(meshBuffersID);
    m_textureIDs.push_back(p_textureID);
    m_normalMapIDs.push_back(p_normID);
    m_geometryInstances.push_back(object.m_geometry);
    m_objects.push_back(object);
    UpdateDescriptorSets();*/
}
ImTextureID OgEngine::RaytracingPipeline::AddUITexture(const char* p_texture)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = nullptr;

    std::string_view p_filePath = p_texture;
    const std::string_view fileName{ p_filePath.data() + (p_filePath.find_last_of('/') + 1) };

    if (ResourceManager::Get<Texture>(fileName) == nullptr)
    {
        width = ResourceManager::Get<Texture>("error.png")->Width();
        height = ResourceManager::Get<Texture>("error.png")->Height();
        pixels = ResourceManager::Get<Texture>("error.png")->Pixels();
    }
    else
    {
        width = ResourceManager::Get<Texture>(fileName)->Width();
        height = ResourceManager::Get<Texture>(fileName)->Height();
        pixels = ResourceManager::Get<Texture>(fileName)->Pixels();
    }

    
    VkDeviceSize bufferSize = width * height * sizeof(glm::u8vec4);
    VkExtent2D extent;
    extent.width = width;
    extent.height = height;
    auto imgSize = extent;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;

    Buffer stagingBuffer;
    CreateVkBuffer(m_vulkanDevice,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        &stagingBuffer, bufferSize);

    stagingBuffer.Map();
    memcpy(stagingBuffer.mapped, pixels, bufferSize);
    stagingBuffer.Unmap();

    //stbi_image_free(ResourceManager::Get<Texture>(p_texture)->Pixels());

    VkImageCreateInfo info = Initializers::imageCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.mipLevels = mipLevels;
    info.arrayLayers = 1;
    info.extent = { extent.width, extent.height, 1 };
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    TextureData data;
    vkCreateImage(m_vulkanDevice.logicalDevice, &info, nullptr, &data.img);

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, data.img, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_vulkanDevice.logicalDevice, &allocInfo, nullptr, &data.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_vulkanDevice.logicalDevice, data.img, data.memory, 0);

    SetImageLayout(cmdBuffer, data.img,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    VkBufferImageCopy copyRegion{};
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.layerCount = 1;

    VkExtent3D extend3D;
    extend3D.width = width;
    extend3D.height = height;
    extend3D.depth = 1;
    copyRegion.imageExtent = extend3D;

    vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, data.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion);

    range.levelCount = 1;

    VkSamplerCreateInfo samplerInfo = Initializers::samplerCreateInfo();
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = FLT_MAX;

    data.info = CreateTextureDescriptor(m_vulkanDevice.logicalDevice, data, samplerInfo, format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);
    CreateTextureMipmaps(data.img, format, width, height, mipLevels);
    stagingBuffer.Destroy();
    return ImGui_ImplVulkan_AddTexture(data.sampler, data.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void OgEngine::RaytracingPipeline::AddTexture(const std::string& p_texture, const TEXTURE_TYPE p_type)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = nullptr;

    const std::string_view p_filePath = p_texture;
    const std::string_view fileName{ p_filePath.data() + (p_filePath.find_last_of('/') + 1) };

    if (ResourceManager::Get<Texture>(fileName) == nullptr)
    {
            pixels = ResourceManager::Get<Texture>("error.png")->Pixels();
            width = ResourceManager::Get<Texture>("error.png")->Width();
            height = ResourceManager::Get<Texture>("error.png")->Height();
    }
    else
    {
        width = ResourceManager::Get<Texture>(fileName)->Width();
        height = ResourceManager::Get<Texture>(fileName)->Height();
        pixels = ResourceManager::Get<Texture>(fileName)->Pixels();
    }


    VkDeviceSize bufferSize = width * height * sizeof(glm::u8vec4);
    VkExtent2D extent;
    extent.width = width;
    extent.height = height;
    auto imgSize = extent;

    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    if (p_type == 1)
    {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;

    Buffer stagingBuffer;
    CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        &stagingBuffer, bufferSize);

    stagingBuffer.Map();
    memcpy(stagingBuffer.mapped, pixels, bufferSize);
    stagingBuffer.Unmap();

    //stbi_image_free(ResourceManager::Get<Texture>(p_texture)->Pixels());

    VkImageCreateInfo info = Initializers::imageCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.mipLevels = mipLevels;
    info.arrayLayers = 1;
    info.extent = { extent.width, extent.height, 1 };
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    TextureData data;
    vkCreateImage(m_vulkanDevice.logicalDevice, &info, nullptr, &data.img);

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, data.img, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_vulkanDevice.logicalDevice, &allocInfo, nullptr, &data.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_vulkanDevice.logicalDevice, data.img, data.memory, 0);

    SetImageLayout(cmdBuffer, data.img,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    VkBufferImageCopy copyRegion{};
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.layerCount = 1;

    VkExtent3D extend3D;
    extend3D.width = width;
    extend3D.height = height;
    extend3D.depth = 1;
    copyRegion.imageExtent = extend3D;

    vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, data.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion);

    range.levelCount = 1;

    VkSamplerCreateInfo samplerInfo = Initializers::samplerCreateInfo();
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = FLT_MAX;

    data.info = CreateTextureDescriptor(m_vulkanDevice.logicalDevice, data, samplerInfo, format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);
    stagingBuffer.Destroy();
    CreateTextureMipmaps(data.img, format, width, height, mipLevels);
    if (p_type == 0)
    {
        m_textures.push_back(data);
        m_textureCtr.emplace_back(p_texture);
    }
    else if (p_type == 1)
    {
        m_normalMaps.push_back(data);
        m_normalMapsCtr.emplace_back(p_texture);
    }
    UpdateDescriptorSets();

}

void OgEngine::RaytracingPipeline::CreateTopLevelAccelerationStructure()
{
    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    CreateTopLevelAccelerationStructure(m_TLAS, m_geometryInstances.size());

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo2{};
    memoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo2.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

    VkMemoryRequirements2 tlasMemsReqs;
    memoryRequirementsInfo2.accelerationStructure = m_TLAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo2, &tlasMemsReqs);

    const VkDeviceSize scratchBufferSize2 = tlasMemsReqs.memoryRequirements.size;

    Buffer scratchBuffer2;
    CreateVkBuffer(m_vulkanDevice,
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        &scratchBuffer2,
        scratchBufferSize2);


    VkAccelerationStructureInfoNV buildInfo2{};
    buildInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo2.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    buildInfo2.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
    buildInfo2.pGeometries = nullptr;
    buildInfo2.geometryCount = 0;
    buildInfo2.instanceCount = m_geometryInstances.size();

    CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        &m_instancesBuffer,
        sizeof(GeometryInstance) * m_geometryInstances.size(),
        m_geometryInstances.data());

    vkCmdBuildAccelerationStructureNV(
        cmdBuffer,
        &buildInfo2,
        m_instancesBuffer.buffer,
        0,
        VK_FALSE,
        m_TLAS.accelerationStructure,
        nullptr,
        scratchBuffer2.buffer,
        0);

    VkMemoryBarrier memoryBarrier = Initializers::memoryBarrier();
    memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);
    scratchBuffer2.Destroy();
}



//VALID
void RaytracingPipeline::CreatePipeline()
{

    VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
    accelerationStructureLayoutBinding.binding = 0;
    accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    accelerationStructureLayoutBinding.descriptorCount = 1;
    accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
    resultImageLayoutBinding.binding = 1;
    resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageLayoutBinding.descriptorCount = 1;
    resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    VkDescriptorSetLayoutBinding uniformBufferBinding{};
    uniformBufferBinding.binding = 2;
    uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferBinding.descriptorCount = 1;
    uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

    VkDescriptorSetLayoutBinding vertexBufferBinding{};
    vertexBufferBinding.binding = 3;
    vertexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertexBufferBinding.descriptorCount = MAX_OBJECTS;
    vertexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding indexBufferBinding{};
    indexBufferBinding.binding = 4;
    indexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indexBufferBinding.descriptorCount = MAX_OBJECTS;
    indexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 5;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = MAX_TEXTURES;
    textureBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding textureIDBinding{};
    textureIDBinding.binding = 6;
    textureIDBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    textureIDBinding.descriptorCount = MAX_OBJECTS;
    textureIDBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding normalMapBinding{};
    normalMapBinding.binding = 11;
    normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalMapBinding.descriptorCount = MAX_TEXTURES;
    normalMapBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding normalMapIDBinding{};
    normalMapIDBinding.binding = 10;
    normalMapIDBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    normalMapIDBinding.descriptorCount = MAX_OBJECTS;
    normalMapIDBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding materialBufferBinding{};
    materialBufferBinding.binding = 7;
    materialBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialBufferBinding.descriptorCount = MAX_OBJECTS;
    materialBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding lightBufferBinding{};
    lightBufferBinding.binding = 12;
    lightBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightBufferBinding.descriptorCount = MAX_OBJECTS;
    lightBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    /*VkDescriptorSetLayoutBinding accumulationImageBinding{};
    accumulationImageBinding.binding = 8;
    accumulationImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    accumulationImageBinding.descriptorCount = 1;
    accumulationImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;*/

    VkDescriptorSetLayoutBinding objectBLASBinding{};
    objectBLASBinding.binding = 9;
    objectBLASBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectBLASBinding.descriptorCount = MAX_OBJECTS;
    objectBLASBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    std::vector<VkDescriptorSetLayoutBinding> bindings({
        accelerationStructureLayoutBinding,
        resultImageLayoutBinding,
        uniformBufferBinding,
        vertexBufferBinding,
        indexBufferBinding,
        textureBinding,
        textureIDBinding,
        normalMapBinding,
        normalMapIDBinding,
        materialBufferBinding,
        objectBLASBinding,
        lightBufferBinding
        //accumulationImageBinding
        });

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(m_vulkanDevice.logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

    vkCreatePipelineLayout(m_vulkanDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
    LoadShaders(m_pipeline);
}
void RaytracingPipeline::ReloadShaders()
{

    vkDestroyPipeline(m_vulkanDevice.logicalDevice, m_pipeline, nullptr);
    LoadShaders(m_pipeline);
    vkFreeCommandBuffers(m_vulkanDevice.logicalDevice, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());
    CreateCommandBuffers();
    StartCastingRays();
}
void RaytracingPipeline::LoadShaders(VkPipeline& p_pipeline)
{
    const uint32_t shaderIndexRaygen = 0;
    const uint32_t shaderIndexMiss = 1;
    const uint32_t shaderIndexShadowMiss = 2;
    const uint32_t shaderIndexClosestHit = 3;

    std::array<VkPipelineShaderStageCreateInfo, 4> shaderStages{};
    shaderStages[shaderIndexRaygen] = LoadShader("Resources/shaders/bin/ray_gen.spv", VK_SHADER_STAGE_RAYGEN_BIT_NV);
    shaderStages[shaderIndexMiss] = LoadShader("Resources/shaders/bin/ray_miss.spv", VK_SHADER_STAGE_MISS_BIT_NV);
    shaderStages[shaderIndexClosestHit] = LoadShader("Resources/shaders/bin/ray_chit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    shaderStages[shaderIndexShadowMiss] = LoadShader("Resources/shaders/bin/ray_shadow.spv", VK_SHADER_STAGE_MISS_BIT_NV);

    std::array<VkRayTracingShaderGroupCreateInfoNV, SHADER_COUNT> groups{};
    for (auto& group : groups)
    {

        group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
        group.generalShader = VK_SHADER_UNUSED_NV;
        group.closestHitShader = VK_SHADER_UNUSED_NV;
        group.anyHitShader = VK_SHADER_UNUSED_NV;
        group.intersectionShader = VK_SHADER_UNUSED_NV;
    }


    // Links shaders and types to ray tracing shader groups
        // Ray generation shader group
    groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_RAYGEN].generalShader = shaderIndexRaygen;
    // Scene miss shader group
    groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_MISS].generalShader = shaderIndexMiss;
    // Shadow miss shader group 
    groups[INDEX_SHADOW_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_SHADOW_MISS].generalShader = shaderIndexShadowMiss;
    // Scene closest hit shader group
    groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
    groups[INDEX_CLOSEST_HIT].generalShader = VK_SHADER_UNUSED_NV;
    groups[INDEX_CLOSEST_HIT].closestHitShader = shaderIndexClosestHit;
    // Shadow closest hit shader group
    groups[INDEX_SHADOW_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
    groups[INDEX_SHADOW_HIT].generalShader = VK_SHADER_UNUSED_NV;
    // Reuse shadow miss shader
    groups[INDEX_SHADOW_HIT].closestHitShader = shaderIndexShadowMiss;

    VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
    rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayPipelineInfo.pStages = shaderStages.data();
    rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups = groups.data();
    rayPipelineInfo.maxRecursionDepth = 2;
    rayPipelineInfo.layout = m_pipelineLayout;
    vkCreateRayTracingPipelinesNV(m_vulkanDevice.logicalDevice, nullptr, 1, &rayPipelineInfo, nullptr, &p_pipeline);
}

inline ImGuiContext* OgEngine::RaytracingPipeline::GetUIContext()
{
    return ImGui::GetCurrentContext();
}

VkPipelineShaderStageCreateInfo RaytracingPipeline::LoadShader(const std::string p_file_name, VkShaderStageFlagBits p_stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = p_stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    shaderStage.module = vks::tools::loadShader(androidApp->activity->assetManager, fileName.c_str(), device);
#else
    shaderStage.module = LoadShaderFile(p_file_name.c_str(), m_vulkanDevice.logicalDevice);
#endif
    shaderStage.pName = "main";
    assert(shaderStage.module != VK_NULL_HANDLE);
    m_shaderModules.push_back(shaderStage.module);
    return shaderStage;
}

void RaytracingPipeline::CreateSynchronizationPrimitives()
{

    VkFenceCreateInfo fenceCreateInfo = Initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    m_waitFences.resize(m_commandBuffers.size());
    for (auto& fence : m_waitFences)
    {
        vkCreateFence(m_vulkanDevice.logicalDevice, &fenceCreateInfo, nullptr, &fence);
    }
}

void RaytracingPipeline::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(m_vulkanDevice.logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
}

void RaytracingPipeline::SetupFramebuffer()
{
    m_swapchainFrameBuffers.resize(m_swapChain.views.size());

    for (size_t i = 0; i < m_swapChain.views.size(); ++i)
    {
        std::array<VkImageView, 2> attachments =
        {
            m_swapChain.views[i],
            m_depthStencil.m_stencilView
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderpass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_width;
        framebufferInfo.height = m_height;
        framebufferInfo.layers = 1u;

        if (vkCreateFramebuffer(m_vulkanDevice.logicalDevice, &framebufferInfo, nullptr, &m_swapchainFrameBuffers[i]) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

VkFormat RaytracingPipeline::FindSupportedFormat(const std::vector<VkFormat>& p_candidates,
    VkImageTiling                p_tiling,
    VkFormatFeatureFlags         p_features) const
{
    for (VkFormat format : p_candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.gpu, format, &props);

        if (p_tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & p_features) == p_features)
            return format;
        if (p_tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & p_features) == p_features)
            return format;
    }

    throw std::runtime_error("failed to find supported format!");
}

void RaytracingPipeline::SetupDepthStencil()
{
    m_depthFormat = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = m_depthFormat;
    imageCI.extent = { m_width, m_height, 1 };
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    CHECK_ERROR(vkCreateImage(m_vulkanDevice.logicalDevice, &imageCI, nullptr, &m_depthStencil.m_stencilImage));
    VkMemoryRequirements memory_requierements{};
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilImage, &memory_requierements);

    VkMemoryAllocateInfo memory_alloc{};
    memory_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc.allocationSize = memory_requierements.size;
    memory_alloc.memoryTypeIndex = GetMemoryType(m_vulkanDevice, memory_requierements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memory_alloc, nullptr, &m_depthStencil.m_stencilMemory));
    CHECK_ERROR(vkBindImageMemory(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilImage, m_depthStencil.m_stencilMemory, 0));

    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = m_depthStencil.m_stencilImage;
    imageViewCI.format = m_depthFormat;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (m_depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT)
    {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    CHECK_ERROR(vkCreateImageView(m_vulkanDevice.logicalDevice, &imageViewCI, nullptr, &m_depthStencil.m_stencilView));
}

void RaytracingPipeline::SetupRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};

    attachments[0].format = m_swapChain.colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].format = m_depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference;
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference;
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &colorReference;
    subpass_description.pDepthStencilAttachment = &depthReference;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;
    subpass_description.pResolveAttachments = nullptr;


    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass_description;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    CHECK_ERROR(vkCreateRenderPass(m_vulkanDevice.logicalDevice, &renderPassInfo, nullptr, &m_renderpass));
}


int OgEngine::RaytracingPipeline::FindObjectID(uint64_t p_id)
{
    std::vector<uint64_t>::iterator it = std::find(m_objectIDs.begin(), m_objectIDs.end(), p_id);

    if (it != m_objectIDs.end())
    {
        return std::distance(m_objectIDs.begin(), it);
    }
    else
    {
        return -1;
    }
}


void RaytracingPipeline::SetImageLayout(const VkCommandBuffer p_cmd_buffer, VkImage p_image, VkImageLayout p_oldImageLayout, VkImageLayout p_newImageLayout, const VkImageSubresourceRange p_subresourceRange, VkPipelineStageFlags p_srcStageMask,
    VkPipelineStageFlags p_dstStageMask)
{

    VkImageMemoryBarrier imageMemoryBarrier = Initializers::imageMemoryBarrier();
    imageMemoryBarrier.oldLayout = p_oldImageLayout;
    imageMemoryBarrier.newLayout = p_newImageLayout;
    imageMemoryBarrier.image = p_image;
    imageMemoryBarrier.subresourceRange = p_subresourceRange;


    switch (p_oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:

        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }


    switch (p_newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:

        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:

        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:

        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:

        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:

        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    vkCmdPipelineBarrier(
        p_cmd_buffer,
        p_srcStageMask,
        p_dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
}

void RaytracingPipeline::SetImageLayout(VkCommandBuffer p_cmdbuffer, VkImage p_image, VkImageAspectFlags p_aspectMask, VkImageLayout p_oldImageLayout, VkImageLayout p_newImageLayout, VkPipelineStageFlags p_srcStageMask, VkPipelineStageFlags p_dstStageMask)
{
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = p_aspectMask;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.layerCount = 1;
    SetImageLayout(p_cmdbuffer, p_image, p_oldImageLayout, p_newImageLayout, subresource_range, p_srcStageMask, p_dstStageMask);
}

void RaytracingPipeline::QueueCmdBufferAndFlush(VkCommandBuffer p_commandBuffer, VkQueue p_queue, bool p_free) const
{
    if (p_commandBuffer == nullptr)
    {
        return;
    }

    CHECK_ERROR(vkEndCommandBuffer(p_commandBuffer));

    VkSubmitInfo submitInfo = Initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &p_commandBuffer;

    VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
    VkFence fence;
    CHECK_ERROR(vkCreateFence(m_vulkanDevice.logicalDevice, &fenceInfo, nullptr, &fence));

    CHECK_ERROR(vkQueueSubmit(p_queue, 1, &submitInfo, fence));

    CHECK_ERROR(vkWaitForFences(m_vulkanDevice.logicalDevice, 1, &fence, VK_TRUE, 100000000000));
    vkQueueWaitIdle(m_graphicsQueue);
    vkDestroyFence(m_vulkanDevice.logicalDevice, fence, nullptr);

    if (p_free)
    {
        vkFreeCommandBuffers(m_vulkanDevice.logicalDevice, m_commandPool, 1, &p_commandBuffer);
    }
}

void RaytracingPipeline::CreateShaderBindingTable()
{

    const uint32_t sbtSize = m_raytracingProperties.shaderGroupHandleSize * SHADER_COUNT;
    CreateVkBuffer(m_vulkanDevice,
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        &m_shaderBindingTable,
        sbtSize);
    m_shaderBindingTable.Map();

    //const auto shaderHandleStorage = new uint8_t[sbtSize];

    auto shaderHandleStorage = new uint8_t[sbtSize];
    // Get shader identifiers
    CHECK_ERROR(vkGetRayTracingShaderGroupHandlesNV(m_vulkanDevice.logicalDevice, m_pipeline, 0, SHADER_COUNT, sbtSize, shaderHandleStorage));
    auto* data = static_cast<uint8_t*>(m_shaderBindingTable.mapped);
    // Copy the shader identifiers to the shader binding table
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_RAYGEN);
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_MISS);
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_SHADOW_MISS);
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_CLOSEST_HIT);
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_SHADOW_HIT);
    m_shaderBindingTable.Unmap();
}

VkDeviceSize RaytracingPipeline::CopyShaderIdentifier(uint8_t* p_data, const uint8_t* p_shaderHandleStorage, uint32_t p_groupIndex) const
{
    const uint32_t shaderGroupHandleSize = m_raytracingProperties.shaderGroupHandleSize;
    memcpy(p_data, p_shaderHandleStorage + p_groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
    p_data += shaderGroupHandleSize;
    return shaderGroupHandleSize;
}

void RaytracingPipeline::CreateDescriptorSets(bool p_resizedWindow)
{
    const std::vector<VkDescriptorPoolSize> poolSizes =
    {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }

    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = Initializers::descriptorPoolCreateInfo(poolSizes, 1);
    vkCreateDescriptorPool(m_vulkanDevice.logicalDevice, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = Initializers::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);

    vkAllocateDescriptorSets(m_vulkanDevice.logicalDevice, &descriptorSetAllocateInfo, &m_descriptorSet);

    VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures = &m_TLAS.accelerationStructure;

    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
    accelerationStructureWrite.dstSet = m_descriptorSet;
    accelerationStructureWrite.dstBinding = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

    VkDescriptorImageInfo storageImageDescriptor{};
    storageImageDescriptor.imageView = m_storageImage.view;
    storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    /*VkDescriptorImageInfo accumulationDescriptor{};
    accumulationDescriptor.imageView = m_accumulationImage.view;
    accumulationDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;*/

    std::vector<VkDescriptorImageInfo> textureInfos;
    for (auto tex : m_textures)
        textureInfos.push_back(tex.info);

    std::vector<VkDescriptorImageInfo> normalMapInfos;
    for (auto norm : m_normalMaps)
        normalMapInfos.push_back(norm.info);

    std::vector<VkDescriptorBufferInfo> vertexDescriptor;
    std::vector<VkDescriptorBufferInfo> indexDescriptor;
    std::vector<VkDescriptorBufferInfo> materialDescriptor;
    std::vector<VkDescriptorBufferInfo> lightDescriptor;

    for (auto buf : m_shaderData.vertexBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        vertexDescriptor.push_back(buf.descriptor);
    }

    for (auto buf : m_shaderData.indexBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        indexDescriptor.push_back(buf.descriptor);
    }

    for (auto buf : m_shaderData.materialBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        materialDescriptor.push_back(buf.descriptor);
    }
    for (auto buf : m_shaderData.lightBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        lightDescriptor.push_back(buf.descriptor);
    }

    if (!p_resizedWindow)
    {
        CHECK_ERROR(CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            &m_shaderData.textureIDBuffer,
            MAX_OBJECTS * sizeof(uint32_t),
            m_textureIDs.data()));

        CHECK_ERROR(CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            &m_shaderData.normalMapIDBuffer,
            MAX_OBJECTS * sizeof(uint32_t),
            m_normalMapIDs.data()));

        CHECK_ERROR(CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            &m_shaderData.objectBLASbuffer,
            MAX_OBJECTS * sizeof(uint32_t),
            m_objectAccIDs.data()));

    }

    VkWriteDescriptorSet textureDescriptor{};
    textureDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptor.dstSet = m_descriptorSet;
    textureDescriptor.dstBinding = 5;
    textureDescriptor.dstArrayElement = 0;
    textureDescriptor.descriptorCount = static_cast<uint32_t>(m_textures.size());
    textureDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptor.pImageInfo = textureInfos.data();

    VkWriteDescriptorSet normalMapDescriptor{};
    normalMapDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    normalMapDescriptor.dstSet = m_descriptorSet;
    normalMapDescriptor.dstBinding = 11;
    normalMapDescriptor.dstArrayElement = 0;
    normalMapDescriptor.descriptorCount = static_cast<uint32_t>(m_normalMaps.size());
    normalMapDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalMapDescriptor.pImageInfo = normalMapInfos.data();

    const VkWriteDescriptorSet resultImageWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
    const VkWriteDescriptorSet uniformBufferWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &m_cameraBuffer.descriptor);
    const VkWriteDescriptorSet vertexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, vertexDescriptor.data(), static_cast<uint32_t>(m_shaderData.vertexBuffer.size()));
    const VkWriteDescriptorSet indexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, indexDescriptor.data(), static_cast<uint32_t>(m_shaderData.indexBuffer.size()));
    const VkWriteDescriptorSet textureIDWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, &m_shaderData.textureIDBuffer.descriptor);
    const VkWriteDescriptorSet materialWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7, materialDescriptor.data(), static_cast<uint32_t>(m_shaderData.materialBuffer.size()));
    const VkWriteDescriptorSet objectBLASWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 9, &m_shaderData.objectBLASbuffer.descriptor);
    const VkWriteDescriptorSet normalMapWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10, &m_shaderData.normalMapIDBuffer.descriptor);
    const VkWriteDescriptorSet lightWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 12, lightDescriptor.data(), static_cast<uint32_t>(m_shaderData.lightBuffer.size()));
    //const VkWriteDescriptorSet accumulationWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8, &accumulationDescriptor);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    {
        accelerationStructureWrite,
        resultImageWrite,
        uniformBufferWrite,
        vertexWrite,
        indexWrite,
        textureDescriptor,
        textureIDWrite,
        materialWrite,
        objectBLASWrite,
        normalMapDescriptor,
        normalMapWrite,
        lightWrite
        //accumulationWrite
    };

    vkUpdateDescriptorSets(m_vulkanDevice.logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void RaytracingPipeline::CreateCamera()
{

    //m_camera.setPosition({ 0, 5, 5 });
    m_camera.SetPerspective(90.0f, static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 1024.0f);
    m_cameraData.viewInverse = glm::inverse(m_camera.matrices.view);
    m_cameraData.projInverse = glm::inverse(m_camera.matrices.perspective);
    m_cameraData.data = glm::vec4(0);
    m_cameraData.settings = glm::vec4(0);
    m_cameraData.samples = glm::vec4(0);

    m_cameraData.samples.x = 2;
    m_cameraData.samples.y = 0;

    CHECK_ERROR(CreateVkBuffer(m_vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        &m_cameraBuffer,
        sizeof(m_cameraData),
        &m_cameraData));
}

void RaytracingPipeline::UpdateCamera()
{

    if(isRefreshing)
       m_cameraData.samples.y = 0;

    if (m_cameraBuffer.buffer == VK_NULL_HANDLE)
        return;

    m_cameraData.viewInverse = glm::inverse(m_camera.matrices.view);
    m_cameraData.projInverse = glm::inverse(m_camera.matrices.perspective);

    if (m_cameraData.data.w > 10000)
        m_cameraData.data.w = 0;

    m_cameraData.data.w += rand() % 1000;
    m_cameraData.samples.y += 1;

    m_cameraData.settings.x = 0;
    if (m_camera.DOF)
        m_cameraData.settings.x = 1;


    m_cameraData.settings.y = 0;
    if (m_camera.useGI)
        m_cameraData.settings.y = 1;

    m_cameraData.settings.z = m_camera.bounceCount;
    m_cameraData.data.z = m_lights.size();
    m_cameraBuffer.Map();
    memcpy(m_cameraBuffer.mapped, &m_cameraData, sizeof(m_cameraData));
    m_cameraBuffer.Unmap();
}

void RaytracingPipeline::StartCastingRays()
{
    VkCommandBufferBeginInfo cmdBufInfo = Initializers::commandBufferBeginInfo();

    const VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    for (size_t i = 0; i < m_commandBuffers.size(); ++i)
    {
        CHECK_ERROR(vkBeginCommandBuffer(m_commandBuffers[i], &cmdBufInfo));

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipeline);
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        // Calculate shader binding offsets, which is pretty straight forward in our example 
        VkDeviceSize bindingOffsetRayGenShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_RAYGEN;
        VkDeviceSize bindingOffsetMissShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_MISS;
        VkDeviceSize bindingOffsetHitShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT;
        VkDeviceSize bindingStride = m_raytracingProperties.shaderGroupHandleSize;

        vkCmdTraceRaysNV(m_commandBuffers[i],
            m_shaderBindingTable.buffer, bindingOffsetRayGenShader,
            m_shaderBindingTable.buffer, bindingOffsetMissShader, bindingStride,
            m_shaderBindingTable.buffer, bindingOffsetHitShader, bindingStride,
            VK_NULL_HANDLE, 0, 0,
            m_sceneReswidth, m_sceneResheight, 1);

        SetImageLayout(
            m_commandBuffers[i],
            m_offScreenPass.color.image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        SetImageLayout(
            m_commandBuffers[i],
            m_storageImage.image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.srcOffset = { 0, 0, 0 };
        copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.dstOffset = { 0, 0, 0 };
        copyRegion.extent = { m_width, m_height, 1 };
        vkCmdCopyImage(m_commandBuffers[i], m_storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_offScreenPass.color.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        SetImageLayout(
            m_commandBuffers[i],
            m_offScreenPass.color.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        SetImageLayout(
            m_commandBuffers[i],
            m_storageImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        CHECK_ERROR(vkEndCommandBuffer(m_commandBuffers[i]));
    }
}

void RaytracingPipeline::SetupPipelineAndBind(bool p_resizedWindow)
{
    m_raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProps{};
    deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps.pNext = &m_raytracingProperties;
    vkGetPhysicalDeviceProperties2KHR(m_vulkanDevice.gpu, &deviceProps);

    m_submitInfo = Initializers::submitInfo();
    m_submitInfo.pWaitDstStageMask = &submitPipelineStages;
    m_submitInfo.waitSemaphoreCount = 0;
    m_submitInfo.pWaitSemaphores = nullptr;
    m_submitInfo.signalSemaphoreCount = 0;
    m_submitInfo.pSignalSemaphores = nullptr;


    if (!p_resizedWindow)
    {
        AddTexture("default.png");
        AddTexture("default.png", TEXTURE_TYPE::NORMAL);

       /* vkQueueWaitIdle(m_graphicsQueue);
        AddEntity(99999, ResourceManager::Get<OgEngine::Mesh>("cube.obj"), 0, mat, 12345);
        UpdateLight(123456789, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0);
        CreateTopLevelAccelerationStructure();*/
        CreateCamera();
    }

    CreateStorageImage(m_storageImage);

    CreatePipeline();
    CreateShaderBindingTable();
    CreateDescriptorSets(p_resizedWindow);


    if (p_resizedWindow)
    {
        RescaleImGUI();
        SetupImGUIFrameBuffers();
    }
    else
    {
        StartCastingRays();
        InitImGUI();
        SetupImGUIFrameBuffers();
        SetupImGUI();
    }
}

void RaytracingPipeline::InitFrame()
{
    const VkResult result = AcquireNextImage(&m_currentBuffer);
    //vkQueueWaitIdle(m_graphicsQueue);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ResizeWindow();
    }
    else
    {
        CHECK_ERROR(result);
    }
}
void RaytracingPipeline::DisplayFrame()
{
    const VkResult result = QueuePresent(m_graphicsQueue, m_currentBuffer);
    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            ResizeWindow();
            return;
        }
        CHECK_ERROR(result);
    }
    CHECK_ERROR(vkQueueWaitIdle(m_graphicsQueue));
}

void RaytracingPipeline::RenderFrame()
{
    UpdateCamera();
    vkQueueWaitIdle(m_presentQueue);
    vkQueueWaitIdle(m_graphicsQueue);
    InitFrame();
    m_submitInfo.commandBufferCount = 1;
    m_submitInfo.pCommandBuffers = &m_commandBuffers[m_currentBuffer];

    VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
    VkFence fence;
    CHECK_ERROR(vkCreateFence(m_vulkanDevice.logicalDevice, &fenceInfo, nullptr, &fence));

    CHECK_ERROR(vkQueueSubmit(m_graphicsQueue, 1, &m_submitInfo, fence));
    CHECK_ERROR(vkWaitForFences(m_vulkanDevice.logicalDevice, 1, &fence, VK_TRUE, 100000000000));
    vkDestroyFence(m_vulkanDevice.logicalDevice, fence, nullptr);

    RenderUI(m_currentBuffer);
    DisplayFrame();
}
