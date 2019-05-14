#pragma once

#include <cstdlib>
#include <vector>
#include <stdio.h>
#include <sstream>
#include "Shared.h"
#include "Platform.h"
#include "BUILD_OPTIONS.h"

class Window;
class Renderer
{
public:
	Renderer();
	~Renderer();

	Window* OpenWindow(uint32_t size_x, uint32_t size_y, std::string name);
	bool Run();

	const VkInstance						GetVulkanInstance()	const;
	const VkPhysicalDevice					GetVulkanPhysicalDevice() const;
	const VkDevice							GetVulkanDevice() const;
	const VkQueue							GetVulkanQueue() const;
	const uint32_t							GetVulkanGraphicsQueueFamilyIndex() const;
	const VkPhysicalDeviceProperties		&	GetVulkanPhysicalDeviceProperties() const;
	const VkPhysicalDeviceMemoryProperties	&	GetVulkanPhysicalDeviceMemoryProperties() const;

private:

	void _SetupLayersAndExtensions();

	void _InitInstance();
	void _DeInitInstance();

	void _InitDevice();
	void _DeInitDevice();

	void _SetupDebug();
	void _InitDebug();
	void _DeInitDebug();

	void pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDevicesCount);
	void getGraphicsFamilyIndex(VkQueueFamilyProperties* queueList, uint32_t queueCount);

	VkInstance	_instance = VK_NULL_HANDLE;
	VkDevice	_device = VK_NULL_HANDLE;
	VkPhysicalDevice _gpu = VK_NULL_HANDLE;
	VkQueue _queue = VK_NULL_HANDLE;
	uint32_t _graphicsFamilyIndex = 0;
	VkPhysicalDeviceFeatures _gpuFeatures = {};
	VkPhysicalDeviceProperties _gpuProperties = {};
	VkPhysicalDeviceMemoryProperties _gpuMemoryProperties = {};

	std::vector<const char*> _instanceLayers;
	std::vector<const char*> _instanceExtensions;
//	std::vector<const char*> _deviceLayers; Depricated
	std::vector<const char*> _deviceExtensions;

	VkDebugReportCallbackEXT _debugReport = nullptr;
	VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo{};

	Window* _window = nullptr;
};

