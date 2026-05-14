#pragma once

#include <glad/glad.h>

enum class SlotGeometry : GLuint {
    Albedo = 0,
    Normal = 1,
    Emissive = 2,
    ORM = 3,
};

enum class SlotDeferred : GLuint {
    Albedo = 0,
    Normal = 1,
    ORM = 2,
    Emissive = 3,
    Depth = 4
};

enum class SlotOther : GLuint {
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
