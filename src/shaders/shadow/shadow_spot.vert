#version 420 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;

uniform mat4 spotLightSpaceMatrix;

void main() {
    gl_Position = spotLightSpaceMatrix * model * vec4(aPos, 1.0);
}