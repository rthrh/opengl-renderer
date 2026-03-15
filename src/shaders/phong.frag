#version 420 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

layout(std140, binding = 0) uniform DirectionalLightBlock
{
    vec4 direction;
    vec4 colorAndIntensity; // rgb - color, a - intensity
};


uniform vec3 viewPos;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

vec3 CalcDirectionalLight();

void main()
{      
    vec3 color = CalcDirectionalLight();
    FragColor = vec4(color, 1.0);
}


vec3 CalcDirectionalLight()
{
    vec3 texColor = texture(diffuseTexture, TexCoords).rgb;
    vec3 specMap = texture(specularTexture, TexCoords).rgb;
    vec3 lightColor = colorAndIntensity.rgb;
    float intensity = colorAndIntensity.a;

    // ambient
    vec3 ambient = 0.10 * texColor; // TODO multiply by light color??

    // diffuse
    vec3 lightDir = normalize(direction.xyz);
    vec3 normal = normalize(Normal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = lightColor * diff * texColor;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    vec3 specular = specMap * spec;
    return (ambient + diffuse + specular);
}
