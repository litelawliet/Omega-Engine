#pragma once
#include <OgRendering/Export.h>

#include <OgRendering/Rendering/Device.h>

#include <OgRendering/Rendering/RaytracingPipeline.h>
#include <OgRendering/Rendering/RasterizerPipeline.h>

#include <vector>
#include <optional>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	[[nodiscard]] bool isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SupportDetails
{
	VkSurfaceCapabilitiesKHR        capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>   presentModes;
};

struct Model
{
	Buffer vertex;
	Buffer indices;
};

namespace OgEngine
{
	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	struct UniformLightInfo
	{
		alignas(16) glm::vec4 position;
		alignas(16) glm::vec4 ambient;
		alignas(16) glm::vec4 diffuse;
		alignas(16) glm::vec4 specular;
	};

	struct UniformMaterialInfo
	{
		alignas(16) glm::vec4 color;
		alignas(4) float rough;
		alignas(1) bool metal;
	};

	using namespace OgEngine;

	class RENDERING_API VulkanContext final
	{
	public:
		VulkanContext();
		~VulkanContext();

		void InitAPI();
		void InitWindow(int p_width, int p_height, const char* p_name, bool p_vsync = false);
		void DestroyContext() const;
		void MainLoop() const;

		void VK_ERROR(VkResult p_result) const;

		void SetRenderingLoop(const bool p_renderingLoop);
		void ChangeWindowTitle(std::string_view p_title, const uint64_t p_fps = 0) const;
		void PollEvents() const;
		bool WindowShouldClose() const;
		double TimeOfContext() const;
		bool IsRendering() const;
		bool IsRaytracing() const;

		GLFWwindow* GetWindow() const;

		OgEngine::RaytracingPipeline* GetRTPipeline() const { return m_RTPipeline; }
		OgEngine::RasterizerPipeline* GetRSPipeline() const { return m_RSPipeline; }
		static bool                                   framebufferResized;
	private:
		void CheckRaytracingSupport();

		void InitSelectedRenderer();

		void InitInstance();
		void InitGpuDevice();
		void InitLogicalDevice();

		// ##### Callback for glfw #####
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

		// ##### INSTANCE CREATION AND DEBUG HANDLER #####
		[[nodiscard]] bool                    CheckValidationLayers() const;
		std::vector<const char*>              GetRequieredExtensions();
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT      p_messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT             p_messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* p_pCallbackData,
			void* p_pUserData);

		void           InitDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& p_createInfo);
		void           SetupDebugMessenger();
		bool           IsPhysicalDeviceSuitable(VkPhysicalDevice p_gpu);
		bool           CheckDeviceExtensionSupport(VkPhysicalDevice device);
		SupportDetails SwapChainSupport(VkPhysicalDevice p_device);
		VkSampleCountFlagBits GetMaxUsableSampleCount();

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice p_gpu);

		VkResult CreateDebugUtilsMessengerEXT(VkInstance                                p_instance,
			const VkDebugUtilsMessengerCreateInfoEXT* p_pCreateInfo,
			const VkAllocationCallbacks* p_pAllocator,
			VkDebugUtilsMessengerEXT* p_pDebugMessenger);
		void DestroyDebugUtilsMessengerEXT(VkInstance                   p_instance,
			VkDebugUtilsMessengerEXT     p_debugMessenger,
			const VkAllocationCallbacks* p_allocator) const;

		// ##### members #####
		const std::vector<const char*> m_validationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> m_gpuExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_NV_RAY_TRACING_EXTENSION_NAME,
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
		};

		//Window Compatibility and GLFW
		GLFWwindow* m_window{};
		VkInstance  m_instance{};
		VkDebugUtilsMessengerEXT m_debugMessenger{};
		uint32_t minImageCount;

		Device m_vulkanDevice;

		//GPU Device
		QueueFamilyIndices m_queueFamilyIndices;

		//Logical Device
		VkQueue m_graphicsQueue{};
		VkQueue m_presentQueue{};

		//Window
		uint32_t m_width;
		uint32_t m_height;

		bool isUsingRaytracing;
		bool isRaytracingAvailable;
		bool isUsingVsync = false;

		//Raytracing Pipeline
		RaytracingPipeline* m_RTPipeline;
		RasterizerPipeline* m_RSPipeline;

		std::atomic<bool> m_renderingLoop{ true };
	};
}
