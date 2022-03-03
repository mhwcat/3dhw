#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <vulkan/vulkan.h>

#include "shader.h"
#include "utils.h"
#include "vkdebug.h"

void loadShaderFromFile(const char* filename, char** shaderBytes, size_t* shaderBytesLen) {
    // Open shader binary file
    FILE* shaderFile = fopen(filename, "rb");
    if (shaderFile == NULL) {
        LOG3DHW("[shader] Failed opening shader file (result: %s)!", strerror(errno));
        exit(-1);  
    }

    // Read file length
    fseek(shaderFile, 0, SEEK_END);         
    *shaderBytesLen = ftell(shaderFile);
    rewind(shaderFile);

    // Allocate and clear memory for shader binary
    *shaderBytes = (char *) malloc(*shaderBytesLen);
    memset(*shaderBytes, '\0', *shaderBytesLen);

    // Read shader binary and close the file
    fread(*shaderBytes, sizeof(char), *shaderBytesLen, shaderFile);
    fclose(shaderFile);

    LOG3DHW("[shader] Read %ld bytes from %s shader file", *shaderBytesLen, filename);
}

void createShaderModules(VulkanData *vkData, VkShaderModule *vertexShaderModule, VkShaderModule *fragmentShaderModule) {
    VkResult vkr;

    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = { 0 };
    vertexShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleCreateInfo.codeSize = vkData->vertexShaderLength;
    vertexShaderModuleCreateInfo.pCode = (const uint32_t *) vkData->vertexShaderBytes;
    if ((vkr = vkCreateShaderModule(vkData->device, &vertexShaderModuleCreateInfo, NULL, vertexShaderModule)) != VK_SUCCESS) {
        LOG3DHW("[shader] Failed creating vertex shader module (result: %s)!", mapVkResultToString(vkr));
        exit(-1);  
    }

    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = { 0 };
    fragmentShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleCreateInfo.codeSize = vkData->fragmentShaderLength;
    fragmentShaderModuleCreateInfo.pCode = (const uint32_t *) vkData->fragmentShaderBytes;
    if ((vkr = vkCreateShaderModule(vkData->device, &fragmentShaderModuleCreateInfo, NULL, fragmentShaderModule)) != VK_SUCCESS) {
        LOG3DHW("[shader] Failed creating fragment shader module (result: %s)!", mapVkResultToString(vkr));
        exit(-1);  
    }

    LOG3DHW("[shader] Created vertex and fragment shader modules")
}