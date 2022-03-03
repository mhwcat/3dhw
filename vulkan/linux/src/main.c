#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

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

static const int WINDOW_WIDTH = 1600;
static const int WINDOW_HEIGHT = 900;
static const char* WINDOW_TITLE = "3dhw-vulkan";
static const char* requiredInstanceExts[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};
static const char* validationLayerNames[] = {
    "VK_LAYER_KHRONOS_validation"
};
static const int MAX_FRAMES_IN_FLIGHT = 2;

int main(int argc, char** argv) {
    // Initializing threads may be required on some implementations for Xlib surface.
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap33.html#platformCreateSurface_xlib
    XInitThreads();

    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        printf("[main] Failed opening display!");
        exit(-1);
    }

    Screen* screen = XDefaultScreenOfDisplay(display); // we need screen dimensions
    Window root = DefaultRootWindow(display); // root window is desktop in this case - XCreateWindow needs it
    int defaultScreenIndex = XDefaultScreen(display);

    // In Vulkan, we cannot use GLX to select appropriate VisualInfo for us, so we match it to support 32-bit TrueColor
    XVisualInfo visualInfo;
    if (!XMatchVisualInfo(display, defaultScreenIndex, 32, TrueColor, &visualInfo)) {
        printf("[main] Failed choosing appropriate XVisualInfo!");
        exit(-1);
    }

    LOG3DHW("[main] Matched visual 0x%lx class %d (%s) with depth %d",
         visualInfo.visualid,
         visualInfo.class,
         visualInfo.class == TrueColor ? "TrueColor" : "unknown",
         visualInfo.depth);

    // Prepare final window attributes (colormap/visualinfo, event mask)
    const Colormap colormap = XCreateColormap(display, root, visualInfo.visual, AllocNone);
    XSetWindowAttributes setWindowAttributes = { 0 };
    setWindowAttributes.colormap = colormap;
    setWindowAttributes.event_mask = ExposureMask; // we need to draw to window
    setWindowAttributes.event_mask |= StructureNotifyMask; // we need to handle window resize events
    setWindowAttributes.background_pixel = 0;
    setWindowAttributes.border_pixel = 0;

    // Create window
    Window window = XCreateWindow(display, root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, visualInfo.depth, InputOutput, 
        visualInfo.visual, CWColormap | CWEventMask | CWBackPixel | CWBorderPixel, &setWindowAttributes);

    XMapWindow(display, window);
    XStoreName(display, window, WINDOW_TITLE);
    // Move window to center of screen (setting x, y in XCreateWindow does not work)
    XMoveWindow(display, window, (screen->width / 2) - (WINDOW_WIDTH / 2), 
        (screen->height / 2) - (WINDOW_HEIGHT / 2));        

    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    // This struct will be used to store window size (we'll update it on window resize event)
    XWindowAttributes windowAttributes;
    XGetWindowAttributes(display, window, &windowAttributes);

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
    createDeviceQueues(&vkData, display, &visualInfo, validationLayerNames, 1);
    createSurface(&vkData, display, window);
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
    float deltaTime = 0.f;
    struct timespec elapsedTime = { 0 };
    long timeDiffVal = 0L;

    UniformBufferObject uniform = { 0 };
    size_t currentFrame = 0;
    bool framebufferResized = false;

    XEvent xEvent;
    bool running = true;
    LOG3DHW("[main] Starting render loop...");
    while (running) {
        // Update delta time based on last time diff between renders
        struct timespec newTime; 
        clock_gettime(CLOCK_MONOTONIC_RAW, &newTime); // CLOCK_MONOTIC_RAW gives us access to raw hardware timer
        timeDiffVal = nanosecDiff(elapsedTime, newTime);
        elapsedTime = newTime;
        deltaTime = (float) timeDiffVal / 1.0e9f; // nanoseconds -> seconds

        // Handle X events first
        while (XPending(display)) {
            XNextEvent(display, &xEvent);

            // ConfigureNotify event is dispatched on window resize event (and other times too),
            // so we mark framebuffer as resized here.
            // Note: this is likely redundant because we'll get VK_ERROR_OUT_OF_DATE_KHR from vkQueuePresentKHR() anyway.
            if (xEvent.type == ConfigureNotify) {
                framebufferResized = true;
            // ClientMessage is dispatched on window close request, but we also have to check
            // if event data equals to atom defined earlier
            } else if (xEvent.type == ClientMessage) {      
                if (xEvent.xclient.data.l[0] == wmDeleteMessage) {
                    LOG3DHW("[main] Received exit event, quitting...");

                    running = false;

                    // We break here becasue we don't want to handle any more X events at this point.
                    // Not doing so results in "X connection broken" error on program shutdown.
                    break; 
                }
            }             
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
            mat4x4_perspective(uniform.proj, fov, ((float) windowAttributes.width / (float) windowAttributes.height), zNear, zFar);

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

                    XGetWindowAttributes(display, window, &windowAttributes);

                    vkDeviceWaitIdle(vkData.device);
                    cleanupSwapchain(&vkData);

                    createSwapchainAndImageViews(&vkData, windowAttributes.width, windowAttributes.height);
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

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    // Destroy Vulkan instance *after* window/display cleanup
    // https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/1894
    vkDestroyInstance(vkData.instance, NULL);
    LOG3DHW("[main] Destroyed instance")

    return EXIT_SUCCESS;
}