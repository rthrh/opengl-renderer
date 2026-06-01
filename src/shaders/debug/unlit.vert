#version 450

#include "include/ubo.glsl"


const vec3 cubePositions[36] = vec3[36](
    // back face
    vec3(-0.5,-0.5,-0.5), vec3( 0.5, 0.5,-0.5), vec3( 0.5,-0.5,-0.5),
    vec3( 0.5, 0.5,-0.5), vec3(-0.5,-0.5,-0.5), vec3(-0.5, 0.5,-0.5),
    // front face
    vec3(-0.5,-0.5, 0.5), vec3( 0.5,-0.5, 0.5), vec3( 0.5, 0.5, 0.5),
    vec3( 0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5), vec3(-0.5,-0.5, 0.5),
    // left
    vec3(-0.5, 0.5, 0.5), vec3(-0.5, 0.5,-0.5), vec3(-0.5,-0.5,-0.5),
    vec3(-0.5,-0.5,-0.5), vec3(-0.5,-0.5, 0.5), vec3(-0.5, 0.5, 0.5),
    // right
    vec3( 0.5, 0.5, 0.5), vec3( 0.5,-0.5,-0.5), vec3( 0.5, 0.5,-0.5),
    vec3( 0.5,-0.5,-0.5), vec3( 0.5, 0.5, 0.5), vec3( 0.5,-0.5, 0.5),
    // bottom
    vec3(-0.5,-0.5,-0.5), vec3( 0.5,-0.5,-0.5), vec3( 0.5,-0.5, 0.5),
    vec3( 0.5,-0.5, 0.5), vec3(-0.5,-0.5, 0.5), vec3(-0.5,-0.5,-0.5),
    // top
    vec3(-0.5, 0.5,-0.5), vec3( 0.5, 0.5, 0.5), vec3( 0.5, 0.5,-0.5),
    vec3( 0.5, 0.5, 0.5), vec3(-0.5, 0.5,-0.5), vec3(-0.5, 0.5, 0.5)
);

flat out vec3 vColor;
flat out float vIntensity;

uniform int lightType;  // 0 = point, 1 = spot

void main() {
    vec3 lightPos;
    vec3 lightColor;
    float intensity;
    float cubeSize = 0.1;

    if (lightType == 0) {
        lightPos   = pointLights.lights[gl_InstanceID].positionAndRange.xyz;
        lightColor = pointLights.lights[gl_InstanceID].colorAndIntensity.rgb;
        intensity  = pointLights.lights[gl_InstanceID].colorAndIntensity.a;
    } else {
        lightPos   = spotLights.lights[gl_InstanceID].position.xyz;
        lightColor = spotLights.lights[gl_InstanceID].colorAndIntensity.rgb;
        intensity  = spotLights.lights[gl_InstanceID].colorAndIntensity.a;
    }

    vec3 worldPos = lightPos + cubePositions[gl_VertexID] * cubeSize;
    gl_Position = camera.projection * camera.view * vec4(worldPos, 1.0);
    vColor = lightColor;
    vIntensity = intensity;
}
