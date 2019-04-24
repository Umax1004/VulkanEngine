#include "Platform.h"


#if VK_USE_PLATFORM_WIN32_KHR

void InitPlatform()
{
}

void DeInitPlatform()
{
}

void AddRequiredPlatformInstanceExtensions(std::vector<const char *> *instance_extensions)
{
	instance_extensions->push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

#elif VK_USE_PLATFORM_XCB_KHR

void InitPlatform()
{
}

void DeInitPlatform()
{
}

void AddRequiredPlatformInstanceExtensions(std::vector<const char *> *instance_extensions)
{
	instance_extensions->push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
}

#endif
