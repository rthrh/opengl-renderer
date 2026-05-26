
#ifdef GLES
    #define SAMPLER_BINDING(slot)
#else
    #define SAMPLER_BINDING(slot) layout(binding = slot)
#endif

SAMPLER_BINDING(0) uniform sampler2D albedoMap;    // RGB8
SAMPLER_BINDING(1) uniform sampler2D normalMap;    // RGB16F
SAMPLER_BINDING(2) uniform sampler2D ormMap;       // RGB8  r=ao g=roughness b=metallic
SAMPLER_BINDING(3) uniform sampler2D emissiveMap;  // RGB16F
SAMPLER_BINDING(4) uniform sampler2D depthMap;     // DEPTH24_STENCIL8
SAMPLER_BINDING(6) uniform samplerCube environmentMap; // skybox
SAMPLER_BINDING(7) uniform sampler2D shadowDirMap;
SAMPLER_BINDING(9) uniform sampler2DArray shadowSpotMap;
SAMPLER_BINDING(10) uniform samplerCube irradianceMap;
SAMPLER_BINDING(11) uniform samplerCube prefilteredMap;
SAMPLER_BINDING(12) uniform sampler2D brdfLUT;
SAMPLER_BINDING(13) uniform sampler2D ssaoMap;
SAMPLER_BINDING(14) uniform samplerCube shadowPointMap0;
SAMPLER_BINDING(15) uniform samplerCube shadowPointMap1;
SAMPLER_BINDING(16) uniform samplerCube shadowPointMap2;
SAMPLER_BINDING(17) uniform samplerCube shadowPointMap3;
