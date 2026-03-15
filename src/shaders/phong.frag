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

uniform vec3 viewPos;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D emissiveTexture;
layout(binding = 3) uniform sampler2D specularTexture;

vec3 CalcDirectionalLight();

void main() {
    FragColor = vec4(CalcDirectionalLight(), 1.0);
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