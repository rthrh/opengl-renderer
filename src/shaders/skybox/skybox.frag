#version 420 core
out vec4 FragColor;
out vec4 BrightColor;

in vec3 WorldPos;

layout(binding = 6) uniform samplerCube environmentMap;

void main()
{		
    vec3 envColor = texture(environmentMap, WorldPos).rgb;
    
    // HDR tonemap and gamma correct TODO its done in bloom atm
    //envColor = envColor / (envColor + vec3(1.0));
    //envColor = pow(envColor, vec3(1.0/2.2)); 
    
    FragColor = vec4(envColor, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0); // no bloom for skybox
}
