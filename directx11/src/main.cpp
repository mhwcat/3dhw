#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define WIN32_LEAN_AND_MEAN // https://devblogs.microsoft.com/oldnewthing/20091130-00/?p=15863
#include <Windows.h>
#include <stdint.h>
#include <d3d11.h>
#include <wrl.h>
#include <dxgi1_6.h>

// Third party libs
#include "linmath.h"

#include "utils.h"

//using namespace DirectX;
//using namespace DX;

using Microsoft::WRL::ComPtr;

const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;
const TCHAR* WINDOW_TITLE = "3dhw-dx11";

typedef struct WindowData {
    HWND windowHandle;
    HDC deviceContextHandle;
    uint32_t currentWidth;
    uint32_t currentHeight;

} WindowData;

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
    SetWindowLongPtr(windowData.windowHandle, GWLP_USERDATA, (LONG_PTR) &windowData);

    IDXGIAdapter1** ptrAdapter = nullptr;
    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory> factory;
    CreateDXGIFactory(IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
    ComPtr<IDXGIFactory6> factory6;
    factory.As(&factory6);

    for (UINT adapterIndex = 0; 
        SUCCEEDED(factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE::DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))); 
        adapterIndex++) 
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        LOG3DHW("Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
        break;
    }

    if (!adapter) {
        LOG3DHW("Suitable graphics adapter not found!");
        exit(-1);
    }

    //*ptrAdapter = adapter.Detach();

    UINT deviceCreationFlags = D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
    static const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0
    };

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL  selectedFeatureLevel;
    HRESULT hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN, nullptr, deviceCreationFlags,
        featureLevels, 1, D3D11_SDK_VERSION, device.GetAddressOf(), &selectedFeatureLevel, context.GetAddressOf());
    if (FAILED(hr)) {
        LOG3DHW("Failed creating D3D11 Device!");
        exit(-1);
    }

    LOG3DHW("Created D3D11 device");

    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug))) {
        ComPtr<ID3D11InfoQueue> infoQueue;
        if (SUCCEEDED(d3dDebug.As(&infoQueue))) {
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_ERROR, true);

            D3D11_MESSAGE_ID messagesToHide[] = {
                D3D11_MESSAGE_ID::D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = 1;
            filter.DenyList.pIDList = messagesToHide;
            infoQueue->AddStorageFilterEntries(&filter);
        }

    }





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
            //mat4x4_identity(uniform.model); // model matrix have to be identity matrix initially
            //mat4x4_translate(uniform.model, 0.f, 0.f, -5.f); // apply translation to model matrix; move cube to the back, to be in front of camera
            // apply in-place rotation to model matrix (-rotationAngle, because of flipped Vulkan coordinates compared to OpenGL)
            //mat4x4_rotate(uniform.model, uniform.model, 0.7f, 0.2f, -0.8f, -rotationAngle);
            rotationAngle += (rotationSpeedRadians * deltaTime);

            // Preparing perspective matrix
            //mat4x4_perspective(uniform.proj, fov, ((float) windowData.currentWidth / (float) windowData.currentHeight), zNear, zFar);

            // Preparing view (world-to-camera) matrix
            //mat4x4_look_at(uniform.view, cameraPos, front, up);

           
        }
    }


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