
layout(std140, binding = 0) uniform DirectionalLightBlock {
    vec4 direction;
    vec4 colorAndIntensity; // rgb = color, a = intensity
} dirLight;

struct PointLight {
    vec4 positionAndRange;  // xyz = position, w = range
    vec4 colorAndIntensity; // rgb = color, a = intensity
};

layout(std140, binding = 1) uniform PointLightBlock {
    ivec4      count;       // x = count
    PointLight lights[16];
} pointLights;

struct SpotLight {
    vec4  position;          // xyz = pos,   w = unused
    vec4  direction;         // xyz = dir,   w = unused
    vec4  colorAndIntensity; // rgb = color, a = intensity
    float range;
    float innerCone;         // cos(innerAngle)
    float outerCone;         // cos(outerAngle)
    float _pad;
};

layout(std140, binding = 2) uniform SpotLightBlock {
    ivec4     count;         // x = count
    SpotLight lights[16];
} spotLights;

layout(std140, binding = 3) uniform CameraBlock {
    mat4 view;
    mat4 projection;
    vec4 position;   // xyz = camera world pos, w = unused
} camera;
