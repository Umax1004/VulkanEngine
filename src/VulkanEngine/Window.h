#pragma once

#include "Platform.h"
#include <string>
#include "Renderer.h"
#include <array>


class Window
{
public:
	Window(Renderer* renderer, uint32_t size_x, uint32_t size_y, std::string name);
	~Window();

	void Close();
	bool Update();
	void DrawFrame();

	std::vector<VkImage> GetSwapchainImages();
	VkSwapchainKHR		 GetSwapchain();
	uint32_t			 GetSwapchainImagesCount();

private:
	void _InitOSWindow();
	void _DeInitOSWindow();
	void _UpdateOSWindow();
	void _InitOSSurface();

	void _InitSurface();
	void _DeInitSurface();

	void _InitSwapchain();
	void _DeInitSwapchain();

	void _InitSwapchainImages();
	void _DeInitSwapchainImages();

	void _InitDepthStencilImage();
	void _DeInitDepthStencilImage();

	void _InitRenderPass();
	void _DeInitRenderPass();

	void _InitFramebuffers();
	void _DeInitFramebuffers();

	void _InitGraphicsPipeline();
	void _DeInitGraphicsPipeline();

	void _InitCommandPool();
	void _DeInitCommandPool();

	void _InitCommandBuffers();
	void _DeInitCommandBuffers();

	void _InitSemaphore();
	void _DeInitSemaphore();

	static std::vector<char> readFile(const std::string& filename);

	VkShaderModule _CreateShaderModule(const std::vector<char>& code);

	Renderer* _renderer;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
	VkRenderPass _renderPass = VK_NULL_HANDLE;

	bool _windowShouldRun = true;

	uint32_t							_surface_size_x = 512;
	uint32_t							_surface_size_y = 512;
	std::string							_window_name;
	uint32_t							_swapchainImageCount = 2;

	std::vector<VkImage>				_swapchainImages;
	std::vector<VkImageView>			_swapchainImageViews;
	std::vector<VkFramebuffer>			_framebuffers;

	VkImage								_depthStencilImage = VK_NULL_HANDLE;
	VkDeviceMemory						_depthStencilImageMemory = VK_NULL_HANDLE;
	VkImageView							_depthStencilImageView = VK_NULL_HANDLE;
	
	VkSurfaceCapabilitiesKHR _surfaceCapabilites{};
	VkSurfaceFormatKHR _surfaceFormat{};

	VkFormat _depthStencilFormat = VK_FORMAT_UNDEFINED;
	bool _stencilAvailable = false;

	VkShaderModule _vertShaderModule = VK_NULL_HANDLE;
	VkShaderModule _fragShaderModule = VK_NULL_HANDLE;

	VkViewport viewport = {};
	VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
	VkPipeline _graphicsPipeline = VK_NULL_HANDLE;
	VkCommandPool _commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> _commandBuffers;
	VkSemaphore _imageAvailableSemaphore;
	VkSemaphore _renderFinishedSemaphore;

#if USE_FRAMEWORK_GLFW
	GLFWwindow						*	_glfw_window = nullptr;
#elif VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE							_win32_instance = NULL;
	HWND								_win32_window = NULL;
	std::string							_win32_class_name;
	static uint64_t						_win32_class_id_counter;
#endif
};

