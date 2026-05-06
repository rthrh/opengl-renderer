#version 420 core

in vec4 FragPos_out;

uniform vec3  lightPos;

#include "include/ubo.glsl"

void main() {
    float lightDistance = length(FragPos_out.xyz - lightPos);
    lightDistance       = lightDistance / Config.pointShadowFarPlane;
    gl_FragDepth        = lightDistance;
}
