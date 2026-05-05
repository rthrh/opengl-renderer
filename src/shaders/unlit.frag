#version 420 core
out vec4 FragColor;
out vec4 BrightColor;

uniform vec3 color;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    //BrightColor = vec4(0.0, 0.0, 0.0, 1.0); // enable disable bloom

}
