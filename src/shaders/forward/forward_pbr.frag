#version 450 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D ormMap;
layout(binding = 3) uniform sampler2D emissiveMap;

layout(binding = 7) uniform sampler2D shadowDirMap;
layout(binding = 8) uniform samplerCubeArray shadowPointMaps;
layout(binding = 9) uniform sampler2DArray shadowSpotMap;

layout(binding = 10) uniform samplerCube irradianceMap;
layout(binding = 11) uniform samplerCube prefilteredMap;
layout(binding = 12) uniform sampler2D brdfLUT;


#include "include/ubo.glsl"
#include "include/brdf.glsl"
#include "include/pbr_lights.glsl"
#include "include/shadows.glsl"

#include "include/material.glsl"
uniform int materialIndex;

uniform bool blendPass;
//TODO update this shader
void main() {
    Material material = materials[materialIndex];

    if (!blendPass && material.alphaMode == 2) discard; // discard BLEND in opaque pass
    if (blendPass && material.alphaMode != 2) discard; // discard other in blend pass

    vec4  albedoSample = texture(albedoMap, TexCoords).rgba;
    vec3 albedo   = albedoSample.rgb * material.baseColorFactor.rgb;

    float alpha = 1.0;
    if (material.alphaMode == 2) // BLEND
        alpha = albedoSample.a * material.baseColorFactor.a;

    vec3 orm      = texture(ormMap, TexCoords).rgb;
    float ao        = orm.r * material.occlusionStrength;
    float roughness = orm.g * material.roughnessFactor;
    roughness = max(0.04, roughness);
    float metallic  = orm.b * material.metallicFactor;

    vec3 emissive = texture(emissiveMap, TexCoords).rgb * material.emissiveFactor.rgb;

    vec3 normalSample = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    //TODO add normal scale
    vec3 N = normalize(TBN * normalSample);
    vec3 V = normalize(camera.position.xyz - FragPos);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);


    // accumulate lights
    vec3 Lo = vec3(0.0);
    float shadow = 0.0;
    if (Config.dirShadowsEnabled) {
        shadow = ShadowDirectionalLight(FragPos, N, Shadow.dirLightSpaceMatrix, Config.dirShadowBiasMin, Config.dirShadowBiasMax);
    }
    Lo += CalcDirectionalLight(N, V, albedo, metallic, roughness, F0) * (1.0 - shadow);

    int pointCount = pointLights.count.x;
    for (int i = 0; i < pointCount; i++) {
        float shadow = 0.0;
        if (i < Config.maxPointShadowCasters && Config.pointShadowsEnabled) {
            vec3 lightPos = pointLights.lights[i].positionAndRange.xyz;
            float farPlane = pointLights.lights[i].positionAndRange.w;
            shadow = ShadowPointLight(FragPos, lightPos, farPlane, Config.pointShadowBias, i);
        }

        Lo += CalcPointLight(pointLights.lights[i], N, V, FragPos,
                             albedo, metallic, roughness, F0) * (1.0 - shadow);
    }

    int spotCount = spotLights.count.x;
    for (int i = 0; i < spotCount; i++) {
        float shadow = 0.0;
        if (i < Config.maxSpotShadowCasters && Config.spotShadowsEnabled) {
            shadow = ShadowSpotLight(FragPos, N, Shadow.spotLightSpaceMatrices[i], Config.spotShadowBiasMin, Config.spotShadowBiasMax, i);
        }
        Lo += CalcSpotLight(spotLights.lights[i], N, V, FragPos,
                            albedo, metallic, roughness, F0) * (1.0 - shadow);
    }

    // IBL ambient + AO
    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = textureLod(prefilteredMap, R,  roughness * Config.maxReflectionLOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;

    vec3 color = ambient + Lo + emissive;
    FragColor = vec4(color, alpha); // any(isnan(color)) TODO NaN sometimes on FragColor
}
