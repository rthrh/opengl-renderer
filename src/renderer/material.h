#pragma once

#include <glm/glm.hpp>
#include "gl/texture.h"

#include <string>
#include <memory>

#include "gl/texture.h"
#include "texture_cache.h"

enum class AlphaMode { Opaque, Mask, Blend };

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#schema-reference-material
struct Material {

    // Helper to init texture handles to valid dummy textures
    static Material Default(const std::shared_ptr<TextureCache>& textureCache) {
        Material m;
        m.baseColorTexture = textureCache->GetDummyTexture(TextureType::Albedo).id;
        m.normalTexture    = textureCache->GetDummyTexture(TextureType::Normal).id;
        m.emissiveTexture  = textureCache->GetDummyTexture(TextureType::Emissive).id;
        m.ormTexture       = textureCache->GetDummyTexture(TextureType::ORM).id;
        return m;
    }

    glm::vec4 baseColorFactor {1.0f};
    glm::vec4 emissiveFactor {1.0f}; // w = unused

    GLuint baseColorTexture = 0;
    GLuint normalTexture = 0;
    GLuint emissiveTexture = 0;
    GLuint ormTexture = 0; // r = ambient occlusion, g = roughness, b = metallic

    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    int doubleSided = false;
};
