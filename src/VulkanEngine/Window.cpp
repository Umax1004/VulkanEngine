#include "Window.h"

Window::Window(Renderer* renderer, uint32_t size_x, uint32_t size_y, std::string name)
{
	_renderer = renderer;
	_surface_size_x = size_x;
	_surface_size_y = size_y;
	_window_name = name;
	_InitOSWindow();
	_InitSurface();
	_InitSwapchain();
	_InitSwapchainImages();
	_InitDepthStencilImage();
	_InitRenderPass();
}

Window::~Window()
{
	_DeInitRenderPass();
	_DeInitDepthStencilImage();
	_DeInitSwapchainImages();
	_DeInitSwapchain();
	_DeInitSurface();
	_DeInitOSWindow();
}

void Window::Close()
{
	_windowShouldRun = false;
}

bool Window::Update()
{
	_UpdateOSWindow();
	return _windowShouldRun;
}

void Window::_InitSurface()
{
	_InitOSSurface();

	VkBool32 WSISupported = false;

	vkGetPhysicalDeviceSurfaceSupportKHR(_renderer->GetVulkanPhysicalDevice(), _renderer->GetVulkanGraphicsQueueFamilyIndex(), _surface, &WSISupported);

	if (!WSISupported)
	{
		assert(0 && "WSI is not supported");
		std::exit(-1);
	}


	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &_surfaceCapabilites);
	if (_surfaceCapabilites.currentExtent.width < UINT32_MAX) {
		_surface_size_x = _surfaceCapabilites.currentExtent.width;
		_surface_size_y = _surfaceCapabilites.currentExtent.height;
	}

	{
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &formatCount, nullptr);
		if (formatCount == 0)
		{
			assert(0 && "Surface format unavailable");
			std::exit(-1);
		}
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &formatCount, formats.data());
		if (formats[0].format == VK_FORMAT_UNDEFINED)
		{
			_surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM;
			_surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		}
		else
		{
			_surfaceFormat = formats[0];
		}
	}

}

void Window::_DeInitSurface()
{
	vkDestroySurfaceKHR( _renderer->GetVulkanInstance(), _surface, nullptr );
}

void Window::_InitSwapchain()
{
	if (_swapchainImageCount < _surfaceCapabilites.minImageCount + 1) _swapchainImageCount = _surfaceCapabilites.minImageCount + 1;
	if (_surfaceCapabilites.maxImageCount > 0) {
		if (_swapchainImageCount > _surfaceCapabilites.maxImageCount) _swapchainImageCount = _surfaceCapabilites.maxImageCount;
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	{
		uint32_t present_mode_count = 0;
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &present_mode_count, nullptr));
		std::vector<VkPresentModeKHR> present_mode_list(present_mode_count);
		ErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(_renderer->GetVulkanPhysicalDevice(), _surface, &present_mode_count, present_mode_list.data()));
		for (auto m : present_mode_list) {
			if (m == VK_PRESENT_MODE_MAILBOX_KHR) present_mode = m;
		}
	}

	VkCompositeAlphaFlagBitsKHR surfaceComposite =
		(_surfaceCapabilites.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
		: (_surfaceCapabilites.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
		: (_surfaceCapabilites.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
		: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = _surface;
	swapchainCreateInfo.minImageCount = _swapchainImageCount;
	swapchainCreateInfo.imageFormat = _surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = _surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent.width = _surface_size_x;
	swapchainCreateInfo.imageExtent.height = _surface_size_y;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = surfaceComposite;
	swapchainCreateInfo.presentMode = present_mode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	ErrorCheck(vkCreateSwapchainKHR(_renderer->GetVulkanDevice(), &swapchainCreateInfo, nullptr, &_swapchain));

	ErrorCheck(vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &_swapchainImageCount, nullptr));
}

void Window::_DeInitSwapchain()
{
	vkDestroySwapchainKHR(_renderer->GetVulkanDevice(), _swapchain, nullptr);
}

void Window::_InitSwapchainImages()
{
	_swapchainImages.resize(_swapchainImageCount);
	_swapchainImageViews.resize(_swapchainImageCount);
	ErrorCheck(vkGetSwapchainImagesKHR(_renderer->GetVulkanDevice(), _swapchain, &_swapchainImageCount, _swapchainImages.data()));

	for (uint32_t i = 0; i < _swapchainImageCount; i++)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = _swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = _surfaceFormat.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		ErrorCheck(vkCreateImageView(_renderer->GetVulkanDevice() , &imageViewCreateInfo, nullptr, &_swapchainImageViews[i]));

	}
}

void Window::_DeInitSwapchainImages()
{
	for (auto view : _swapchainImageViews)
	{
		vkDestroyImageView(_renderer->GetVulkanDevice(), view, nullptr);
	}
}

void Window::_InitDepthStencilImage()
{
	{
		std::vector<VkFormat> try_formats{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D16_UNORM
		};
		for (auto f : try_formats) {
			VkFormatProperties format_properties{};
			vkGetPhysicalDeviceFormatProperties(_renderer->GetVulkanPhysicalDevice(), f, &format_properties);
			if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				_depthStencilFormat = f;
				break;
			}
		}
		if (_depthStencilFormat == VK_FORMAT_UNDEFINED) {
			assert(0 && "Depth stencil format not selected.");
			std::exit(-1);
		}
		if ((_depthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
			(_depthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT) ||
			(_depthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT) ||
			(_depthStencilFormat == VK_FORMAT_S8_UINT)) {
			_stencilAvailable = true;
		}
	}
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = _depthStencilFormat;
	imageCreateInfo.extent.width = _surface_size_x;
	imageCreateInfo.extent.height = _surface_size_y;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateImage(_renderer->GetVulkanDevice(), &imageCreateInfo, nullptr, &_depthStencilImage);

	VkMemoryRequirements imageMemoryRequirements{};
	vkGetImageMemoryRequirements(_renderer->GetVulkanDevice(), _depthStencilImage, &imageMemoryRequirements);

	uint32_t memoryIndex = FindMemoryTypeIndex(&_renderer->GetVulkanPhysicalDeviceMemoryProperties(), &imageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = imageMemoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryIndex;

	vkAllocateMemory(_renderer->GetVulkanDevice(), &memoryAllocateInfo, nullptr, &_depthStencilImageMemory);
	vkBindImageMemory(_renderer->GetVulkanDevice(), _depthStencilImage, _depthStencilImageMemory, 0);


	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = _depthStencilImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = _depthStencilFormat;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (_stencilAvailable ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	vkCreateImageView(_renderer->GetVulkanDevice(), &imageViewCreateInfo, nullptr, &_depthStencilImageView);
}

void Window::_DeInitDepthStencilImage()
{
	vkDestroyImageView(_renderer->GetVulkanDevice(), _depthStencilImageView, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), _depthStencilImageMemory, nullptr);
	vkDestroyImage(_renderer->GetVulkanDevice(), _depthStencilImage, nullptr);
}

void Window::_InitRenderPass()
{

	std::array<VkAttachmentDescription, 2> attachments{};
	attachments[0].flags = 0;
	attachments[0].format = _depthStencilFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[1].flags = 0;
	attachments[1].format = _surfaceFormat.format;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference subPass0DepthStencilAttachment{};
	subPass0DepthStencilAttachment.attachment = 0;
	subPass0DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 1> subPass0ColorAttachments{};
	subPass0ColorAttachments[0].attachment = 1;
	subPass0ColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDescription, 1> subPasses{};
	subPasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPasses[0].colorAttachmentCount = subPass0ColorAttachments.size();
	subPasses[0].pColorAttachments = subPass0ColorAttachments.data();	
	subPasses[0].pDepthStencilAttachment = &subPass0DepthStencilAttachment;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachments.size();
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = subPasses.size();
	renderPassCreateInfo.pSubpasses = subPasses.data();

	ErrorCheck(vkCreateRenderPass(_renderer->GetVulkanDevice(), &renderPassCreateInfo, nullptr, &_renderPass));
}

void Window::_DeInitRenderPass()
{
	vkDestroyRenderPass(_renderer->GetVulkanDevice(), _renderPass, nullptr);
}

std::vector<VkImage> Window::GetSwapchainImages()
{
	return _swapchainImages;
}

VkSwapchainKHR Window::GetSwapchain()
{
	return _swapchain;
}

uint32_t Window::GetSwapchainImagesCount()
{
	return _swapchainImageCount;
}
