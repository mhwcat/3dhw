#include <stdlib.h>
#include <stdbool.h>

#include "shader.h"
#include "glad/gl.h"
#include "utils.h"

bool handleShaderOperationResult(GLuint id, GLenum status) {
    int success = -1;
    char infoLog[256];

    if (status == GL_COMPILE_STATUS) {
        glGetShaderiv(id, status, &success);
        if (!success) {
            glGetShaderInfoLog(id, 256, NULL, infoLog);
            LOG3DHW("[shader] Shader compilation failed for ID %d with error: %s", id, infoLog);
        }
    } else if (status == GL_LINK_STATUS) {
        glGetProgramiv(id, status, &success);
        if (!success) {
            glGetProgramInfoLog(id, 256, NULL, infoLog);
            LOG3DHW("[shader] Program linking failed for ID %d with error: %s", id, infoLog);
        }
    } else {
        LOG3DHW("[shader] Unknown status to check for: %d", status);
    }

    return success;
}

GLuint loadAndLinkShaderProgram() {
    // Compile vertex shader
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderId, 1, &GLSL_VERTEX_SHADER, NULL);
    glCompileShader(vertexShaderId);
    if (!handleShaderOperationResult(vertexShaderId, GL_COMPILE_STATUS)) {
        exit(-1);
    }

    // Compile fragment shader
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderId, 1, &GLSL_FRAGMENT_SHADER, NULL);
    glCompileShader(fragmentShaderId);
    if (!handleShaderOperationResult(fragmentShaderId, GL_COMPILE_STATUS)) {
        exit(-1);
    }

    // Create and link shader program
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    glLinkProgram(programId);
    if (!handleShaderOperationResult(programId, GL_LINK_STATUS)) {
        exit(-1);
    }

    // Detach and delete shaders after linking program
    glDetachShader(programId, vertexShaderId);
    glDetachShader(programId, fragmentShaderId);

    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId); 

    LOG3DHW("[shader] Compiled and linked shader program (vertexShaderId=%d, fragmentShaderId=%d, programId=%d)", vertexShaderId, fragmentShaderId, programId);

    return programId;
}