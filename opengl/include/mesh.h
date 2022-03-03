#pragma once

#include "glad/gl.h"

static const float CUBE_DATA[] = {
    // Vertex coords (x, y, z)  Texture coords (u, v)   
    -1.0f,   -1.0f,   1.0f,     0.0f, 0.0f,
    1.0f,    -1.0f,   1.0f,     0.0f, 1.0f,
    1.0f,    1.0f,    1.0f,     1.0f, 1.0f,
    -1.0f,   -1.0f,   1.0f,     0.0f, 0.0f,
    1.0f,    1.0f,    1.0f,     1.0f, 1.0f,
    -1.0f,   1.0f,    1.0f,     1.0f, 0.0f,
    1.0f,    -1.0f,   1.0f,     0.0f, 0.0f,
    1.0f,    -1.0f,   -1.0f,    0.0f, 1.0f,
    1.0f,    1.0f,    -1.0f,    1.0f, 1.0f,
    1.0f,    -1.0f,   1.0f,     0.0f, 0.0f,
    1.0f,    1.0f,    -1.0f,    1.0f, 1.0f,
    1.0f,    1.0f,    1.0f,     1.0f, 0.0f,
    1.0f,    -1.0f,   -1.0f,    0.0f, 0.0f,
    -1.0f,   -1.0f,   -1.0f,    0.0f, 1.0f,
    -1.0f,   1.0f,    -1.0f,    1.0f, 1.0f,
    1.0f,    -1.0f,   -1.0f,    0.0f, 0.0f,
    -1.0f,   1.0f,    -1.0f,    1.0f, 1.0f,
    1.0f,    1.0f,    -1.0f,    1.0f, 0.0f,
    -1.0f,   -1.0f,   -1.0f,    0.0f, 0.0f,
    -1.0f,   -1.0f,   1.0f,     0.0f, 1.0f,
    -1.0f,   1.0f,    1.0f,     1.0f, 1.0f,
    -1.0f,   -1.0f,   -1.0f,    0.0f, 0.0f,
    -1.0f,   1.0f,    1.0f,     1.0f, 1.0f,
    -1.0f,   1.0f,    -1.0f,    1.0f, 0.0f,
    -1.0f,   1.0f,    1.0f,     0.0f, 0.0f,
    1.0f,    1.0f,    1.0f,     0.0f, 1.0f,
    1.0f,    1.0f,    -1.0f,    1.0f, 1.0f,
    -1.0f,   1.0f,    1.0f,     0.0f, 0.0f,
    1.0f,    1.0f,    -1.0f,    1.0f, 1.0f,
    -1.0f,   1.0f,    -1.0f,    1.0f, 0.0f,
    -1.0f,   -1.0f,   1.0f,     0.0f, 0.0f,
    -1.0f,   -1.0f,   -1.0f,    0.0f, 1.0f,
    1.0f,    -1.0f,   -1.0f,    1.0f, 1.0f,
    -1.0f,   -1.0f,   1.0f,     0.0f, 0.0f,
    1.0f,    -1.0f,   -1.0f,    1.0f, 1.0f,
    1.0f,    -1.0f,   1.0f,     1.0f, 0.0f 
};

// Load cube vertices and UVs to GPU buffer 
// Later we'll need only vertex array object ID in order to draw textured cube
GLuint loadCubeMesh();