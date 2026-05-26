#version 420 core
out vec4 FragColor;

in vec3 WorldPos;

#include "include/samplers.glsl"

void main()
{
    vec3 envColor = texture(environmentMap, WorldPos).rgb;

    FragColor = vec4(envColor, 1.0);
}
