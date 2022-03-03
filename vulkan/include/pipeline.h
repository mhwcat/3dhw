#pragma once

#include "vkdata.h"

void createDescriptorSetLayout(VulkanData* vkData);

void createGraphicsPipeline(VulkanData* vkData);

void createRenderPass(VulkanData* vkData);

void createCommandBuffers(VulkanData* vkData);

void createCommandPool(VulkanData* vkData);

void createDescriptorPool(VulkanData* vkData);

void createDescriptorSets(VulkanData* vkData);