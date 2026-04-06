#version 450 core

out vec4 FragColor;
in vec2 TexCoords;

// binding indices should match GBufferPbrTextureType enum
layout(binding = 0) uniform sampler2D gPosition;  // RGB16F
layout(binding = 1) uniform sampler2D gAlbedo;    // RGB8
layout(binding = 2) uniform sampler2D gNormal;    // RGB16F
layout(binding = 3) uniform sampler2D gORM;       // RGB8  r=ao g=roughness b=metallic
layout(binding = 4) uniform sampler2D gEmissive;  // RGB16F
layout(binding = 5) uniform sampler2D gDepth;     // DEPTH24_STENCIL8



float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far - near - z * (far - near));
}

void main() {
    int debug = 4;
    if (debug == 0) {
        // position — scaled down
        vec3 pos = texture(gPosition, TexCoords).rgb;
        FragColor = vec4(abs(pos) * 0.1, 1.0);
    }
    else if (debug == 1) {
        FragColor = vec4(texture(gAlbedo, TexCoords).rgb, 1.0);
    }
    else if (debug == 2) {
        vec3 N = texture(gNormal, TexCoords).rgb;
        FragColor = vec4(N * 0.5 + 0.5, 1.0);
    }
    else if (debug == 3) {
        float ao = texture(gORM, TexCoords).r;
        FragColor = vec4(vec3(ao), 1.0);
    }
    else if (debug == 4) {
        float roughness = texture(gORM, TexCoords).g;
        FragColor = vec4(vec3(roughness), 1.0);
    }
    else if (debug == 5) {
        float metallic = texture(gORM, TexCoords).b;
        FragColor = vec4(vec3(metallic), 1.0);
    }
    else if (debug == 6) {
        vec3 emissive = texture(gEmissive, TexCoords).rgb;
        FragColor = vec4(emissive, 1.0);
    }
    else if (debug == 7) {
        float depth  = texture(gDepth, TexCoords).r;
        float linear = linearizeDepth(depth, 0.1, 100.0) / 100.0;
        FragColor = vec4(vec3(linear), 1.0);
    }
}