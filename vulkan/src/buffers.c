#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "buffers.h"
#include "device.h"
#include "vkdata.h"
#include "utils.h"
#include "vkdebug.h"

// Map used buffer types to string (mapping is incomplete!)
static inline const char* bufferUsageEnumToString(VkBufferUsageFlags usage) {
    return usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ? "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT" :
        usage == VK_BUFFER_USAGE_INDEX_BUFFER_BIT ? "VK_BUFFER_USAGE_INDEX_BUFFER_BIT" :
        usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ? "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT" :
        usage == VK_BUFFER_USAGE_TRANSFER_SRC_BIT ? "VK_BUFFER_USAGE_TRANSFER_SRC_BIT" :
        "UNKNOWN";
}

void createUniformBuffers(VulkanData* vkData) {
    vkData->uniformBuffers = (VkBuffer*) malloc(vkData->imageCount * sizeof(VkBuffer));
    vkData->uniformBufferMemories = (VkDeviceMemory *) malloc(vkData->imageCount * sizeof(VkDeviceMemory));

    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        createBuffer(vkData, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &vkData->uniformBuffers[i], &vkData->uniformBufferMemories[i]);
    }
}

void createBuffer(VulkanData* vkData, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
    VkResult vkr;
    const char* usageStr = bufferUsageEnumToString(usage);
    
    VkBufferCreateInfo bufferCreateInfo = { 0 };
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if ((vkr = vkCreateBuffer(vkData->device, &bufferCreateInfo, NULL, buffer)) != VK_SUCCESS) {
        LOG3DHW("[buffers] Failed creating buffer for %s (result: %s)!", usageStr, mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[buffers] Created buffer for %s", usageStr);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkData->device, *buffer, &memRequirements);
    uint32_t suitableMemoryIndex = findMemoryIndex(vkData, memRequirements.memoryTypeBits, memoryProperties);

    LOG3DHW("[buffers] Selected memory %d for buffer %s", suitableMemoryIndex, usageStr);

    VkMemoryAllocateInfo bufferMemoryAllocateInfo = { 0 };
    bufferMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    bufferMemoryAllocateInfo.allocationSize = memRequirements.size;
    bufferMemoryAllocateInfo.memoryTypeIndex = suitableMemoryIndex;
    if ((vkr = vkAllocateMemory(vkData->device, &bufferMemoryAllocateInfo, NULL, bufferMemory)) != VK_SUCCESS) {
        LOG3DHW("[buffers] Failed allocating memory for buffer %s (result: %s)!", usageStr, mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[buffers] Allocated memory for buffer %s", usageStr);

    if ((vkr = vkBindBufferMemory(vkData->device, *buffer, *bufferMemory, 0)) != VK_SUCCESS) {
        LOG3DHW("[buffers] Failed binding memory for buffer %s (result: %s)!", usageStr, mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[buffers] Bound memory for buffer %s", usageStr);
}

void destroyBuffer(VulkanData* vkData, VkBuffer buffer, VkDeviceMemory bufferMemory) {
    vkDestroyBuffer(vkData->device, buffer, NULL);
    vkFreeMemory(vkData->device, bufferMemory, NULL);
}