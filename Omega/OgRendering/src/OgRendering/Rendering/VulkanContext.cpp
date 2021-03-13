#include <OgRendering/Rendering/VulkanContext.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <set>

#include <OgRendering/Utils/Debugger.h>
#include <OgRendering/Managers/InputManager.h>

bool OgEngine::VulkanContext::framebufferResized = false;

bool Is64BitProcess(void)
{
#if defined(_WIN64)
	return true;   // 64-bit program
#else
	return false;
#endif
}

OgEngine::VulkanContext::VulkanContext()
{
}

OgEngine::VulkanContext::~VulkanContext()
{
}

void OgEngine::VulkanContext::InitAPI()
{
	InitInstance();
	SetupDebugMessenger();
	InitGpuDevice();
	InitLogicalDevice();
	CheckRaytracingSupport();
	InitSelectedRenderer();
}

void OgEngine::VulkanContext::InitWindow(const int p_width, const int p_height, const char* p_name, bool p_vsync)
{

	DBG_ASSERT_MSG(glfwInit(), "GLFW couldn't initialize.\n");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(p_width, p_height, p_name, nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);

	std::cout << p_name << "'s window (" << p_width << 'x' << p_height << ") created.\n";

	m_width = p_width;
	m_height = p_height;
	isUsingVsync = p_vsync;

	OgEngine::InputManager::SetWindow(&m_window);
	OgEngine::InputManager::SetAllCallbacks();
}

void OgEngine::VulkanContext::DestroyContext() const
{
	if (m_RSPipeline)
		m_RSPipeline->CleanPipeline();

	if (m_RTPipeline)
		m_RTPipeline->CleanPipeline();

	vkDestroyDevice(m_vulkanDevice.logicalDevice, nullptr);

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_instance, m_vulkanDevice.surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);

	glfwTerminate();

	if (m_RSPipeline != nullptr)
	{
		delete m_RSPipeline;
	}
	if (m_RTPipeline != nullptr)
	{
		delete m_RTPipeline;
	}

	std::cout << "\nContext successfully destroyed.\n";
}

void OgEngine::VulkanContext::MainLoop() const
{
	double previousTime = glfwGetTime();
	double currentTime = 0.0;
	uint64_t frameCount = 0u;

	float dt = 0.0f;

	while (!glfwWindowShouldClose(m_window) || m_renderingLoop.load())
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		currentTime = glfwGetTime();
		glfwPollEvents();

		frameCount++;
		if (currentTime - previousTime >= 1.0)
		{
			frameCount = 0u;
			previousTime = currentTime;
		}

		// Rendering using the selected pipeline
		if (isUsingRaytracing)
		{
			m_RTPipeline->UpdateTransforms();
			m_RTPipeline->RenderFrame();
		}
		else
		{
			m_RSPipeline->RenderFrame();
		}

		auto stopTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
	}
}

VkResult OgEngine::VulkanContext::CreateDebugUtilsMessengerEXT(VkInstance p_instance, const VkDebugUtilsMessengerCreateInfoEXT* p_pCreateInfo, const VkAllocationCallbacks* p_pAllocator, VkDebugUtilsMessengerEXT* p_pDebugMessenger)
{
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
		p_instance, "vkCreateDebugUtilsMessengerEXT"));
	if (func != nullptr)
	{
		return func(p_instance, p_pCreateInfo, p_pAllocator, p_pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


void OgEngine::VulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance p_instance,
	VkDebugUtilsMessengerEXT p_debugMessenger, const VkAllocationCallbacks* p_allocator) const
{
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
		p_instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr)
	{
		func(p_instance, p_debugMessenger, p_allocator);
	}
}

void OgEngine::VulkanContext::InitInstance()
{
	bool validationLayersChecked = CheckValidationLayers();
	DBG_ASSERT_MSG(validationLayersChecked, "Validation layers requested, but not available.");

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Omega";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Omega";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;


	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();

		debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = DebugCallback;
		createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	auto extensions = GetRequieredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create instance.");
	

	m_vulkanDevice.instance = m_instance;
	
	result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_vulkanDevice.surface);
	DBG_ASSERT_VULKAN_MSG(result, "Failed to create window surface.");
}

void OgEngine::VulkanContext::InitGpuDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("No GPU support found for the renderer.");

	std::vector<VkPhysicalDevice> GPUs(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, GPUs.data());
	memset(&m_vulkanDevice.gpuProperties, 0, sizeof(m_vulkanDevice.gpuProperties));

	int id = 0;
	for (VkPhysicalDevice gpu : GPUs)
	{
		vkGetPhysicalDeviceProperties(gpu, &m_vulkanDevice.gpuProperties);
		std::cout << "GPU " << id << ": "<< m_vulkanDevice.gpuProperties.deviceName << '\n';
		id++;
	}
	memset(&m_vulkanDevice.gpuProperties, 0, sizeof(m_vulkanDevice.gpuProperties));

	id = 0;
	std::cout << "Select chosen GPU: ";
	std::cin >> id;
	if (!IsPhysicalDeviceSuitable(GPUs[id]))
		throw std::runtime_error("Invalid GPU!");
	m_vulkanDevice.gpu = GPUs[id];
	m_vulkanDevice.msaaSamples = GetMaxUsableSampleCount();

	vkGetPhysicalDeviceProperties(m_vulkanDevice.gpu, &m_vulkanDevice.gpuProperties);
	vkGetPhysicalDeviceMemoryProperties(m_vulkanDevice.gpu, &m_vulkanDevice.gpuMemoryProperties);

	std::cout << "Selected GPU: " << m_vulkanDevice.gpuProperties.deviceName << '\n';

	if (m_vulkanDevice.gpu == nullptr)
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
}

void OgEngine::VulkanContext::InitLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_vulkanDevice.gpu);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	m_vulkanDevice.gpuEnabledFeatures.samplerAnisotropy = VK_TRUE;
	m_vulkanDevice.gpuEnabledFeatures.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
	indexingFeatures.pNext = nullptr;
	indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	indexingFeatures.runtimeDescriptorArray = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.pNext = &indexingFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_gpuExtensions.size());
	createInfo.ppEnabledExtensionNames = m_gpuExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_vulkanDevice.gpu, &createInfo, nullptr, &m_vulkanDevice.logicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device.");
	}

	vkGetDeviceQueue(m_vulkanDevice.logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_vulkanDevice.logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);

	m_vulkanDevice.graphicFamily = std::make_optional(indices.graphicsFamily.value());
	m_vulkanDevice.presentFamily = std::make_optional(indices.presentFamily.value());
}


bool OgEngine::VulkanContext::CheckValidationLayers() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}


	return true;
}

std::vector<const char*> OgEngine::VulkanContext::GetRequieredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
	extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL OgEngine::VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT p_messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT p_messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* p_pCallbackData,
	void* p_pUserData)
{
	std::cout << p_pCallbackData->pMessage << "\n\n";
	return VK_FALSE;
}

void OgEngine::VulkanContext::InitDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& p_createInfo)
{
	p_createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	if constexpr (enableValidationLayers)
	{
		p_createInfo.messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		p_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	}
	else
	{
		p_createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		p_createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	}

	p_createInfo.pfnUserCallback = DebugCallback;
	p_createInfo.pUserData = nullptr; // Optional
	p_createInfo.pNext = nullptr;
	p_createInfo.flags = 0;
}

void OgEngine::VulkanContext::SetupDebugMessenger()
{
	if constexpr (!enableValidationLayers)
		return;
	VkDebugUtilsMessengerCreateInfoEXT infos;
	InitDebugMessenger(infos);

	DBG_ASSERT_VULKAN_MSG(
		CreateDebugUtilsMessengerEXT(m_instance, &infos, nullptr, &m_debugMessenger),
		"Failed to setup debug messenger.");
}

bool OgEngine::VulkanContext::IsPhysicalDeviceSuitable(VkPhysicalDevice p_gpu)
{
	vkGetPhysicalDeviceMemoryProperties(p_gpu, &m_vulkanDevice.gpuMemoryProperties);
	VkPhysicalDeviceFeatures p_features;
	p_features.shaderStorageBufferArrayDynamicIndexing = true;

	vkGetPhysicalDeviceFeatures(p_gpu, &p_features);
	m_vulkanDevice.gpuFeatures.push_back(p_features);
	const QueueFamilyIndices indices = FindQueueFamilies(p_gpu);

	const bool extensionsSupported = CheckDeviceExtensionSupport(p_gpu);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		const SupportDetails swapChainSupport = SwapChainSupport(p_gpu);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	else
	{
		std::cout << "Extension isn't supported by the GPU.\n";
	}
	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool OgEngine::VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_gpuExtensions.begin(), m_gpuExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	if (std::find(requiredExtensions.begin(), requiredExtensions.end(), "VK_NV_RAY_TRACING_EXTENSION_NAME") == requiredExtensions.end())
		isRaytracingAvailable = true;
	else
		isRaytracingAvailable = false;
	return true;
}

SupportDetails OgEngine::VulkanContext::SwapChainSupport(VkPhysicalDevice p_device)
{
	SupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, m_vulkanDevice.surface, &details.capabilities);
	minImageCount = details.capabilities.minImageCount;

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, m_vulkanDevice.surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, m_vulkanDevice.surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, m_vulkanDevice.surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, m_vulkanDevice.surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSampleCountFlagBits OgEngine::VulkanContext::GetMaxUsableSampleCount() {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_vulkanDevice.gpu, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

QueueFamilyIndices OgEngine::VulkanContext::FindQueueFamilies(VkPhysicalDevice p_gpu)
{
	/**
	A queue family just describes a set of queues with identical properties.
	The device supports three kinds of queues:

	One kind can do graphics, compute, transfer, and sparse binding operations
	*/

	QueueFamilyIndices indices;

	//Get QueueFamily size from the GPU
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(p_gpu, &queueFamilyCount, nullptr);

	//Get actual info of the GPU's QueueFamily
	m_vulkanDevice.queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(p_gpu, &queueFamilyCount, m_vulkanDevice.queueFamilyProperties.data());

	int i = 0;
	for (const auto& queueFamily : m_vulkanDevice.queueFamilyProperties)
	{
		//Finding support for image rendering (presentation)
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(p_gpu, i, m_vulkanDevice.surface, &presentSupport);

		if (presentSupport)
		{
			indices.presentFamily = i;
		}
		//GraphicFamily and Present Family might very likely be the same id, but for support compatibility purpose,
		//we separate them into 2 queue Famlilies

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		if (indices.isComplete())
			break;

		i++;
	}
	return indices;
}

void OgEngine::VulkanContext::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<OgEngine::VulkanContext*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

void OgEngine::VulkanContext::VK_ERROR(VkResult p_result) const
{
	std::cout << "VK_RESULT CODE: " << p_result << '\n';
	if (p_result != VK_SUCCESS)
	{
		throw std::runtime_error("VK_RESULT NOT SUCCESS");
	}
}

void OgEngine::VulkanContext::SetRenderingLoop(const bool p_renderingLoop)
{
	m_renderingLoop.store(p_renderingLoop);
}

void OgEngine::VulkanContext::ChangeWindowTitle(std::string_view p_title, const uint64_t p_fps) const
{
	glfwSetWindowTitle(m_window, std::string(p_title.data() + std::to_string(p_fps)).c_str());
}

void OgEngine::VulkanContext::PollEvents() const
{
	glfwPollEvents();
}

bool OgEngine::VulkanContext::WindowShouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

double OgEngine::VulkanContext::TimeOfContext() const
{
	return glfwGetTime();
}

bool OgEngine::VulkanContext::IsRendering() const
{
	return m_renderingLoop.load();
}

GLFWwindow* OgEngine::VulkanContext::GetWindow() const
{
	return m_window;
}

bool OgEngine::VulkanContext::IsRaytracing() const
{
	return isUsingRaytracing;
}

void OgEngine::VulkanContext::CheckRaytracingSupport()
{
	std::string input{};
	bool useRT = false;

	if (!Is64BitProcess())
	{
		std::cout << "Raytracing isn't supported on x86 platform.\n";
		isUsingRaytracing = false;
		return;
	}

	if (!isRaytracingAvailable)
	{
		isUsingRaytracing = false;
		std::cout << "Rasterisation renderer selected.\n";
		return;
	}

	std::cout << "\nDo you want to use Raytracing rendering? (yes/no)\n -> ";
	std::cin >> input;
	std::transform(input.begin(), input.end(), input.begin(),
		[](unsigned char c) { return std::tolower(c); });

	if (input == "yes")
		useRT = true;
	else if (input == "no")
		useRT = false;
	else
	{
		useRT = false;
		std::cout << "Invalid answer, selecting Rasterization.\n";
		isUsingRaytracing = false;
		return;
	}

	if (useRT == true)
	{
		std::cout << "Raytracing renderer selected.\n";
		isUsingRaytracing = true;
		return;
	}
	else
	{
		std::cout << "Rasterization renderer selected.\n";
		isUsingRaytracing = false;
		return;
	}
}

void OgEngine::VulkanContext::InitSelectedRenderer()
{
	if (isUsingRaytracing)
	{

		m_RTPipeline = new OgEngine::RaytracingPipeline(m_vulkanDevice, m_width, m_height, m_graphicsQueue, m_presentQueue, m_window, minImageCount);
		m_RTPipeline->SetupRaytracingPipeline();
	}
	else
	{
		m_RSPipeline = new OgEngine::RasterizerPipeline(m_window, m_vulkanDevice, m_graphicsQueue, m_presentQueue,
			m_width, m_height);

		m_RSPipeline->SetupPipeline();
	}
}
