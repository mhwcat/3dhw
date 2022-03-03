#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
//#include <unistd.h> // for usleep()

// Third party libs
#include "glad/gl.h"
#include "glad/glx.h"
#include "linmath.h"

// Internal stuff
#include "gldebug.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"

static const int WINDOW_WIDTH = 1600;
static const int WINDOW_HEIGHT = 900;
static const char* WINDOW_TITLE = "3dhw-opengl";

typedef struct LinuxWindowInfo {
    Display* display;
    Window window;
    GLXContext glContext;
} LinuxWindowInfo;

LinuxWindowInfo createWindowAndGLContext() {
    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        LOG3DHW("[window-linux] Failed opening display!\n");
        exit(-1);
    }

    Screen* screen = XDefaultScreenOfDisplay(display); // we need screen dimensions
    Window root = DefaultRootWindow(display); // root window is desktop in this case - XCreateWindow needs it

    // Load GLX functions from GLAD
    if (!gladLoaderLoadGLX(display, 0)) {
        LOG3DHW("[window-linux] Failed loading GLX\n");
        exit(-1);
    }

    // Request required visual attributes (RGBA, 24bit depth buffer, double-buffering)
    GLint glxAttributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo* visualInfo = glXChooseVisual(display, 0, glxAttributes);
    if (visualInfo == NULL) {
        LOG3DHW("[window-linux] Failed choosing appropriate XVisualInfo!\n");
        exit(-1);
    }

    // Prepare final window attributes (colormap/visualinfo, event mask)
    const Colormap colormap = XCreateColormap(display, root, visualInfo->visual, AllocNone);
    XSetWindowAttributes setWindowAttributes;
    setWindowAttributes.colormap = colormap;
    setWindowAttributes.event_mask = ExposureMask; // we need to draw to window
    setWindowAttributes.event_mask |= StructureNotifyMask; // we need to handle window resize events

    // Create window
    Window window = XCreateWindow(display, root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, visualInfo->depth, InputOutput, 
        visualInfo->visual, CWColormap | CWEventMask, &setWindowAttributes);

    XMapWindow(display, window);
    XStoreName(display, window, WINDOW_TITLE);
    // Move window to center of screen (setting x, y in XCreateWindow does not work)
    XMoveWindow(display, window, (screen->width / 2) - (WINDOW_WIDTH / 2), 
        (screen->height / 2) - (WINDOW_HEIGHT / 2));

    // Create GL context and make it current
    GLXContext glContext = glXCreateContext(display, visualInfo, NULL, GL_TRUE);
    glXMakeCurrent(display, window, glContext);

    // After creating context load GL functions from GLAD
    if (!gladLoaderLoadGL()) {
        LOG3DHW("[window-linux] Failed loading GL\n");
        exit(-1);
    }

    // Return window info struct on exit
    LinuxWindowInfo linuxWindowInfo;
    linuxWindowInfo.display = display;
    linuxWindowInfo.window = window;
    linuxWindowInfo.glContext = glContext;

    return linuxWindowInfo;
}

int main(int argc, char** argv) {
    LinuxWindowInfo linuxWindowInfo = createWindowAndGLContext();
    // In order to handle window quit event on X11, we have to create Atom "WM_DELETE_WINDOW"
    Atom wmDeleteMessage = XInternAtom(linuxWindowInfo.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(linuxWindowInfo.display, linuxWindowInfo.window, &wmDeleteMessage, 1);
        
    // Enable OpenGL debug and map the output to callback in gldebug.h
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallbackFunction, NULL);

    // Enable backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Load and compile shader program
    GLuint shaderProgramId = loadAndLinkShaderProgram();

    // Load cube vertices data to GPU
    GLuint meshVao = loadCubeMesh();

    // Load texture
    GLuint textureId = loadTexture();
    
    // This struct will be used to store window size (we'll update it on window resize event)
    XWindowAttributes windowAttributes;
    XGetWindowAttributes(linuxWindowInfo.display, linuxWindowInfo.window, &windowAttributes);
    glViewport(0, 0, windowAttributes.width, windowAttributes.height); // set initial viewport size
    
    // Cube rotation vars
    float rotationAngle = 0.f;
    const float rotationSpeedRadians = (45.f * (M_PI / 180.f)); // rotate by 45 degree / s

    // GL matrices setup
    float modelMat[4][4] = {{0.f}};
    float perspectiveMat[4][4] = {{0.f}};
    float viewMat[4][4] = {{0.f}};

    // Camera vectors setup
    const float cameraPos[] = {0.f, 0.f, 0.f}; // we position our camera at [0, 0, 0] in world space
    const float front[] = {0.f, 0.f, -1.f}; // front is where camera is looking, in our case -Z
    const float up[] = {0.f, 1.f, 0.f}; // up is where camera "sees" top, in our case +Y
    const float fov = 45.f; // camera field of view
    const float zNear = 0.01f; // near plane
    const float zFar = 1000.f; // far plane

    // Delta time between consecutive renders, used as "smoothing" value for rotation
    // Normally it would be applied to all movement on screen. 
    // Even better solution would be to use fixed timestep (especially if there is any physics 
    // simulation involved), which is not implemented here.
    // ref: https://gafferongames.com/post/fix_your_timestep/
    float deltaTime = 0.f;
    struct timespec elapsedTime = {0};
    long timeDiffVal = 0L;

    XEvent xEvent;
    bool running = true;
    while (running) {
        // Update delta time based on last time diff between renders
        struct timespec newTime; 
        clock_gettime(CLOCK_MONOTONIC_RAW, &newTime); // CLOCK_MONOTIC_RAW gives us access to raw hardware timer
        timeDiffVal = nanosecDiff(elapsedTime, newTime);
        elapsedTime = newTime;
        deltaTime = (float) timeDiffVal / 1.0e9f; // nanoseconds -> seconds

        // Handle X events first
        while (XPending(linuxWindowInfo.display)) {
            XNextEvent(linuxWindowInfo.display, &xEvent);

            // ConfigureNotify event is dispatched on window resize event (and other times too),
            // so we query for current screen size and force GL viewport resize
            if (xEvent.type == ConfigureNotify) {
                XGetWindowAttributes(linuxWindowInfo.display, linuxWindowInfo.window, &windowAttributes);
                glViewport(0, 0, windowAttributes.width, windowAttributes.height);
            // ClientMessage is dispatched on window close request, but we also have to check
            // if event data equals to atom defined earlier
            } else if (xEvent.type == ClientMessage) {      
                if ((unsigned long) xEvent.xclient.data.l[0] == wmDeleteMessage) {
                    printf("[main] Received exit event, quitting...\n");

                    glXMakeCurrent(linuxWindowInfo.display, None, NULL);
                    glXDestroyContext(linuxWindowInfo.display, linuxWindowInfo.glContext);
                    XDestroyWindow(linuxWindowInfo.display, linuxWindowInfo.window);
                    XCloseDisplay(linuxWindowInfo.display);

                    running = false;

                    // We break here becasue we don't want to handle any more X events at this point.
                    // Not doing so results in "X connection broken" error on program shutdown.
                    break; 
                }
            }             
        }

        // Drawing begins here
        if (running) {
            // Clear screen
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glClearColor((99.f / 255.f), (139.f / 255.f), (235.f / 255.f), 1.f);

            // Preparing model matrix
            mat4x4_identity(modelMat); // model matrix have to be identity matrix initially
            mat4x4_translate(modelMat, 0.f, 0.f, -5.f); // apply translation to model matrix; move cube to the back, to be in front of camera
            mat4x4_rotate(modelMat, modelMat, 0.7f, 0.2f, -0.8f, rotationAngle); // apply in-place rotation to model matrix
            rotationAngle += (rotationSpeedRadians * deltaTime);

            // Preparing perspective matrix
            mat4x4_perspective(perspectiveMat, fov, ((float) windowAttributes.width / (float) windowAttributes.height), zNear, zFar);

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

            // Buffer swap at the end of render loop
            glXSwapBuffers(linuxWindowInfo.display, linuxWindowInfo.window);

            // Uncomment this (and unistd.h header) to see deltaTime in action - consistent cube rotation regardless of FPS.
            // This simulates frame time of ~100ms (10FPS) 
            // usleep(100000);
        }
    }  

    return EXIT_SUCCESS;
}