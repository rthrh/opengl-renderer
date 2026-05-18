#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2D scene;
layout (binding = 1) uniform sampler2D bloomBlur;

#include "include/ubo.glsl"

void main() {
    vec3 hdrColor   = texture(scene,    TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    vec3 result;
    if (Config.bloomEnabled == 1) {
        result = mix(hdrColor, bloomColor, Config.bloomStrength);
    } else {
        result = hdrColor;
    }

    // Tonemapping
    result = vec3(1.0) - exp(-result * Config.exposure);
    result = pow(result, vec3(1.0 / Config.gamma));

    FragColor = vec4(result, 1.0);
}
