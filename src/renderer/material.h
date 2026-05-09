#pragma once

#include <glm/glm.hpp>
#include "gl/texture.h"

#include <string>
#include "gl/texture.h"
#include "texture_cache.h"

enum class AlphaMode { Opaque, Mask, Blend };


// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#schema-reference-material
struct Material {
    TextureHandle baseColorTexture;
    glm::vec4 baseColorFactor {1.0f};

    TextureHandle normalTexture;
    float normalScale = 1.0f;

    TextureHandle emissiveTexture;
    glm::vec3 emissiveFactor {1.0f};
    TextureHandle ormTexture; // r = ambient occlusion, g = roughness, b = metallic
    float occlusionStrength = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
};
