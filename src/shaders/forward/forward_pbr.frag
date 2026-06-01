#version 450 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;

#include "include/samplers.glsl"
#include "include/ubo.glsl"
#include "include/brdf.glsl"
#include "include/pbr_lights.glsl"
#include "include/shadows.glsl"
#include "include/material.glsl"

uniform int materialIndex;

void main() {
    Material material = materials[materialIndex];

    vec4  albedoSample = texture(albedoMap, TexCoords).rgba;
    vec3 albedo   = albedoSample.rgb * material.baseColorFactor.rgb;

    float alpha = albedoSample.a * material.baseColorFactor.a;

    vec3 orm      = texture(ormMap, TexCoords).rgb;
    float ao        = orm.r * material.occlusionStrength;
    float roughness = orm.g * material.roughnessFactor;

    float metallic  = orm.b * material.metallicFactor;
    vec3 emissive = texture(emissiveMap, TexCoords).rgb * material.emissiveFactor.rgb;

    vec3 normalSample = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    //TODO add normal scale
    vec3 N = normalize(TBN * normalSample);
    vec3 V = normalize(camera.position.xyz - FragPos);

    // Improved Geometric Specular Antialiasing https://www.jp.square-enix.com/tech/library/pdf/ImprovedGeometricSpecularAA(slides).pdf
    const float SIGMA2 = 0.25;
    const float KAPPA  = 0.18;

    vec3 dndu = dFdx(N);
    vec3 dndv = dFdy(N);
    float variance = SIGMA2 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(variance, KAPPA);
    float filteredRoughness2 = clamp(roughness * roughness + kernelRoughness2, 0.0, 1.0);
    roughness = sqrt(filteredRoughness2);
    roughness = max(0.04, roughness); // at lower roughness, specular highlights are broken

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
        if (i < MAX_POINT_SHADOW_CASTERS && Config.pointShadowsEnabled) {
            vec3 lightPos = pointLights.lights[i].positionAndRange.xyz;
            float range = pointLights.lights[i].positionAndRange.w;
            shadow = ShadowPointLight(FragPos, N, lightPos, range, Config.pointShadowBiasMin, Config.pointShadowBiasMax, i);
        }
        Lo += CalcPointLight(pointLights.lights[i], N, V, FragPos,
                             albedo, metallic, roughness, F0) * (1.0 - shadow);
    }

    int spotCount = spotLights.count.x;
    for (int i = 0; i < spotCount; i++) {
        float shadow = 0.0;
        if (i < MAX_SPOT_SHADOW_CASTERS && Config.spotShadowsEnabled) {
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
    float MAX_REFLECTION_LOD = 4.0;
    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = textureLod(prefilteredMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    vec3 color = ambient + Lo + emissive;
    FragColor = vec4(color, alpha); // any(isnan(color)) TODO NaN sometimes on FragColor
}
