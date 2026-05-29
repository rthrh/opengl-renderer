#version 450

flat in vec3 vColor;
flat in float vIntensity;

out vec4 FragColor;

void main() {
    //FragColor = vec4(min(vColor * vIntensity, vec3(1.0)), 1.0);
    FragColor = vec4(vColor, 1.0);
}
