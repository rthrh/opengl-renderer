#version 450 core

// output textures
in vec2 TexCoords;

// samplers
layout(binding = 0) uniform sampler2D albedoMap;

// Material SSBO
struct Material {
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    uvec4 textureHandles; // unused here
    float normalScale;
    float occlusionStrength;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    int alphaMode;
    int doubleSided;
    float _pad;
};

layout(std430, binding = 0) readonly buffer MaterialBuffer {
    Material materials[];
};

uniform int materialIndex;

void main() {
    Material material = materials[materialIndex];
    float alpha = texture(albedoMap, TexCoords).a * material.baseColorFactor.a;
    if (alpha < material.alphaCutoff) discard;
}
