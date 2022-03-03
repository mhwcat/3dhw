#pragma once

#include <vulkan/vulkan.h>

#include "vkdata.h"

typedef struct UniformBufferObject {
    float model[4][4];
    float view[4][4];
    float proj[4][4];
} UniformBufferObject;

void createUniformBuffers(VulkanData* vkData);

void createBuffer(VulkanData* vkData, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);

void destroyBuffer(VulkanData* vkData, VkBuffer buffer, VkDeviceMemory bufferMemory);
