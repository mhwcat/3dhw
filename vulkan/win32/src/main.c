#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define WIN32_LEAN_AND_MEAN // https://devblogs.microsoft.com/oldnewthing/20091130-00/?p=15863
#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

// Third party libs
#include "linmath.h"

// Internal stuff
#include "vkdata.h"
#include "vkdebug.h"
#include "device.h"
#include "swapchain.h"
#include "shader.h"
#include "buffers.h"
#include "utils.h"
#include "cube.h"
#include "pipeline.h"
#include "texture.h"
#include "linmath.h"

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;
const TCHAR* WINDOW_TITLE = "3dhw-vulkan";

typedef struct WindowData {
    HWND windowHandle;
    HDC deviceContextHandle;
    uint32_t currentWidth;
    uint32_t currentHeight;

} WindowData;

static const char* requiredInstanceExts[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};
static const char* validationLayerNames[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const int MAX_FRAMES_IN_FLIGHT = 2;
static bool running = false;
static bool framebufferResized = false;

void initWindow(WindowData* windowData, HINSTANCE hInstance);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void printLastError(const TCHAR* message);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
    WindowData windowData = { 0 };
    initWindow(&windowData, hInstance);
    ShowWindow(windowData.windowHandle, nShowCmd);
    // Pass pointer to WindowData to window, so we can use it alter in WindowProc()
    SetWindowLongPtr(windowData.windowHandle, GWLP_USERDATA, &windowData);

    VulkanData vkData = { 0 };
    VkResult vkr; // global var for holding VkResults

    // Prepare application info
    VkApplicationInfo appInfo = { 0 };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = WINDOW_TITLE;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "n/a";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // If we want to debug VkInstance creation, we have to prepare VkDebugUtilsMessengerCreateInfoEXT 
    // in advance and pass it to pNext of VkInstanceCreateInfo. 
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = { 0 };
    debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // Uncomment VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT to enable verbose logging
    debugUtilsMessengerCreateInfo.messageSeverity =
        /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerCreateInfo.pfnUserCallback = vkDebugCallback3DHW;
    debugUtilsMessengerCreateInfo.pUserData = NULL;

    // Setup Vulkan instance create info with instance extensions and validation layers we want.
    VkInstanceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 3;
    createInfo.ppEnabledExtensionNames = requiredInstanceExts;
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayerNames;
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugUtilsMessengerCreateInfo;

    VkInstance instance;
    if ((vkr = vkCreateInstance(&createInfo, NULL, &instance)) != VK_SUCCESS) {
        LOG3DHW("[main] Failed creating VkInstance (result: %s)!", mapVkResultToString(vkr));
        exit(-1);
    }

    LOG3DHW("[main] Created Vulkan instance");
    vkData.instance = instance;

    prepareDebug(debugUtilsMessengerCreateInfo, &vkData);
    pickPhysicalDevice(&vkData);
    createDeviceQueues(&vkData, validationLayerNames, 1);
    createSurface(&vkData, hInstance, windowData.windowHandle);
    createSwapchainAndImageViews(&vkData, WINDOW_WIDTH, WINDOW_HEIGHT);
    createRenderPass(&vkData);
    createDescriptorSetLayout(&vkData);
    loadShaderFromFile("vert.spv", &vkData.vertexShaderBytes, &vkData.vertexShaderLength);
    loadShaderFromFile("frag.spv", &vkData.fragmentShaderBytes, &vkData.fragmentShaderLength);
    createGraphicsPipeline(&vkData);
    createFramebuffers(&vkData);
    createBuffer(&vkData, sizeof(CUBE_DATA), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &vkData.vertexBuffer, &vkData.vertexBufferMemory); // Create vertex buffer & copy vertices
    copyDataToBuffer(&vkData, vkData.vertexBufferMemory, sizeof(CUBE_DATA), (const float*) CUBE_DATA);
    createUniformBuffers(&vkData);
    createCommandPool(&vkData);
    createTextureImage(&vkData, "assets/texture.jpg");
    createTextureImageView(&vkData);
    createTextureImageSampler(&vkData);
    createDescriptorPool(&vkData);
    createDescriptorSets(&vkData);
    createCommandBuffers(&vkData);

    vkData.maxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
    createSynchronizationPrimitives(&vkData);

    // Cube rotation vars
    float rotationAngle = 0.f;
    const float rotationSpeedRadians = (45.f * ((float) M_PI / 180.f)); // rotate by 45 degree / s

    // GL matrices setup
    float modelMat[4][4] = { { 0.f } };
    float perspectiveMat[4][4] = { { 0.f } };
    float viewMat[4][4] = { { 0.f } };

    // Camera vectors setup
    const float cameraPos[] = { 0.f, 0.f, 0.f }; // we position our camera at [0, 0, 0] in world space
    const float front[] = { 0.f, 0.f, -1.f }; // front is where camera is looking, in our case -Z
    const float up[] = { 0.f, 1.f, 0.f }; // up is where camera "sees" top, in our case +Y
    const float fov = 45.f; // camera field of view
    const float zNear = 0.01f; // near plane
    const float zFar = 1000.f; // far plane

    // Delta time between consecutive renders, used as "smoothing" value for rotation
    // Normally it would be applied to all movement on screen. 
    // Even better solution would be to use fixed timestep (especially if there is any physics 
    // simulation involved), which is not implemented here.
    // ref: https://gafferongames.com/post/fix_your_timestep/
    float deltaTime = 1.f;
    LARGE_INTEGER elapsedTime = { 0 };
    LONGLONG timeDiffVal = 0LL;

    UniformBufferObject uniform = { 0 };

    // Message loop handling
    MSG msg = { 0 };
    running = true;
    size_t currentFrame = 0;
    while (running) {
        LARGE_INTEGER newTime, freq;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&newTime);
        timeDiffVal = (newTime.QuadPart - elapsedTime.QuadPart);
        // Elapsed time * nanoseconds per tick / 1.0e9 nanoseconds per second
        deltaTime = ((float) timeDiffVal * (1.0e9f / (float) freq.QuadPart)) / 1.0e9f;
        elapsedTime = newTime;

        while (PeekMessage(&msg, windowData.windowHandle, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Drawing begins here
        if (running) {
            // Preparing model matrix
            mat4x4_identity(uniform.model); // model matrix have to be identity matrix initially
            mat4x4_translate(uniform.model, 0.f, 0.f, -5.f); // apply translation to model matrix; move cube to the back, to be in front of camera
            // apply in-place rotation to model matrix (-rotationAngle, because of flipped Vulkan coordinates compared to OpenGL)
            mat4x4_rotate(uniform.model, uniform.model, 0.7f, 0.2f, -0.8f, -rotationAngle);
            rotationAngle += (rotationSpeedRadians * deltaTime);

            // Preparing perspective matrix
            mat4x4_perspective(uniform.proj, fov, ((float) windowData.currentWidth / (float) windowData.currentHeight), zNear, zFar);

            // Preparing view (world-to-camera) matrix
            mat4x4_look_at(uniform.view, cameraPos, front, up);

            if ((vkr = vkWaitForFences(vkData.device, 1, &vkData.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX)) != VK_SUCCESS) {
                LOG3DHW("[main] Failed waiting for fences (result: %s)!", mapVkResultToString(vkr));
                exit(-1);
            }

            uint32_t imageIndex;
            if ((vkr = vkAcquireNextImageKHR(vkData.device, vkData.swapchain, UINT64_MAX, vkData.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex)) != VK_SUCCESS) {
                LOG3DHW("[main] Failed acquiring next image (result: %s)!", mapVkResultToString(vkr));
                exit(-1);
            }

            if (vkData.imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
                if ((vkr = vkWaitForFences(vkData.device, 1, &vkData.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX)) != VK_SUCCESS) {
                    LOG3DHW("[main] Failed waiting for fences (result: %s)!", mapVkResultToString(vkr));
                    exit(-1);
                }
            }
            vkData.imagesInFlight[imageIndex] = vkData.inFlightFences[currentFrame];

            VkSemaphore waitSemaphores[] = {
                vkData.imageAvailableSemaphores[currentFrame]
            };
            VkSemaphore signalSemapshores[] = {
                vkData.renderFinishedSemaphores[currentFrame]
            };
            VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            };

            // Copy model, view, projection matrices to uniform buffer
            copyDataToBuffer(&vkData, vkData.uniformBufferMemories[imageIndex], sizeof(uniform), &uniform);

            VkSubmitInfo submitInfo = { 0 };
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &vkData.commandBuffers[imageIndex];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemapshores;

            if ((vkr = vkResetFences(vkData.device, 1, &vkData.inFlightFences[currentFrame])) != VK_SUCCESS) {
                LOG3DHW("[main] Failed resetting fences (result: %s)!", mapVkResultToString(vkr));
                exit(-1);
            }

            if ((vkr = vkQueueSubmit(vkData.graphicsQueue, 1, &submitInfo, vkData.inFlightFences[currentFrame])) != VK_SUCCESS) {
                LOG3DHW("[main] Failed submitting to queue (result: %s)!", mapVkResultToString(vkr));
                exit(-1);
            }

            VkSwapchainKHR swapchains[] = {
                vkData.swapchain
            };

            VkPresentInfoKHR presentInfo = { 0 };
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemapshores;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &imageIndex;

            if ((vkr = vkQueuePresentKHR(vkData.presentQueue, &presentInfo)) != VK_SUCCESS) {
                LOG3DHW("[main] Failed presenting to queue (result: %s)!", mapVkResultToString(vkr));

                // Swapchain is no longer valid (probably due to window resize) - recreate
                if (vkr == VK_ERROR_OUT_OF_DATE_KHR || vkr == VK_SUBOPTIMAL_KHR || framebufferResized) {
                    framebufferResized = false;

                    vkDeviceWaitIdle(vkData.device);
                    cleanupSwapchain(&vkData);

                    createSwapchainAndImageViews(&vkData, windowData.currentWidth, windowData.currentHeight);
                    createRenderPass(&vkData);
                    createGraphicsPipeline(&vkData);
                    createFramebuffers(&vkData);
                    createCommandBuffers(&vkData);
                }
            }

            currentFrame = (currentFrame + 1) % vkData.maxFramesInFlight;
        }
    }

    vkDeviceWaitIdle(vkData.device);

    cleanup(&vkData);

    // Destroy Vulkan instance *after* window/display cleanup
    // https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/1894
    vkDestroyInstance(vkData.instance, NULL);
    LOG3DHW("[main] Destroyed instance")

    return EXIT_SUCCESS;
}

void initWindow(WindowData* windowData, HINSTANCE hInstance) {
    // Create and register window class (https://docs.microsoft.com/en-us/windows/win32/winmsg/about-window-classes)
    // We're using same window class for our window
    const TCHAR* windowClassName = "3dhw-window-class";
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
    windowClass.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    windowClass.hbrBackground = NULL; // setting background color will cause our render to disappear during window resizing
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = windowClassName;
    if (!RegisterClassEx(&windowClass)) {
        printLastError("[window-win32] Failed creating window class!");
        exit(-1);
    }

    // Prepare window size ajdusted for title bar, borders etc.
    RECT finalWindowSize = { 0 };
    finalWindowSize.right = WINDOW_WIDTH;
    finalWindowSize.bottom = WINDOW_HEIGHT;
    AdjustWindowRect(&finalWindowSize, WS_OVERLAPPEDWINDOW, false);

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa?redirectedfrom=MSDN
    HWND windowHandle = CreateWindowEx(
        0, // extended window style: https://docs.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
        windowClassName,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW, // window style: https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
        CW_USEDEFAULT, CW_USEDEFAULT, // use default position
        finalWindowSize.right - finalWindowSize.left, finalWindowSize.bottom - finalWindowSize.top,
        NULL, // parent window (null in our case)
        NULL, // menu (null in our case)
        hInstance, // instance handle
        NULL
    );
    if (windowHandle == NULL) {
        printLastError("[window-win32] Failed creating window handle!");
        exit(-1);
    }

    // Setting pixel format descriptor: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-pixelformatdescriptor
    PIXELFORMATDESCRIPTOR pixelFormatDesc = { 0 };
    pixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixelFormatDesc.nVersion = 1;
    pixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER; // we want to have drawing support with vsync
    pixelFormatDesc.iPixelType = PFD_TYPE_RGBA;
    pixelFormatDesc.cColorBits = 32; // 32-bit color buffer
    pixelFormatDesc.cRedBits = 0;
    pixelFormatDesc.cRedShift = 0;
    pixelFormatDesc.cGreenBits = 0;
    pixelFormatDesc.cGreenShift = 0;
    pixelFormatDesc.cBlueBits = 0;
    pixelFormatDesc.cBlueShift = 0;
    pixelFormatDesc.cAlphaBits = 0;
    pixelFormatDesc.cAlphaShift = 0;
    pixelFormatDesc.cAccumBits = 0;
    pixelFormatDesc.cAccumRedBits = 0;
    pixelFormatDesc.cAccumGreenBits = 0;
    pixelFormatDesc.cAccumBlueBits = 0;
    pixelFormatDesc.cAccumAlphaBits = 0;
    pixelFormatDesc.cDepthBits = 24; // 24-bit depth buffer
    pixelFormatDesc.cStencilBits = 8; // 8-bit stencil buffer
    pixelFormatDesc.cAuxBuffers = 0; // aux buffers are not supported according to MSDN
    pixelFormatDesc.iLayerType = 0; 
    pixelFormatDesc.bReserved = 0;
    pixelFormatDesc.dwLayerMask = 0;
    pixelFormatDesc.dwVisibleMask = 0;
    pixelFormatDesc.dwDamageMask = 0; 

    HDC deviceContextHandle = GetDC(windowHandle);

    int pixelFormat = ChoosePixelFormat(deviceContextHandle, &pixelFormatDesc);
    if (!SetPixelFormat(deviceContextHandle, pixelFormat, &pixelFormatDesc)) {
        printLastError("[window-win32] Failed setting pixel format for window!");
        exit(-1);
    }

    //HDC deviceContextHandle = GetDC(windowHandle);

    windowData->windowHandle = windowHandle;
    windowData->deviceContextHandle = deviceContextHandle;
    windowData->currentWidth = finalWindowSize.right - finalWindowSize.left;
    windowData->currentHeight = finalWindowSize.bottom - finalWindowSize.top;
}

// Handling window events (open, destroy, repaint etc.)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WindowData* windowData = (WindowData*) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_DESTROY:
            LOG3DHW("[window-win32] Received WM_DESTROY message, quitting...");

            if (running) {
                running = false;
            }

            if (windowData->deviceContextHandle) {
                ReleaseDC(hwnd, windowData->deviceContextHandle);
            }

            PostQuitMessage(0);

            return 0;
        // On window resize we want to save new dimensions to our data struct.
        // We don't have to query window for current size; new size is passed in lParam on low (width) and high (height) parts.
        case WM_SIZE:
            unsigned int newWidth = LOWORD(lParam);
            unsigned int newHeight = HIWORD(lParam);

            if (windowData != NULL) {
                windowData->currentWidth = newWidth;
                windowData->currentHeight = newHeight;
            }
            
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void printLastError(const TCHAR* message) {
    TCHAR errorMessage[512] = { '\0' };
    DWORD lastError = GetLastError();

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, lastError, MAKELANGID(LANG_SYSTEM_DEFAULT, SUBLANG_DEFAULT),
        errorMessage, (sizeof(errorMessage) / sizeof(TCHAR)), NULL);

    LOG3DHW("%s Error: %s", message, errorMessage);
}