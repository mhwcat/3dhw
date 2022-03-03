# 3dhw
Set of "hello world"-style examples for all modern graphics APIs and platforms (on desktop PC) that goes step further than drawing a triangle.


https://user-images.githubusercontent.com/829477/156614229-b85c5640-44bf-4574-bd53-29baecaabebd.mp4


## Purpose
Aim of this project is to provide minimal examples of basic 3D object rendering (in this case - rotating textured cube with photo of my cat on it). All examples have comments and are written in a way that should be understandable for beginners wanting to learn basics of various graphics APIs. Every example follows this steps:
1) Setup resizable, closable window
2) Setup event handling
3) Setup given graphics API and provide a way to draw to created window surface
4) Load cube vertices, UVs, texture and shaders
5) Draw rotating cube using correct delta timestep and handle platform events (in a loop)
6) Cleanup and exit

## Structure
All examples are written in C and have CMake support. Common code shared between different platforms is in root directory of each API (e.g. `opengl/src`). Platform implementation is contained in `main.c` file (with small exceptions, like surface creation in Vulkan) for given platform (e.g. `opengl/linux/main.c`).

Only external dependency is [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) (required for Vulkan examples). Internally project uses [GLAD2](https://gen.glad.sh/) (providing loaders for GL, GLX and WGL), [linmath.h](https://github.com/datenwolf/linmath.h) (for linear algebra) and [stb_image](https://github.com/nothings/stb) (for image loading). For platform dependent stuff like window creation and event handling, native OS APIs are used (Win32 on Windows, Xlib on Linux).

OpenGL target version is 4.3 (core profile) - this is first version supporting [debug output](https://www.khronos.org/opengl/wiki/Debug_Output) in core. Vulkan code is written against [1.0 specification](https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html) and heavily based on [vulkan-tutorial.com](https://vulkan-tutorial.com/).

Examples will print verbose information about current program state to stdout or debugger (on Windows). Error handling is rudimentary on purpose - any error will cause application to exit with -1 code. Each graphics API is setup to use available debugging tools (debug callback in OpenGL, debug callback + validation layers in Vulkan).

## Implementation status
| API | OS | Status |
|---|---|:-:|
| OpenGL | Windows | :heavy_check_mark: |
| OpenGL | Linux (X11) | :heavy_check_mark: |
| Vulkan | Windows | :heavy_check_mark: |
| Vulkan | Linux (X11) | :heavy_check_mark: |
| DirectX 11 | Windows | :x: |
| DirectX 12 | Windows | :x: |
| Metal | macOS | :x: |

## Building
### Vulkan caveats
CMake will try to automatically find Vulkan SDK path based on environment variable, but if it fails, set it manually in `VULKAN_SDK_DIR` CMake variable.  

Vulkan uses precompiled SPIR-V shaders, so before running example, shaders have to be compiled using `glslc` (provided with Vulkan SDK):
```
[path-to-vulkan-sdk]/bin/glslc [path-to-3dhw]/vulkan/shaders/main.vert -o [path-to-build-dir]/vert.spv
[path-to-vulkan-sdk]/bin/glslc [path-to-3dhw]/vulkan/shaders/main.frag -o [path-to-build-dir]/frag.spv
```
### OpenGL/Vulkan on Linux
Install required OS dependencies: `libx11-dev`, `libxrandr-dev`, `mesa-common-dev`.
```bash
cd 3dhw/[opengl,vulkan]/linux
mkdir build && cd build/
cmake ..
make
```

### OpenGL/Vulkan on Windows
Use [CMake](https://cmake.org/download/) to generate Visual Studio solution and build from there.

## Todo
* Implement DirectX and Metal
* Better code documentation for Vulkan
* Investigate Vulkan validation layers errors on Windows
* Wayland support on Linux
