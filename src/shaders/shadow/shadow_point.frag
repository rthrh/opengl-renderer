#version 420 core

in vec4 FragPos_out;

uniform vec3  lightPos;
uniform float farPlane;

void main() {
    float lightDistance = length(FragPos_out.xyz - lightPos);
    lightDistance       = lightDistance / farPlane;
    gl_FragDepth        = lightDistance;
}