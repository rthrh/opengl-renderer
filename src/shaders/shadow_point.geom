#version 420 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 shadowMatrices[6];

in  vec3 FragPos[];  // ← read world-space pos from vertex shader (vec3, matches vert output)
out vec4 FragPos_out; // ← send to fragment shader as vec4

void main() {
    for (int face = 0; face < 6; face++) {
        gl_Layer = face;
        for (int i = 0; i < 3; i++) {
            FragPos_out = vec4(FragPos[i], 1.0); // ← world-space position
            gl_Position = shadowMatrices[face] * vec4(FragPos[i], 1.0);
            EmitVertex();
        }
        EndPrimitive();
    }
}