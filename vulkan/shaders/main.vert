#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;

layout(location = 0) out vec2 fragTexCoords;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    // -inTexCoords.x; flip horizontally, because tex coords in cube.h are according to OpenGL coordinates system
    fragTexCoords = vec2(-inTexCoords.x, inTexCoords.y); 
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
}