#version 450 core

out vec4 FragColor;
out vec4 BrightColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D emissiveMap;
layout(binding = 3) uniform sampler2D ormMap;

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

void main() {
    vec4  albedoSample = texture(albedoMap, TexCoords).rgba;
    vec3  albedo    = albedoSample.rgb;
    vec3  orm       = texture(ormMap, TexCoords).rgb;
    float ao        = orm.r;
    float roughness = orm.g;
    float metallic  = orm.b;
    roughness = max(0.04, roughness);

    vec3 emissive = texture(emissiveMap, TexCoords).rgb;

    vec3 normalSample = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);
    vec3 V = normalize(camera.position.xyz - FragPos);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // accumulate lights
    vec3 Lo = vec3(0.0);
    float shadow = ShadowDirectionalLight(FragPos, N, Shadow.dirLightSpaceMatrix, Config.dirShadowBiasMin, Config.dirShadowBiasMax);
    Lo += CalcDirectionalLight(N, V, albedo, metallic, roughness, F0) * (1.0 - shadow);

    int pointCount = pointLights.count.x;
    for (int i = 0; i < pointCount; i++) {
        float shadow = 0.0;
        if (i < 4) {
            vec3 lightPos = pointLights.lights[i].positionAndRange.xyz;
            shadow = ShadowPointLight(FragPos, lightPos, Config.pointShadowFarPlane, Config.pointShadowBias, i);
        }
        Lo += CalcPointLight(pointLights.lights[i], N, V, FragPos,
                             albedo, metallic, roughness, F0) * (1.0 - shadow);
    }

    int spotCount = spotLights.count.x;
    for (int i = 0; i < spotCount; i++) {
        float shadow = ShadowSpotLight(FragPos, N, Shadow.spotLightSpaceMatrices[i], Config.spotShadowBiasMin, Config.spotShadowBiasMax, i);
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
    FragColor = vec4(color, albedoSample.a); // any(isnan(color)) TODO NaN sometimes on FragColor

    // For Bloom pass and tone mapping + gamma
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > Config.brightnessThreshold)
        BrightColor = vec4(FragColor.rgb, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
