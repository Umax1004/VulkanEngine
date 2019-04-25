#pragma once
#include "BUILD_OPTIONS.h"
#include <stdint.h>
#include <vector>
#include "Shared.h"
void InitPlatform();
void DeInitPlatform();
void AddRequiredPlatformInstanceExtensions(std::vector<const char *> *instance_extensions);

// GLFW
#if BUILD_USE_GLFW

// Define as a build option 
#define USE_FRAMEWORK_GLFW 1
//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
// For Windows Message Box
#if defined( _WIN32 )
#undef APIENTRY
#include <windows.h>
#endif 

// WINDOWS
#elif defined( _WIN32 )

#define VK_USE_PLATFORM_WIN32_KHR 1
#include <windows.h>

// LINUX ( Via XCB library )
#elif defined( __linux )
// there are other ways to create windows on linux,
// requirements might change based on the available libraries and distro used
// for example: xlib, wayland and mir
// xcb seems like a popular and well supported option on X11, until wayland and mir take over

#define VK_USE_PLATFORM_XCB_KHR 1
#include <xcb/xcb.h>

#else
// platform not yet supported
#error Platform not yet supported
#endif

#include <vulkan/vulkan.h>
