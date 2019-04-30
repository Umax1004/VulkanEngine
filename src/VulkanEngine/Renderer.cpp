#include "Renderer.h"
#include "Window.h"

Renderer::Renderer()
{
	_SetupLayersAndExtensions();
	_SetupDebug();
	_InitInstance();
	_InitDebug();
	_InitDevice();
}


Renderer::~Renderer()
{
	delete _window;
	_DeInitDevice();
	_DeInitDebug();
	_DeInitInstance();
}

Window * Renderer::OpenWindow(uint32_t size_x, uint32_t size_y, std::string name)
{
	_window = new Window(this, size_x,size_y,name);
	return _window;
}

bool Renderer::Run()
{
	if (_window != nullptr )
	{
		return _window->Update();
	}
	return true;
}

const VkInstance Renderer::GetVulkanInstance() const
{
	return _instance;
}

const VkPhysicalDevice Renderer::GetVulkanPhysicalDevice() const
{
	return _gpu;
}

const VkDevice Renderer::GetVulkanDevice() const
{
	return _device;
}

const VkQueue Renderer::GetVulkanQueue() const
{
	return _queue;
}

const uint32_t Renderer::GetVulkanGraphicsQueueFamilyIndex() const
{
	return _graphicsFamilyIndex;
}

const VkPhysicalDeviceProperties & Renderer::GetVulkanPhysicalDeviceProperties() const
{
	return _gpuProperties;
}

const VkPhysicalDeviceMemoryProperties & Renderer::GetVulkanPhysicalDeviceMemoryProperties() const
{
	return _gpuMemoryProperties;
}

void Renderer::_SetupLayersAndExtensions()
{
	//_instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
	_instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	AddRequiredPlatformInstanceExtensions(&_instanceExtensions);
	_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Renderer::_InitInstance()
{
	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.apiVersion = VK_API_VERSION_1_1;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pApplicationName = "Vulkan Engine";


	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = _instanceLayers.size();
	instanceCreateInfo.ppEnabledLayerNames = _instanceLayers.data();
	instanceCreateInfo.enabledExtensionCount = _instanceExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = _instanceExtensions.data();
	instanceCreateInfo.pNext = &debugCallbackCreateInfo;

	ErrorCheck(vkCreateInstance(&instanceCreateInfo, nullptr, &_instance));

}

void Renderer::_DeInitInstance()
{
	vkDestroyInstance(_instance, nullptr);
	_instance = VK_NULL_HANDLE;
}

void Renderer::_InitDevice()
{
	{
		uint32_t gpuCount = 0;
		vkEnumeratePhysicalDevices(_instance, &gpuCount, nullptr);
		std::vector<VkPhysicalDevice> gpuList(gpuCount);
		vkEnumeratePhysicalDevices(_instance, &gpuCount, gpuList.data());
		pickPhysicalDevice(gpuList.data(), gpuCount);
		vkGetPhysicalDeviceProperties(_gpu, &_gpuProperties);
		vkGetPhysicalDeviceMemoryProperties(_gpu, &_gpuMemoryProperties);
	}

	{
		uint32_t familyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &familyCount, nullptr);
		std::vector<VkQueueFamilyProperties> familyPropertyList(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &familyCount, familyPropertyList.data());
		getGraphicsFamilyIndex(familyPropertyList.data(), familyCount);
	}

	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> layerPropertiesList(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layerPropertiesList.data());
		std::cout << "Instanced layer properties: \n";
		for (auto &i: layerPropertiesList)
		{
			std::cout << "  " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << std::endl;
	}
	{
		uint32_t layerCount = 0;
		vkEnumerateDeviceLayerProperties(_gpu,&layerCount, nullptr);
		std::vector<VkLayerProperties> layerPropertiesList(layerCount);
		vkEnumerateDeviceLayerProperties(_gpu,&layerCount, layerPropertiesList.data());
		std::cout << "Device layer properties: \n";
		for (auto &i : layerPropertiesList)
		{
			std::cout << "  " << i.layerName << "\t\t | " << i.description << std::endl;
		}
		std::cout << std::endl;
	}

	float queuePriorities[] { 1.0f };

	VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.queueFamilyIndex = _graphicsFamilyIndex;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
//	deviceCreateInfo.enabledLayerCount = _deviceLayers.size();
//	deviceCreateInfo.ppEnabledLayerNames = _deviceLayers.data();
	deviceCreateInfo.enabledExtensionCount = _deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = _deviceExtensions.data();



	ErrorCheck(vkCreateDevice( _gpu, &deviceCreateInfo, nullptr, &_device));

	vkGetDeviceQueue(_device, _graphicsFamilyIndex, 0, &_queue);

}

void Renderer::_DeInitDevice()
{
	vkDestroyDevice(_device, nullptr);
	_device = 0;
}

#if BUILD_ENABLE_VULKAN_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT obj_flags,
	uint64_t src_obj,
	size_t location,
	int32_t msg_code,
	const char* layer_prefix,
	const char* msg,
	void * user_data
	)
{
	std::ostringstream stream;
	stream << "VKDBG: ";
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		stream << "INFO: ";
	}
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		stream << "WARNING: ";
	}
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		stream << "PERFORMANCE: ";
	}
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		stream << "ERROR: ";
	}
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		stream << "DEBUG: ";
	}
	stream << "@[" << layer_prefix << "]: ";
	stream << msg << std::endl;
	std::cout << stream.str();

#if defined( _WIN32 )
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		MessageBox(NULL, stream.str().c_str(), "Vulkan Error!", 0);
	}
#endif

	return false;
}

void Renderer::_SetupDebug()
{
	debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debugCallbackCreateInfo.pfnCallback = VulkanDebugCallback;
	debugCallbackCreateInfo.flags =
//		VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
//		VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT | 0;

	_instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
	_instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	//_deviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
}

PFN_vkCreateDebugReportCallbackEXT		fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT		fvkDestroyDebugReportCallbackEXT = nullptr;

void Renderer::_InitDebug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");
	if (nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT)
	{
		assert(0 && "Vulkan ERROR: Can't fetch debug function pointer");
		std::exit(-1);
	}

	fvkCreateDebugReportCallbackEXT(_instance, &debugCallbackCreateInfo, nullptr, &_debugReport);
}

void Renderer::_DeInitDebug()
{
	fvkDestroyDebugReportCallbackEXT(_instance, _debugReport, nullptr);
	_debugReport = VK_NULL_HANDLE;
}
#else
void Renderer::_SetupDebug(){
	_instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
	_instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
}
void Renderer::_InitDebug(){}
void Renderer::_DeInitDebug(){}
#endif //BUILD_ENABLE_VULKAN_DEBUG

void Renderer::pickPhysicalDevice(VkPhysicalDevice * physicalDevices, uint32_t physicalDevicesCount)
{
	bool found = false;
	for (uint32_t i = 0; i < physicalDevicesCount; i++)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			printf("Picking discrete GPU %s\n", &props.deviceName);
			_gpu = physicalDevices[i];
			found = true ;
			break;
			return;
		}
	}

	if (physicalDevicesCount > 0 && !found)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevices[0], &props);
		printf("Picking fallback GPU %s\n", &props.deviceName);
		_gpu = physicalDevices[0];
	}
	else if (!found)
	{
		printf("Hey No Device Available");
		_gpu = VK_NULL_HANDLE;
	}
}

void Renderer::getGraphicsFamilyIndex(VkQueueFamilyProperties * queueList, uint32_t queueCount)
{
	bool found = false;
	for (uint32_t i = 0; i < queueCount; i++)
	{
		if (queueList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			found = true;
			_graphicsFamilyIndex = i;
		}
	}

	if (!found)
	{
		assert(0 && "Vulkan ERROR: Queue Family support for Graphics not found");
		std::exit(-1);
	}
}
