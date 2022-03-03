#include <stdlib.h>
#include <vulkan/vulkan.h>

#include "vkdata.h"
#include "swapchain.h"
#include "vkdebug.h"
#include "buffers.h"
#include "utils.h"

void cleanup(VulkanData* vkData) {
    cleanupSwapchain(vkData);

    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        destroyBuffer(vkData, vkData->uniformBuffers[i], vkData->uniformBufferMemories[i]);
    }
    LOG3DHW("[vkdata] Destroyed uniform buffers and memories");

    vkDestroyDescriptorPool(vkData->device, vkData->descriptorPool, NULL);
    LOG3DHW("[vkdata] Destroyed descriptor pool");

    vkDestroySampler(vkData->device, vkData->textureSampler, NULL);
    vkDestroyImageView(vkData->device, vkData->textureImageView, NULL);
    vkDestroyImage(vkData->device, vkData->textureImage, NULL);
    vkFreeMemory(vkData->device, vkData->textureImageMemory, NULL);
    LOG3DHW("[vkdata] Destroyed texture sampler, image view and image");

    vkDestroyDescriptorSetLayout(vkData->device, vkData->descriptorSetLayout, NULL);
    LOG3DHW("[vkdata] Destroyed descriptor set layouts");
    
    destroyBuffer(vkData, vkData->vertexBuffer, vkData->vertexBufferMemory);
    LOG3DHW("[vkdata] Destroyed vertex buffer");

    for (uint32_t i = 0; i < vkData->maxFramesInFlight; i++) {
        vkDestroyFence(vkData->device, vkData->inFlightFences[i], NULL);
        vkDestroySemaphore(vkData->device, vkData->renderFinishedSemaphores[i], NULL);
        vkDestroySemaphore(vkData->device, vkData->imageAvailableSemaphores[i], NULL);
    }
    LOG3DHW("[vkdata] Destroyed semaphores and fences");

    vkDestroyCommandPool(vkData->device, vkData->commandPool, NULL);
    LOG3DHW("[vkdata] Destroyed command pool");

    vkDestroyDevice(vkData->device, NULL);
    LOG3DHW("[vkdata] Destroyed logical device");

    vkDestroySurfaceKHR(vkData->instance, vkData->surface, NULL);
    LOG3DHW("[vkdata] Destroyed surface");

    destroyDebug(vkData);

    free(vkData->inFlightFences);
    free(vkData->renderFinishedSemaphores);
    free(vkData->imageAvailableSemaphores);
    free(vkData->imagesInFlight);
    free(vkData->fragmentShaderBytes);
    free(vkData->vertexShaderBytes);
    free(vkData->uniformBuffers);
    free(vkData->uniformBufferMemories);
    free(vkData->descriptorSets);
}