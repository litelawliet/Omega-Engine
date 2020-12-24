#pragma once
#include <OgRendering/Utils/volk.h>

#include <vector>
#include <string>
#include <optional>

struct Device
{
    VkPhysicalDevice gpu;
    /** @brief Logical device representation (application's view of the device) */
    VkDevice logicalDevice;
    /** @brief Properties of the physical device including limits that the application can check against */
    VkPhysicalDeviceProperties gpuProperties;
    /** @brief Features of the physical device that an application can use to check if a feature is supported */
    std::vector<VkPhysicalDeviceFeatures> gpuFeatures;
    /** @brief Features that have been enabled for use on the physical device */
    VkPhysicalDeviceFeatures gpuEnabledFeatures;
    /** @brief Memory types and heaps of the physical device */
    VkPhysicalDeviceMemoryProperties gpuMemoryProperties;
    /** @brief Queue family properties of the physical device */
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    /** @brief KHR Surface of te device */
    VkSurfaceKHR surface;
    /** @brief List of extensions supported by the device */
    std::vector<std::string> supportedExtensions;
    /** @brief Vulkan Instance */
    VkInstance instance;
    /** @brief Device MSAA sample */
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::optional<uint32_t> graphicFamily{};
    std::optional<uint32_t> presentFamily{};
};