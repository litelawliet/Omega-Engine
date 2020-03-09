#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include <string>
#include <optional>

struct Device
{
    VkPhysicalDevice m_gpu;
    /** @brief Logical device representation (application's view of the device) */
    VkDevice m_logicalDevice;
    /** @brief Properties of the physical device including limits that the application can check against */
    VkPhysicalDeviceProperties m_gpuProperties;
    /** @brief Features of the physical device that an application can use to check if a feature is supported */
    std::vector<VkPhysicalDeviceFeatures> m_gpuFeatures;
    /** @brief Features that have been enabled for use on the physical device */
    VkPhysicalDeviceFeatures m_gpuEnabledFeatures;
    /** @brief Memory types and heaps of the physical device */
    VkPhysicalDeviceMemoryProperties m_gpuMemoryProperties;
    /** @brief Queue family properties of the physical device */
    std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
    /** @brief KHR Surface of te device */
    VkSurfaceKHR m_surface;
    /** @brief List of extensions supported by the device */
    std::vector<std::string> m_supportedExtensions;
    /** @brief Vulkan Instance */
    VkInstance m_instance;

    std::optional<uint32_t> m_gpuGraphicFamily{};
    std::optional<uint32_t> m_gpuPresentFamily{};
};