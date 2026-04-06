#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

uniform mat4 model;

out vec2 TexCoords;
out mat3 TBN;
out vec3 Normal; // to remove?

void main() {
    // generates a fullscreen triangle from 3 vertices with no VBO
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 texcoords[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    TexCoords   = texcoords[gl_VertexID];
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);

    // TBN
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = aNormal;

    vec3 T = normalize(normalMatrix * aTangent);
    //vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 B = cross(aNormal, aTangent); //TODO broken handedness sign
    TBN = mat3(T, B, N);
}