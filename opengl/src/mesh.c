#include "mesh.h"
#include "glad/gl.h"
#include "utils.h"

GLuint loadCubeMesh() {
    GLuint vao = -1;
    GLuint vbo = -1;

    // Generate vertex array and buffer
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Bind vertex array and buffer, copy vertices data to buffer
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_DATA), &CUBE_DATA[0], GL_STATIC_DRAW);

    // Set pointer to vertices in buffer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);

    // Set pointer to texture coords in buffer
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    
    // Unbind vertex array
    glBindVertexArray(0);

    LOG3DHW("[mesh] Loaded cube data to buffer (vao=%d, vbo=%d)", vao, vbo); 

    return vao;
}