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
layout(binding = 3) uniform sampler2D ormMap;

void main() {
    vec3 orm = texture(ormMap, TexCoords).rgb;
    float ao = orm.r;
    float roughness = orm.g;
    float metallic = orm.b;

    // normal map → world space via TBN
    vec3 normalSample = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);

    gPosition = FragPos;
    gAlbedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    gNormal   = N;//normalize(Normal);
    gORM      = vec3(ao, roughness, metallic);
    gEmissive = pow(texture(emissiveMap, TexCoords).rgb, vec3(2.2)) * 10.0f; // TODO emissive is multiplied so it passes brighness check for bloom pass
}
