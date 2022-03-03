#pragma once

#include <vulkan/vulkan.h>

#include "vkdata.h"

void copyDataToBuffer(VulkanData* vkData, VkDeviceMemory bufferMemory, VkDeviceSize size, const void* dataToCopy);

void createTextureImage(VulkanData* vkData, const char* textureFilename);

void createTextureImageView(VulkanData* vkData);

void createTextureImageSampler(VulkanData* vkData);