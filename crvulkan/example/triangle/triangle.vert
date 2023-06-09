#version 450

layout(location = 0) out vec2 texCoord;

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec4 pos = vec4(position, 0.0, 1.0);
    gl_Position = ubo.proj * ubo.view * ubo.model * pos;
    texCoord = color.xy;
}
