#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "texture.h"
#include "device.h"
#include "vkdata.h"
#include "utils.h"
#include "buffers.h"
#include "vkdebug.h"

static VkCommandBuffer beginCommandBuffer(VulkanData* vkData) {
    VkResult vkr;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = { 0 };
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = vkData->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if ((vkr = vkAllocateCommandBuffers(vkData->device, &commandBufferAllocateInfo, &commandBuffer)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed allocating command buffers (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = { 0 };
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if ((vkr = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed beginning command buffer (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    return commandBuffer;
}

static void endCommandBuffer(VulkanData* vkData, VkCommandBuffer commandBuffer) {
    VkResult vkr;

    if ((vkr = vkEndCommandBuffer(commandBuffer)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed ending command buffer (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if ((vkr = vkQueueSubmit(vkData->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed submiting to graphics queue (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    vkQueueWaitIdle(vkData->graphicsQueue);

    vkFreeCommandBuffers(vkData->device, vkData->commandPool, 1, &commandBuffer);
}

static void copyBuffer(VulkanData* vkData, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginCommandBuffer(vkData);

    VkBufferCopy copyRegion = { 0 };
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endCommandBuffer(vkData, commandBuffer);
}

static void transitionImageLayout(VulkanData* vkData, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginCommandBuffer(vkData);

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    VkImageMemoryBarrier imageMemoryBarrier = { 0 };
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        LOG3DHW("[texture] Unsupported layout transition!");
        exit(-1);
    }

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);

    endCommandBuffer(vkData, commandBuffer);
}

static void copyBufferToImage(VulkanData* vkData, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginCommandBuffer(vkData);

    VkOffset3D imageOffset = { 0, 0, 0 };
    VkExtent3D imageExtent = { width, height, 1 };
    VkBufferImageCopy region = { 0 };
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = imageOffset;
    region.imageExtent = imageExtent;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endCommandBuffer(vkData, commandBuffer);
}

void copyDataToBuffer(VulkanData* vkData, VkDeviceMemory bufferMemory, VkDeviceSize size, const void* dataToCopy) {
    VkResult vkr;

    void* bufferData;
    if ((vkr = vkMapMemory(vkData->device, bufferMemory, 0, size, 0, &bufferData)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed mapping memory for buffer (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    memcpy(bufferData, dataToCopy, size);
    vkUnmapMemory(vkData->device, bufferMemory);
}

void createTextureImage(VulkanData* vkData, const char* textureFilename) {
    VkResult vkr;

    int width, height, channels;
    stbi_uc* image = stbi_load(textureFilename, &width, &height, &channels, STBI_rgb_alpha);
    if (!image) {
        LOG3DHW("[texture] Failed loading %s texture!", textureFilename);
        exit(-1);
    }

    VkDeviceSize textureSize = width * height * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(vkData, textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
    copyDataToBuffer(vkData, stagingBufferMemory, textureSize, image);

    stbi_image_free(image);

    VkImageCreateInfo imageCreateInfo = { 0 };
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = 1;
    if ((vkr = vkCreateImage(vkData->device, &imageCreateInfo, NULL, &vkData->textureImage)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed creating texture image (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    VkMemoryRequirements imageMemReqs;
    vkGetImageMemoryRequirements(vkData->device, vkData->textureImage, &imageMemReqs);
    uint32_t suitableMemoryIndex = findMemoryIndex(vkData, imageMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo imageMemAllocInfo = { 0 };
    imageMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    imageMemAllocInfo.allocationSize = imageMemReqs.size;
    imageMemAllocInfo.memoryTypeIndex = suitableMemoryIndex;
    
    if ((vkr = vkAllocateMemory(vkData->device, &imageMemAllocInfo, NULL, &vkData->textureImageMemory)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed allocating memory for image (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    if ((vkr = vkBindImageMemory(vkData->device, vkData->textureImage, vkData->textureImageMemory, 0)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed binding memory for image (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    transitionImageLayout(vkData, vkData->textureImage, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(vkData, stagingBuffer, vkData->textureImage, width, height);

    transitionImageLayout(vkData, vkData->textureImage, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(vkData->device, stagingBuffer, NULL);
    vkFreeMemory(vkData->device, stagingBufferMemory, NULL);
}

void createTextureImageView(VulkanData* vkData) {
    VkResult vkr;
    
    VkImageViewCreateInfo imageViewCreateInfo = { 0 };
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = vkData->textureImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    if ((vkr = vkCreateImageView(vkData->device, &imageViewCreateInfo, NULL, &vkData->textureImageView)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed creating texture image view (result: %s)!", mapVkResultToString(vkr));
        exit(-1); 
    }
}

void createTextureImageSampler(VulkanData* vkData) {
    VkResult vkr;

    VkSamplerCreateInfo samplerCreateInfo = { 0 };
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.mipLodBias = 0.f;
    samplerCreateInfo.minLod = 0.f;
    samplerCreateInfo.maxLod = 0.f;

    if ((vkr = vkCreateSampler(vkData->device, &samplerCreateInfo, NULL, &vkData->textureSampler)) != VK_SUCCESS) {
        LOG3DHW("[texture] Failed creating texture sampler (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }
}