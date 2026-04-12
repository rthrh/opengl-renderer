#version 420 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;

layout(std140, binding = 4) uniform ShadowBlock {
    mat4 lightSpaceMatrix;
};

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}