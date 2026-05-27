
// Material SSBO
struct Material {
    vec4  baseColorFactor;
    vec4  emissiveFactor;
    uvec4 textureHandles; // unused here
    float normalScale;
    float occlusionStrength;
    float metallicFactor;
    float roughnessFactor;
    int   alphaMode;
    float alphaCutoff;
    int   doubleSided;
    float _pad;
};

// GLES treats Material SSBO as UBO
#ifdef GLES
    const int MAX_MATERIALS = 256;
    layout(std140) uniform MaterialBuffer {
        Material materials[MAX_MATERIALS];
    };
#else
    layout(std140, binding = 10) readonly buffer MaterialBuffer {
        Material materials[];
    };
#endif