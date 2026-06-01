
#ifdef GLES
    #define UBO_BINDING(slot) layout(std140)
#else
    #define UBO_BINDING(slot) layout(std140, binding = slot)
#endif


UBO_BINDING(0) uniform DirectionalLightBlock {
    vec4 direction;
    vec4 colorAndIntensity; // rgb = color, a = intensity
} dirLight;

struct PointLight {
    vec4 positionAndRange;  // xyz = position, w = range
    vec4 colorAndIntensity; // rgb = color, a = intensity
};

UBO_BINDING(1) uniform PointLightBlock {
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

UBO_BINDING(2) uniform SpotLightBlock {
    ivec4     count;         // x = count
    SpotLight lights[16];
} spotLights;

UBO_BINDING(3) uniform CameraBlock {
    mat4 view;
    mat4 projection;
    vec4 position;   // xyz = camera world pos, w = unused
} camera;

UBO_BINDING(4) uniform ShadowBlock {
    mat4 dirLightSpaceMatrix;
    mat4 spotLightSpaceMatrices[4]; // TODO 4 MAX move it
} Shadow;

UBO_BINDING(5) uniform ConfigBlock {
    bool  lightCubesEnabled;

    bool  bloomEnabled;
    float exposure;
    float gamma;
    float bloomStrength;

    bool  ssaoEnabled;
    float ssaoRadius;
    float ssaoBias;
    int   ssaoKernel;

    bool  dirShadowsEnabled;
    bool  pointShadowsEnabled;
    bool  spotShadowsEnabled;

    float pointShadowBiasMin;
    float pointShadowBiasMax;

    float dirShadowBiasMin;
    float dirShadowBiasMax;

    float spotShadowBiasMin;
    float spotShadowBiasMax;

    bool  fxaaEnable;
    float fxaaEdgeThresholdMin;
    float fxaaEdgeThresholdMax;
    float fxaaSubpixelQuality;

    float _pad0;
    float _pad1;
} Config;

#define MAX_POINT_SHADOW_CASTERS 4
#define MAX_SPOT_SHADOW_CASTERS 4
