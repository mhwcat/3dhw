#pragma once

#include <vulkan/vulkan.h>

#include "vkdata.h"

void loadShaderFromFile(const char* filename, char** shaderBytes, size_t* shaderBytesLen);

void createShaderModules(VulkanData *vkData, VkShaderModule *vertexShaderModule, VkShaderModule *fragmentShaderModule);
