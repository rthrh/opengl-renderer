#version 420 core
out vec4 FragColor;
in vec3 WorldPos;

layout(binding = 6) uniform samplerCube environmentMap;

void main()
{		
    vec3 envColor = texture(environmentMap, WorldPos).rgb;
    
    // HDR tonemap and gamma correct TODO its done in bloom atm
    //envColor = envColor / (envColor + vec3(1.0));
    //envColor = pow(envColor, vec3(1.0/2.2)); 
    
    FragColor = vec4(envColor, 1.0);
}
