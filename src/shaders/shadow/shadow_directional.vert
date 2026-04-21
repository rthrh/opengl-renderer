#version 420 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;

#include "include/ubo.glsl"

void main() {
    gl_Position = Shadow.dirLightSpaceMatrix * model * vec4(aPos, 1.0);
}
