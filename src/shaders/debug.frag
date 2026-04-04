#version 450 core

out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gMaterial;
layout(binding = 4) uniform sampler2D gDepth;

float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far - near - z * (far - near));
}

void main() {

        FragColor = vec4(texture(gPosition, TexCoords).rgb, 1.0);
        FragColor = vec4(texture(gNormal, TexCoords).rgb * 0.5 + 0.5, 1.0); // remap [-1,1] to [0,1]
        //FragColor = vec4(texture(gAlbedo, TexCoords).rgb, 1.0);
        //FragColor = vec4(texture(gMaterial, TexCoords).rgb, 1.0);
        //FragColor = vec4(texture(gMaterial, TexCoords).rgb, 1.0);
        float depth = texture(gDepth, TexCoords).r;
        float linear = linearizeDepth(depth, 0.1, 100.0) / 100.0; // normalize to [0,1]
        FragColor = vec4(vec3(linear), 1.0);
}