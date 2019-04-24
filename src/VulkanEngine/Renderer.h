#pragma once

#include <cstdlib>
#include <vector>
#include <stdio.h>
#include <sstream>
#include "Shared.h"
#include "Platform.h"


class Renderer
{
public:
	Renderer();
	~Renderer();

private:
	void _InitInstance();
	void _DeInitInstance();

	void _InitDevice();
	void _DeInitDevice();

	void _SetupDebug();
	void _InitDebug();
	void _DeInitDebug();

	void pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDevicesCount);
	void getGraphicsFamilyIndex(VkQueueFamilyProperties* queueList, uint32_t queueCount);

	VkInstance	_instance = nullptr;
	VkDevice	_device = nullptr;
	VkPhysicalDevice _gpu = nullptr;
	uint32_t _graphicsFamilyBit = 0;
	VkPhysicalDeviceProperties _gpuProperties = {};

	std::vector<const char*> _instanceLayers;
	std::vector<const char*> _instanceExtensions;
	std::vector<const char*> _deviceLayers;
	std::vector<const char*> _deviceExtensions;

	VkDebugReportCallbackEXT _debugReport = nullptr;
	VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo{};
};

