#pragma once

#include <glad/glad.h>

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
};

// Helper to get GLuint from slot enum
template<class T>
GLuint slot(T t) {
    return static_cast<GLuint>(t);
}

//Initialized samplers for non-DSA opengl path
void InitSamplers(const Shader& shader) {
    if constexpr (! USE_DSA) {
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
    }
}
