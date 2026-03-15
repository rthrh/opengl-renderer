#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
//layout (location = 4) in vec3 aBitangent;

out vec2 TexCoords;
out vec3 Normal; // to remove?
out vec3 FragPos;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aTexCoords;
    FragPos   = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = aNormal;

    vec3 T = normalize(normalMatrix * aTangent);
    //vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 B = cross(aNormal, aTangent); //TODO broken handedness sign
    TBN = mat3(T, B, N);

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}