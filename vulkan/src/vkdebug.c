#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "vkdebug.h"
#include "vkdata.h"
#include "utils.h"

void prepareDebug(VkDebugUtilsMessengerCreateInfoEXT createInfo, VulkanData* vkData) {
    VkResult vkr;

    // Debug utils messenger is part of Vulkan extension, so we need to get function pointers fo create/destroy functions
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = 
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkData->instance, "vkCreateDebugUtilsMessengerEXT");

    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    if ((vkr = vkCreateDebugUtilsMessengerEXT(vkData->instance, &createInfo, NULL, &debugUtilsMessenger)) != VK_SUCCESS) {
        LOG3DHW("[vkdebug] Failed creating DebugUtilsMessenger (result: %s)", mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[vkdebug] Created DebugUtilsMessenger");

    vkData->debugUtilsMessenger = debugUtilsMessenger;
}

void destroyDebug(VulkanData* vkData) {
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = 
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkData->instance, "vkDestroyDebugUtilsMessengerEXT");

    if (vkData->debugUtilsMessenger != NULL) {
        vkDestroyDebugUtilsMessengerEXT(vkData->instance, vkData->debugUtilsMessenger, NULL);

        LOG3DHW("[vkdebug] Destroyed DebugUtilsMessenger");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback3DHW(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    char severityStr[8] = { '\0' };
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        strncpy(severityStr, "ERROR", 6);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        strncpy(severityStr, "WARN", 5);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        strncpy(severityStr, "INFO", 5);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        strncpy(severityStr, "VERBOSE", 8);
    } else {
        strncpy(severityStr, "UNKNOWN", 8);
    }

    char typeStr[16] = { '\0' };
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        strncpy(typeStr, "VALIDATION", 11);
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        strncpy(typeStr, "GENERAL", 8);
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        strncpy(typeStr, "PERF", 5);
    } else {
        strncpy(typeStr, "UNKNOWN", 8);
    }

    LOG3DHW("[vkdebug] [%s] [%s] %s", severityStr, typeStr, pCallbackData->pMessage);

    return VK_FALSE;
}