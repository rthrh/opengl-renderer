#version 420 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D scene;
layout(binding = 1) uniform sampler2D bloomBlur;

#include "include/ubo.glsl"

void main()
{             
    const float gamma = Config.gamma;
    const float exposure = Config.exposure;
    const int bloom = Config.bloomEnabled;

    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    if(bloom == 1)
        hdrColor += bloomColor; // additive blending

    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}
