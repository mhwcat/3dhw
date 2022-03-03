#pragma once

#include "vkdata.h"

#ifdef _WIN32
#include <Windows.h>
void createSurface(VulkanData* vkData, HINSTANCE hInstance, HWND hwnd);
#else
#include <X11/Xlib.h>
void createSurface(VulkanData* vkData, Display* display, Window window);
#endif

void createSwapchainAndImageViews(VulkanData* vkData, uint32_t width, uint32_t height);

void createFramebuffers(VulkanData* vkData);

void cleanupSwapchain(VulkanData* vkData);