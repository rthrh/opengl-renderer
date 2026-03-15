#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>




struct MaterialSlots {
    GLuint albedoMap    = 0;
    GLuint normalMap    = 0;
    GLuint emissiveMap    = 0;
    GLuint metallicMap  = 0;
    GLuint roughnessMap = 0;
    GLuint aoMap        = 0;

    inline static GLuint whiteDummy      = 0;
    inline static GLuint blackDummy      = 0;
    inline static GLuint flatNormalDummy = 0;

    enum TextureType : GLint
    {
        Albedo,
        Normal,
        Emissive,
        Metallic,
        Roughness,
        AO
    };

    void bind() const {
        bindSlot(Albedo,    albedoMap,    whiteDummy);
        bindSlot(Normal,    normalMap,    flatNormalDummy);
        bindSlot(Emissive,  emissiveMap,  blackDummy);
        bindSlot(Metallic,  metallicMap,  whiteDummy);
        bindSlot(Roughness, roughnessMap, whiteDummy);
        bindSlot(AO,        aoMap,        whiteDummy);
    }

    void initDummies() {
        uint8_t white[]      = {255, 255, 255, 255};
        uint8_t black[]      = {0, 0, 0, 0};
        uint8_t flatNormal[] = {128, 128, 255, 255};
        whiteDummy      = createDummy(white);
        blackDummy      = createDummy(black);
        flatNormalDummy = createDummy(flatNormal);
    }

private:
    static void bindSlot(GLint unit, GLuint tex, GLuint fallback) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex != 0 ? tex : fallback);
    }

    static GLuint createDummy(uint8_t rgba[4]) {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return tex;
    }
};

struct alignas(16) MaterialUBO {
    glm::vec4 albedoFactor{1.0f};
    float     metallicFactor{1.0f};
    float     roughnessFactor{1.0f};
    float     aoStrength{1.0f};
    float     _pad{0.0f};
};
