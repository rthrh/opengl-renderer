#pragma once

#include <gl/headers.h>
#include "gl/dsa_config.h"

// Should match sampler bindings in shader files
enum class TextureSlot : GLuint {
    Albedo = 0,
    Normal = 1,
    ORM = 2,
    Emissive = 3,
    Depth = 4,
    Skybox = 6,
    ShadowDirectional = 7,
    ShadowPoint = 8,
    ShadowSpot = 9,
    Irradiance = 10,
    PrefilterEnv = 11,
    BrdfLUT = 12,
    SSAO = 13,
    ShadowPoint0 = 14,
    ShadowPoint1 = 15,
    ShadowPoint2 = 16,
    ShadowPoint3 = 17,
};

// Helper to get GLuint from slot enum
template<class T>
GLuint slot(T t) {
    return static_cast<GLuint>(t);
}




// Initialize samplers for non-DSA opengl path
//TODO would be nice if it was set only for shaders that need it, not for every of them
void InitSamplers(const Shader& shader) {
    #ifdef USE_GL_DSA
        using enum TextureSlot;
        shader.Activate();
        shader.SetInt("albedoMap",       slot(Albedo));
        shader.SetInt("normalMap",       slot(Normal));
        shader.SetInt("ormMap",          slot(ORM));
        shader.SetInt("emissiveMap",     slot(Emissive));
        shader.SetInt("depthMap",        slot(Depth));
        shader.SetInt("environmentMap",  slot(Skybox));
        shader.SetInt("shadowDirMap",    slot(ShadowDirectional));
        shader.SetInt("shadowPointMaps", slot(ShadowPoint));
        shader.SetInt("shadowSpotMap",   slot(ShadowSpot));
        shader.SetInt("irradianceMap",   slot(Irradiance));
        shader.SetInt("prefilteredMap",  slot(PrefilterEnv));
        shader.SetInt("brdfLUT",         slot(BrdfLUT));
        shader.SetInt("ssaoMap",         slot(SSAO));

        shader.SetInt("screenTexture", 0); //fxaa

    #endif
}


// Initialize UBO bindings for non-DSA opengl path
void InitUboBindings(const Shader& shader) {
    #ifndef USE_GL_DSA
        using enum TextureSlot;
        shader.Activate();
        shader.SetUboBinding("DirectionalLightBlock", 0);
        shader.SetUboBinding("PointLightBlock", 1);
        shader.SetUboBinding("SpotLightBlock", 2);
        shader.SetUboBinding("CameraBlock", 3);
        shader.SetUboBinding("ShadowBlock", 4);
        shader.SetUboBinding("ConfigBlock", 5);
        shader.SetUboBinding("MaterialBuffer", 10);

    #endif
}

void InitBindings(const Shader& shader) {
    InitSamplers(shader);
    InitUboBindings(shader);
}
