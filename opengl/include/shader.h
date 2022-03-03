#pragma once

#include <stdbool.h>

#include "glad/gl.h"

static const char* GLSL_VERTEX_SHADER =
"#version 430 core                                                      \n"     
"                                                                       \n"
"layout (location = 0) in vec3 in_position;                             \n"
"layout (location = 1) in vec2 in_tex_coords;                           \n"
"                                                                       \n"
"out vec3 fragment_position;                                            \n"
"out vec2 tex_coords;                                                   \n"
"                                                                       \n"
"uniform mat4 model;                                                    \n"
"uniform mat4 view;                                                     \n"
"uniform mat4 projection;                                               \n"
"                                                                       \n"
"void main() {                                                          \n"
"    fragment_position = vec3(model * vec4(in_position, 1.0));          \n"
"                                                                       \n"
"    tex_coords = in_tex_coords;                                        \n"
"                                                                       \n"    
"    gl_Position = projection * view * vec4(fragment_position, 1.0);    \n"
"}                                                                      ";

static const char* GLSL_FRAGMENT_SHADER = 
"#version 430 core                                                      \n"
"                                                                       \n"
"in vec2 tex_coords;                                                    \n"
"                                                                       \n"
"out vec4 color;                                                        \n"
"                                                                       \n"
"uniform sampler2D texture_diffuse1;                                    \n"
"                                                                       \n"
"void main() {                                                          \n"
"    color = vec4(vec3(texture(texture_diffuse1, tex_coords)), 1.0);    \n"
"}                                                                      ";  

bool handleShaderOperationResult(GLuint id, GLenum status);

GLuint loadAndLinkShaderProgram();