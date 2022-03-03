#pragma once

#include <stdio.h>
#include <string.h>
#include "glad/gl.h"
#include "utils.h"

static void glDebugCallbackFunction(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {

    char sourceStr[16] = {'\0'};
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            strncpy(sourceStr, "API", 4);
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            strncpy(sourceStr, "WINSYS", 7);
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            strncpy(sourceStr, "SHADERCOMP", 11);
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            strncpy(sourceStr, "THIRDP", 7);
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            strncpy(sourceStr, "APP", 4);
            break;
        case GL_DEBUG_SOURCE_OTHER:
            strncpy(sourceStr, "OTHER", 6);
            break;
        default:
            strncpy(sourceStr, "UNKNOWN", 8);
    }

    char typeStr[16] = {'\0'};
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            strncpy(typeStr, "ERROR", 6);
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            strncpy(typeStr, "DEPRECATED", 11);
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            strncpy(typeStr, "UNDEFINED", 10);
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            strncpy(typeStr, "PORTABILITY", 12);
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            strncpy(typeStr, "PERF", 5);
            break;
        case GL_DEBUG_TYPE_MARKER:
            strncpy(typeStr, "MARKER", 7);
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            strncpy(typeStr, "PSHGRP", 7);
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            strncpy(typeStr, "POPGRP", 7);
            break;
        case GL_DEBUG_TYPE_OTHER:
            strncpy(typeStr, "OTHER", 6);
            break;
        default:
            strncpy(typeStr, "UNKNOWN", 8);
    }

    char severityStr[16] = {'\0'};
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            strncpy(severityStr, "HIGH", 5);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            strncpy(severityStr, "MEDIUM", 7);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            strncpy(severityStr, "LOW", 4);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            strncpy(severityStr, "NOTIF", 6);
            break;
        default:
            strncpy(severityStr, "UNKNOWN", 8);
    }

    LOG3DHW("[gldebug] [%s->%s] [%s] %s",  sourceStr, typeStr, severityStr, message);
}