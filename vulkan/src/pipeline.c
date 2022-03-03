#include <stdlib.h>
#include <vulkan/vulkan.h>

#include "pipeline.h"
#include "vkdata.h"
#include "shader.h"
#include "buffers.h"
#include "cube.h"
#include "utils.h"
#include "vkdebug.h"

void createDescriptorSetLayout(VulkanData* vkData) {
    VkResult vkr;

    VkDescriptorSetLayoutBinding uboLayoutBinding = { 0 };
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = NULL;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = { 0 };
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = NULL;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {
        uboLayoutBinding, samplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { 0 };
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 2;
    descriptorSetLayoutCreateInfo.pBindings = bindings;

    if ((vkr = vkCreateDescriptorSetLayout(vkData->device, &descriptorSetLayoutCreateInfo, NULL, &vkData->descriptorSetLayout)) != VK_SUCCESS) {
       LOG3DHW("Failed creating descriptor set layout (result: %s)!", mapVkResultToString(vkr));
       exit(-1);
    }
}

void createGraphicsPipeline(VulkanData* vkData) {
    VkResult vkr;
    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;
    createShaderModules(vkData, &vertexShaderModule, &fragmentShaderModule);

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = { 0 };
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageCreateInfo.module = vertexShaderModule;
    vertexShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = { 0 };
    fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageCreateInfo.module = fragmentShaderModule;
    fragmentShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo
    };

    VkVertexInputBindingDescription bindingDescription = { 0 };
    bindingDescription.binding = 0;
    bindingDescription.stride = VERTEX_OFFSET * sizeof(float);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexAttributeDescPosition = { 0 };
    vertexAttributeDescPosition.binding = 0;
    vertexAttributeDescPosition.location = 0;
    vertexAttributeDescPosition.offset = 0;
    vertexAttributeDescPosition.format = VK_FORMAT_R32G32B32_SFLOAT;

    VkVertexInputAttributeDescription vertexAttributeDescTexCoords = { 0 };
    vertexAttributeDescTexCoords.binding = 0;
    vertexAttributeDescTexCoords.location = 1;
    vertexAttributeDescTexCoords.offset = TEX_COORDS_OFFSET * sizeof(float);
    vertexAttributeDescTexCoords.format = VK_FORMAT_R32G32_SFLOAT;

    VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
        vertexAttributeDescPosition, vertexAttributeDescTexCoords
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { 0 };
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = { 0 };
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = { 0 };
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float) vkData->extent.width;
    viewport.height = (float) vkData->extent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkOffset2D scissorOffset = { .x = 0, .y = 0 };
    VkRect2D scissor = { 0 };
    scissor.offset = scissorOffset;
    scissor.extent = vkData->extent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { 0 };
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = { 0 };
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = { 0 };
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = { 0 };
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { 0 };
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { 0 };
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 1;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { 0 };
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &vkData->descriptorSetLayout;

    VkPipelineLayout pipelineLayout;
    if ((vkr = vkCreatePipelineLayout(vkData->device, &pipelineLayoutCreateInfo, NULL, &pipelineLayout)) != VK_SUCCESS) {
        LOG3DHW("[pipeline] Failed creating pipeline layout (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }    

    LOG3DHW("[pipeline] Created pipeline layout");

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = { 0 };
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = NULL;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = NULL;
    graphicsPipelineCreateInfo.layout = pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = vkData->renderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    if ((vkr != vkCreateGraphicsPipelines(vkData->device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &pipeline)) != VK_SUCCESS) {
        LOG3DHW("[pipeline] Failed creating graphics pipeline (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[pipeline] Created pipeline");

    vkData->pipelineLayout = pipelineLayout;
    vkData->pipeline = pipeline;

    vkDestroyShaderModule(vkData->device, fragmentShaderModule, NULL);
    vkDestroyShaderModule(vkData->device, vertexShaderModule, NULL);
}

void createRenderPass(VulkanData* vkData) {
    VkResult vkr;

    VkAttachmentDescription colorAttachment = { 0 };
    colorAttachment.format = vkData->surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference = { 0 };
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    VkSubpassDependency subpassDependency = { 0 };
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = { 0 };
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkRenderPass renderPass;
    if ((vkr = vkCreateRenderPass(vkData->device, &renderPassCreateInfo, NULL, &renderPass)) != VK_SUCCESS) {
        LOG3DHW("[pipeline] Failed creating render pass (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[pipeline] Created render pass");

    vkData->renderPass = renderPass;
}

void createCommandBuffers(VulkanData* vkData) {
    VkResult vkr;

    vkData->commandBuffers = (VkCommandBuffer*) malloc(vkData->imageCount * sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = { 0 };
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = vkData->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = vkData->imageCount;
    if ((vkr = vkAllocateCommandBuffers(vkData->device, &commandBufferAllocateInfo, vkData->commandBuffers)) != VK_SUCCESS) {
        LOG3DHW("[pipeline] Failed allocating command buffers (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        VkCommandBufferBeginInfo commandBufferBeginInfo = { 0 };
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if ((vkr = vkBeginCommandBuffer(vkData->commandBuffers[i], &commandBufferBeginInfo)) != VK_SUCCESS) {
            LOG3DHW("[pipeline] Failed beginning command buffer (result: %s)!", mapVkResultToString(vkr));
            exit(-1);
        }

        VkClearValue clearValue = { 0 };
        VkClearColorValue clearColorValue = { { (99.f / 255.f), (139.f / 255.f), (235.f / 255.f), 1.f } };
        clearValue.color = clearColorValue;

        VkOffset2D renderAreaOffset = { 0, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo = { 0 };
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.framebuffer = vkData->framebuffers[i];
        renderPassBeginInfo.renderPass = vkData->renderPass;
        renderPassBeginInfo.renderArea.extent = vkData->extent;
        renderPassBeginInfo.renderArea.offset = renderAreaOffset;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(vkData->commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(vkData->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkData->pipeline);
        VkBuffer vertexBuffers[] = {
            vkData->vertexBuffer
        };
        VkDeviceSize bufferOffsets[] = { 0 };
        vkCmdBindVertexBuffers(vkData->commandBuffers[i], 0, 1, vertexBuffers, bufferOffsets);

        vkCmdBindDescriptorSets(vkData->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkData->pipelineLayout, 0, 1, &vkData->descriptorSets[i], 0, NULL);

        vkCmdDraw(vkData->commandBuffers[i], 36, 1, 0, 0);
        vkCmdEndRenderPass(vkData->commandBuffers[i]);

        if ((vkr = vkEndCommandBuffer(vkData->commandBuffers[i])) != VK_SUCCESS) {
            LOG3DHW("[pipeline] Failed ending command buffer (result: %s)!", mapVkResultToString(vkr));
            exit(-1);
        }
    }
}

void createCommandPool(VulkanData* vkData) {
    VkResult vkr;

    VkCommandPoolCreateInfo commandPoolCreateInfo = { 0 };
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = vkData->graphicsQueueFamilyIndex;
    commandPoolCreateInfo.flags = 0;
    if ((vkr = vkCreateCommandPool(vkData->device, &commandPoolCreateInfo, NULL, &vkData->commandPool) != VK_SUCCESS)) {
        LOG3DHW("[pipeline] Failed creating command pool (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }
}

void createDescriptorPool(VulkanData* vkData) {
    VkResult vkr;

    VkDescriptorPoolSize descriptorPoolSize = { 0 };
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize.descriptorCount = vkData->imageCount;

    VkDescriptorPoolSize samplerDescriptorPoolSize = { 0 };
    samplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;;
    samplerDescriptorPoolSize.descriptorCount = vkData->imageCount;

    VkDescriptorPoolSize descriptorPoolSizes[] = {
        descriptorPoolSize, samplerDescriptorPoolSize
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = { 0 };
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.poolSizeCount = 2;
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
    descriptorPoolCreateInfo.maxSets = vkData->imageCount;
    if ((vkr = vkCreateDescriptorPool(vkData->device, &descriptorPoolCreateInfo, NULL, &vkData->descriptorPool)) != VK_SUCCESS) {
        LOG3DHW("[pipeline] Failed creating descriptor pool (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }
}

void createDescriptorSets(VulkanData* vkData) {
    VkResult vkr;

    VkDescriptorSetLayout* descriptorSetLayouts = (VkDescriptorSetLayout*) malloc(vkData->imageCount * sizeof(VkDescriptorSetLayout));
    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        descriptorSetLayouts[i] = vkData->descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo descSetsAllocateInfo = { 0 };
    descSetsAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descSetsAllocateInfo.descriptorPool = vkData->descriptorPool;
    descSetsAllocateInfo.descriptorSetCount = vkData->imageCount;
    descSetsAllocateInfo.pSetLayouts = descriptorSetLayouts;
   
    vkData->descriptorSets = (VkDescriptorSet*) malloc(vkData->imageCount * sizeof(VkDescriptorSet));
    if ((vkr = vkAllocateDescriptorSets(vkData->device, &descSetsAllocateInfo, vkData->descriptorSets)) != VK_SUCCESS) {
        LOG3DHW("[pipeline] Failed allocating descriptor sets (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    for (uint32_t i = 0; i < vkData->imageCount; i++) {
        VkDescriptorBufferInfo descriptorBufferInfo = { 0 };
        descriptorBufferInfo.buffer = vkData->uniformBuffers[i];
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo descriptorImageInfo = {0};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = vkData->textureImageView;
        descriptorImageInfo.sampler = vkData->textureSampler;

        VkWriteDescriptorSet writeDescriptorSet = { 0 };
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = vkData->descriptorSets[i];
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

        VkWriteDescriptorSet samplerWriteDescriptorSet = { 0 };
        samplerWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerWriteDescriptorSet.dstSet = vkData->descriptorSets[i];
        samplerWriteDescriptorSet.dstBinding = 1;
        samplerWriteDescriptorSet.dstArrayElement = 0;
        samplerWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerWriteDescriptorSet.descriptorCount = 1;
        samplerWriteDescriptorSet.pImageInfo = &descriptorImageInfo; 

        VkWriteDescriptorSet writeDescriptorSets[] = {
            writeDescriptorSet, samplerWriteDescriptorSet
        };
       
        vkUpdateDescriptorSets(vkData->device, 2, writeDescriptorSets, 0, NULL);
    }

    free(descriptorSetLayouts);
}
