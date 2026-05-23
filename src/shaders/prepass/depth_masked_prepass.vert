#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;
layout (location = 4) in mat4 aInstanceMatrix; // slots 4, 5, 6, 7

out vec2 TexCoords;

#include "include/ubo.glsl"

void main()
{
    TexCoords = aTexCoords;
    gl_Position = camera.projection * camera.view * aInstanceMatrix * vec4(aPos, 1.0);
}
