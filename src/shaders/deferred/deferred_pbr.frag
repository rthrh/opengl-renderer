#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

// From gbuffer
layout(binding = 0) uniform sampler2D albedoMap;    // RGB8
layout(binding = 1) uniform sampler2D normalMap;    // RGB16F
layout(binding = 2) uniform sampler2D ormMap;       // RGB8  r=ao g=roughness b=metallic
layout(binding = 3) uniform sampler2D emissiveMap;  // RGB16F
layout(binding = 4) uniform sampler2D depthMap;     // DEPTH24_STENCIL8

layout(binding = 7) uniform sampler2D shadowDirMap;
layout(binding = 8) uniform samplerCubeArray shadowPointMaps;
layout(binding = 9) uniform sampler2DArray shadowSpotMap;
layout(binding = 10) uniform samplerCube irradianceMap;
layout(binding = 11) uniform samplerCube prefilteredMap;
layout(binding = 12) uniform sampler2D brdfLUT;
layout(binding = 13) uniform sampler2D ssaoMap;

#include "include/ubo.glsl"
#include "include/brdf.glsl"
#include "include/pbr_lights.glsl"
#include "include/shadows.glsl"

// get world space position from depth map
vec3 ReconstructPosition(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = inverse(camera.projection) * clip;
    view /= view.w;
    return (inverse(camera.view) * view).xyz;
}

void main() {
    vec3 albedo = texture(albedoMap,TexCoords).rgb;
    vec3 orm = texture(ormMap, TexCoords).rgb;
    float ao = orm.r;
    float ssao = texture(ssaoMap, TexCoords).r;
    ao = ao * ssao;
    float roughness = orm.g;
    roughness = max(0.04, roughness); // at lower roughness, specular highlights are broken

    float metallic = orm.b;
    vec3 emissive = texture(emissiveMap, TexCoords).rgb;

    float depth = texture(depthMap, TexCoords).r;
    //if (dot(N, V) < 0.0) N = -N; // flip normal if it points away from camera - turns black artifacts into grey TODO check
    if (depth == 1.0) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0);
        return;
    }

    vec3 FragPos = ReconstructPosition(TexCoords, depth);

    vec3 N = normalize(texture(normalMap, TexCoords).rgb);
    vec3 V = normalize(camera.position.xyz - FragPos);


    // F0: base reflectance
    // dielectrics: 0.04, metals: tinted by albedo
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // accumulate lights
    vec3 Lo = vec3(0.0);
    float shadow = 0.0;
    if (Config.shadowsEnabled == 1) {
        shadow = ShadowDirectionalLight(FragPos, N, Shadow.dirLightSpaceMatrix, Config.dirShadowBiasMin, Config.dirShadowBiasMax);
    }
    Lo += CalcDirectionalLight(N, V, albedo, metallic, roughness, F0) * (1.0 - shadow);

    int pointCount = pointLights.count.x;
    for (int i = 0; i < pointCount; i++) {
        float shadow = 0.0;
        if (i < Config.maxPointShadowCasters && Config.shadowsEnabled == 1) {
            vec3 lightPos = pointLights.lights[i].positionAndRange.xyz;
            float range = pointLights.lights[i].positionAndRange.w;
            shadow = ShadowPointLight(FragPos, lightPos, range, Config.pointShadowBias, i);
        }

        Lo += CalcPointLight(pointLights.lights[i], N, V, FragPos,
                             albedo, metallic, roughness, F0) * (1.0 - shadow);
    }

    int spotCount = spotLights.count.x;
    for (int i = 0; i < spotCount; i++) {
        float shadow = 0.0;
        if (i < Config.maxSpotShadowCasters && Config.shadowsEnabled == 1) {
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
    FragColor = vec4(color, 1.0);
}
