#version 450 core

out vec4 FragColor;
out vec4 BrightColor;

in vec2 TexCoords;

//gbuffer                                           // TODO target formats
//TODO positionMap can be reconstructed from depthMap
layout(binding = 0) uniform sampler2D positionMap;  // RGB16F
layout(binding = 1) uniform sampler2D albedoMap;    // RGB8
layout(binding = 2) uniform sampler2D normalMap;    // RGB16F
layout(binding = 3) uniform sampler2D ormMap;       // RGB8  r=ao g=roughness b=metallic
layout(binding = 4) uniform sampler2D emissiveMap;  // RGB16F
layout(binding = 5) uniform sampler2D depthMap;     // DEPTH24_STENCIL8

layout(binding = 7) uniform sampler2D shadowDirMap;
layout(binding = 8) uniform samplerCubeArray shadowPointMaps;
layout(binding = 9) uniform sampler2DArray shadowSpotMap;

uniform float farPlane;

#include "common/ubo.glsl"
#include "common/brdf.glsl"
#include "common/pbr_lights.glsl"
#include "common/shadows.glsl"

void main() {
    vec3 albedo = texture(albedoMap,TexCoords).rgb;
    vec3 orm = texture(ormMap, TexCoords).rgb;
    float ao = orm.r;
    float roughness = orm.g;
    float metallic = orm.b;
    vec3 emissive = texture(emissiveMap, TexCoords).rgb;
    vec3 FragPos = texture(positionMap, TexCoords).rgb;

    vec3 N = normalize(texture(normalMap, TexCoords).rgb);
    vec3 V = normalize(camera.position.xyz - FragPos);
    //if (dot(N, V) < 0.0) N = -N; // flip normal if it points away from camera - turns black artifacts into grey TODO check
    float depth = texture(depthMap, TexCoords).r;
    if (depth == 1.0) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0); // skybox should not be lit
        return;
    }

    // F0: base reflectance
    // dielectrics: 0.04, metals: tinted by albedo
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // accumulate lights
    vec3 Lo = vec3(0.0);
    float shadow = ShadowDirectionalLight(FragPos, N, Shadow.dirLightSpaceMatrix);
    Lo += CalcDirectionalLight(N, V, albedo, metallic, roughness, F0) * (1.0 - shadow);

    int pointCount = pointLights.count.x;
    for (int i = 0; i < pointCount; i++) {
        float shadow = 0.0;
        if (i < 4) { // TODO max 4 shadow casters
            vec3 lightPos = pointLights.lights[i].positionAndRange.xyz;
            shadow = ShadowPointLight(FragPos, lightPos, i);  
        }

        Lo += CalcPointLight(pointLights.lights[i], N, V, FragPos,
                             albedo, metallic, roughness, F0) * (1.0 - shadow);
    }

    int spotCount = spotLights.count.x;
    for (int i = 0; i < spotCount; i++) {
        float shadow = ShadowSpotLight(FragPos, N, Shadow.spotLightSpaceMatrices[i], i);
        Lo += CalcSpotLight(spotLights.lights[i], N, V, FragPos,
                            albedo, metallic, roughness, F0) * (1.0 - shadow);
    }
    // ambient + AO
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo + emissive;
    FragColor = vec4(color, 1.0);

    // For Bloom pass and tone mapping + gamma
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);


}

