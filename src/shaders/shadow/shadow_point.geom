#version 420 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in  vec3 FragPos[];
out vec4 FragPos_out;

uniform mat4 shadowMatrices[6];
uniform int lightIndex;


void main() {
    for (int face = 0; face < 6; face++) {
        gl_Layer = lightIndex * 6 + face;
        for (int i = 0; i < 3; i++) {
            FragPos_out = vec4(FragPos[i], 1.0);
            gl_Position = shadowMatrices[face] * vec4(FragPos[i], 1.0);
            EmitVertex();
        }
        EndPrimitive();
    }
}
