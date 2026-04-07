#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

layout(std140, binding = 0) uniform DirectionalLightBlock {
    vec4 direction;
    vec4 colorAndIntensity; // rgb = color, a = intensity
};

struct PointLight {
    vec4 positionAndRange;  // xyz = position, w = range
    vec4 colorAndIntensity; // rgb = color, a = intensity
};

layout(std140, binding = 1) uniform PointLightBlock {
    ivec4      count;       // x = count
    PointLight lights[16];
} pointLights;

struct SpotLight {
    vec4  position;          // xyz = pos,   w = unused
    vec4  direction;         // xyz = dir,   w = unused
    vec4  colorAndIntensity; // rgb = color, a = intensity
    float range;
    float innerCone;         // cos(innerAngle)
    float outerCone;         // cos(outerAngle)
    float _pad;
};

layout(std140, binding = 2) uniform SpotLightBlock {
    ivec4     count;         // x = count
    SpotLight lights[16];
} spotLights;

uniform vec3 viewPos;

//gbuffer
//layout(binding = 0) uniform sampler2D albedoMap;
//layout(binding = 1) uniform sampler2D normalMap;
//layout(binding = 2) uniform sampler2D emissiveMap;
//layout(binding = 3) uniform sampler2D metallicMap;
//layout(binding = 4) uniform sampler2D roughnessMap;
//layout(binding = 5) uniform sampler2D aoMap;

layout(binding = 0) uniform sampler2D positionMap;  // RGB16F
layout(binding = 1) uniform sampler2D albedoMap;    // RGB8
layout(binding = 2) uniform sampler2D normalMap;    // RGB16F
layout(binding = 3) uniform sampler2D ormMap;       // RGB8  r=ao g=roughness b=metallic
layout(binding = 4) uniform sampler2D emissiveMap;  // RGB16F
layout(binding = 5) uniform sampler2D depthMap;     // DEPTH24_STENCIL8

const float PI = 3.14159265359;

float D_GGX(float NdotH, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float G_SchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

// Core BRDF — shared by all three light types
// Takes pre-computed L (toward light) and radiance (light color * intensity * attenuation)
vec3 CalcPBR(vec3 N, vec3 V, vec3 L,
             vec3 albedo, float metallic, float roughness,
             vec3 F0, vec3 radiance) {

    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float D = D_GGX(NdotH, roughness);
    vec3  F = F_Schlick(HdotV, F0);
    float G = G_Smith(NdotV, NdotL, roughness);

    vec3 specular = (D * F * G) / max(4.0 * NdotV * NdotL, 0.001);

    vec3 kD      = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

// Light calculation functions
vec3 CalcDirectionalLight(vec3 N, vec3 V,
                          vec3 albedo, float metallic, float roughness, vec3 F0);

vec3 CalcPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos,
                    vec3 albedo, float metallic, float roughness, vec3 F0);

vec3 CalcSpotLight(SpotLight light, vec3 N, vec3 V, vec3 fragPos,
                   vec3 albedo, float metallic, float roughness, vec3 F0);

void main() {
    vec3  albedo    = texture(albedoMap,    TexCoords).rgb;
    vec3  orm       = texture(ormMap,      TexCoords).rgb;
    float ao        = orm.r;
    float roughness = orm.g;
    float metallic  = orm.b;
    vec3  emissive  = texture(emissiveMap,  TexCoords).rgb;
    vec3 FragPos = texture(positionMap,  TexCoords).rgb;

    vec3 N = normalize(texture(normalMap, TexCoords).rgb); // TODO no TBN - shall be done in g buffer?
    vec3 V = normalize(viewPos - FragPos);
    //if (dot(N, V) < 0.0) N = -N; // flip normal if it points away from camera - turns black artifacts into grey TODO check
    float depth = texture(depthMap, TexCoords).r;
    if (depth == 1.0) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0);
        return;
    }

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


vec3 CalcDirectionalLight(vec3 N, vec3 V,
                          vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3  lightColor = colorAndIntensity.rgb;
    float intensity  = colorAndIntensity.a;
    vec3  L          = normalize(-direction.xyz); // toward light

    vec3 radiance = lightColor * intensity;

    return CalcPBR(N, V, L, albedo, metallic, roughness, F0, radiance);
}

vec3 CalcPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos,
                    vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3  lightColor = light.colorAndIntensity.rgb;
    float intensity  = light.colorAndIntensity.a;
    vec3  position   = light.positionAndRange.xyz;
    float range      = light.positionAndRange.w;

    vec3  L        = normalize(position - fragPos);
    float distance = length(position - fragPos);

    float attenuation = clamp(1.0 - (distance / range), 0.0, 1.0);
    attenuation = attenuation * attenuation;

    vec3 radiance = lightColor * intensity * attenuation;

    return CalcPBR(N, V, L, albedo, metallic, roughness, F0, radiance);
}

vec3 CalcSpotLight(SpotLight light, vec3 N, vec3 V, vec3 fragPos,
                   vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3  lightColor = light.colorAndIntensity.rgb;
    float intensity  = light.colorAndIntensity.a;
    vec3  position   = light.position.xyz;
    float range      = light.range;

    vec3  L        = normalize(position - fragPos);
    float distance = length(position - fragPos);

    float attenuation = clamp(1.0 - (distance / range), 0.0, 1.0);
    attenuation = attenuation * attenuation;

    float theta     = dot(L, normalize(-light.direction.xyz));
    float epsilon   = light.innerCone - light.outerCone;
    float spotFactor = clamp((theta - light.outerCone) / epsilon, 0.0, 1.0);

    vec3 radiance = lightColor * intensity * attenuation * spotFactor;

    return CalcPBR(N, V, L, albedo, metallic, roughness, F0, radiance);
}