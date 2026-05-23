
// Material SSBO
struct Material {
    vec4  baseColorFactor;
    vec4  emissiveFactor;
    uvec4 textureHandles; // unused here
    float normalScale;
    float occlusionStrength;
    float metallicFactor;
    float roughnessFactor;
    int  alphaMode;
    float alphaCutoff;
    int  doubleSided;
    float _pad;
};

layout(std430, binding = 0) readonly buffer MaterialBuffer {
    Material materials[];
};

