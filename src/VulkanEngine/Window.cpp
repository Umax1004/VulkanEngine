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
	_InitRenderPass();
	_InitDescriptorSetLayout();
	_InitGraphicsPipeline();
	_InitCommandPool();
	_InitColorResources();
	_InitDepthStencilImage();
	_InitFramebuffers();
	_InitTextureImage();
	_InitTextureImageView();
	_InitTextureSampler();
	_LoadModel();
	_InitVertexBuffers();
	_InitIndexBuffers();
	_InitUniformBuffers();
	_InitDescriptorPool();
	_InitDescriptorSets();
	_InitCommandBuffers();
	_InitSyncObjects();
}

Window::~Window()
{
	_DeInitSyncObjects();
	_DeInitCommandBuffers();
	_DeInitDescriptorPool();
	_DeInitUniformBuffers();
	_DeInitIndexBuffers();
	_DeInitVertexBuffers();
	_DeInitTextureSampler();
	_DeInitTextureImageView();
	_DeInitTextureImage();
	_DeInitFramebuffers();
	_DeInitDepthStencilImage();
	_DeInitColorResources();
	_DeInitCommandPool();
	_DeInitGraphicsPipeline();
	_DeInitDescriptorSetLayout();
	_DeInitRenderPass();
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

void Window::DrawFrame()
{
	vkWaitForFences(_renderer->GetVulkanDevice(), 1, &_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(_renderer->GetVulkanDevice(), 1, &_inFlightFences[currentFrame]);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_renderer->GetVulkanDevice(), _swapchain, UINT64_MAX, _imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		_ReInitSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	_UpdateUniformBuffers(imageIndex);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];
	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	ErrorCheck(vkQueueSubmit(_renderer->GetVulkanQueue(), 1, &submitInfo, _inFlightFences[currentFrame]));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(_renderer->GetVulkanQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		_ReInitSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	//vkQueueWaitIdle(_renderer->GetVulkanQueue());

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

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
		_swapchainImageViews[i] = _CreateImageView(_swapchainImages[i], _surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	}
}

void Window::_DeInitSwapchainImages()
{
	for (auto view : _swapchainImageViews)
	{
		vkDestroyImageView(_renderer->GetVulkanDevice(), view, nullptr);
	}
}

void Window::_InitColorResources()
{
	VkFormat colorFormat = _surfaceFormat.format;

	_CreateImage(_surface_size_x, _surface_size_y, 1, _renderer->GetMaxSampleCount(), colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _colorImage, _colorImageMemory);
	_colorImageView = _CreateImageView(_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	_TransitionImageLayout(_colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

void Window::_DeInitColorResources()
{
	vkDestroyImageView(_renderer->GetVulkanDevice(), _colorImageView, nullptr);
	vkDestroyImage(_renderer->GetVulkanDevice(), _colorImage, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), _colorImageMemory, nullptr);
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
	/*VkImageCreateInfo imageCreateInfo{};
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
	vkCreateImage(_renderer->GetVulkanDevice(), &imageCreateInfo, nullptr, &_depthStencilImage);*/

	_CreateImage(_surface_size_x, _surface_size_y, 1, _renderer->GetMaxSampleCount(), _depthStencilFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthStencilImage, _depthStencilImageMemory);

	/*VkMemoryRequirements imageMemoryRequirements{};
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

	vkCreateImageView(_renderer->GetVulkanDevice(), &imageViewCreateInfo, nullptr, &_depthStencilImageView);*/

	_depthStencilImageView = _CreateImageView(_depthStencilImage, _depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	_TransitionImageLayout(_depthStencilImage, _depthStencilFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Window::_DeInitDepthStencilImage()
{
	vkDestroyImageView(_renderer->GetVulkanDevice(), _depthStencilImageView, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), _depthStencilImageMemory, nullptr);
	vkDestroyImage(_renderer->GetVulkanDevice(), _depthStencilImage, nullptr);
}

void Window::_InitRenderPass()
{

	std::array<VkAttachmentDescription, 3> attachments{};
	attachments[0].flags = 0;
	attachments[0].format = _surfaceFormat.format;
	attachments[0].samples = _renderer->GetMaxSampleCount();
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

	attachments[1].flags = 0;
	attachments[1].format = _depthStencilFormat;
	attachments[1].samples = _renderer->GetMaxSampleCount();
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference subPass0DepthStencilAttachment{};
	subPass0DepthStencilAttachment.attachment = 1;
	subPass0DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 1> subPass0ColorAttachments{};
	subPass0ColorAttachments[0].attachment = 0;
	subPass0ColorAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = _surfaceFormat.format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[2] = colorAttachmentResolve;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDescription, 1> subPasses{};
	subPasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPasses[0].colorAttachmentCount = subPass0ColorAttachments.size();
	subPasses[0].pColorAttachments = subPass0ColorAttachments.data();	
	subPasses[0].pDepthStencilAttachment = &subPass0DepthStencilAttachment;
	subPasses[0].pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = subPasses.size();
	renderPassCreateInfo.pSubpasses = subPasses.data();
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	ErrorCheck(vkCreateRenderPass(_renderer->GetVulkanDevice(), &renderPassCreateInfo, nullptr, &_renderPass));
}

void Window::_DeInitRenderPass()
{
	vkDestroyRenderPass(_renderer->GetVulkanDevice(), _renderPass, nullptr);
}

void Window::_InitFramebuffers()
{
	_framebuffers.resize(_swapchainImageCount);
	for (uint32_t i = 0; i < _swapchainImageCount; i++)
	{
		std::array<VkImageView, 3> attachments{};
		attachments[0] = _colorImageView;
		attachments[1] = _depthStencilImageView;
		attachments[2] = _swapchainImageViews[i];

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = _renderPass;
		framebufferCreateInfo.attachmentCount = attachments.size();
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.width = _surface_size_x;
		framebufferCreateInfo.height = _surface_size_y;
		framebufferCreateInfo.layers = 1;
		
		ErrorCheck(vkCreateFramebuffer(_renderer->GetVulkanDevice(), &framebufferCreateInfo, nullptr, &_framebuffers[i]));
	}
}

void Window::_DeInitFramebuffers()
{
	for (auto f : _framebuffers) {
		vkDestroyFramebuffer(_renderer->GetVulkanDevice(), f, nullptr);
	}
}

void Window::_InitDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t> (bindings.size());
	layoutInfo.pBindings = bindings.data();

	ErrorCheck(vkCreateDescriptorSetLayout(_renderer->GetVulkanDevice(), &layoutInfo, nullptr, &_descriptorSetLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
}

void Window::_DeInitDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(_renderer->GetVulkanDevice(), _descriptorSetLayout, nullptr);
}

void Window::_InitGraphicsPipeline()
{
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	_vertShaderModule = _CreateShaderModule(vertShaderCode);
	_fragShaderModule = _CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = _vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = _fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)_surface_size_x;
	viewport.height = (float)_surface_size_y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;


	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { _surface_size_x, _surface_size_y};

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
	multisampling.rasterizationSamples = _renderer->GetMaxSampleCount();

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkDynamicState dynamicStates[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_LINE_WIDTH,
	VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 3;
	dynamicState.pDynamicStates = dynamicStates;


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; 
	pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilInfo{};
	pipelineDepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilInfo.depthTestEnable = VK_TRUE;
	pipelineDepthStencilInfo.depthWriteEnable = VK_TRUE;
	pipelineDepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	pipelineDepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	pipelineDepthStencilInfo.minDepthBounds = 0.0f; // Optional
	pipelineDepthStencilInfo.maxDepthBounds = 1.0f; // Optional
	pipelineDepthStencilInfo.stencilTestEnable = VK_FALSE;
	pipelineDepthStencilInfo.front = {}; // Optional
	pipelineDepthStencilInfo.back = {}; // Optional

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = _renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.pDepthStencilState = &pipelineDepthStencilInfo;

	if (vkCreateGraphicsPipelines(_renderer->GetVulkanDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

void Window::_DeInitGraphicsPipeline()
{
	vkDestroyPipeline(_renderer->GetVulkanDevice(), _graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(_renderer->GetVulkanDevice(), _pipelineLayout, nullptr);
	vkDestroyShaderModule(_renderer->GetVulkanDevice(), _fragShaderModule, nullptr);
	vkDestroyShaderModule(_renderer->GetVulkanDevice(), _vertShaderModule, nullptr);
}

void Window::_InitCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = _renderer->GetVulkanGraphicsQueueFamilyIndex();

	ErrorCheck(vkCreateCommandPool(_renderer->GetVulkanDevice(), &poolInfo, nullptr, &_commandPool));

}

void Window::_DeInitCommandPool()
{
	vkDestroyCommandPool(_renderer->GetVulkanDevice(), _commandPool, nullptr);
}

void Window::_InitTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	_CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory);
	stbi_image_free(pixels);

	_CreateImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory);
	_TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	_CopyBufferToImage(stagingBuffer, _textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	//_TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
	_GenerateMipMaps(_textureImage, VK_FORMAT_R8G8B8A8_UNORM , texWidth, texHeight, mipLevels);

	vkDestroyBuffer(_renderer->GetVulkanDevice(), stagingBuffer, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, nullptr);
	
}

void Window::_DeInitTextureImage()
{
	vkDestroyImage(_renderer->GetVulkanDevice(), _textureImage, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), _textureImageMemory, nullptr);
}

void Window::_InitTextureImageView()
{
	_textureImageView = _CreateImageView(_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void Window::_DeInitTextureImageView()
{
	vkDestroyImageView(_renderer->GetVulkanDevice(), _textureImageView, nullptr);
}

void Window::_InitTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	ErrorCheck(vkCreateSampler(_renderer->GetVulkanDevice(), &samplerInfo, nullptr, &_textureSampler));
}

void Window::_DeInitTextureSampler()
{
	vkDestroySampler(_renderer->GetVulkanDevice(), _textureSampler, nullptr);
}

void Window::_LoadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);

		}
	}
}

void Window::_InitVertexBuffers()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	
	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory);

	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _vertexBuffer, _vertexBufferMemory);

	_CopyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

	vkDestroyBuffer(_renderer->GetVulkanDevice(), stagingBuffer, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, nullptr);
}

void Window::_DeInitVertexBuffers()
{
	vkDestroyBuffer(_renderer->GetVulkanDevice(), _vertexBuffer, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), _vertexBufferMemory, nullptr);
}

void Window::_InitIndexBuffers()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(_renderer->GetVulkanDevice(), stagingBufferMemory);

	_CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory);

	_CopyBuffer(stagingBuffer, _indexBuffer, bufferSize);

	vkDestroyBuffer(_renderer->GetVulkanDevice(), stagingBuffer, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), stagingBufferMemory, nullptr);
}

void Window::_DeInitIndexBuffers()
{
	vkDestroyBuffer(_renderer->GetVulkanDevice(), _indexBuffer, nullptr);
	vkFreeMemory(_renderer->GetVulkanDevice(), _indexBufferMemory, nullptr);
}

void Window::_InitUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	_uniformBuffers.resize(_swapchainImages.size());
	_uniformBuffersMemory.resize(_swapchainImages.size());

	for (size_t i = 0; i < _swapchainImages.size(); i++) {
		_CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffers[i], _uniformBuffersMemory[i]);
	}
}

void Window::_DeInitUniformBuffers()
{
	for (size_t i = 0; i < _swapchainImages.size(); i++) {
		vkDestroyBuffer(_renderer->GetVulkanDevice(), _uniformBuffers[i], nullptr);
		vkFreeMemory(_renderer->GetVulkanDevice(), _uniformBuffersMemory[i], nullptr);
	}
}

void Window::_UpdateUniformBuffers(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.proj = glm::perspective(glm::radians(45.0f), _surface_size_x / (float)_surface_size_y, 0.1f, 10.0f);

	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(_renderer->GetVulkanDevice(), _uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(_renderer->GetVulkanDevice(), _uniformBuffersMemory[currentImage]);
}

void Window::_InitDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(_swapchainImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(_swapchainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(_swapchainImages.size());

	ErrorCheck(vkCreateDescriptorPool(_renderer->GetVulkanDevice(), &poolInfo, nullptr, &_descriptorPool));
}

void Window::_DeInitDescriptorPool()
{
	vkDestroyDescriptorPool(_renderer->GetVulkanDevice(), _descriptorPool, nullptr);
}

void Window::_InitDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(_swapchainImages.size(), _descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(_swapchainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(_swapchainImages.size());
	ErrorCheck(vkAllocateDescriptorSets(_renderer->GetVulkanDevice(), &allocInfo, descriptorSets.data()));

	for (size_t i = 0; i < _swapchainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = _uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = _textureImageView;
		imageInfo.sampler = _textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;


		vkUpdateDescriptorSets(_renderer->GetVulkanDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Window::_DeInitDescriptorSets()
{
}

void Window::_InitCommandBuffers()
{
	_commandBuffers.resize(_framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

	ErrorCheck(vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &allocInfo, _commandBuffers.data()));

	for (size_t i = 0; i < _commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		ErrorCheck(vkBeginCommandBuffer(_commandBuffers[i], &beginInfo));

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color.float32[0] = 0.0f;
		clearValues[0].color.float32[1] = 0.0f;
		clearValues[0].color.float32[2] = 1.0f;
		clearValues[0].color.float32[3] = 1.0f;
		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[1].depthStencil.stencil = 0;

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _renderPass;
		renderPassInfo.framebuffer = _framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = {_surface_size_x,_surface_size_y};
		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

		VkBuffer vertexBuffers[] = { _vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
		vkCmdDrawIndexed(_commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(_commandBuffers[i]);

		ErrorCheck(vkEndCommandBuffer(_commandBuffers[i]));
	}

}

void Window::_DeInitCommandBuffers()
{
	vkFreeCommandBuffers(_renderer->GetVulkanDevice(), _commandPool, _commandBuffers.size(), _commandBuffers.data());
}

void Window::_InitSyncObjects()
{
	_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]));
		ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]));
		ErrorCheck(vkCreateFence(_renderer->GetVulkanDevice(), &fenceInfo, nullptr, &_inFlightFences[i]));
	}
}

void Window::_DeInitSyncObjects()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(_renderer->GetVulkanDevice(), _imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(_renderer->GetVulkanDevice(), _renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(_renderer->GetVulkanDevice(), _inFlightFences[i], nullptr);
	}
}

void Window::_CleanUpOldSwapChain()
{
	_DeInitCommandBuffers();
	_DeInitDescriptorPool();
	_DeInitUniformBuffers();
	//_DeInitVertexBuffers();
	_DeInitFramebuffers();
	_DeInitDepthStencilImage();
	_DeInitColorResources();
	_DeInitGraphicsPipeline();
	_DeInitRenderPass();
	_DeInitSwapchainImages();
	_DeInitSwapchain();
	_DeInitSurface();

	//_DeInitOSWindow();
}

void Window::_ReInitSwapChain()
{
	vkDeviceWaitIdle(_renderer->GetVulkanDevice());
	while (_surface_size_x == 0 || _surface_size_y == 0)
	{
		_GetWindowSize();
		_WaitForEvents();
	}
	_CleanUpOldSwapChain();

	_InitSurface();
	_InitSwapchain();
	_InitSwapchainImages();
	_InitRenderPass();
	_InitGraphicsPipeline();
	_InitColorResources();
	_InitDepthStencilImage();
	_InitFramebuffers();
	//_InitVertexBuffers();
	_InitUniformBuffers();
	_InitDescriptorPool();
	_InitDescriptorSets();
	_InitCommandBuffers();

}

std::vector<char> Window::readFile(const std::string & filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

VkShaderModule Window::_CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_renderer->GetVulkanDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void Window::_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(_renderer->GetVulkanDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(_renderer->GetVulkanDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(&_renderer->GetVulkanPhysicalDeviceMemoryProperties(), &memRequirements, properties);

	if (vkAllocateMemory(_renderer->GetVulkanDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(_renderer->GetVulkanDevice(), buffer, bufferMemory, 0);
}

void Window::_CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	_EndSingleTimeCommands(commandBuffer);
}

void Window::_CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = numSamples;
	imageInfo.flags = 0; // Optional
	ErrorCheck(vkCreateImage(_renderer->GetVulkanDevice(), &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_renderer->GetVulkanDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(&_renderer->GetVulkanPhysicalDeviceMemoryProperties(), &memRequirements, properties);

	ErrorCheck(vkAllocateMemory(_renderer->GetVulkanDevice(), &allocInfo, nullptr, &imageMemory));

	vkBindImageMemory(_renderer->GetVulkanDevice(), image, imageMemory, 0);

}

VkCommandBuffer Window::_BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_renderer->GetVulkanDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Window::_EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_renderer->GetVulkanQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_renderer->GetVulkanQueue());

	vkFreeCommandBuffers(_renderer->GetVulkanDevice(), _commandPool, 1, &commandBuffer);
}

void Window::_TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (_HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	_EndSingleTimeCommands(commandBuffer);
}

void Window::_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	_EndSingleTimeCommands(commandBuffer);
}

VkImageView Window::_CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	/*viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;*/
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	ErrorCheck(vkCreateImageView(_renderer->GetVulkanDevice(), &viewInfo, nullptr, &imageView));

	return imageView;
}

bool Window::_HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Window::_GenerateMipMaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_renderer->GetVulkanPhysicalDevice(), imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	_EndSingleTimeCommands(commandBuffer);
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

