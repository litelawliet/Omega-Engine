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
#define INDEX_SHADOW_MISS 3
#define INDEX_CLOSEST_HIT 2
#define SHADER_COUNT 4

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

void RaytracingPipeline::InitSwapChain(uint32_t p_width, uint32_t p_height, bool p_vsync)
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

    uint32_t queueFamilyIndices[] = 
    {
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
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    CHECK_ERROR(vkCreateSwapchainKHR(m_vulkanDevice.logicalDevice, &createInfo, nullptr, &m_swapChain.swapChain));

    CHECK_ERROR(vkGetSwapchainImagesKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, &imageCount, nullptr));
    m_swapChain.images.resize(imageCount);
    CHECK_ERROR(vkGetSwapchainImagesKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, &imageCount, m_swapChain.images.data()));

    m_swapChain.colorFormat = surfaceFormat.format;
    m_swapChain.extent = extent;
    m_swapChain.imageCount = imageCount;
    m_swapChain.views.resize(m_swapChain.images.size());

    for (uint32_t i = 0; i < m_swapChain.images.size(); ++i)
    {
        m_swapChain.views[i] = CreateImageView(m_swapChain.images[i], m_swapChain.colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1u);
    }
}

uint32_t RaytracingPipeline::GetMemoryType(uint32_t p_typeBits, VkMemoryPropertyFlags p_properties, VkBool32* p_memTypeFound) const
{
    for (uint32_t i = 0; i < m_vulkanDevice.gpuMemoryProperties.memoryTypeCount; i++)
    {
        if ((p_typeBits & 1) == 1)
        {
            if ((m_vulkanDevice.gpuMemoryProperties.memoryTypes[i].propertyFlags & p_properties) == p_properties)
            {
                if (p_memTypeFound)
                {
                    *p_memTypeFound = true;
                }
                return i;
            }
        }
        p_typeBits >>= 1;
    }

    if (p_memTypeFound)
    {
        *p_memTypeFound = false;
        return 0;
    }
    throw std::runtime_error("Could not find a matching memory type");
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

    for (auto& sem : m_imageAvailableSemaphores)
        vkDestroySemaphore(m_vulkanDevice.logicalDevice, sem, nullptr);

    for (auto& sem : m_renderFinishedSemaphores)
        vkDestroySemaphore(m_vulkanDevice.logicalDevice, sem, nullptr);

    vkDestroyPipeline(m_vulkanDevice.logicalDevice, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanDevice.logicalDevice, m_pipelineLayout, nullptr);
    vkDestroySwapchainKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, nullptr);
    vkDestroyDescriptorPool(m_vulkanDevice.logicalDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanDevice.logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyImage(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilImage, nullptr);
    vkDestroyImageView(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilView, nullptr);
    vkFreeMemory(m_vulkanDevice.logicalDevice, m_depthStencil.m_stencilMemory, nullptr);

    vkQueueWaitIdle(m_graphicsQueue);
    for (auto accel : m_BLAS)
        vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, accel.accelerationStructure, nullptr);

    vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, m_TLAS.accelerationStructure, nullptr);

    vkDestroyPipelineCache(m_vulkanDevice.logicalDevice, m_pipelineCache, nullptr);

    vkDestroyCommandPool(m_vulkanDevice.logicalDevice, m_commandPool, nullptr);
    DestroyShaderBuffers(false);
    vkQueueWaitIdle(m_graphicsQueue);
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

void OgEngine::RaytracingPipeline::SetupRenderPass(ORenderPass& renderPass)
{
    renderPass.width = m_width;
    renderPass.height = m_height;

    VkFormat fbDepthFormat;
    VkBool32 validDepthFormat = GetSupportedDepthFormat(m_vulkanDevice.gpu, &fbDepthFormat);
    assert(validDepthFormat);


    VkImageCreateInfo image = Initializers::imageCreateInfo();
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = VK_FORMAT_B8G8R8A8_UNORM;
    image.extent.width = renderPass.width;
    image.extent.height = renderPass.height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
    VkMemoryRequirements memReqs;

    CHECK_ERROR(vkCreateImage(m_vulkanDevice.logicalDevice, &image, nullptr, &renderPass.color.image));
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, renderPass.color.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memAlloc, nullptr, &renderPass.color.mem));
    CHECK_ERROR(vkBindImageMemory(m_vulkanDevice.logicalDevice, renderPass.color.image, renderPass.color.mem, 0));

    VkImageViewCreateInfo colorImageView = Initializers::imageViewCreateInfo();
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;
    colorImageView.image = renderPass.color.image;
    CHECK_ERROR(vkCreateImageView(m_vulkanDevice.logicalDevice, &colorImageView, nullptr, &renderPass.color.view));

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
    CHECK_ERROR(vkCreateSampler(m_vulkanDevice.logicalDevice, &samplerInfo, nullptr, &renderPass.sampler));

    image.format = fbDepthFormat;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    CHECK_ERROR(vkCreateImage(m_vulkanDevice.logicalDevice, &image, nullptr, &renderPass.depth.image));
    vkGetImageMemoryRequirements(m_vulkanDevice.logicalDevice, renderPass.depth.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memAlloc, nullptr, &renderPass.depth.mem));
    CHECK_ERROR(vkBindImageMemory(m_vulkanDevice.logicalDevice, renderPass.depth.image, renderPass.depth.mem, 0));

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
    depthStencilView.image = renderPass.depth.image;
    CHECK_ERROR(vkCreateImageView(m_vulkanDevice.logicalDevice, &depthStencilView, nullptr, &renderPass.depth.view));


    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChain.colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

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
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    CHECK_ERROR(vkCreateRenderPass(m_vulkanDevice.logicalDevice, &renderPassInfo, nullptr, &renderPass.renderPass));

    renderPass.frameBuffers.resize(m_swapChain.views.size());
    for (size_t i = 0; i < m_swapChain.views.size(); i++) 
    {
        VkImageView attachments[] = 
        {
             m_swapChain.views[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChain.extent.width;
        framebufferInfo.height = m_swapChain.extent.height;
        framebufferInfo.layers = 1;

        CHECK_ERROR(vkCreateFramebuffer(m_vulkanDevice.logicalDevice, &framebufferInfo, nullptr, &renderPass.frameBuffers[i]));
    }

    renderPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    renderPass.descriptor.imageView = renderPass.color.view;
    renderPass.descriptor.sampler = renderPass.sampler;
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

void RaytracingPipeline::CreateTextureMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) const
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
VkDescriptorImageInfo RaytracingPipeline::CreateTextureDescriptor(const VkDevice& p_device, TextureData& p_image, const VkSamplerCreateInfo& p_samplerCreateInfo, const VkFormat& p_format, const VkImageLayout& p_layout)
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

void RaytracingPipeline::SetupRaytracingPipeline()
{
    ConfigureRaytracingCommands();
    FindQueueFamilies();
    CreateCommandPool();
    InitSwapChain(m_width, m_height, false);
    SetupRenderPass(m_mainRenderPass);
    CreateCommandBuffers();
    //SetupDepthStencil();
    CreatePipelineCache();
    InitSyncObjects();
    SetupPipelineAndBind();
}

void RaytracingPipeline::InitSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapChain.images.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        if (vkCreateSemaphore(m_vulkanDevice.logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_vulkanDevice.logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_vulkanDevice.logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
void RaytracingPipeline::DestroyObject(uint64_t p_id)
{
    std::vector<uint64_t>::iterator it = std::find(m_objectIDs.begin(), m_objectIDs.end(), p_id);

    if (it != m_objectIDs.end())
    {

        int id = std::distance(m_objectIDs.begin(), it);

        m_objectIDs.erase(it);
        m_objectBlasIDs.erase(m_objectBlasIDs.begin() + id);
        m_textureIDs.erase(m_textureIDs.begin() + id);
        m_normalMapIDs.erase(m_normalMapIDs.begin() + id);
        m_instances.erase(m_instances.begin() + id);
        m_shaderData.materialBuffer.erase(m_shaderData.materialBuffer.begin() + id);
        m_objects[id].m_geometryBuffer.Destroy();

        //vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, m_BLAS[id].accelerationStructure, nullptr);
        std::vector<std::pair<Mesh*, int>>::iterator it;
        int meshID = 0;
        /*for (it = m_instanceTracker.begin(); it != m_instanceTracker.end(); it++)
        {
            if (it->first->MeshName().compare(m_objects[id].m_mesh->MeshName()) == 0)
            {
                it->second--;
                if (it->second <= 0)
                {
                    vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, m_BLAS[meshID].accelerationStructure, nullptr);
                    m_shaderData.indexBuffer.erase(m_shaderData.indexBuffer.begin() + meshID);
                    m_shaderData.vertexBuffer.erase(m_shaderData.vertexBuffer.begin() + meshID);
                    m_instanceTracker.erase(it);
                }
                break;
            }

            meshID++;
        }*/

        m_objects.erase(m_objects.begin() + id);

        return;
    }
    ReloadPipeline();
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
    ReloadPipeline();
}

void OgEngine::RaytracingPipeline::DestroyAllObjects()
{

    m_objectIDs.erase(m_objectIDs.begin() + 1, m_objectIDs.end());
    m_objectBlasIDs.erase(m_objectBlasIDs.begin() + 1, m_objectBlasIDs.end());
    m_textureIDs.erase(m_textureIDs.begin() + 1, m_textureIDs.end());
    m_normalMapIDs.erase(m_normalMapIDs.begin() + 1, m_normalMapIDs.end());
    m_instances.erase(m_instances.begin() + 1, m_instances.end());
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
    m_instanceTracker.erase(m_instanceTracker.begin() + 1, m_instanceTracker.end());
    m_shaderData.indexBuffer.erase(m_shaderData.indexBuffer.begin() + 1, m_shaderData.indexBuffer.end());
    m_shaderData.vertexBuffer.erase(m_shaderData.vertexBuffer.begin() + 1, m_shaderData.vertexBuffer.end());

    ReloadPipeline();
}

void OgEngine::RaytracingPipeline::UpdateTLAS()
{
    if (m_objects.size() == 0)
        return;

    const VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    m_instances.clear();

    for (auto object : m_objects)
    {
        m_instances.push_back(object.m_geometry);
    }

    Buffer instanceBuffer;
    CreateBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &instanceBuffer,
        sizeof(GeometryInstance) * m_instances.size(),
        m_instances.data());

    AccelerationStructure newDataAS;
    CreateTopLevelAccelerationStructure(newDataAS, m_instances.size());

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
    buildInfo.instanceCount = m_instances.size();

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

    if (m_TLAS.accelerationStructure != VK_NULL_HANDLE)
    {
        vkCmdCopyAccelerationStructureNV(cmdBuffer, m_TLAS.accelerationStructure, newDataAS.accelerationStructure, VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_NV);
        QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue, true);
        vkDestroyAccelerationStructureNV(m_vulkanDevice.logicalDevice, newDataAS.accelerationStructure, nullptr);
        vkFreeMemory(m_vulkanDevice.logicalDevice, newDataAS.memory, nullptr);
    }
    else
        m_TLAS = newDataAS;

    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue, true);

    scratchBuffer.Destroy();
    instanceBuffer.Destroy();
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
    m_sceneID = ImGui_ImplVulkan_AddTexture(m_mainRenderPass.sampler, m_mainRenderPass.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
    std::vector<uint64_t>::iterator it = std::find(m_lightsIDs.begin(), m_lightsIDs.end(), p_id);
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

    }
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

    ReloadPipeline();
}



VkResult RaytracingPipeline::AcquireNextImage(uint32_t* p_imageIndex) const
{
    if (m_swapChain.swapChain == VK_NULL_HANDLE)
        throw std::runtime_error("No swapchain available, swapchain -> NULL");

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
    vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCreateAccelerationStructureNV"));
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
    vkCmdCopyAccelerationStructureNV = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.logicalDevice, "vkCmdCopyAccelerationStructureNV"));
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

void RaytracingPipeline::CreateBottomLevelAccelerationStructure(const VkGeometryNV* p_geometries)
{
    AccelerationStructure accel{};
    VkAccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    accelerationStructureInfo.instanceCount = 0;
    accelerationStructureInfo.geometryCount = 1;
    accelerationStructureInfo.pGeometries = p_geometries;
    accelerationStructureInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_NV;

    VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    accelerationStructureCI.info = accelerationStructureInfo;

    CHECK_ERROR(vkCreateAccelerationStructureNV(m_vulkanDevice.logicalDevice, &accelerationStructureCI, nullptr, &accel.accelerationStructure));

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = accel.accelerationStructure;

    VkMemoryRequirements2 memoryRequirements2{};
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo, &memoryRequirements2);

    VkMemoryAllocateInfo memoryAllocateInfo = Initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.logicalDevice, &memoryAllocateInfo, nullptr, &accel.memory));

    VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    accelerationStructureMemoryInfo.accelerationStructure = accel.accelerationStructure;
    accelerationStructureMemoryInfo.memory = accel.memory;
    CHECK_ERROR(vkBindAccelerationStructureMemoryNV(m_vulkanDevice.logicalDevice, 1, &accelerationStructureMemoryInfo));

    CHECK_ERROR(vkGetAccelerationStructureHandleNV(m_vulkanDevice.logicalDevice, accel.accelerationStructure, sizeof(uint64_t), &accel.handle));
    m_BLAS.push_back(accel);
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
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(memory_requierements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
    vkQueueWaitIdle(m_graphicsQueue);
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
    m_objectBlasIDs.push_back(meshBuffersID);
    m_textureIDs.push_back(p_textureID);
    m_normalMapIDs.push_back(p_normID);
    m_instances.push_back(object.m_geometry);
    m_objects.push_back(object);
    ReloadPipeline();
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
    CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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
    allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
    if (p_type == TEXTURE_TYPE::NORMAL)
    {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;

    Buffer stagingBuffer;
    CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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
    allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
    if (p_type == TEXTURE_TYPE::TEXTURE)
    {
        m_textures.push_back(data);
        m_textureCtr.emplace_back(p_texture);
    }
    else if (p_type == TEXTURE_TYPE::NORMAL)
    {
        m_normalMaps.push_back(data);
        m_normalMapsCtr.emplace_back(p_texture);
    }
    ReloadPipeline();
}

void OgEngine::RaytracingPipeline::CreateTopLevelAccelerationStructure()
{
    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    CreateTopLevelAccelerationStructure(m_TLAS, m_instances.size());

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo2{};
    memoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo2.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

    VkMemoryRequirements2 tlasMemsReqs;
    memoryRequirementsInfo2.accelerationStructure = m_TLAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo2, &tlasMemsReqs);

    const VkDeviceSize scratchBufferSize2 = tlasMemsReqs.memoryRequirements.size;

    Buffer scratchBuffer2;
    CreateBuffer(
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &scratchBuffer2,
        scratchBufferSize2);


    VkAccelerationStructureInfoNV buildInfo2{};
    buildInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo2.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    buildInfo2.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_NV;
    buildInfo2.pGeometries = nullptr;
    buildInfo2.geometryCount = 0;
    buildInfo2.instanceCount = m_instances.size();

    CreateBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_instancesBuffer,
        sizeof(GeometryInstance) * m_instances.size(),
        m_instances.data());

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


void RaytracingPipeline::ReloadPipeline()
{
    vkQueueWaitIdle(m_graphicsQueue);
    vkFreeCommandBuffers(m_vulkanDevice.logicalDevice, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());
    CreateCommandBuffers();
    vkDestroyPipeline(m_vulkanDevice.logicalDevice, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanDevice.logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanDevice.logicalDevice, m_descriptorSetLayout, nullptr);
    vkQueueWaitIdle(m_graphicsQueue);
    vkDeviceWaitIdle(m_vulkanDevice.logicalDevice);
    CreatePipeline();
    vkQueueWaitIdle(m_graphicsQueue);
    CreateDescriptorSets();
    vkQueueWaitIdle(m_graphicsQueue);
    DispatchRays();
}

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
    vertexBufferBinding.descriptorCount = m_shaderData.vertexBuffer.size();
    vertexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding indexBufferBinding{};
    indexBufferBinding.binding = 4;
    indexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indexBufferBinding.descriptorCount = m_shaderData.indexBuffer.size();
    indexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding textureIDBinding{};
    textureIDBinding.binding = 5;
    textureIDBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    textureIDBinding.descriptorCount = m_textureIDs.size() == 0 ? 1 : m_textureIDs.size();
    textureIDBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding materialBufferBinding{};
    materialBufferBinding.binding = 6;
    materialBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    materialBufferBinding.descriptorCount = m_shaderData.materialBuffer.size();
    materialBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding objectBLASBinding{};
    objectBLASBinding.binding = 7;
    objectBLASBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectBLASBinding.descriptorCount = m_objectBlasIDs.size() == 0 ? 1 : m_objectBlasIDs.size();
    objectBLASBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding normalMapIDBinding{};
    normalMapIDBinding.binding = 8;
    normalMapIDBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    normalMapIDBinding.descriptorCount = m_normalMapIDs.size() == 0 ? 1 : m_normalMapIDs.size();
    normalMapIDBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding lightBufferBinding{};
    lightBufferBinding.binding = 9;
    lightBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightBufferBinding.descriptorCount = m_shaderData.lightBuffer.size();
    lightBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 10;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = m_textures.size();
    textureBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

    VkDescriptorSetLayoutBinding normalMapBinding{};
    normalMapBinding.binding = 11;
    normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalMapBinding.descriptorCount = m_normalMaps.size();
    normalMapBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

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
        });

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    CHECK_ERROR(vkCreateDescriptorSetLayout(m_vulkanDevice.logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

    CHECK_ERROR(vkCreatePipelineLayout(m_vulkanDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));
    LoadShaders();
}
void RaytracingPipeline::ReloadShaders()
{
    CHECK_ERROR(vkQueueWaitIdle(m_graphicsQueue));
    vkFreeCommandBuffers(m_vulkanDevice.logicalDevice, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());
    CreateCommandBuffers();
    vkDestroyPipeline(m_vulkanDevice.logicalDevice, m_pipeline, nullptr);
    CreatePipeline();
    CreateDescriptorSets();
    DispatchRays();
}
void RaytracingPipeline::LoadShaders()
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
    // Scene closest hit shader group
    groups[INDEX_CLOSEST_HIT].closestHitShader = shaderIndexClosestHit;
    groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
    // Scene miss shader group
    groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_MISS].generalShader = shaderIndexMiss;
    // Shadow miss shader group 
    groups[INDEX_SHADOW_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_SHADOW_MISS].generalShader = shaderIndexShadowMiss;

    VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
    rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayPipelineInfo.pStages = shaderStages.data();
    rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups = groups.data();
    rayPipelineInfo.maxRecursionDepth = 8;
    rayPipelineInfo.layout = m_pipelineLayout;
    vkCreateRayTracingPipelinesNV(m_vulkanDevice.logicalDevice, nullptr, 1, &rayPipelineInfo, nullptr, &m_pipeline);
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

void RaytracingPipeline::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(m_vulkanDevice.logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
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

/*void RaytracingPipeline::SetupDepthStencil()
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
    memory_alloc.memoryTypeIndex = GetMemoryType(memory_requierements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
}*/

VkResult RaytracingPipeline::CreateBuffer(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memoryPropertyFlags, Buffer* p_buffer, VkDeviceSize p_size, void* p_data) const
{
    p_buffer->device = m_vulkanDevice.logicalDevice;


    VkBufferCreateInfo bufferCreateInfo = Initializers::bufferCreateInfo(p_usageFlags, p_size);
    vkCreateBuffer(m_vulkanDevice.logicalDevice, &bufferCreateInfo, nullptr, &p_buffer->buffer);

    VkMemoryRequirements memory_requierements;
    VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(m_vulkanDevice.logicalDevice, p_buffer->buffer, &memory_requierements);
    memAlloc.allocationSize = memory_requierements.size;

    memAlloc.memoryTypeIndex = GetMemoryType(memory_requierements.memoryTypeBits, p_memoryPropertyFlags);
    vkAllocateMemory(m_vulkanDevice.logicalDevice, &memAlloc, nullptr, &p_buffer->memory);

    p_buffer->alignment = memory_requierements.alignment;
    p_buffer->size = memAlloc.allocationSize;
    p_buffer->usageFlags = p_usageFlags;
    p_buffer->memoryPropertyFlags = p_memoryPropertyFlags;


    if (p_data != nullptr)
    {
        p_buffer->Map();
        memcpy(p_buffer->mapped, p_data, p_size);
        if ((p_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            CHECK_ERROR(p_buffer->Flush());

        p_buffer->Unmap();
    }


    p_buffer->SetupDescriptor();


    return p_buffer->Bind();
}

uint32_t OgEngine::RaytracingPipeline::GetAlignedSize(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
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

int OgEngine::RaytracingPipeline::CheckForExistingMesh(Mesh* p_mesh)
{
    std::vector<std::pair<Mesh*, int>>::iterator it;
    int meshID = 0;
    for (it = m_instanceTracker.begin(); it != m_instanceTracker.end(); it++)
    {
        if (it->first->MeshName().compare(p_mesh->MeshName()) == 0)
        {
            it->second++;
            break;
        }

        ++meshID;
    }

    if (it == m_instanceTracker.end())
    {
        VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        Buffer vertexBuffer;
        Buffer indexBuffer;
        CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &vertexBuffer, p_mesh->Vertices().size() * sizeof(Vertex),
            (void*)p_mesh->Vertices().data()));

        CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &indexBuffer, p_mesh->Indices().size() * sizeof(uint32_t),
            (void*)p_mesh->Indices().data()));

        m_meshVertexBuffers.push_back(vertexBuffer);
        m_meshIndexBuffers.push_back(indexBuffer);

        m_shaderData.vertexBuffer.push_back(vertexBuffer);
        m_shaderData.indexBuffer.push_back(indexBuffer);


        VkGeometryNV geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
        geometry.geometry.triangles.vertexData = m_meshVertexBuffers[meshID].buffer;
        geometry.geometry.triangles.vertexOffset = 0;
        geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(p_mesh->Vertices().size());
        geometry.geometry.triangles.vertexStride = sizeof(Vertex);
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.indexData = m_meshIndexBuffers[meshID].buffer;
        geometry.geometry.triangles.indexOffset = 0;
        geometry.geometry.triangles.indexCount = static_cast<uint32_t>(p_mesh->Indices().size());;
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.transformData = nullptr;
        geometry.geometry.triangles.transformOffset = 0;
        geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

        CreateBottomLevelAccelerationStructure(&geometry);
        VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
        memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
        memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

        VkMemoryRequirements2 blasMemsReqs;
        memoryRequirementsInfo.accelerationStructure = m_BLAS.back().accelerationStructure;
        vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.logicalDevice, &memoryRequirementsInfo, &blasMemsReqs);

        const VkDeviceSize scratchBufferSize = blasMemsReqs.memoryRequirements.size;

        Buffer scratchBuffer;
        CHECK_ERROR(CreateBuffer(
            VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &scratchBuffer,
            scratchBufferSize));

        VkAccelerationStructureInfoNV buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_NV;

        vkCmdBuildAccelerationStructureNV(
            cmdBuffer,
            &buildInfo,
            nullptr,
            0,
            VK_FALSE,
            m_BLAS.back().accelerationStructure,
            nullptr,
            scratchBuffer.buffer,
            0);

        VkMemoryBarrier memoryBarrier = Initializers::memoryBarrier();
        memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
        memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
        QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);
        scratchBuffer.Destroy();
        m_instanceTracker.push_back(std::make_pair(p_mesh, 1));
        ReloadPipeline();
    }
    return meshID;
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
    const uint32_t handleSize = m_raytracingProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = GetAlignedSize(m_raytracingProperties.shaderGroupHandleSize, m_raytracingProperties.shaderGroupBaseAlignment);
    const uint32_t groupCount = static_cast<uint32_t>(SHADER_COUNT);
    const uint32_t sbtSize = groupCount * handleSizeAligned;

    CreateBuffer(
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &m_shaderBindingTable,
        sbtSize);
    m_shaderBindingTable.Map();

    //const auto shaderHandleStorage = new uint8_t[sbtSize];

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    // Get shader identifiers
    CHECK_ERROR(vkGetRayTracingShaderGroupHandlesNV(m_vulkanDevice.logicalDevice, m_pipeline, 0, SHADER_COUNT, sbtSize, shaderHandleStorage.data()));
    auto* data = static_cast<uint8_t*>(m_shaderBindingTable.mapped);
    // Copy the shader identifiers to the shader binding table
    data += CopyShaderIdentifier(data, shaderHandleStorage.data(), INDEX_RAYGEN);
    data += CopyShaderIdentifier(data, shaderHandleStorage.data(), INDEX_MISS);
    data += CopyShaderIdentifier(data, shaderHandleStorage.data(), INDEX_SHADOW_MISS);
    data += CopyShaderIdentifier(data, shaderHandleStorage.data(), INDEX_CLOSEST_HIT);
    
    m_shaderBindingTable.Unmap();
}

VkDeviceSize RaytracingPipeline::CopyShaderIdentifier(uint8_t* p_data, const uint8_t* p_shaderHandleStorage, uint8_t p_groupIndex) const
{
    const uint32_t shaderGroupHandleSize = m_raytracingProperties.shaderGroupHandleSize;
    memcpy(p_data, p_shaderHandleStorage + p_groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
    p_data += shaderGroupHandleSize;
    return shaderGroupHandleSize;
}

void RaytracingPipeline::CreateDescriptorSets()
{
    if (m_descriptorPool != VK_NULL_HANDLE)
    {
        CHECK_ERROR(vkFreeDescriptorSets(m_vulkanDevice.logicalDevice, m_descriptorPool, 1, &m_descriptorSet));
        vkDestroyDescriptorPool(m_vulkanDevice.logicalDevice, m_descriptorPool, nullptr);
    }

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
    CHECK_ERROR(vkCreateDescriptorPool(m_vulkanDevice.logicalDevice, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool));

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = Initializers::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);

    CHECK_ERROR(vkAllocateDescriptorSets(m_vulkanDevice.logicalDevice, &descriptorSetAllocateInfo, &m_descriptorSet));
    
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
    for (const auto& tex : m_textures)
        textureInfos.push_back(tex.info);

    std::vector<VkDescriptorImageInfo> normalMapInfos;
    for (const auto& norm : m_normalMaps)
        normalMapInfos.push_back(norm.info);


    std::vector<VkDescriptorBufferInfo> vertexDescriptor;
    for (auto buf : m_shaderData.vertexBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        vertexDescriptor.push_back(buf.descriptor);
    }

    std::vector<VkDescriptorBufferInfo> indexDescriptor;
    for (auto buf : m_shaderData.indexBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        indexDescriptor.push_back(buf.descriptor);
    }

    std::vector<VkDescriptorBufferInfo> materialDescriptor;
    for (auto buf : m_shaderData.materialBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        materialDescriptor.push_back(buf.descriptor);
    }

    std::vector<VkDescriptorBufferInfo> lightDescriptor;
    for (auto buf : m_shaderData.lightBuffer)
    {
        buf.alignment = VK_WHOLE_SIZE;
        lightDescriptor.push_back(buf.descriptor);
    }

    VkWriteDescriptorSet textureDescriptor{};
    textureDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptor.dstSet = m_descriptorSet;
    textureDescriptor.dstBinding = 10;
    textureDescriptor.dstArrayElement = 0;
    textureDescriptor.descriptorCount = m_textures.size();
    textureDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptor.pImageInfo = textureInfos.data();

    VkWriteDescriptorSet normalMapDescriptor{};
    normalMapDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    normalMapDescriptor.dstSet = m_descriptorSet;
    normalMapDescriptor.dstBinding = 11;
    normalMapDescriptor.dstArrayElement = 0;
    normalMapDescriptor.descriptorCount = m_normalMaps.size();
    normalMapDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalMapDescriptor.pImageInfo = normalMapInfos.data();

    const VkWriteDescriptorSet resultImageWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
    const VkWriteDescriptorSet uniformBufferWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &m_cameraBuffer.descriptor);
    const VkWriteDescriptorSet vertexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, vertexDescriptor.data(), vertexDescriptor.size());
    const VkWriteDescriptorSet indexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, indexDescriptor.data(), indexDescriptor.size());
    const VkWriteDescriptorSet textureIDWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, &m_shaderData.textureIDBuffer.descriptor);
    const VkWriteDescriptorSet materialWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, materialDescriptor.data(), materialDescriptor.size());
    const VkWriteDescriptorSet objectBLASWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7, &m_shaderData.objectBLASbuffer.descriptor);
    const VkWriteDescriptorSet normalMapWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8, &m_shaderData.normalMapIDBuffer.descriptor);
    const VkWriteDescriptorSet lightWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 9, lightDescriptor.data(), lightDescriptor.size());
    
    m_writeDescriptorSets.clear();
    
    if(m_TLAS.accelerationStructure != VK_NULL_HANDLE)
        m_writeDescriptorSets.push_back(accelerationStructureWrite);

    if(resultImageWrite.descriptorCount > 0)
        m_writeDescriptorSets.push_back(resultImageWrite);

    if (uniformBufferWrite.descriptorCount > 0)
        m_writeDescriptorSets.push_back(uniformBufferWrite);

    if (vertexWrite.descriptorCount > 0)
        m_writeDescriptorSets.push_back(vertexWrite);

    if (indexWrite.descriptorCount > 0)
        m_writeDescriptorSets.push_back(indexWrite);

    if (m_textures.size() > 0)
        m_writeDescriptorSets.push_back(textureDescriptor);

    if (m_textureIDs.size() > 0)
        m_writeDescriptorSets.push_back(textureIDWrite);

    if (materialWrite.descriptorCount > 0)
        m_writeDescriptorSets.push_back(materialWrite);

    if (m_objectBlasIDs.size() > 0)
        m_writeDescriptorSets.push_back(objectBLASWrite);

    if (m_normalMaps.size() > 0)
        m_writeDescriptorSets.push_back(normalMapDescriptor);

    if (m_normalMapIDs.size() > 0)
        m_writeDescriptorSets.push_back(normalMapWrite);

    if (lightWrite.descriptorCount > 0)
        m_writeDescriptorSets.push_back(lightWrite);

    vkUpdateDescriptorSets(m_vulkanDevice.logicalDevice, m_writeDescriptorSets.size(), m_writeDescriptorSets.data(), 0, nullptr);
}

void RaytracingPipeline::CreateCamera()
{

    m_camera.SetPerspective(90.0f, static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 1024.0f);
    m_cameraData.viewInverse = glm::inverse(m_camera.matrices.view);
    m_cameraData.projInverse = glm::inverse(m_camera.matrices.perspective);
    m_cameraData.data = glm::vec4(0);
    m_cameraData.settings = glm::vec4(0);
    m_cameraData.samples = glm::vec4(0);

    m_cameraData.samples.x = 2;
    m_cameraData.samples.y = 0;

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_cameraBuffer,
        sizeof(m_cameraData),
        &m_cameraData));
}

void RaytracingPipeline::UpdateCamera()
{

    if (isRefreshing)
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

void RaytracingPipeline::DispatchRays()
{
    VkCommandBufferBeginInfo cmdBufInfo = Initializers::commandBufferBeginInfo();
    const VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    for (size_t i = 0; i < m_commandBuffers.size(); ++i)
    {
        CHECK_ERROR(vkBeginCommandBuffer(m_commandBuffers[i], &cmdBufInfo));
             
        VkDeviceSize bindingOffsetRayGenShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_RAYGEN;
        VkDeviceSize bindingOffsetMissShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_MISS;
        VkDeviceSize bindingOffsetHitShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT;
        VkDeviceSize bindingStride = m_raytracingProperties.shaderGroupHandleSize;

      /*VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_mainRenderPass.renderPass;
        renderPassInfo.framebuffer = m_mainRenderPass.frameBuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChain.extent;

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);*/

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipeline);
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
        
        vkCmdTraceRaysNV(m_commandBuffers[i],
            m_shaderBindingTable.buffer, bindingOffsetRayGenShader,
            m_shaderBindingTable.buffer, bindingOffsetMissShader, bindingStride,
            m_shaderBindingTable.buffer, bindingOffsetHitShader, bindingStride,
            VK_NULL_HANDLE, 0, 0,
            m_sceneReswidth, m_sceneResheight, 1);

        //vkCmdEndRenderPass(m_commandBuffers[i]);

        SetImageLayout(
            m_commandBuffers[i],
            m_mainRenderPass.color.image,
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
        vkCmdCopyImage(m_commandBuffers[i], m_storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_mainRenderPass.color.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        SetImageLayout(
            m_commandBuffers[i],
            m_mainRenderPass.color.image,
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
    vkQueueWaitIdle(m_graphicsQueue);
    vkQueueWaitIdle(m_presentQueue);
    CleanPipeline();
    SetupRaytracingPipeline();
}

void RaytracingPipeline::SetupPipelineAndBind()
{
    m_raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &m_raytracingProperties;
    vkGetPhysicalDeviceProperties2(m_vulkanDevice.gpu, &deviceProps2);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


    m_submitInfo = Initializers::submitInfo();
    const VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    m_submitInfo.pWaitDstStageMask = &flags;
    m_submitInfo.waitSemaphoreCount = 0;
    m_submitInfo.pWaitSemaphores = nullptr;
    m_submitInfo.signalSemaphoreCount = 0;
    m_submitInfo.pSignalSemaphores = nullptr;

    //AddTexture("default.png");
    //AddTexture("default.png", TEXTURE_TYPE::NORMAL);
    /*RTMaterial mat;
    mat.albedo = glm::vec4(1, 1, 1, 0);
    mat.data.z = 987654;
    mat.data.x = 0.0;*/
    //AddEntity(99999, ResourceManager::Get<OgEngine::Mesh>("cube.obj"), 0, mat, 12345);
    //UpdateLight(123456789, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0);
    //CreateTopLevelAccelerationStructure();

    vkQueueWaitIdle(m_graphicsQueue);
    CreateCamera();

    CreateStorageImage(m_storageImage);
    CreatePipeline();
    CreateShaderBindingTable();

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_shaderData.textureIDBuffer,
        MAX_OBJECTS * sizeof(uint32_t),
        m_textureIDs.data()));

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_shaderData.normalMapIDBuffer,
        MAX_OBJECTS * sizeof(uint32_t),
        m_normalMapIDs.data()));

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_shaderData.objectBLASbuffer,
        MAX_OBJECTS * sizeof(uint32_t),
        m_objectBlasIDs.data()));

    CreateDescriptorSets();
    DispatchRays();

    InitImGUI();
    SetupImGUIFrameBuffers();
    SetupImGUI();
}

void RaytracingPipeline::RenderFrame()
{
    UpdateCamera();
    CHECK_ERROR(vkWaitForFences(m_vulkanDevice.logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX));

    uint32_t imageIndex;
    CHECK_ERROR(vkAcquireNextImageKHR(m_vulkanDevice.logicalDevice, m_swapChain.swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex));

    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        CHECK_ERROR(vkWaitForFences(m_vulkanDevice.logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX));
    }
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    CHECK_ERROR(vkResetFences(m_vulkanDevice.logicalDevice, 1, &m_inFlightFences[m_currentFrame]));

    CHECK_ERROR(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapChain.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    RenderUI(imageIndex);
    vkQueuePresentKHR(m_presentQueue, &presentInfo);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    /*m_submitInfo.commandBufferCount = 1;
    m_submitInfo.pCommandBuffers = &m_commandBuffers[m_imageIndex];

    VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
    VkFence fence;
    CHECK_ERROR(vkCreateFence(m_vulkanDevice.logicalDevice, &fenceInfo, nullptr, &fence));

    CHECK_ERROR(vkQueueSubmit(m_graphicsQueue, 1, &m_submitInfo, fence));
    CHECK_ERROR(vkWaitForFences(m_vulkanDevice.logicalDevice, 1, &fence, VK_TRUE, 100000000000));
    vkDestroyFence(m_vulkanDevice.logicalDevice, fence, nullptr);

    RenderUI(m_imageIndex);*/
}
