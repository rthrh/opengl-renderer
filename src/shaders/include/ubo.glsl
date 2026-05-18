
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

layout(std140, binding = 4) uniform ShadowBlock {
    mat4 dirLightSpaceMatrix;
    mat4 spotLightSpaceMatrices[4]; // TODO 4 MAX move it
} Shadow;

layout(std140, binding = 5) uniform ConfigBlock {
    int   bloomEnabled;
    float exposure;
    float gamma;
    float bloomStrength;

    int   ssaoEnabled;
    float ssaoRadius;
    float ssaoBias;
    int   ssaoKernel;

    int   shadowsEnabled;
    int   maxPointShadowCasters;
    float pointShadowBias;

    float dirShadowBiasMin;
    float dirShadowBiasMax;
    int   maxSpotShadowCasters;
    float spotShadowBiasMin;

    float spotShadowBiasMax;
    float maxReflectionLOD;
    float _pad0;
    float _pad1;
    float _pad2;
} Config;