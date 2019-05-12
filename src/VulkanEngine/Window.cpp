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
	_InitGraphicsPipeline();
	_InitFramebuffers();
	_InitCommandPool();
	_InitCommandBuffers();
	_InitSemaphore();
}

Window::~Window()
{
	_DeInitSemaphore();
	_DeInitCommandBuffers();
	_DeInitCommandPool();
	_DeInitFramebuffers();
	_DeInitGraphicsPipeline();
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

void Window::DrawFrame()
{
	uint32_t imageIndex;
	vkAcquireNextImageKHR(_renderer->GetVulkanDevice(), _swapchain, UINT64_MAX, _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];
	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	ErrorCheck(vkQueueSubmit(_renderer->GetVulkanQueue(), 1, &submitInfo, VK_NULL_HANDLE));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(_renderer->GetVulkanQueue(), &presentInfo);

	vkQueueWaitIdle(_renderer->GetVulkanQueue());

	
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

void Window::_InitFramebuffers()
{
	_framebuffers.resize(_swapchainImageCount);
	for (uint32_t i = 0; i < _swapchainImageCount; i++)
	{
		std::array<VkImageView, 2> attachments{};
		attachments[0] = _depthStencilImageView;
		attachments[1] = _swapchainImageViews[i];

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

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; 
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; 

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
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(_renderer->GetVulkanDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilInfo{};
	pipelineDepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilInfo.stencilTestEnable = VK_FALSE;

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

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _renderPass;
		renderPassInfo.framebuffer = _framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = {_surface_size_x,_surface_size_y};
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
		vkCmdDraw(_commandBuffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(_commandBuffers[i]);

		ErrorCheck(vkEndCommandBuffer(_commandBuffers[i]));
	}

}

void Window::_DeInitCommandBuffers()
{

}

void Window::_InitSemaphore()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphore));
	ErrorCheck(vkCreateSemaphore(_renderer->GetVulkanDevice(), &semaphoreInfo, nullptr, &_renderFinishedSemaphore));
}

void Window::_DeInitSemaphore()
{
	vkDestroySemaphore(_renderer->GetVulkanDevice(), _imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(_renderer->GetVulkanDevice(), _renderFinishedSemaphore, nullptr);
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

