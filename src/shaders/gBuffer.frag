#version 450 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gDiffuse;
layout (location = 3) out vec3 gSpecular;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D texDiffuse;
uniform sampler2D texSpecular;
uniform sampler2D texNormal;

void main() {
    gPosition = FragPos;
    gNormal   = normalize(Normal);
    gDiffuse  = texture(texDiffuse, TexCoords).rgb;
    gSpecular = texture(texSpecular, TexCoords).rgb;
    FragColor = vec4(gSpecular, 1.0);
}