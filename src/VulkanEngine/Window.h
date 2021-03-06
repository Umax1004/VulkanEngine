#pragma once


#include "Platform.h"
#include <string>
#include "Renderer.h"
#include <array>
#include "Vertex.h"
#include <chrono>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <unordered_map>
const int MAX_FRAMES_IN_FLIGHT = 2;


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

	void _InitColorResources();
	void _DeInitColorResources();

	void _InitDepthStencilImage();
	void _DeInitDepthStencilImage();

	void _InitRenderPass();
	void _DeInitRenderPass();

	void _InitFramebuffers();
	void _DeInitFramebuffers();

	void _InitDescriptorSetLayout();
	void _DeInitDescriptorSetLayout();

	void _InitGraphicsPipeline();
	void _DeInitGraphicsPipeline();

	void _InitCommandPool();
	void _DeInitCommandPool();

	void _InitTextureImage();
	void _DeInitTextureImage();

	void _InitTextureImageView();
	void _DeInitTextureImageView();

	void _InitTextureSampler();
	void _DeInitTextureSampler();

	void _LoadModel();

	void _InitVertexBuffers();
	void _DeInitVertexBuffers();

	void _InitIndexBuffers();
	void _DeInitIndexBuffers();

	void _InitUniformBuffers();
	void _DeInitUniformBuffers();
	void _UpdateUniformBuffers(uint32_t currentImage);

	void _InitDescriptorPool();
	void _DeInitDescriptorPool();

	void _InitDescriptorSets();
	void _DeInitDescriptorSets();

	void _InitCommandBuffers();
	void _DeInitCommandBuffers();

	void _InitSyncObjects();
	void _DeInitSyncObjects();

	void _CleanUpOldSwapChain();
	void _ReInitSwapChain();

	void _CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void _CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void _CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkCommandBuffer _BeginSingleTimeCommands();
	void _EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void _TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void _CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkImageView _CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	bool _HasStencilComponent(VkFormat format);
	void _GenerateMipMaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	void _GetWindowSize();
	void _WaitForEvents();

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

	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;
	size_t currentFrame = 0;

	bool framebufferResized = false;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkBuffer _vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory _vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer _indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory _indexBufferMemory = VK_NULL_HANDLE;
	VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

	VkImage _textureImage = VK_NULL_HANDLE;
	VkDeviceMemory _textureImageMemory = VK_NULL_HANDLE;
	VkImageView _textureImageView = VK_NULL_HANDLE;
	VkSampler _textureSampler = VK_NULL_HANDLE;
	uint32_t mipLevels;

	VkImage _colorImage;
	VkDeviceMemory _colorImageMemory;
	VkImageView _colorImageView;

	std::vector<VkBuffer> _uniformBuffers;
	std::vector<VkDeviceMemory> _uniformBuffersMemory;
	std::vector<VkDescriptorSet> descriptorSets;

	const std::string MODEL_PATH = "models/chalet.obj";
	const std::string TEXTURE_PATH = "textures/chalet.jpg";

#if USE_FRAMEWORK_GLFW
	GLFWwindow						*	_glfw_window = nullptr;
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
#elif VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE							_win32_instance = NULL;
	HWND								_win32_window = NULL;
	std::string							_win32_class_name;
	static uint64_t						_win32_class_id_counter;
#endif
};

