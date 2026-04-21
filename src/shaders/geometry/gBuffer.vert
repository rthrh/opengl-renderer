#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out mat3 TBN;

uniform mat4 model;

#include "include/ubo.glsl"

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz; 
    TexCoords = aTexCoords;
    
    //mat3 normalMatrix = transpose(inverse(mat3(model)));
    //Normal = normalMatrix * aNormal;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent.xyz);
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 B = cross(N, T);// * aTangent.w; //TODO add handness sign
    TBN = mat3(T, B, N);

    gl_Position = camera.projection * camera.view * worldPos;
}
