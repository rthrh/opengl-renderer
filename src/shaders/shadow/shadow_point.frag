#version 420 core

in vec3 FragPos;

uniform vec3 lightPos;
uniform float farPlane;

#include "include/ubo.glsl"

void main() {
    float lightDistance = length(FragPos - lightPos);
    lightDistance       = lightDistance / farPlane;
    gl_FragDepth        = lightDistance;
}
