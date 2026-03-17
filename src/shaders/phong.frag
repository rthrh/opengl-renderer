#version 420 core

#define SHININESS 32.0

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

layout(std140, binding = 0) uniform DirectionalLightBlock {
    vec4 direction;
    vec4 colorAndIntensity; // rgb = color, a = intensity
};

struct PointLight {
    vec4  positionAndRange; // xyz = position, w = range
    vec4  colorAndIntensity; // rgb = color, a = intensity
};

layout(std140, binding = 1) uniform PointLightBlock {
    ivec4    count; // x - count
    PointLight lights[16];
} pointLights;



struct SpotLight {
    vec4  position;         // xyz = pos, w = unused
    vec4  direction;        // xyz = dir, w = unused
    vec4  colorAndIntensity; // rgb = color, a = intensity
    float range;
    float innerCone;        // cos(innerAngle)
    float outerCone;        // cos(outerAngle)
    float _pad;
};

layout(std140, binding = 2) uniform SpotLightBlock {
    ivec4 count; // x = count
    SpotLight lights[16];
} spotLights;



uniform vec3 viewPos;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D emissiveTexture;
layout(binding = 3) uniform sampler2D specularTexture;

vec3 CalcDirectionalLight(vec3 normal, vec3 diffuseSample, vec3 specularSample, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 diffuseSample, vec3 specularSample, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 diffuseSample, vec3 specularSample, vec3 fragPos, vec3 viewDir);

void main() {
    // properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // samples
    vec3 normalSample = texture(normalTexture, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);
    vec3 diffuseSample = texture(diffuseTexture,  TexCoords).rgb;
    vec3 specularSample  = texture(specularTexture, TexCoords).rgb;

    // calc lights
    vec3 directionalLight = CalcDirectionalLight(N, diffuseSample, specularSample, viewDir);

    vec3 pointlight = vec3(0.0);
    int pointCount = pointLights.count.x;
    for(int i = 0; i < pointCount; i++)
        pointlight += CalcPointLight(pointLights.lights[i], N, diffuseSample, specularSample, FragPos, viewDir);

    vec3 spotlight = vec3(0.0);
    int spotCount = spotLights.count.x;
    for(int i = 0; i < spotCount; i++)
        spotlight += CalcSpotLight(spotLights.lights[i], N, diffuseSample, specularSample, FragPos, viewDir);

    vec3 emissive = texture(emissiveTexture, TexCoords).rgb;
    vec3 result = directionalLight + pointlight + spotlight + emissive;

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirectionalLight(vec3 N, vec3 diffuseSample, vec3 specSample, vec3 viewDir) {
    vec3  lightColor = colorAndIntensity.rgb;
    float intensity  = colorAndIntensity.a;
    vec3  lightDir   = normalize(direction.xyz);

    // blinn-phong
    vec3 H = normalize(lightDir + viewDir);

    float diff = max(dot(N, lightDir), 0.0);
    float spec = pow(max(dot(N, H), 0.0), SHININESS);

    vec3 ambient  = 0.10 * lightColor * diffuseSample;
    vec3 diffuse  = lightColor * intensity * diff * diffuseSample;
    vec3 specular = lightColor * intensity * spec * specSample;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 diffuseSample, vec3 specSample, vec3 fragPos, vec3 viewDir)
{
    vec3  lightColor = light.colorAndIntensity.rgb;
    float intensity  = light.colorAndIntensity.a;
    vec3  position = light.positionAndRange.xyz;
    float range = light.positionAndRange.w;

    vec3 lightDir = normalize(position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);
    // attenuation
    float distance = length(position - fragPos);
    float attenuation = clamp(1.0 - (distance / range), 0.0, 1.0);
    attenuation = attenuation * attenuation; // square for more natural curve
    // combine results
    vec3 ambient = attenuation * lightColor * intensity * diffuseSample;
    vec3 diffuse = attenuation * lightColor * intensity * diff * diffuseSample;
    vec3 specular = attenuation * lightColor * intensity * spec * specSample;

    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 diffuseSample, vec3 specSample, vec3 fragPos, vec3 viewDir)
{
    vec3  lightColor = light.colorAndIntensity.rgb;
    float intensity  = light.colorAndIntensity.a;
    vec3  position = light.position.xyz;
    float range = light.range;

    vec3 lightDir = normalize(position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);
    // attenuation
    float distance = length(position - fragPos);
    float attenuation = clamp(1.0 - (distance / range), 0.0, 1.0);
    attenuation = attenuation * attenuation; // square for more natural curve
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction.xyz)); 
    float epsilon = light.innerCone - light.outerCone;
    float spotFactor = clamp((theta - light.outerCone) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = attenuation * lightColor * intensity * spotFactor * diffuseSample;
    vec3 diffuse = attenuation * lightColor * intensity * diff * spotFactor * diffuseSample;
    vec3 specular = attenuation * lightColor * intensity * spec * spotFactor * specSample;

    return (ambient + diffuse + specular);
}