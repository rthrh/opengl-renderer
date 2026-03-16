#version 420 core

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

vec3 CalcDirectionalLight();
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    // properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec4 dirLight = vec4(CalcDirectionalLight(), 1.0);
    vec3 pointlight = vec3(0.0);

    int count = pointLights.count.x;
    for(int i = 0; i < count; i++)
        pointlight += CalcPointLight(pointLights.lights[i], norm, FragPos, viewDir);

    vec4 result = dirLight + vec4(pointlight, 1.0);
    FragColor = vec4(result);
}

vec3 CalcDirectionalLight() {
    vec3 texColor = texture(diffuseTexture,  TexCoords).rgb;
    vec3 specMap  = texture(specularTexture, TexCoords).rgb;
    vec3 emissive = texture(emissiveTexture, TexCoords).rgb;

    // normal map — N used everywhere below
    vec3 normalSample = texture(normalTexture, TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);

    vec3  lightColor = colorAndIntensity.rgb;
    float intensity  = colorAndIntensity.a;
    vec3  lightDir   = normalize(direction.xyz);

    // blinn-phong
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(lightDir + V);

    float diff = max(dot(N, lightDir), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0);

    vec3 ambient  = 0.10 * lightColor * texColor;
    vec3 diffuse  = lightColor * intensity * diff * texColor;
    vec3 specular = lightColor * intensity * spec * specMap;

    return ambient + diffuse + specular + emissive;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    float constant = 1.0; float linear = 0.09f; float quadratic = 0.032;
    vec3 lightColor = light.colorAndIntensity.rgb;
    float intensity  = light.colorAndIntensity.a;
    vec3 position = light.positionAndRange.xyz;
    float range = light.positionAndRange.w;
    float shininess = 32.0;

    vec3 texColor = texture(diffuseTexture,  TexCoords).rgb;
    vec3 specMap  = texture(specularTexture, TexCoords).rgb;

    vec3 lightDir = normalize(position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // attenuation
    float distance = length(position - fragPos);
    //float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
    float attenuation = clamp(1.0 - (distance / range), 0.0, 1.0);
    // combine results
    vec3 ambient = attenuation * lightColor * intensity * texColor;
    vec3 diffuse = attenuation * lightColor * intensity * diff * texColor;
    vec3 specular = attenuation * lightColor * intensity * spec * specMap;
    return (ambient + diffuse + specular);
}