// std140 array rule: each element padded to 16-byte multiple
// MaterialGPU is 32 bytes — already compliant
struct MaterialGPU {
    vec4  albedoFactor;     // offset  0
    float metallicFactor;   // offset 16
    float roughnessFactor;  // offset 20
    float aoStrength;       // offset 24
    float _pad;             // offset 28
};                          // stride: 32

layout(std140, binding = 0) uniform MaterialBuffer {
    MaterialGPU materials[512]; // matches m_maxMaterials default
};

uniform int u_materialIndex;

MaterialGPU mat = materials[u_materialIndex];

// No branching — dummy textures handle missing maps
vec4  albedo    = texture(albedoMap,    v_uv) * mat.albedoFactor;
float metallic  = texture(metallicMap,  v_uv).r * mat.metallicFactor;
float roughness = texture(roughnessMap, v_uv).r * mat.roughnessFactor;
float ao        = texture(aoMap,        v_uv).r * mat.aoStrength;

vec3 normalSample = texture(normalMap, v_uv).rgb * 2.0 - 1.0;
vec3 normal       = normalize(v_TBN * normalSample);