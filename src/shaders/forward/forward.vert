#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 aTangent;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

uniform mat4 model;

#include "include/ubo.glsl"

void main() {
    TexCoords = aTexCoords;
    FragPos   = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = aNormal;

    vec3 T = normalize(mat3(model) * aTangent.xyz);
    vec3 N = normalize(mat3(model) * aNormal);
    T = normalize(T - dot(T, N) * N); // re-orthogonalize T with respect to N
    vec3 B = cross(N, T) * aTangent.w; // multiply by handness sign
    TBN = mat3(T, B, N);

    gl_Position = camera.projection * camera.view * model * vec4(aPos, 1.0);
}
