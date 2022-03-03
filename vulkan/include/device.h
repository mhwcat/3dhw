#pragma once

#include <vulkan/vulkan.h>

#include "vkdata.h"

static const char* requiredDeviceExts[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef _WIN32
VkBool32 checkPresentationSupport(VulkanData* vkData, int queueFamilyIndex);
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
VkBool32 checkPresentationSupport(VulkanData* vkData, int queueFamilyIndex, Display* display, XVisualInfo* visualInfo);
#endif

void pickPhysicalDevice(VulkanData* vkData);

#ifdef _WIN32
void createDeviceQueues(VulkanData* vkData, const char* validationLayerNames[], int validationLayerCount);
#else
#include <X11/Xlib.h>
void createDeviceQueues(VulkanData* vkData, Display* display, XVisualInfo* visualInfo, const char* validationLayerNames[], int validationLayerCount);
#endif

void createSynchronizationPrimitives(VulkanData* vkData);

uint32_t findMemoryIndex(VulkanData* vkData, uint32_t typeFilter, VkMemoryPropertyFlags requiredMemPropertyFlags);