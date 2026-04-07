#version 450 core

// output textures
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gAlbedo;
layout (location = 2) out vec3 gNormal;
layout (location = 3) out vec3 gORM;
layout (location = 4) out vec3 gEmissive;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

// samplers
layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D emissiveMap;
layout(binding = 3) uniform sampler2D metallicMap;
layout(binding = 4) uniform sampler2D roughnessMap;
layout(binding = 5) uniform sampler2D aoMap;

void main() {
    float ao = texture(aoMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;
    float metallic = texture(metallicMap, TexCoords).r;

    // normal map → world space via TBN
    vec3 normalSample = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);

    gPosition = FragPos;
    gAlbedo   = texture(albedoMap, TexCoords).rgb;
    gNormal   = N;//normalize(Normal);
    gORM      = vec3(ao, roughness, metallic);
    gEmissive = texture(emissiveMap, TexCoords).rgb;
}