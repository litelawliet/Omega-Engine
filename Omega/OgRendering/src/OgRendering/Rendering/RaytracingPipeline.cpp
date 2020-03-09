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
#define INDEX_CLOSEST_HIT 2

using namespace OgEngine;


void RaytracingPipeline::CHECK_ERROR(VkResult result)
{
    if (result != VK_SUCCESS)
    {
        std::cout << result << '\n';
        throw std::runtime_error(("Error with Vulkan function"));
    }
}

void RaytracingPipeline::SetupSwapchain(uint32_t width, uint32_t height, bool vsync)
{
    if (!GetSupportedDepthFormat(m_vulkanDevice.m_gpu, &m_depthFormat))
        throw std::runtime_error("can't find suitable format");

    VkSwapchainKHR oldSwapchain = m_swapChain.swapChain;

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCaps;
    CHECK_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanDevice.m_gpu, m_vulkanDevice.m_surface, &surfCaps));

    // Get available present modes
    uint32_t presentModeCount;
    CHECK_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(m_vulkanDevice.m_gpu, m_vulkanDevice.m_surface, &presentModeCount, nullptr));
    assert(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    CHECK_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(m_vulkanDevice.m_gpu, m_vulkanDevice.m_surface, &presentModeCount, presentModes.data()));

    VkExtent2D swapchain_extent = {};
    // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
    if (surfCaps.currentExtent.width == static_cast<uint32_t>(-1))
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchain_extent.width = width;
        swapchain_extent.height = height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchain_extent = surfCaps.currentExtent;
        width = surfCaps.currentExtent.width;
        height = surfCaps.currentExtent.height;
    }


    // Select a present mode for the swapchain

    // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
    // This mode waits for the vertical blank ("v-sync")
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // If v-sync is not requested, try to find a mailbox mode
    // It's the lowest latency non-tearing present mode available
    if (!vsync)
    {
        for (size_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if ((swapchain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
            {
                swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Determine the number of images
    uint32_t desired_swapchain_images = surfCaps.minImageCount + 1;
    if ((surfCaps.maxImageCount > 0) && (desired_swapchain_images > surfCaps.maxImageCount))
    {
        desired_swapchain_images = surfCaps.maxImageCount;
    }

    // Find the transformation of the surface
    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        // We prefer a non-rotated transform
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = surfCaps.currentTransform;
    }

    // Find a supported composite alpha format (not all devices support alpha opaque)
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Simply select the first composite alpha format available
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags)
    {
        if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag)
        {
            compositeAlpha = compositeAlphaFlag;
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = nullptr;
    swapchain_ci.surface = m_vulkanDevice.m_surface;
    swapchain_ci.minImageCount = desired_swapchain_images;
    swapchain_ci.imageFormat = m_swapChain.colorFormat;
    swapchain_ci.imageColorSpace = m_swapChain.colorSpace;
    swapchain_ci.imageExtent = { swapchain_extent.width, swapchain_extent.height };
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;
    swapchain_ci.presentMode = swapchain_present_mode;
    swapchain_ci.oldSwapchain = oldSwapchain;
    // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
    swapchain_ci.clipped = VK_TRUE;
    swapchain_ci.compositeAlpha = compositeAlpha;

    // Enable transfer source on swap chain images if supported
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // Enable transfer destination on swap chain images if supported
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    CHECK_ERROR(vkCreateSwapchainKHR(m_vulkanDevice.m_logicalDevice, &swapchain_ci, nullptr, &m_swapChain.swapChain));

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if (oldSwapchain != nullptr)
    {
        for (uint32_t i = 0; i < m_swapChain.imageCount; i++)
        {
            vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_swapChain.buffers[i].view, nullptr);
        }
        vkDestroySwapchainKHR(m_vulkanDevice.m_logicalDevice, oldSwapchain, nullptr);
    }
    CHECK_ERROR(vkGetSwapchainImagesKHR(m_vulkanDevice.m_logicalDevice, m_swapChain.swapChain, &m_swapChain.imageCount, nullptr));

    // Get the swap chain images
    m_swapChain.images.resize(m_swapChain.imageCount);
    CHECK_ERROR(vkGetSwapchainImagesKHR(m_vulkanDevice.m_logicalDevice, m_swapChain.swapChain, &m_swapChain.imageCount, m_swapChain.images.data()));

    // Get the swap chain buffers containing the image and imageview
    m_swapChain.buffers.resize(m_swapChain.imageCount);
    for (uint32_t i = 0; i < m_swapChain.imageCount; i++)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = nullptr;
        colorAttachmentView.format = m_swapChain.colorFormat;
        colorAttachmentView.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;

        m_swapChain.buffers[i].image = m_swapChain.images[i];

        colorAttachmentView.image = m_swapChain.buffers[i].image;

        vkCreateImageView(m_vulkanDevice.m_logicalDevice, &colorAttachmentView, nullptr, &m_swapChain.buffers[i].view);
    }
}

uint32_t RaytracingPipeline::GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const
{
    for (uint32_t i = 0; i < m_vulkanDevice.m_gpuMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((m_vulkanDevice.m_gpuMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memTypeFound)
                {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound)
    {
        *memTypeFound = false;
        return 0;
    }
    throw std::runtime_error("Could not find a matching memory type");
}

VkCommandBuffer RaytracingPipeline::CreateCommandBuffer(VkCommandBufferLevel level, bool begin) const
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = Initializers::commandBufferAllocateInfo(m_commandPool, level, 1);

    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(m_vulkanDevice.m_logicalDevice, &cmdBufAllocateInfo, &cmdBuffer);

    // If requested, also start recording for the new command buffer
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = Initializers::commandBufferBeginInfo();
        vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
    }

    return cmdBuffer;
}


void RaytracingPipeline::CleanUp()
{
    //if (enableValidationLayers)
     //   DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

    if (m_swapChain.swapChain != nullptr)
    {
        for (uint32_t i = 0; i < m_swapChain.imageCount; i++)
        {
            vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_swapChain.buffers[i].view, nullptr);
        }
    }
    if (m_vulkanDevice.m_surface != nullptr)
    {
        vkDestroySwapchainKHR(m_vulkanDevice.m_logicalDevice, m_swapChain.swapChain, nullptr);
        //vkDestroySurfaceKHR(instance, device.surface, nullptr);
    }
    m_swapChain.swapChain = nullptr;

    for (auto frame_buffer : m_swapchainFrameBuffers)
    {
        vkDestroyFramebuffer(m_vulkanDevice.m_logicalDevice, frame_buffer, nullptr);
    }

    vkDestroyPipeline(m_vulkanDevice.m_logicalDevice, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanDevice.m_logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_vulkanDevice.m_logicalDevice, m_renderpass, nullptr);

    vkDestroySwapchainKHR(m_vulkanDevice.m_logicalDevice, m_swapChain.swapChain, nullptr);
    vkDestroyDescriptorPool(m_vulkanDevice.m_logicalDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanDevice.m_logicalDevice, m_descriptorSetLayout, nullptr);
    
    vkDestroyImage(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilImage, nullptr);
    vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilView, nullptr);
    vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilMemory, nullptr);

    for (auto accel : m_BLAS)
        vkDestroyAccelerationStructureNV(m_vulkanDevice.m_logicalDevice, accel.accelerationStructure, nullptr);

    vkDestroyAccelerationStructureNV(m_vulkanDevice.m_logicalDevice, m_TLAS.accelerationStructure, nullptr);
    
    vkDestroyPipelineCache(m_vulkanDevice.m_logicalDevice, m_pipelineCache, nullptr);

    for (auto fence : m_waitFences)
        vkDestroyFence(m_vulkanDevice.m_logicalDevice, fence, nullptr);

    vkDestroySemaphore(m_vulkanDevice.m_logicalDevice, m_semaphores.presentComplete, nullptr);
    vkDestroySemaphore(m_vulkanDevice.m_logicalDevice, m_semaphores.renderComplete, nullptr);
    
    vkDestroyCommandPool(m_vulkanDevice.m_logicalDevice, m_commandPool, nullptr);

    DestroyShaderBuffers(false);

   // glfwDestroyWindow(GetWindow());
    //glfwTerminate();
}
void OgEngine::RaytracingPipeline::ResizeCleanup()
{
    if (m_swapChain.swapChain != nullptr)
    {
        for (uint32_t i = 0; i < m_swapChain.imageCount; i++)
        {
            vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_swapChain.buffers[i].view, nullptr);
        }
    }
    if (m_vulkanDevice.m_surface != nullptr)
    {
        vkDestroySwapchainKHR(m_vulkanDevice.m_logicalDevice, m_swapChain.swapChain, nullptr);
    }

    m_swapChain.swapChain = VK_NULL_HANDLE;
    for (auto frame_buffer : m_swapchainFrameBuffers)
    {
        vkDestroyFramebuffer(m_vulkanDevice.m_logicalDevice, frame_buffer, nullptr);
    }

    vkFreeCommandBuffers(m_vulkanDevice.m_logicalDevice, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());
    vkDestroyPipeline(m_vulkanDevice.m_logicalDevice, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_vulkanDevice.m_logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_vulkanDevice.m_logicalDevice, m_renderpass, nullptr);

    vkDestroySwapchainKHR(m_vulkanDevice.m_logicalDevice, m_swapChain.swapChain, nullptr);
    vkDestroyDescriptorPool(m_vulkanDevice.m_logicalDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_vulkanDevice.m_logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyImage(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilImage, nullptr);
    vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilView, nullptr);
    vkDestroyImage(m_vulkanDevice.m_logicalDevice, m_storageImage.image, nullptr);
    vkDestroyImageView(m_vulkanDevice.m_logicalDevice, m_storageImage.view, nullptr);

    vkFreeMemory(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilMemory, nullptr);

    vkDestroyPipelineCache(m_vulkanDevice.m_logicalDevice, m_pipelineCache, nullptr);

    vkDestroyCommandPool(m_vulkanDevice.m_logicalDevice, m_commandPool, nullptr);

    DestroyShaderBuffers(true);
}
void OgEngine::RaytracingPipeline::SetupTestEditor()
{
    ImGui::Begin("Editor");
    {
        if(ImGui::CollapsingHeader("Objects"))
        {
            for (int i = 0; i < m_objects.size(); ++i)
            {
                std::string name = "Object: " + std::to_string(i);
                if (ImGui::CollapsingHeader(name.c_str()))
                {
                    float pos[3] = { m_objects[i].m_geometry.transform[0].w,
                                        m_objects[i].m_geometry.transform[1].w,
                                        m_objects[i].m_geometry.transform[2].w };
                    name = "pos id:" + std::to_string(i);
                    ImGui::InputFloat3(name.c_str(), pos, 2);
                    m_objects[i].m_geometry.transform[0].w = pos[0];
                    m_objects[i].m_geometry.transform[1].w = pos[1];
                    m_objects[i].m_geometry.transform[2].w = pos[2];
                }
            }
        }
    }
    ImGui::End();

}
void OgEngine::RaytracingPipeline::PrepareIMGUIFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void OgEngine::RaytracingPipeline::DrawImGUI()
{
    ImGui::Render();
}
void OgEngine::RaytracingPipeline::DestroyShaderBuffers(bool p_resizedWindow)
{
    m_shaderBindingTable.unmap();
    m_shaderBindingTable.destroy();

    if (!p_resizedWindow)
    {
        m_cameraBuffer.unmap();
        m_cameraBuffer.destroy();
        for (auto buf : m_shaderData.vertexBuffer)
        {
            buf.unmap();
            buf.destroy();
        }
        for (auto buf : m_shaderData.indexBuffer)
        {
            buf.unmap();
            buf.destroy();
        }
    }
}
void OgEngine::RaytracingPipeline::GenerateMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) const
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_vulkanDevice.m_gpu, p_imageFormat, &formatProperties);

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
VkDescriptorImageInfo OgEngine::RaytracingPipeline::Create2DDescriptor(const VkDevice& device, const VkImage& image, const VkSamplerCreateInfo& samplerCreateInfo, const VkFormat& format, const VkImageLayout& layout)
{
    VkImageViewCreateInfo viewInfo = Initializers::imageViewCreateInfo();
    viewInfo.image = image;
    viewInfo.format = format;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, ~0u, 0, 1 };

    VkImageView view;
    vkCreateImageView(m_vulkanDevice.m_logicalDevice, &viewInfo, nullptr, &view);

    VkSampler sampler;
    vkCreateSampler(m_vulkanDevice.m_logicalDevice, &samplerCreateInfo, nullptr, &sampler);

    VkDescriptorImageInfo info = Initializers::descriptorImageInfo(sampler, view, layout);
    return info;
}
void OgEngine::RaytracingPipeline::DestroyObjects()
{
    /*for (auto obj : m_models)
    {
        obj->m_geometryBuffer.unmap();
        obj->m_indexBuffer.unmap();
        obj->m_vertBuffer.unmap();
        
        obj->m_geometryBuffer.destroy();
        obj->m_indexBuffer.destroy();
        obj->m_vertBuffer.destroy();
    }*/
}
void OgEngine::RaytracingPipeline::SetupRaytracingPipeline()
{
    SetRaytracingCmd();
    InitSwapchain();
    CreateCommandPool();
    SetupSwapchain(m_width, m_height, false);
    CreateCommandBuffers();
    CreateSynchronizationPrimitives();
    SetupDepthStencil();
    SetupRenderPass();
    CreatePipelineCache();
    SetupFramebuffer();

    InitRaytracingRenderer(false);
}

void OgEngine::RaytracingPipeline::UpdateShaderData(std::shared_ptr<Mesh> p_mesh)
{
}

void OgEngine::RaytracingPipeline::UpdateTLAS()
{

    const VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    m_instances.clear();
    for (auto object : m_objects)
        m_instances.push_back(object.m_geometry);

    //Get all instances and create a buffer with all of them
    Buffer instanceBuffer;
    CreateBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &instanceBuffer,
        sizeof(GeometryInstance) * m_instances.size(),
        m_instances.data());

    //Generate TLAS
    AccelerationStructure newDataAS;
    CreateTopLevelAccelerationStructure(newDataAS, m_instances.size());

    //Get memory requirements
    VkMemoryRequirements2 memReqTopLevelAS;
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo2{};
    memoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo2.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    memoryRequirementsInfo2.accelerationStructure = newDataAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.m_logicalDevice, &memoryRequirementsInfo2, &memReqTopLevelAS);

    const VkDeviceSize scratchBufferSize = memReqTopLevelAS.memoryRequirements.size;

    //Generate Scratch buffer
    Buffer scratchBuffer;
    CreateBuffer(
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &scratchBuffer,
        scratchBufferSize);

    //Generate build info for TLAS
    VkAccelerationStructureInfoNV buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_NV;
    buildInfo.pGeometries = nullptr;
    buildInfo.geometryCount = 0;
    buildInfo.instanceCount = m_instances.size();

    //Build Actual TLAS
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
    vkDestroyAccelerationStructureNV(m_vulkanDevice.m_logicalDevice, newDataAS.accelerationStructure, nullptr);
    vkFreeMemory(m_vulkanDevice.m_logicalDevice, newDataAS.memory, nullptr);

    scratchBuffer.destroy();
    instanceBuffer.destroy();
}

void OgEngine::RaytracingPipeline::UpdateDescriptorSets()
{

    //Acceleration Structure
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

    std::vector<VkDescriptorImageInfo> infos;
    for (auto tex : m_textures)
        infos.push_back(tex.info);

    std::vector<VkDescriptorBufferInfo> vertexDescriptor;
    std::vector<VkDescriptorBufferInfo> indexDescriptor;

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

    m_shaderData.textureIDBuffer.map();
    memcpy(m_shaderData.textureIDBuffer.mapped, textureIDs.data(), textureIDs.size() * sizeof(uint32_t));
    m_shaderData.textureIDBuffer.unmap();

    VkWriteDescriptorSet textureDescriptor{};
    textureDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptor.dstSet = m_descriptorSet;
    textureDescriptor.dstBinding = 5;
    textureDescriptor.dstArrayElement = 0;
    textureDescriptor.descriptorCount = static_cast<uint32_t>(m_textures.size());
    textureDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptor.pImageInfo = infos.data();


    const VkWriteDescriptorSet resultImageWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
    const VkWriteDescriptorSet uniformBufferWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &m_cameraBuffer.descriptor);
    const VkWriteDescriptorSet vertexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, vertexDescriptor.data(), static_cast<uint32_t>(m_shaderData.vertexBuffer.size()));
    const VkWriteDescriptorSet indexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, indexDescriptor.data(), static_cast<uint32_t>(m_shaderData.indexBuffer.size()));
    const VkWriteDescriptorSet textureIDWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, &m_shaderData.textureIDBuffer.descriptor);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    {
        accelerationStructureWrite,
        resultImageWrite,
        uniformBufferWrite,
        vertexWrite,
        indexWrite,
        textureDescriptor,
        textureIDWrite
    };

    vkUpdateDescriptorSets(m_vulkanDevice.m_logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

}

void OgEngine::RaytracingPipeline::InitImGUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    SetupImGUIStyle();
    //ImGui::StyleColorsDark();
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

    if (vkCreateRenderPass(m_vulkanDevice.m_logicalDevice, &info, nullptr, &m_ImGUIrenderPass) != VK_SUCCESS) 
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

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfoIMGUI = Initializers::descriptorPoolCreateInfo(ImGUIpoolSizes, 1);
    vkCreateDescriptorPool(m_vulkanDevice.m_logicalDevice, &descriptorPoolCreateInfoIMGUI, nullptr, &m_ImGUIdescriptorPool);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_vulkanDevice.m_instance;
    init_info.PhysicalDevice = m_vulkanDevice.m_gpu;
    init_info.Device = m_vulkanDevice.m_logicalDevice;
    init_info.QueueFamily = m_vulkanDevice.m_gpuGraphicFamily.value();
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
    //imGuiCommandPools.resize(imageViews.size());
    m_ImGUIcommandBuffers.resize(m_swapChain.buffers.size());
    for (size_t i = 0; i < m_swapChain.buffers.size(); i++) 
    {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = m_swapChain.queueNodeIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        CHECK_ERROR(vkCreateCommandPool(m_vulkanDevice.m_logicalDevice, &cmdPoolInfo, nullptr, &m_ImGUIcommandPool));

        m_ImGUIcommandBuffers.resize(m_swapChain.imageCount);

        VkCommandBufferAllocateInfo cmdBufAllocateInfo =
            Initializers::commandBufferAllocateInfo(
                m_ImGUIcommandPool,
                VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                static_cast<uint32_t>(m_ImGUIcommandBuffers.size()));

        CHECK_ERROR(vkAllocateCommandBuffers(m_vulkanDevice.m_logicalDevice, &cmdBufAllocateInfo, m_ImGUIcommandBuffers.data()));
    }
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
    colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
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
    //colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
    //colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
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

    // Create frame buffers for every swap chain image
    m_ImGUIframeBuffers.resize(m_swapChain.imageCount);
    for (uint32_t i = 0; i < m_ImGUIframeBuffers.size(); i++)
    {
        attachment[0] = m_swapChain.buffers[i].view;
        CHECK_ERROR(vkCreateFramebuffer(m_vulkanDevice.m_logicalDevice, &frameBufferCreateInfo, nullptr, &m_ImGUIframeBuffers[i]));
    }
}

void OgEngine::RaytracingPipeline::RescaleImGUI()
{
    ImGui::SetNextWindowSize({ (float)m_width, (float)m_height });
    for (int i = 0; i < m_ImGUIframeBuffers.size(); ++i)
        vkDestroyFramebuffer(m_vulkanDevice.m_logicalDevice, m_ImGUIframeBuffers[i], nullptr);
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

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    vkCmdBeginRenderPass(cmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
    vkCmdEndRenderPass(cmdBuffer);

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

    vkDeviceWaitIdle(m_vulkanDevice.m_logicalDevice);
    ResizeCleanup();
    vkQueueWaitIdle(m_graphicsQueue);
    vkQueueWaitIdle(m_presentQueue);
    //m_currentBuffer = 0;

    //SetRaytracingCmd();
    InitSwapchain();
    CreateCommandPool();
    SetupSwapchain(m_width, m_height, false);
    CreateCommandBuffers();
    CreateSynchronizationPrimitives();
    SetupDepthStencil();
    SetupRenderPass();
    CreatePipelineCache();
    SetupFramebuffer();

    InitRaytracingRenderer(true);

}

void OgEngine::RaytracingPipeline::AddModelInGame(uint64_t p_id, std::shared_ptr<Mesh> p_mesh)
{
    Model object = Model(p_mesh, true);
    m_objectIDs.push_back(p_id);

    CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &object.m_vertBuffer, object.m_mesh->Vertices().size() * sizeof(Vertex),
        (void*)object.m_mesh->Vertices().data());

    CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &object.m_indexBuffer, object.m_mesh->Indices().size() * sizeof(uint32_t),
        (void*)object.m_mesh->Indices().data());


    VkGeometryNV geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    geometry.geometry.triangles.vertexData = object.m_vertBuffer.buffer;
    geometry.geometry.triangles.vertexOffset = 0;
    geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(object.m_mesh->Vertices().size());
    geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.indexData = object.m_indexBuffer.buffer;
    geometry.geometry.triangles.indexOffset = 0;
    geometry.geometry.triangles.indexCount = static_cast<uint32_t>(object.m_mesh->Indices().size());;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.transformData = nullptr;
    geometry.geometry.triangles.transformOffset = 0;
    geometry.geometry.aabbs = {};
    geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    CreateBottomLevelAccelerationStructure(&geometry);


    CreateBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &object.m_geometryBuffer,
        sizeof(GeometryInstance),
        &object.m_geometry);

    // Acceleration structure build requires some scratch space to store temporary information
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

    VkMemoryRequirements2 blasMemsReqs;
    memoryRequirementsInfo.accelerationStructure = m_BLAS.back().accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.m_logicalDevice, &memoryRequirementsInfo, &blasMemsReqs);

    const VkDeviceSize scratchBufferSize = blasMemsReqs.memoryRequirements.size;

    Buffer scratchBuffer;
    CreateBuffer(
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &scratchBuffer,
        scratchBufferSize);

    VkAccelerationStructureInfoNV buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

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
    scratchBuffer.destroy();

    object.m_geometry.instanceId = TLASinstanceID;
    object.m_geometry.accelerationStructureHandle = m_BLAS.back().handle;
    TLASinstanceID++;

    m_BLASmeshes.push_back(object.m_mesh);
    m_InstanceRefToBLAS.push_back(m_BLASmeshes.size() - 1);

    m_instances.push_back(object.m_geometry);
    m_objects.push_back(object);
    std::cout << "Model Added \n";
}

void OgEngine::RaytracingPipeline::RemoveModelInGame()
{

}

void OgEngine::RaytracingPipeline::UpdateObject(uint64_t p_id, const GPM::Matrix4F& p_transform, const std::shared_ptr<Mesh>& p_mesh, uint32_t p_texID)
{
    std::vector<uint64_t>::iterator it = std::find(m_objectIDs.begin(), m_objectIDs.end(), p_id);

    if (it != m_objectIDs.end())
    {
        int id = std::distance(m_objectIDs.begin(), it);
        m_objects[id].ConvertTransform(p_transform);
        return;
    }

    AddObject(p_id, p_mesh, p_texID);
    m_objects.back().ConvertTransform(p_transform);
}



VkResult RaytracingPipeline::AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex) const
{
    if (m_swapChain.swapChain == VK_NULL_HANDLE)
        std::runtime_error("No swapchain available, swapchain -> NULL");

    // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
    // With that we don't have to handle VK_NOT_READY
    return vkAcquireNextImageKHR(m_vulkanDevice.m_logicalDevice, m_swapChain.swapChain, UINT64_MAX, presentCompleteSemaphore, static_cast<VkFence>(nullptr), imageIndex);
}

VkResult RaytracingPipeline::QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = nullptr) const
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapChain.swapChain;
    presentInfo.pImageIndices = &imageIndex;
    // Check if a wait semaphore has been specified to wait for before presenting the image
    if (waitSemaphore != nullptr)
    {
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.waitSemaphoreCount = 1;
    }
    return vkQueuePresentKHR(queue, &presentInfo);
}

VkShaderModule RaytracingPipeline::CreateShaderModule(const std::vector<char>& code) const
{
    //Set the shader
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    //Create the shader
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_vulkanDevice.m_logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void OgEngine::RaytracingPipeline::SetRaytracingCmd()
{
    vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkCreateAccelerationStructureNV"));
    vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkDestroyAccelerationStructureNV"));
    vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkBindAccelerationStructureMemoryNV"));
    vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkGetAccelerationStructureHandleNV"));
    vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkGetAccelerationStructureMemoryRequirementsNV"));
    vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkCmdBuildAccelerationStructureNV"));
    vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkCreateRayTracingPipelinesNV"));
    vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkGetRayTracingShaderGroupHandlesNV"));
    vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkCmdTraceRaysNV"));
    vkCmdSetCheckpointNV = reinterpret_cast<PFN_vkCmdSetCheckpointNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkCmdSetCheckpointNV"));
    vkGetQueueCheckpointDataNV = reinterpret_cast<PFN_vkGetQueueCheckpointDataNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkGetQueueCheckpointDataNV"));
    vkCmdCopyAccelerationStructureNV = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(vkGetDeviceProcAddr(m_vulkanDevice.m_logicalDevice, "vkCmdCopyAccelerationStructureNV"));
}

void RaytracingPipeline::InitSwapchain()
{
    // Get available queue family properties
    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanDevice.m_gpu, &queueCount, nullptr);
    assert(queueCount >= 1);

    std::vector<VkQueueFamilyProperties> queueProps(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanDevice.m_gpu, &queueCount, queueProps.data());

    // Iterate over each queue to learn whether it supports presenting:
    // Find a queue with present support
    // Will be used to present the swap chain images to the windowing system
    std::vector<VkBool32> supportsPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(m_vulkanDevice.m_gpu, i, m_vulkanDevice.m_surface, &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueCount; i++)
    {
        if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphicsQueueNodeIndex == UINT32_MAX)
            {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE)
            {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX)
    {
        // If there's no queue that supports both present and graphics
        // try to find a separate present queue
        for (uint32_t i = 0; i < queueCount; ++i)
        {
            if (supportsPresent[i] == VK_TRUE)
            {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }

    // Exit if either a graphics or a presenting queue hasn't been found
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
    {
        throw std::runtime_error("Could not find a graphics and/or presenting queue!");
    }

    // todo : Add support for separate graphics and presenting queue
    if (graphicsQueueNodeIndex != presentQueueNodeIndex)
    {
        throw std::runtime_error("Separate graphics and presenting queues are not supported yet!");
    }

    m_swapChain.queueNodeIndex = graphicsQueueNodeIndex;

    // Get list of supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanDevice.m_gpu, m_vulkanDevice.m_surface, &formatCount, nullptr);
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanDevice.m_gpu, m_vulkanDevice.m_surface, &formatCount, surfaceFormats.data());

    // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
    // there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
    if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
    {
        m_swapChain.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
        m_swapChain.colorSpace = surfaceFormats[0].colorSpace;
    }
    else
    {
        // iterate over the list of available surface format and
        // check for the presence of VK_FORMAT_B8G8R8A8_UNORM
        bool found_B8G8R8A8_UNORM = false;
        for (auto&& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                m_swapChain.colorFormat = surfaceFormat.format;
                m_swapChain.colorSpace = surfaceFormat.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }

        // in case VK_FORMAT_B8G8R8A8_UNORM is not available
        // select the first available color format
        if (!found_B8G8R8A8_UNORM)
        {
            m_swapChain.colorFormat = surfaceFormats[0].format;
            m_swapChain.colorSpace = surfaceFormats[0].colorSpace;
        }
    }
}

void RaytracingPipeline::CreateCommandPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = m_swapChain.queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CHECK_ERROR(vkCreateCommandPool(m_vulkanDevice.m_logicalDevice, &cmdPoolInfo, nullptr, &m_commandPool));
}

void RaytracingPipeline::CreateCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    m_commandBuffers.resize(m_swapChain.imageCount);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        Initializers::commandBufferAllocateInfo(
            m_commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<uint32_t>(m_commandBuffers.size()));

    CHECK_ERROR(vkAllocateCommandBuffers(m_vulkanDevice.m_logicalDevice, &cmdBufAllocateInfo, m_commandBuffers.data()));
}

void RaytracingPipeline::CreateBottomLevelAccelerationStructure(const VkGeometryNV* geometries)
{
    AccelerationStructure accel{};
    VkAccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    accelerationStructureInfo.instanceCount = 0;
    accelerationStructureInfo.geometryCount = 1;
    accelerationStructureInfo.pGeometries = geometries;
    accelerationStructureInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_NV;

    VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    accelerationStructureCI.info = accelerationStructureInfo;

    vkCreateAccelerationStructureNV(m_vulkanDevice.m_logicalDevice, &accelerationStructureCI, nullptr, &accel.accelerationStructure);

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = accel.accelerationStructure;

    VkMemoryRequirements2 memoryRequirements2{};
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.m_logicalDevice, &memoryRequirementsInfo, &memoryRequirements2);

    VkMemoryAllocateInfo memoryAllocateInfo = Initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &memoryAllocateInfo, nullptr, &accel.memory);

    VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    accelerationStructureMemoryInfo.accelerationStructure = accel.accelerationStructure;
    accelerationStructureMemoryInfo.memory = accel.memory;
    vkBindAccelerationStructureMemoryNV(m_vulkanDevice.m_logicalDevice, 1, &accelerationStructureMemoryInfo);

    vkGetAccelerationStructureHandleNV(m_vulkanDevice.m_logicalDevice, accel.accelerationStructure, sizeof(uint64_t), &accel.handle);
    m_BLAS.push_back(accel);
}
//VALID
void RaytracingPipeline::CreateTopLevelAccelerationStructure(AccelerationStructure& accelerationStruct, uint32_t instanceCount)
{
    VkAccelerationStructureInfoNV accelerationStructureInfo{};
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    accelerationStructureInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_NV;
    accelerationStructureInfo.instanceCount = instanceCount;
    accelerationStructureInfo.geometryCount = 0;

    VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
    accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    accelerationStructureCI.info = accelerationStructureInfo;
    vkCreateAccelerationStructureNV(m_vulkanDevice.m_logicalDevice, &accelerationStructureCI, nullptr, &accelerationStruct.accelerationStructure);

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = accelerationStruct.accelerationStructure;

    VkMemoryRequirements2 memoryRequirements2{};
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.m_logicalDevice, &memoryRequirementsInfo, &memoryRequirements2);

    VkMemoryAllocateInfo memoryAllocateInfo = Initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &memoryAllocateInfo, nullptr, &accelerationStruct.memory);
    VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
    accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    accelerationStructureMemoryInfo.accelerationStructure = accelerationStruct.accelerationStructure;
    accelerationStructureMemoryInfo.memory = accelerationStruct.memory;
    vkBindAccelerationStructureMemoryNV(m_vulkanDevice.m_logicalDevice, 1, &accelerationStructureMemoryInfo);

    vkGetAccelerationStructureHandleNV(m_vulkanDevice.m_logicalDevice, accelerationStruct.accelerationStructure, sizeof(uint64_t), &accelerationStruct.handle);
}
//VALID
void RaytracingPipeline::CreateStorageImage()
{
    VkImageCreateInfo image = Initializers::imageCreateInfo();
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = m_swapChain.colorFormat;
    image.extent.width = m_width;
    image.extent.height = m_height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(m_vulkanDevice.m_logicalDevice, &image, nullptr, &m_storageImage.image);

    VkMemoryRequirements memory_requierements;
    vkGetImageMemoryRequirements(m_vulkanDevice.m_logicalDevice, m_storageImage.image, &memory_requierements);
    VkMemoryAllocateInfo memoryAllocateInfo = Initializers::memoryAllocateInfo();
    memoryAllocateInfo.allocationSize = memory_requierements.size;
    memoryAllocateInfo.memoryTypeIndex = GetMemoryType(memory_requierements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &memoryAllocateInfo, nullptr, &m_storageImage.memory);
    vkBindImageMemory(m_vulkanDevice.m_logicalDevice, m_storageImage.image, m_storageImage.memory, 0);

    VkImageViewCreateInfo colorImageView = Initializers::imageViewCreateInfo();
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = m_swapChain.colorFormat;
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;
    colorImageView.image = m_storageImage.image;
    vkCreateImageView(m_vulkanDevice.m_logicalDevice, &colorImageView, nullptr, &m_storageImage.view);

    const VkCommandBuffer cmd_buffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    SetImageLayout(cmd_buffer, m_storageImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    QueueCmdBufferAndFlush(cmd_buffer, m_graphicsQueue);
}
//VALID
VkResult RaytracingPipeline::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data) const
{
    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = Initializers::bufferCreateInfo(usageFlags, size);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m_vulkanDevice.m_logicalDevice, &bufferCreateInfo, nullptr, buffer);

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memory_requirements;
    VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(m_vulkanDevice.m_logicalDevice, *buffer, &memory_requirements);
    memAlloc.allocationSize = memory_requirements.size;
    // Find a memory type index that fits the properties of the buffer
    memAlloc.memoryTypeIndex = GetMemoryType(memory_requirements.memoryTypeBits, memoryPropertyFlags);
    vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &memAlloc, nullptr, memory);

    // If a pointer to the buffer data has been passed, map the buffer and copy over the data
    if (data != nullptr)
    {
        void* mapped;
        vkMapMemory(m_vulkanDevice.m_logicalDevice, *memory, 0, size, 0, &mapped);
        memcpy(mapped, data, size);
        // If host coherency hasn't been requested, do a manual flush to make writes visible
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            VkMappedMemoryRange mappedRange = Initializers::mappedMemoryRange();
            mappedRange.memory = *memory;
            mappedRange.offset = 0;
            mappedRange.size = size;
            vkFlushMappedMemoryRanges(m_vulkanDevice.m_logicalDevice, 1, &mappedRange);
        }
        vkUnmapMemory(m_vulkanDevice.m_logicalDevice, *memory);
    }

    // Attach the memory to the buffer object
    vkBindBufferMemory(m_vulkanDevice.m_logicalDevice, *buffer, *memory, 0);

    return VK_SUCCESS;
}
VkBool32 RaytracingPipeline::GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
{
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
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
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}
void RaytracingPipeline::AddObject(uint64_t p_id, std::shared_ptr<Mesh> p_mesh, uint32_t p_textureID)
{
    Model object = Model(p_mesh, true);
    m_objectIDs.push_back(p_id);

    //WORKING VERSION
    ResourceManager::WaitForAll();

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    CHECK_ERROR(CreateBuffer( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &object.m_vertBuffer, object.m_mesh->Vertices().size() * sizeof(Vertex),
    (void*)object.m_mesh->Vertices().data()));

    CHECK_ERROR(CreateBuffer( VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &object.m_indexBuffer, object.m_mesh->Indices().size() * sizeof(uint32_t),
    (void*)object.m_mesh->Indices().data()));

    Buffer vertexBuffer;
    Buffer indexBuffer;

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &vertexBuffer, object.m_mesh->Vertices().size() * sizeof(Vertex),
        (void*)object.m_mesh->Vertices().data()));

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &indexBuffer, object.m_mesh->Indices().size() * sizeof(uint32_t),
        (void*)object.m_mesh->Indices().data()));

    m_shaderData.vertexBuffer.push_back(vertexBuffer);
    m_shaderData.indexBuffer.push_back(indexBuffer);
    textureIDs.push_back(p_textureID);

    VkGeometryNV geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    geometry.geometry.triangles.vertexData = object.m_vertBuffer.buffer;
    geometry.geometry.triangles.vertexOffset = 0;
    geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(object.m_mesh->Vertices().size());
    geometry.geometry.triangles.vertexStride = sizeof(Vertex);
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.indexData = object.m_indexBuffer.buffer;
    geometry.geometry.triangles.indexOffset = 0;
    geometry.geometry.triangles.indexCount = static_cast<uint32_t>(object.m_mesh->Indices().size());;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.transformData = nullptr;
    geometry.geometry.triangles.transformOffset = 0;
    geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    CreateBottomLevelAccelerationStructure(&geometry);

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &object.m_geometryBuffer,
        sizeof(GeometryInstance),
        &object.m_geometry));

    // Acceleration structure build requires some scratch space to store temporary information
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
    memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

    VkMemoryRequirements2 blasMemsReqs;
    memoryRequirementsInfo.accelerationStructure = m_BLAS.back().accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.m_logicalDevice, &memoryRequirementsInfo, &blasMemsReqs);

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

    object.m_geometry.instanceId = TLASinstanceID;
    object.m_geometry.accelerationStructureHandle = m_BLAS.back().handle;
    TLASinstanceID++;

    //m_BLASmeshes.push_back(object.second.m_mesh);
    //m_InstanceRefToBLAS.push_back(m_BLASmeshes.size() - 1);

    m_instances.push_back(object.m_geometry);
    m_objects.push_back(object);
    std::cout << "Object Added\n";
    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);
    scratchBuffer.destroy();
}

void OgEngine::RaytracingPipeline::AddTexture(const char* p_texture)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels =
        stbi_load(p_texture, &width, &height, &channels, STBI_rgb_alpha);
    
    if (!pixels)
    {
        pixels =
            stbi_load("Resources/textures/error.png", &width, &height, &channels, STBI_rgb_alpha);
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

    stagingBuffer.map();
    memcpy(stagingBuffer.mapped, pixels, bufferSize);
    stagingBuffer.unmap();

    stbi_image_free(pixels);

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
    vkCreateImage(m_vulkanDevice.m_logicalDevice, &info, nullptr, &data.img);

    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = info.mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 1;


    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_vulkanDevice.m_logicalDevice, data.img, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &allocInfo, nullptr, &data.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }


    vkBindImageMemory(m_vulkanDevice.m_logicalDevice, data.img , data.memory, 0);

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

    GenerateMipmaps(data.img, format, width, height, mipLevels);
    range.levelCount = 1;

    SetImageLayout(cmdBuffer, data.img,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    range,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);



    VkSamplerCreateInfo samplerInfo = Initializers::samplerCreateInfo();
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = FLT_MAX;

    data.info = Create2DDescriptor(m_vulkanDevice.m_logicalDevice, data.img, samplerInfo, format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    m_textures.push_back(data);
    QueueCmdBufferAndFlush(cmdBuffer, m_graphicsQueue);

}

void OgEngine::RaytracingPipeline::CreateTLAS()
{
    VkCommandBuffer cmdBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    CreateTopLevelAccelerationStructure(m_TLAS, m_instances.size());

    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo2{};
    memoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo2.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

    VkMemoryRequirements2 tlasMemsReqs;
    memoryRequirementsInfo2.accelerationStructure = m_TLAS.accelerationStructure;
    vkGetAccelerationStructureMemoryRequirementsNV(m_vulkanDevice.m_logicalDevice, &memoryRequirementsInfo2, &tlasMemsReqs);

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
    buildInfo2.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_NV;
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
    scratchBuffer2.destroy();
}



//VALID
void RaytracingPipeline::CreatePipeline(bool resizedWindow)
{

    //Binding Uniforms to specific shader,
    //here we set the bindings for the RAYGEN shader (VK_SHADER_STAGE_RAYGEN_BIT_NV)

    VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
    accelerationStructureLayoutBinding.binding = 0;
    accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    accelerationStructureLayoutBinding.descriptorCount = 1;
    accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

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

    //create a Binding vector for Uniform bindings
    std::vector<VkDescriptorSetLayoutBinding> bindings({
        accelerationStructureLayoutBinding,
        resultImageLayoutBinding,
        uniformBufferBinding,
        vertexBufferBinding,
        indexBufferBinding,
        textureBinding,
        textureIDBinding
        });

    //Create the buffer that will map the shader uniforms to the actual shader
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(m_vulkanDevice.m_logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

    vkCreatePipelineLayout(m_vulkanDevice.m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);

    const uint32_t shader_index_ray = 0;
    const uint32_t shaderIndexMiss = 1;
    const uint32_t shaderIndexClosestHit = 2;

    std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages{};
    shaderStages[shader_index_ray] = LoadShader("Resources/shaders/bin/ray_gen.spv", VK_SHADER_STAGE_RAYGEN_BIT_NV);
    shaderStages[shaderIndexMiss] = LoadShader("Resources/shaders/bin/ray_miss.spv", VK_SHADER_STAGE_MISS_BIT_NV);
    shaderStages[shaderIndexClosestHit] = LoadShader("Resources/shaders/bin/ray_chit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    std::array<VkRayTracingShaderGroupCreateInfoNV, 3> groups{};
    for (auto& group : groups)
    {
        // Init all groups with some default values
        group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
        group.generalShader = VK_SHADER_UNUSED_NV;
        group.closestHitShader = VK_SHADER_UNUSED_NV;
        group.anyHitShader = VK_SHADER_UNUSED_NV;
        group.intersectionShader = VK_SHADER_UNUSED_NV;
    }

    // Links shaders and types to ray tracing shader groups
    groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_RAYGEN].generalShader = shader_index_ray;
    groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groups[INDEX_MISS].generalShader = shaderIndexMiss;
    groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
    groups[INDEX_CLOSEST_HIT].generalShader = VK_SHADER_UNUSED_NV;
    groups[INDEX_CLOSEST_HIT].closestHitShader = shaderIndexClosestHit;

    VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
    rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayPipelineInfo.pStages = shaderStages.data();
    rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups = groups.data();
    rayPipelineInfo.maxRecursionDepth = 1;
    rayPipelineInfo.layout = m_pipelineLayout;
    vkCreateRayTracingPipelinesNV(m_vulkanDevice.m_logicalDevice, nullptr, 1, &rayPipelineInfo, nullptr, &m_pipeline);
}

VkPipelineShaderStageCreateInfo RaytracingPipeline::LoadShader(const std::string file_name, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    shaderStage.module = vks::tools::loadShader(androidApp->activity->assetManager, fileName.c_str(), device);
#else
    shaderStage.module = LoadShaderFile(file_name.c_str(), m_vulkanDevice.m_logicalDevice);
#endif
    shaderStage.pName = "main"; // todo : make param
    assert(shaderStage.module != VK_NULL_HANDLE);
    m_shaderModules.push_back(shaderStage.module);
    return shaderStage;
}

void RaytracingPipeline::CreateSynchronizationPrimitives()
{
    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo = Initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    m_waitFences.resize(m_commandBuffers.size());
    for (auto& fence : m_waitFences)
    {
        vkCreateFence(m_vulkanDevice.m_logicalDevice, &fenceCreateInfo, nullptr, &fence);
    }
}

void RaytracingPipeline::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(m_vulkanDevice.m_logicalDevice, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
}

void RaytracingPipeline::SetupFramebuffer()
{
    VkImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = m_depthStencil.m_stencilView;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = nullptr;
    frameBufferCreateInfo.renderPass = m_renderpass;
    frameBufferCreateInfo.attachmentCount = 2;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = m_width;
    frameBufferCreateInfo.height = m_height;
    frameBufferCreateInfo.layers = 1;

    // Create frame buffers for every swap chain image
    m_swapchainFrameBuffers.resize(m_swapChain.imageCount);
    for (uint32_t i = 0; i < m_swapchainFrameBuffers.size(); i++)
    {
        attachments[0] = m_swapChain.buffers[i].view;
        CHECK_ERROR(vkCreateFramebuffer(m_vulkanDevice.m_logicalDevice, &frameBufferCreateInfo, nullptr, &m_swapchainFrameBuffers[i]));
    }
}

void RaytracingPipeline::SetupDepthStencil()
{
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

    CHECK_ERROR(vkCreateImage(m_vulkanDevice.m_logicalDevice, &imageCI, nullptr, &m_depthStencil.m_stencilImage));
    VkMemoryRequirements memory_requierements{};
    vkGetImageMemoryRequirements(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilImage, &memory_requierements);

    VkMemoryAllocateInfo memory_alloc{};
    memory_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc.allocationSize = memory_requierements.size;
    memory_alloc.memoryTypeIndex = GetMemoryType(memory_requierements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CHECK_ERROR(vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &memory_alloc, nullptr, &m_depthStencil.m_stencilMemory));
    CHECK_ERROR(vkBindImageMemory(m_vulkanDevice.m_logicalDevice, m_depthStencil.m_stencilImage, m_depthStencil.m_stencilMemory, 0));

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
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (m_depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT)
    {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    CHECK_ERROR(vkCreateImageView(m_vulkanDevice.m_logicalDevice, &imageViewCI, nullptr, &m_depthStencil.m_stencilView));
}

void RaytracingPipeline::SetupRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};
    // Color attachment
    attachments[0].format = m_swapChain.colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // Depth attachment
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

    // Subpass dependencies for layout transitions
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

    CHECK_ERROR(vkCreateRenderPass(m_vulkanDevice.m_logicalDevice, &renderPassInfo, nullptr, &m_renderpass));
}

/**
* Create a buffer on the device
*
* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
* @param buffer Pointer to a vk::Vulkan buffer object
* @param size Size of the buffer in byes
* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
*
* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
*/
VkResult RaytracingPipeline::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, Buffer* buffer, VkDeviceSize size, void* data) const
{
    buffer->device = m_vulkanDevice.m_logicalDevice;

    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = Initializers::bufferCreateInfo(usageFlags, size);
    vkCreateBuffer(m_vulkanDevice.m_logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer);

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memory_requierements;
    VkMemoryAllocateInfo memAlloc = Initializers::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(m_vulkanDevice.m_logicalDevice, buffer->buffer, &memory_requierements);
    memAlloc.allocationSize = memory_requierements.size;
    // Find a memory type index that fits the properties of the buffer
    memAlloc.memoryTypeIndex = GetMemoryType(memory_requierements.memoryTypeBits, memoryPropertyFlags);
    vkAllocateMemory(m_vulkanDevice.m_logicalDevice, &memAlloc, nullptr, &buffer->memory);

    buffer->alignment = memory_requierements.alignment;
    buffer->size = memAlloc.allocationSize;
    buffer->usageFlags = usageFlags;
    buffer->memoryPropertyFlags = memoryPropertyFlags;

    // If a pointer to the buffer data has been passed, map the buffer and copy over the data
    if (data != nullptr)
    {
        buffer->map();
        memcpy(buffer->mapped, data, size);
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            CHECK_ERROR(buffer->flush());

        buffer->unmap();
    }

    // Initialize a default descriptor that covers the whole buffer size
    buffer->setupDescriptor();

    // Attach the memory to the buffer object
    return buffer->bind();
}

void RaytracingPipeline::SetImageLayout(const VkCommandBuffer cmd_buffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, const VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier = Initializers::imageMemoryBarrier();
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source 
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        break;
    }

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(
        cmd_buffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
}
//VALID
void RaytracingPipeline::SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = aspectMask;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.layerCount = 1;
    SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresource_range, srcStageMask, dstStageMask);
}
//VALID
void RaytracingPipeline::QueueCmdBufferAndFlush(VkCommandBuffer commandBuffer, VkQueue queue, bool free) const
{
    if (commandBuffer == nullptr)
    {
        return;
    }

    CHECK_ERROR(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = Initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
    VkFence fence;
    CHECK_ERROR(vkCreateFence(m_vulkanDevice.m_logicalDevice, &fenceInfo, nullptr, &fence));

    // Submit to the queue
    CHECK_ERROR(vkQueueSubmit(queue, 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    CHECK_ERROR(vkWaitForFences(m_vulkanDevice.m_logicalDevice, 1, &fence, VK_TRUE, 100000000000));
    vkQueueWaitIdle(m_graphicsQueue);
    vkDestroyFence(m_vulkanDevice.m_logicalDevice, fence, nullptr);

    if (free)
    {
        vkFreeCommandBuffers(m_vulkanDevice.m_logicalDevice, m_commandPool, 1, &commandBuffer);
    }
}

void RaytracingPipeline::CreateShaderBindingTable()
{
    // Create buffer for the shader binding table
    const uint32_t sbtSize = m_raytracingProperties.shaderGroupHandleSize * 3;
    CreateBuffer(
        VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &m_shaderBindingTable,
        sbtSize);
    m_shaderBindingTable.map();

    const auto shaderHandleStorage = new uint8_t[sbtSize];
    // Get shader identifiers
    vkGetRayTracingShaderGroupHandlesNV(m_vulkanDevice.m_logicalDevice, m_pipeline, 0, 3, sbtSize, shaderHandleStorage);
    auto* data = static_cast<uint8_t*>(m_shaderBindingTable.mapped);
    // Copy the shader identifiers to the shader binding table
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_RAYGEN);
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_MISS);
    data += CopyShaderIdentifier(data, shaderHandleStorage, INDEX_CLOSEST_HIT);
    m_shaderBindingTable.unmap();
}

VkDeviceSize RaytracingPipeline::CopyShaderIdentifier(uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex) const
{
    const uint32_t shaderGroupHandleSize = m_raytracingProperties.shaderGroupHandleSize;
    memcpy(data, shaderHandleStorage + groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
    data += shaderGroupHandleSize;
    return shaderGroupHandleSize;
}

void RaytracingPipeline::CreateDescriptorSets(bool resizedWindow)
{
    const std::vector<VkDescriptorPoolSize> poolSizes =
    {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_OBJECTS },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }

    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = Initializers::descriptorPoolCreateInfo(poolSizes, 1);
    vkCreateDescriptorPool(m_vulkanDevice.m_logicalDevice, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = Initializers::descriptorSetAllocateInfo(m_descriptorPool, &m_descriptorSetLayout, 1);
    
    vkAllocateDescriptorSets(m_vulkanDevice.m_logicalDevice, &descriptorSetAllocateInfo, &m_descriptorSet);

    //Acceleration Structure
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

    std::vector<VkDescriptorImageInfo> infos;
    for (auto tex : m_textures)
        infos.push_back(tex.info);

    std::vector<VkDescriptorBufferInfo> vertexDescriptor;
    std::vector<VkDescriptorBufferInfo> indexDescriptor;

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

    if(!resizedWindow)
        CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &m_shaderData.textureIDBuffer,
            MAX_OBJECTS * sizeof(uint32_t),
            textureIDs.data()));

    VkWriteDescriptorSet textureDescriptor{};
    textureDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptor.dstSet = m_descriptorSet;
    textureDescriptor.dstBinding = 5;
    textureDescriptor.dstArrayElement = 0;
    textureDescriptor.descriptorCount = static_cast<uint32_t>(m_textures.size());
    textureDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptor.pImageInfo = infos.data();


    const VkWriteDescriptorSet resultImageWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
    const VkWriteDescriptorSet uniformBufferWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &m_cameraBuffer.descriptor);
    const VkWriteDescriptorSet vertexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, vertexDescriptor.data(), static_cast<uint32_t>(m_shaderData.vertexBuffer.size()));
    const VkWriteDescriptorSet indexWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, indexDescriptor.data(), static_cast<uint32_t>(m_shaderData.indexBuffer.size()));
    const VkWriteDescriptorSet textureIDWrite = Initializers::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, &m_shaderData.textureIDBuffer.descriptor);
    
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = 
    {
        accelerationStructureWrite,
        resultImageWrite,
        uniformBufferWrite,
        vertexWrite,
        indexWrite,
        textureDescriptor,
        textureIDWrite
    };

    vkUpdateDescriptorSets(m_vulkanDevice.m_logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

void RaytracingPipeline::CreateUniformBuffer()
{

    m_camera.setPosition({ 0, 5, 5});
    m_camera.setPerspective(90.0f, static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 1024.0f);
    m_cameraData.viewInverse = GPM::Matrix4F::Inverse(m_camera.matrices.view);
    m_cameraData.projInverse = GPM::Matrix4F::Inverse(m_camera.matrices.perspective);

    CHECK_ERROR(CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &m_cameraBuffer,
        sizeof(m_cameraData),
        &m_cameraData));
    CHECK_ERROR(m_cameraBuffer.map());
}

void RaytracingPipeline::StartPathTracing()
{
    VkCommandBufferBeginInfo cmdBufInfo = Initializers::commandBufferBeginInfo();

    const VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    for (size_t i = 0; i < m_commandBuffers.size(); ++i)
    {
        CHECK_ERROR(vkBeginCommandBuffer(m_commandBuffers[i], &cmdBufInfo));

        /*
            Dispatch the ray tracing commands
        */
        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipeline);
        vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        // Calculate shader binding offsets, which is pretty straight forward in our example 
        const VkDeviceSize bindingOffsetRayGenShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_RAYGEN;
        const VkDeviceSize bindingOffsetMissShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_MISS;
        const VkDeviceSize bindingOffsetHitShader = m_raytracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT;
        const VkDeviceSize bindingStride = m_raytracingProperties.shaderGroupHandleSize;

        vkCmdTraceRaysNV(m_commandBuffers[i],
            m_shaderBindingTable.buffer, bindingOffsetRayGenShader,
            m_shaderBindingTable.buffer, bindingOffsetMissShader, bindingStride,
            m_shaderBindingTable.buffer, bindingOffsetHitShader, bindingStride,
            nullptr, 0, 0,
            m_width, m_height, 1);

        /*
            Copy raytracing output to swap chain image
        */

        // Prepare current swapchain image as transfer destination
        SetImageLayout(
            m_commandBuffers[i],
            m_swapChain.images[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        // Prepare ray tracing output image as transfer source
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
        vkCmdCopyImage(m_commandBuffers[i], m_storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swapChain.images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // Transition swap chain image back for presentation
        SetImageLayout(
            m_commandBuffers[i],
            m_swapChain.images[i],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            subresource_range,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        // Transition ray tracing output image back to general layout
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

void RaytracingPipeline::InitRaytracingRenderer(bool resizedWindow)
{

    // Query the ray tracing properties of the current implementation, we will need them later on
    m_raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    VkPhysicalDeviceProperties2 deviceProps2{};
    deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps2.pNext = &m_raytracingProperties;
    vkGetPhysicalDeviceProperties2(m_vulkanDevice.m_gpu, &deviceProps2);

    if (!resizedWindow)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = Initializers::semaphoreCreateInfo();
        CHECK_ERROR(vkCreateSemaphore(m_vulkanDevice.m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphores.presentComplete));
        CHECK_ERROR(vkCreateSemaphore(m_vulkanDevice.m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphores.renderComplete));
    }
    // Set up submit info structure;
    // Semaphores will stay the same during application lifetime
    // Command buffer submission info is set by each example
    m_submitInfo = Initializers::submitInfo();
    m_submitInfo.pWaitDstStageMask = &submitPipelineStages;
    m_submitInfo.waitSemaphoreCount = 1;
    m_submitInfo.pWaitSemaphores = &m_semaphores.presentComplete;
    m_submitInfo.signalSemaphoreCount = 1;
    m_submitInfo.pSignalSemaphores = &m_semaphores.renderComplete;


    if(!resizedWindow)
    {
        AddTexture("Resources/textures/default.png");
        AddTexture("Resources/textures/link.png");
        AddTexture("Resources/textures/lucy.png");


        vkQueueWaitIdle(m_graphicsQueue);
        AddObject(99999, ResourceManager::Get<OgEngine::Mesh>("cube.obj"), 1);
        CreateTLAS();
        CreateUniformBuffer();
    }
    
    CreateStorageImage();

    CreatePipeline(resizedWindow);
    CreateShaderBindingTable();
    CreateDescriptorSets(resizedWindow);

    StartPathTracing();

    if (resizedWindow)
    {
        RescaleImGUI();
        SetupImGUIFrameBuffers();
    }
    else
    {
        InitImGUI();
        SetupImGUIFrameBuffers();
        SetupImGUI();
    }
}

void RaytracingPipeline::PrepareFrame()
{
    // Acquire the next image from the swap chain
    const VkResult result = AcquireNextImage(m_semaphores.presentComplete, &m_currentBuffer);
    RenderUI(m_currentBuffer);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        ResizeWindow();
    }
    else
    {
        CHECK_ERROR(result);
    }
}
void RaytracingPipeline::SubmitFrame()
{
    const VkResult result = QueuePresent(m_graphicsQueue, m_currentBuffer, m_semaphores.renderComplete);
    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Swap chain is no longer compatible with the surface and needs to be recreated
            //windowResize();
            return;
        }
        CHECK_ERROR(result);
    }
    CHECK_ERROR(vkQueueWaitIdle(m_graphicsQueue));
}

void RaytracingPipeline::Draw()
{
    /*ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame(); 
    SetupTestEditor();
    //ImGui::ShowDemoWindow();
    ImGui::Render();*/

    PrepareFrame();
    m_submitInfo.commandBufferCount = 1;
    m_submitInfo.pCommandBuffers = &m_commandBuffers[m_currentBuffer];

    VkFenceCreateInfo fenceInfo = Initializers::fenceCreateInfo(0);
    VkFence fence;
    CHECK_ERROR(vkCreateFence(m_vulkanDevice.m_logicalDevice, &fenceInfo, nullptr, &fence));

    CHECK_ERROR(vkQueueSubmit(m_graphicsQueue, 1, &m_submitInfo, fence));
    CHECK_ERROR(vkWaitForFences(m_vulkanDevice.m_logicalDevice, 1, &fence, VK_TRUE, 100000000000));
    vkQueueWaitIdle(m_graphicsQueue);
    vkDestroyFence(m_vulkanDevice.m_logicalDevice, fence, nullptr);

    RenderUI(m_currentBuffer);
    SubmitFrame();
    UpdateDescriptorSets();
}
