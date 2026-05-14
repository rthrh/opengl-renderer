#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 aTangent;
layout (location = 4) in mat4 aInstanceMatrix;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;


#include "include/ubo.glsl"

void main() {
    vec4 worldPos = aInstanceMatrix * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
    Normal = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent.xyz);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N); // re-orthogonalize T with respect to N
    vec3 B = cross(N, T) * aTangent.w; // multiply by handness sign
    TBN = mat3(T, B, N);

    gl_Position = camera.projection * camera.view * worldPos;
}
