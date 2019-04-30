#pragma once

#include <assert.h>
#include <iostream>
#include <vulkan/vulkan.h>
#include "BUILD_OPTIONS.h"

void ErrorCheck(VkResult result);

uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties * gpuMemoryProperties, const VkMemoryRequirements * memoryRequirements, const VkMemoryPropertyFlags memoryProperties);