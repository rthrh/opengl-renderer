#version 420 core

in vec3 FragPos;

uniform vec3 lightPos;

#include "include/ubo.glsl"

void main() {
    float lightDistance = length(FragPos - lightPos);
    lightDistance       = lightDistance / Config.pointShadowFarPlane;
    gl_FragDepth        = lightDistance;
}
