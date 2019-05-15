#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "Renderer.h"
#include <conio.h>
#include "Window.h"
#include <vulkan/vulkan.h>
#include "Shared.h"

VkCommandPool createCommandPool(VkDevice device, uint32_t familyIndex)
{
	VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	createInfo.queueFamilyIndex = familyIndex;

	VkCommandPool commandPool = 0;
	ErrorCheck(vkCreateCommandPool(device, &createInfo, 0, &commandPool));

	return commandPool;
}

VkSemaphore createSemaphore(VkDevice device)
{
	VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore semaphore = 0;
	ErrorCheck(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

	return semaphore;
}

int main()
{
	Renderer r;
	Window* window = r.OpenWindow(800, 600, "Test");

	/*VkSemaphore acquireSemaphore = createSemaphore(r.GetVulkanDevice());
	assert(acquireSemaphore);

	VkSemaphore releaseSemaphore = createSemaphore(r.GetVulkanDevice());
	assert(releaseSemaphore);


	VkCommandPool commandPool = createCommandPool(r.GetVulkanDevice(), r.GetVulkanGraphicsQueueFamilyIndex());
	assert(commandPool);

	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = 0;
	ErrorCheck(vkAllocateCommandBuffers(r.GetVulkanDevice(), &allocateInfo, &commandBuffer));*/

	while (r.Run())
	{
		window->DrawFrame();
		/*
		uint32_t imageIndex = 0;
		for (uint32_t imageIndex = 0; imageIndex < window->GetSwapchainImagesCount(); imageIndex++)
		{
			ErrorCheck(vkAcquireNextImageKHR(r.GetVulkanDevice(), window->GetSwapchain(), ~0ull, acquireSemaphore, VK_NULL_HANDLE, &imageIndex));

			ErrorCheck(vkResetCommandPool(r.GetVulkanDevice(), commandPool, 0));

			VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			ErrorCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo));

			VkClearColorValue color = { 1, 0, 1, 1 };

			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.levelCount = 1;
			range.layerCount = 1;

			std::vector<VkImage> image = window->GetSwapchainImages();
			vkCmdClearColorImage(commandBuffer, image[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);

			ErrorCheck(vkEndCommandBuffer(commandBuffer));

			VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &acquireSemaphore;
			submitInfo.pWaitDstStageMask = &submitStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &releaseSemaphore;

			vkQueueSubmit(r.GetVulkanQueue(), 1, &submitInfo, VK_NULL_HANDLE);

			VkSwapchainKHR swapchain = window->GetSwapchain();

			VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &releaseSemaphore;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain;
			presentInfo.pImageIndices = &imageIndex;

			ErrorCheck(vkQueuePresentKHR(r.GetVulkanQueue(), &presentInfo));

			ErrorCheck(vkDeviceWaitIdle(r.GetVulkanDevice()));
		}*/
		
	}

	vkDeviceWaitIdle(r.GetVulkanDevice());
	return 0;
}