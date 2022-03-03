#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.h>

#include "swapchain.h"
#include "vkdata.h"
#include "buffers.h"
#include "utils.h"
#include "vkdebug.h"

inline static const char* presentModeEnumToString(VkPresentModeKHR presentMode) {
    return presentMode == VK_PRESENT_MODE_FIFO_KHR ? "VK_PRESENT_MODE_FIFO_KHR" : 
        presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR ? "VK_PRESENT_MODE_FIFO_RELAXED_KHR" :
        presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ? "VK_PRESENT_MODE_IMMEDIATE_KHR" :
        presentMode == VK_PRESENT_MODE_MAILBOX_KHR ? "VK_PRESENT_MODE_MAILBOX_KHR" :
        presentMode == VK_PRESENT_MODE_MAX_ENUM_KHR ? "VK_PRESENT_MODE_MAX_ENUM_KHR" :
        presentMode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR ? "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR" :
        presentMode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR ? "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR" : "UNKNOWN";
}

// Yeah, maybe later
// inline static const char* surfaceFormatEnumToString(VkSurfaceFormatKHR surfaceFormat) {
//    
// }

#ifdef _WIN32
void createSurface(VulkanData* vkData, HINSTANCE hInstance, HWND hwnd) {
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { 0 };
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = hInstance;
    surfaceCreateInfo.hwnd = hwnd;

    VkSurfaceKHR surface;
    VkResult vkr;
    if ((vkr = vkCreateWin32SurfaceKHR(vkData->instance, &surfaceCreateInfo, NULL, &surface)) != VK_SUCCESS) {
        LOG3DHW("[swapchain] Failed creating surface (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[swapchain] Created Win32 surface");

    vkData->surface = surface;
}
#else
void createSurface(VulkanData* vkData, Display* display, Window window) {
    VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = { 0 };
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.dpy = display;
    surfaceCreateInfo.window = window;

    VkSurfaceKHR surface;
    VkResult vkr;
    if ((vkr = vkCreateXlibSurfaceKHR(vkData->instance, &surfaceCreateInfo, NULL, &surface)) != VK_SUCCESS) {
        LOG3DHW("[swapchain] Failed creating Xlib surface (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[swapchain] Created Xlib surface");

    vkData->surface = surface;
}
#endif

void createSwapchainAndImageViews(VulkanData* vkData, uint32_t width, uint32_t height) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkData->physicalDevice, vkData->surface, &surfaceCapabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkData->physicalDevice, vkData->surface, &formatCount, NULL);
    VkSurfaceFormatKHR* surfaceFormats = (VkSurfaceFormatKHR*) malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkData->physicalDevice, vkData->surface, &formatCount, surfaceFormats);
    if (formatCount == 0) {
        LOG3DHW("[swapchain] No surface formats available!");
        exit(-1);
    }
    LOG3DHW("[swapchain] Found %d surface formats", formatCount);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkData->physicalDevice, vkData->surface, &presentModeCount, NULL);
    VkPresentModeKHR* presentModes = (VkPresentModeKHR*) malloc(presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkData->physicalDevice, vkData->surface, &presentModeCount, presentModes);
    if (presentModeCount == 0) {
        LOG3DHW("[swapchain] No present modes available!");
        exit(-1);
    }
    LOG3DHW("[swapchain] Found %d present modes", presentModeCount);

    VkSurfaceFormatKHR suitableSurfaceFormat;
    bool suitableSurfaceFormatFound = false;
    for (uint32_t i = 0; i < formatCount; i++) {
        if (surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
            suitableSurfaceFormat = surfaceFormats[i];
            suitableSurfaceFormatFound = true;
        }
    }
    if (!suitableSurfaceFormatFound) {
        LOG3DHW("[swapchain] No suitable surface format found!");
        exit(-1);
    }

    VkPresentModeKHR suitablePresentMode;
    for (uint32_t i = 0; i < presentModeCount; i++) {
        LOG3DHW("[swapchain] Found present mode: %s", presentModeEnumToString(presentModes[i]));
    }
    // Using FIFO, because it's guaranteed to be available and provides vsync
    suitablePresentMode = VK_PRESENT_MODE_FIFO_KHR;

    LOG3DHW("[swapchain] Selected present mode: %s", presentModeEnumToString(suitablePresentMode));

    uint32_t extentWidth, extentHeight;
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        extentWidth = surfaceCapabilities.currentExtent.width;
        extentHeight = surfaceCapabilities.currentExtent.height;
    } else {
        extentWidth = clampi(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extentHeight = clampi(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }
    VkExtent2D extent = {
        .width = extentWidth,
        .height = extentHeight
    };

    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    // If we're using separate queue families for graphics and presentation, we have to set concurrent image sharing mode
    VkSharingMode imageSharingMode = vkData->graphicsQueueFamilyIndex == vkData->presentQueueFamilyIndex ? 
        VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = { 0 };
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = vkData->surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = suitableSurfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = suitableSurfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = imageSharingMode;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = suitablePresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult vkr;
    VkSwapchainKHR swapchain;
    if ((vkr = vkCreateSwapchainKHR(vkData->device, &swapchainCreateInfo, NULL, &swapchain)) != VK_SUCCESS) {
        LOG3DHW("[swapchain] Failed creating swapchain (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[swapchain] Created swapchain");

    uint32_t swapchainImageCount;
    if ((vkr = vkGetSwapchainImagesKHR(vkData->device, swapchain, &swapchainImageCount, NULL)) != VK_SUCCESS) {
        LOG3DHW("[swapchain] Failed getting swapchain images (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }
    VkImage* swapchainImages = (VkImage*) malloc(swapchainImageCount * sizeof(VkImage));
    if ((vkr = vkGetSwapchainImagesKHR(vkData->device, swapchain, &swapchainImageCount, swapchainImages)) != VK_SUCCESS) {
        LOG3DHW("[swapchain] Failed getting swapchain images (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    VkImageView* imageViews = (VkImageView*) malloc(swapchainImageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VkImageViewCreateInfo imageViewCreateInfo = { 0 };
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = suitableSurfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if ((vkr = vkCreateImageView(vkData->device, &imageViewCreateInfo, NULL, &imageViews[i])) != VK_SUCCESS) {
            LOG3DHW("[swapchain] Failed creating image view %d (result: %s)!", i, mapVkResultToString(vkr));
            exit(-1);
        }

        LOG3DHW("[swapchain] Created swapchain image view %d", i);
    }

    vkData->swapchain = swapchain;
    vkData->images = swapchainImages;
    vkData->imageViews = imageViews;
    vkData->imageCount = swapchainImageCount;
    vkData->extent = extent;
    vkData->surfaceFormat = suitableSurfaceFormat;

    free(surfaceFormats);
    free(presentModes);
}

void createFramebuffers(VulkanData* vkData) {
    VkResult vkr;
    VkFramebuffer* framebuffers = (VkFramebuffer*) malloc(vkData->imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        VkImageView attachments = {
            vkData->imageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo = { 0 };
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = vkData->renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &attachments;
        framebufferCreateInfo.width = vkData->extent.width;
        framebufferCreateInfo.height = vkData->extent.height;
        framebufferCreateInfo.layers = 1;
        if ((vkr = vkCreateFramebuffer(vkData->device, &framebufferCreateInfo, NULL, &framebuffers[i])) != VK_SUCCESS) {
            LOG3DHW("[swapchain] Failed creating framebuffer %d (result: %s)", i, mapVkResultToString(vkr));
            exit(-1);
        }

        LOG3DHW("[swapchain] Created framebuffer %d", i);
    }

    vkData->framebuffers = framebuffers;
}

void cleanupSwapchain(VulkanData* vkData) {
    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        vkDestroyFramebuffer(vkData->device, vkData->framebuffers[i], NULL);
    }
    LOG3DHW("[swapchain] Destroyed framebuffers");

    vkFreeCommandBuffers(vkData->device, vkData->commandPool, vkData->imageCount, vkData->commandBuffers);
    LOG3DHW("[swapchain] Destroyed command buffers");

    vkDestroyPipeline(vkData->device, vkData->pipeline, NULL);
    LOG3DHW("[swapchain] Destroyed pipeline");

    vkDestroyPipelineLayout(vkData->device, vkData->pipelineLayout, NULL);
    LOG3DHW("[swapchain] Destroyed pipeline layout");

    vkDestroyRenderPass(vkData->device, vkData->renderPass, NULL);
    LOG3DHW("[swapchain] Destroyed render pass");

    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        vkDestroyImageView(vkData->device, vkData->imageViews[i], NULL);
    }
    LOG3DHW("[swapchain] Destroyed image views");

    vkDestroySwapchainKHR(vkData->device, vkData->swapchain, NULL);
    LOG3DHW("[swapchain] Destroyed swapchain");

    free(vkData->framebuffers);
    free(vkData->commandBuffers);
    free(vkData->imageViews);
    free(vkData->images);
}