#version 420 core
layout(location = 0) in vec3 aPos;
layout(location = 4) in mat4 aInstanceMatrix;

#include "include/ubo.glsl"

void main() {
    gl_Position = Shadow.dirLightSpaceMatrix * aInstanceMatrix * vec4(aPos, 1.0);
}
