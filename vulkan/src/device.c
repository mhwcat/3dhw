#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.h>

#include "device.h"
#include "vkdata.h"
#include "utils.h"
#include "vkdebug.h"

#ifdef _WIN32
VkBool32 checkPresentationSupport(VulkanData* vkData, int queueFamilyIndex) {
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(vkData->physicalDevice, queueFamilyIndex);
}
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
VkBool32 checkPresentationSupport(VulkanData* vkData, int queueFamilyIndex, Display* display, XVisualInfo* visualInfo) {
    return vkGetPhysicalDeviceXlibPresentationSupportKHR(vkData->physicalDevice, queueFamilyIndex, display, 
        XVisualIDFromVisual(visualInfo->visual));
}
#endif

void pickPhysicalDevice(VulkanData* vkData) {
    uint32_t physicalDevicesCount = 0;
    vkEnumeratePhysicalDevices(vkData->instance, &physicalDevicesCount, NULL);
    if (physicalDevicesCount == 0) {
        LOG3DHW("[device] No physical devices found!");
        exit(-1);
    }

    VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*) malloc(physicalDevicesCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(vkData->instance, &physicalDevicesCount, physicalDevices);

    int suitableDeviceIndex = -1;
    for (uint32_t i = 0; i < physicalDevicesCount; i++) {
        VkPhysicalDeviceProperties props = { 0 };
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

        VkPhysicalDeviceFeatures features = { 0 };
        vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

        uint32_t deviceExtensionsCount;
        vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &deviceExtensionsCount, NULL);
        bool swapchainSupported = false;
        if (deviceExtensionsCount > 0) {
            VkExtensionProperties* deviceExtensionProps = (VkExtensionProperties*) malloc(deviceExtensionsCount * sizeof(VkExtensionProperties));
            vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &deviceExtensionsCount, deviceExtensionProps);

            for (uint32_t i = 0; i < deviceExtensionsCount; i++) {
                if (strncmp(requiredDeviceExts[0], deviceExtensionProps[i].extensionName, strlen(requiredDeviceExts[0])) == 0) {
                    swapchainSupported = true;
                }
            }

            free(deviceExtensionProps);
        }

        LOG3DHW("[device] Found physical device %d: %s", i, props.deviceName);

        // This is really simplified physical device selection condition:
        // we only check if device is discrete GPU and has swapchain support.
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && swapchainSupported) {
            suitableDeviceIndex = i;
            break;
        }
    }

    if (suitableDeviceIndex < 0) {
        LOG3DHW("[device] No suitable physical device found!");
        exit(-1);
    }

    LOG3DHW("[device] Physical device %d selected", suitableDeviceIndex);

    vkData->physicalDevice = physicalDevices[suitableDeviceIndex];

    free(physicalDevices);
}

#ifdef _WIN32
void createDeviceQueues(VulkanData* vkData, const char* validationLayerNames[], int validationLayerCount) {
#else
void createDeviceQueues(VulkanData* vkData, Display* display, XVisualInfo* visualInfo, const char* validationLayerNames[], int validationLayerCount) {
#endif
    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties queueFamilyProps = { 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(vkData->physicalDevice, &queueFamilyCount, NULL);
    if (queueFamilyCount == 0) {
        LOG3DHW("[device] No queue families found!");
        exit(-1);
    }

    LOG3DHW("[device] Found %d queue families", queueFamilyCount);
    
    VkQueueFamilyProperties* queueFamilyProperties = (VkQueueFamilyProperties*) malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(vkData->physicalDevice, &queueFamilyCount, queueFamilyProperties);

    // Queue family picking for graphics and presentation queues. 
    // Usually queue family index for both of these will be the same, but we want to properly check it. 
    // Additionally, if queue families are separate, we have to set correct imageSharingMode when creating swapchain later.
    int graphicsQueueFamilyIndex = -1;
    int presentQueueFamilyIndex = -1;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        // Select graphics queue family index based on VK_QUEUE_GRAPHICS_BIT present in queueFlags.
        if ((graphicsQueueFamilyIndex < 0) && (queueFamilyProperties->queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            graphicsQueueFamilyIndex = i;
        }

        // Select presentation queue family index based on presentation support (checking is platform-dependent) 
#ifdef _WIN32
        VkBool32 supportsPresentation = checkPresentationSupport(vkData, i);
#else
        VkBool32 supportsPresentation = checkPresentationSupport(vkData, i, display, visualInfo);
#endif
        if ((presentQueueFamilyIndex < 0) && supportsPresentation) {
            presentQueueFamilyIndex = i;
        }
    }   
    
    if (graphicsQueueFamilyIndex < 0 || presentQueueFamilyIndex < 0) {
        LOG3DHW("[device] No suitable queue families for graphics and/or presentation found!");
        exit(-1);        
    }

    LOG3DHW("[device] Selected queue family indices: Graphics: %d, Presentation: %d", graphicsQueueFamilyIndex, presentQueueFamilyIndex);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = { 0 };
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
    deviceQueueCreateInfo.flags = 0;

    // We currently don't need any physical device features (like geometry shader support)
    VkPhysicalDeviceFeatures physicalDeviceFeatures = { 0 };

    // Fill logical device create info, passing information of device queues, validation layers, and device extensions,
    // then create logical device.
    VkDeviceCreateInfo deviceCreateInfo = { 0 };
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
    deviceCreateInfo.enabledLayerCount = validationLayerCount;
    deviceCreateInfo.ppEnabledLayerNames = validationLayerNames;
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExts;
    deviceCreateInfo.enabledExtensionCount = 1;

    VkDevice device;
    VkResult vkr;
    if ((vkr = vkCreateDevice(vkData->physicalDevice, &deviceCreateInfo, NULL, &device)) != VK_SUCCESS) {
        LOG3DHW("[device] Failed creating logical device (result: %s)!", mapVkResultToString(vkr));
        exit(-1);  
    }

    LOG3DHW("[device] Created logical device");

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);

    vkData->graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
    vkData->presentQueueFamilyIndex = presentQueueFamilyIndex;
    vkData->graphicsQueue = graphicsQueue;
    vkData->presentQueue = presentQueue;
    vkData->device = device;

    free(queueFamilyProperties);
}

void createSynchronizationPrimitives(VulkanData* vkData) {
    VkResult vkr;

    VkSemaphoreCreateInfo semaphoreCreateInfo = { 0 };
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = { 0 };
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkData->imageAvailableSemaphores = (VkSemaphore*) malloc(vkData->maxFramesInFlight * sizeof(VkSemaphore));
    vkData->renderFinishedSemaphores = (VkSemaphore*) malloc(vkData->maxFramesInFlight * sizeof(VkSemaphore));
    vkData->inFlightFences = (VkFence*) malloc(vkData->maxFramesInFlight * sizeof(VkFence));
    vkData->imagesInFlight = (VkFence*) malloc(vkData->imageCount * sizeof(VkFence));
    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        vkData->imagesInFlight[i] = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < vkData->maxFramesInFlight; i++) {
        if ((vkr = vkCreateSemaphore(vkData->device, &semaphoreCreateInfo, NULL, &vkData->imageAvailableSemaphores[i])) != VK_SUCCESS) {
            LOG3DHW("[device] Failed creating semaphore (result: %s)!", mapVkResultToString(vkr));
            exit(-1);
        }

        if ((vkr = vkCreateSemaphore(vkData->device, &semaphoreCreateInfo, NULL, &vkData->renderFinishedSemaphores[i])) != VK_SUCCESS) {
            LOG3DHW("[device] Failed creating semaphore (result: %s)!", mapVkResultToString(vkr));
            exit(-1);
        }

        if ((vkr = vkCreateFence(vkData->device, &fenceCreateInfo, NULL, &vkData->inFlightFences[i])) != VK_SUCCESS) {
            LOG3DHW("[device] Failed creating fence (result: %s)!", mapVkResultToString(vkr));
            exit(-1);
        }
    }

    LOG3DHW("[device] Created synchronization primitives");   
}

// Utility method for finding appropriate memory for given filter and properties
uint32_t findMemoryIndex(VulkanData* vkData, uint32_t typeFilter, VkMemoryPropertyFlags requiredMemPropertyFlags) {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemProperties;
    vkGetPhysicalDeviceMemoryProperties(vkData->physicalDevice, &physicalDeviceMemProperties);

    int suitableMemoryIndex = -1;
    for (uint32_t i = 0; i < physicalDeviceMemProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && ((physicalDeviceMemProperties.memoryTypes[i].propertyFlags & requiredMemPropertyFlags) == requiredMemPropertyFlags)) {
            suitableMemoryIndex = i;
            break;
        }
    }

    if (suitableMemoryIndex < 0) {
        LOG3DHW("[device] Suitable memory not found!");
        exit(-1);
    }

    return (uint32_t) suitableMemoryIndex;
}