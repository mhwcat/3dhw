#include <stdio.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define WIN32_LEAN_AND_MEAN // https://devblogs.microsoft.com/oldnewthing/20091130-00/?p=15863
#include <Windows.h>

// Third party libs
#include "glad/wgl.h"
#include "glad/gl.h"
#include "linmath.h"

// Internal stuff
#include "gldebug.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "utils.h"

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;
const TCHAR* WINDOW_TITLE = "3dhw-opengl";

typedef struct WindowData {
    HWND windowHandle;
    HDC deviceContextHandle;
    HGLRC glContext;
    unsigned int currentWidth;
    unsigned int currentHeight;
} WindowData;

static bool running = false;

void initGlContext(HINSTANCE hInstance, WindowData* windowData);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void printLastError(const TCHAR* message);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
    WindowData windowData = { 0 };
    initGlContext(hInstance, &windowData);
    ShowWindow(windowData.windowHandle, nShowCmd);

    SetWindowLongPtr(windowData.windowHandle, GWLP_USERDATA, &windowData);

    // Enable OpenGL debug and map the output to callback in gldebug.h
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallbackFunction, NULL);

    // Enable backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT); // set initial viewport size

    // Load and compile shader program
    GLuint shaderProgramId = loadAndLinkShaderProgram();

    // Load cube vertices data to GPU
    GLuint meshVao = loadCubeMesh();

    // Load texture
    GLuint textureId = loadTexture();

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

    // Message loop handling
    MSG msg = { 0 };
    running = true;
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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor((99.f / 255.f), (139.f / 255.f), (235.f / 255.f), 1.f);

        // Preparing model matrix
        mat4x4_identity(modelMat); // model matrix have to be identity matrix initially
        mat4x4_translate(modelMat, 0.f, 0.f, -5.f); // apply translation to model matrix; move cube to the back, to be in front of camera
        mat4x4_rotate(modelMat, modelMat, 0.7f, 0.2f, -0.8f, rotationAngle); // apply in-place rotation to model matrix
        rotationAngle += (rotationSpeedRadians * deltaTime);

        // Preparing perspective matrix
        mat4x4_perspective(perspectiveMat, fov, ((float) windowData.currentWidth / (float) windowData.currentHeight), zNear, zFar);

        // Preparing view (world-to-camera) matrix
        mat4x4_look_at(viewMat, cameraPos, front, up);

        // Setting uniforms in shader program (we have to only pass pointer to first element of matrix)
        glUseProgram(shaderProgramId);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "projection"), 1, false, &perspectiveMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "view"), 1, false, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramId, "model"), 1, false, &modelMat[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(shaderProgramId, "texture_diffuse1"), 0);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Drawing cube
        glBindVertexArray(meshVao);
        glDrawArrays(GL_TRIANGLES, 0, 36); // our cube have 108 vertices -> 36 triangles
        glBindVertexArray(0);

        SwapBuffers(windowData.deviceContextHandle);

        // Uncomment this to see deltaTime in action - consistent cube rotation regardless of FPS.
        // This simulates frame time of ~100ms (10FPS) 
        // Sleep(100);
    }

    return 0;
}

void initGlContext(HINSTANCE hInstance, WindowData* windowData) {
    // Create and register window class (https://docs.microsoft.com/en-us/windows/win32/winmsg/about-window-classes)
    // We're using same window class for our fake and real window
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

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa?redirectedfrom=MSDN
    HWND fakeWindowHandle = CreateWindowEx(
        0, // extended window style: https://docs.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
        windowClassName,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW, // window style: https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
        CW_USEDEFAULT, CW_USEDEFAULT, // use default position
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, // parent window (null in our case)
        NULL, // menu (null in our case)
        hInstance, // instance handle
        NULL
    );
    if (fakeWindowHandle == NULL) {
        printLastError("[window-win32] Failed creating fake window handle!");
        exit(-1);
    }

    // In order to get latest OpenGL context, we have to first create dummy legacy context and then load GLAD WGL
    // Setting pixel format descriptor: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-pixelformatdescriptor
    PIXELFORMATDESCRIPTOR pixelFormatDesc = { 0 };
    pixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixelFormatDesc.nVersion = 1;
    pixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; // we want to have OpenGL support with vsync
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
    pixelFormatDesc.iLayerType = 0; // this is ignored on current GL versions
    pixelFormatDesc.bReserved = 0;
    pixelFormatDesc.dwLayerMask = 0; // this is ignored on current GL versions
    pixelFormatDesc.dwVisibleMask = 0;
    pixelFormatDesc.dwDamageMask = 0; // this is ignored on current GL versions

    HDC fakeDeviceContextHandle = GetDC(fakeWindowHandle);

    int pixelFormat = ChoosePixelFormat(fakeDeviceContextHandle, &pixelFormatDesc);
    if (!SetPixelFormat(fakeDeviceContextHandle, pixelFormat, &pixelFormatDesc)) {
        printLastError("[window-win32] Failed setting pixel format for fake window!");
        exit(-1);
    }

    HGLRC fakeGlContext = wglCreateContext(fakeDeviceContextHandle);
    if (!wglMakeCurrent(fakeDeviceContextHandle, fakeGlContext)) {
        printLastError("[window-win32] Failed setting current (fake) GL context!");
        exit(-1);
    }

    // At this point we have fake GL context as current, so we can load WGL from GLAD
    if (!gladLoaderLoadWGL(fakeDeviceContextHandle)) {
        LOG3DHW("[window-win32] Failed loading WGL!");
        exit(-1);
    }

    // Prepare window size ajdusted for title bar, borders etc.
    RECT finalWindowSize = { 0 };
    finalWindowSize.right = WINDOW_WIDTH;
    finalWindowSize.bottom = WINDOW_HEIGHT;
    AdjustWindowRect(&finalWindowSize, WS_OVERLAPPEDWINDOW, false);

    // Now we can request modern GL context, but before that we have to create window... again :)
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

    HDC deviceContextHandle = GetDC(windowHandle);

    const int pixelAttribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0
    };

    int pixelFormatId;
    int numFormats;
    bool status = wglChoosePixelFormatARB(deviceContextHandle, pixelAttribs, NULL, 1, &pixelFormatId, &numFormats);
    if (status == false || numFormats == 0) {
        LOG3DHW("[window-win32] Failed choosing pixel format for window!");
        exit(-1);
    }

    PIXELFORMATDESCRIPTOR pfd = { 0 };
    DescribePixelFormat(deviceContextHandle, pixelFormatId, sizeof(pfd), &pfd);
    if (!SetPixelFormat(deviceContextHandle, pixelFormatId, &pfd)) {
        printLastError("[window-win32] Failed setting pixel format for window!");
        exit(-1);
    }

    int attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB,
        WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        0
    };

    // We can now create modern GL context
    HGLRC glContext = wglCreateContextAttribsARB(deviceContextHandle, NULL, attributes);

    // Remove temporary GL context, device context handle and window
    wglMakeCurrent(NULL, NULL); 
    wglDeleteContext(fakeGlContext); 
    ReleaseDC(fakeWindowHandle, fakeDeviceContextHandle);
    DestroyWindow(fakeWindowHandle);
    
    // Make "true" GL context a current one
    if (!wglMakeCurrent(deviceContextHandle, glContext)) {
        LOG3DHW("[window-win32] Failed setting current GL context!");
        exit(-1);
    }

    // Having real GL context as current one, we can safely load GL from GLAD
    if (!gladLoaderLoadGL()) {
        LOG3DHW("[window-win32] Failed loading GL!");
        exit(-1);
    }

    // Print GL version
    LOG3DHW("[window-win32] OpenGL version: %s", glGetString(GL_VERSION));

    windowData->windowHandle = windowHandle;
    windowData->deviceContextHandle = deviceContextHandle;
    windowData->glContext = glContext;
    windowData->currentWidth = finalWindowSize.right - finalWindowSize.left;
    windowData->currentHeight = finalWindowSize.bottom - finalWindowSize.top;
}

// Handling window events (open, destroy, repaint etc.)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WindowData* windowData = GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
        // On window destroy we want to destroy GL context and post quit message
        case WM_DESTROY:
            LOG3DHW("[window-win32] Received WM_DESTROY message, quitting...");

            if (running) {
                running = false;
            }

            // We have to null-check here, because WM_DESTROY event will be sent during GL context init (when destroying fake window)
            // and we don't have all information here yet.
            if (windowData != NULL) {
                if (windowData->glContext) {
                    wglMakeCurrent(NULL, NULL);
                    wglDeleteContext(windowData->glContext);
                }
                if (windowData->deviceContextHandle) {
                    ReleaseDC(hwnd, windowData->deviceContextHandle);
                }
            }

            PostQuitMessage(0);

            return 0;
        // On window resize we want to resize GL viewport and save new dimensions to our data struct.
        // We don't have to query window for current size; new size is passed in lParam on low (width) and high (height) parts.
        case WM_SIZE:
            unsigned int newWidth = LOWORD(lParam);
            unsigned int newHeight = HIWORD(lParam);

            if (windowData != NULL) {
                windowData->currentWidth = newWidth;
                windowData->currentHeight = newHeight;

                glViewport(0, 0, newWidth, newHeight);
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