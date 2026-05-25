#version 450 core

// output textures
layout (location = 0) out vec3 gAlbedo;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gORM;
layout (location = 3) out vec3 gEmissive;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

// samplers
layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D ormMap;
layout(binding = 3) uniform sampler2D emissiveMap;

#include "include/material.glsl"
uniform int materialIndex;

void main() {
    Material material = materials[materialIndex];
    vec4 albedoSample = texture(albedoMap, TexCoords);
    float alpha = albedoSample.a * material.baseColorFactor.a;
    if (alpha < material.alphaCutoff)
        discard;

    // TODO double gamma correction here + SRGB texture upload?
    //vec3 albedo = texture(albedoMap, TexCoords).rgb * material.baseColorFactor.xyz;
    vec3 albedo = pow(albedoSample.rgb, vec3(2.2)) * material.baseColorFactor.xyz;

    //vec3 emissive = texture(emissiveMap, TexCoords).rgb * material.emissiveFactor.xyz * 5.0;
    vec3 emissive = pow(texture(emissiveMap, TexCoords).rgb, vec3(2.2)) * material.emissiveFactor.xyz * 5.0; // TODO emissive is multiplied so it passes brighness check for bloom pass
    vec3 orm = texture(ormMap, TexCoords).rgb;
    float ao = orm.r * material.occlusionStrength;
    float roughness = orm.g * material.roughnessFactor;
    float metallic = orm.b * material.metallicFactor;

    // Apply TBN to normal map
    vec3 normalSample = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);

    gAlbedo = albedo;
    gNormal   = N;
    gORM      = vec3(ao, roughness, metallic);
    gEmissive = emissive;
}
