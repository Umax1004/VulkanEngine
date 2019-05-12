/* -----------------------------------------------------
This source code is public domain ( CC0 )
The code is provided as-is without limitations, requirements and responsibilities.
Creators and contributors to this source code are provided as a token of appreciation
and no one associated with this source code can be held responsible for any possible
damages or losses of any kind.

Original file creator:  Teagan Chouinard (GLFW)
Contributors:
Niko Kauppi (Code maintenance)
----------------------------------------------------- */

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Window.h"
#include "Shared.h"
#include "Renderer.h"

#include <assert.h>
#include <iostream>

#if USE_FRAMEWORK_GLFW

void Window::_InitOSWindow()
{
	int rc = glfwInit();
	assert(rc == GLFW_TRUE);
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	_glfw_window = glfwCreateWindow((int) _surface_size_x,(int) _surface_size_y, _window_name.c_str(), nullptr, nullptr );
	glfwSetWindowUserPointer(_glfw_window, this);
	glfwSetFramebufferSizeCallback(_glfw_window, framebufferResizeCallback);
	//glfwGetFramebufferSize( _glfw_window, (int*)&_surface_size_x, (int*)&_surface_size_y );
}

void Window::_DeInitOSWindow()
{
	glfwDestroyWindow( _glfw_window );
	glfwTerminate();
}

void Window::_UpdateOSWindow()
{
	glfwPollEvents();

	if( glfwWindowShouldClose( _glfw_window ) ) {
		Close();
	}
}

void Window::_InitOSSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	createInfo.hinstance = GetModuleHandle(0);
	createInfo.hwnd = glfwGetWin32Window(_glfw_window);
	if (VK_SUCCESS != vkCreateWin32SurfaceKHR(_renderer->GetVulkanInstance(), &createInfo, nullptr, &_surface)) {
		glfwTerminate();
		assert(0 && "GLFW could not create the window surface.");
		return;
	}
}

void Window::_GetWindowSize()
{
	int width, height = 0;
	glfwGetFramebufferSize(_glfw_window,  &width, &height);
	_surface_size_x = width;
	_surface_size_y = height;
}

void Window::_WaitForEvents()
{
	glfwWaitEvents();
}

void Window::framebufferResizeCallback(GLFWwindow * window, int width, int height)
{
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
	app->_surface_size_x = width;
	app->_surface_size_y = height;
}

#endif // USE_FRAMEWORK_GLFW
