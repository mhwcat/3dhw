#pragma once

#include <vulkan/vulkan.h>

typedef struct VulkanData {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;
    VkSwapchainKHR swapchain;
    uint32_t imageCount;
    VkImage* images;
    VkImageView* imageViews;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet* descriptorSets;
    VkRenderPass renderPass;
    VkPipeline pipeline;
    VkFramebuffer* framebuffers;
    VkCommandBuffer* commandBuffers;
    VkCommandPool commandPool;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer* uniformBuffers;
    VkDeviceMemory* uniformBufferMemories;
    VkDescriptorPool descriptorPool;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    // Synchornization primitives
    uint32_t maxFramesInFlight;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* inFlightFences;
    VkFence* imagesInFlight;

    // Shaders
    char* vertexShaderBytes;
    size_t vertexShaderLength;
    char* fragmentShaderBytes;
    size_t fragmentShaderLength;

} VulkanData;

void cleanup(VulkanData* vkData);