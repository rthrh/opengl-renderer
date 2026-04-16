#version 420 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D emissiveMap;
layout(binding = 3) uniform sampler2D metallicMap;
layout(binding = 4) uniform sampler2D roughnessMap;
layout(binding = 5) uniform sampler2D aoMap;

#include "common/ubo.glsl"
#include "common/brdf.glsl"
#include "common/pbr_lights.glsl"

void main() {
    vec3  albedo    = pow(texture(albedoMap,    TexCoords).rgb, vec3(2.2)); // sRGB -> linear
    float metallic  = texture(metallicMap,  TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).g;
    float ao        = texture(aoMap,        TexCoords).b;
    vec3  emissive  = pow(texture(emissiveMap,  TexCoords).rgb, vec3(2.2));

    vec3 normalSample = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);
    vec3 V = normalize(camera.position.xyz - FragPos);

    // F0: base reflectance
    // dielectrics: 0.04, metals: tinted by albedo
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // accumulate lights
    vec3 Lo = vec3(0.0);
    Lo += CalcDirectionalLight(N, V, albedo, metallic, roughness, F0);

    int pointCount = pointLights.count.x;
    for (int i = 0; i < pointCount; i++)
        Lo += CalcPointLight(pointLights.lights[i], N, V, FragPos,
                             albedo, metallic, roughness, F0);

    int spotCount = spotLights.count.x;
    for (int i = 0; i < spotCount; i++)
        Lo += CalcSpotLight(spotLights.lights[i], N, V, FragPos,
                            albedo, metallic, roughness, F0);

    // ambient + AO
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo + emissive;

    // tone mapping + gamma
    color = color / (color + vec3(1.0)); // Reinhard
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
