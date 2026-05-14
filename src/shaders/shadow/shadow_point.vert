#version 420 core
layout(location = 0) in vec3 aPos;
layout(location = 4) in mat4 aInstanceMatrix;

out vec3 FragPos;

uniform mat4 lightSpaceMatrix;

void main() {
    FragPos     = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    gl_Position = lightSpaceMatrix * vec4(FragPos, 1.0);
}
