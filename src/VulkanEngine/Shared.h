#pragma once

#include <assert.h>
#include <iostream>
#include <vulkan/vulkan.h>
#include "BUILD_OPTIONS.h"
#include <vector>
#include <fstream>

void ErrorCheck(VkResult result);

uint32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties * gpuMemoryProperties, const VkMemoryRequirements * memoryRequirements, const VkMemoryPropertyFlags memoryProperties);


