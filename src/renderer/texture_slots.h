#pragma once

#include <glad/glad.h>

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
