#pragma once

#include <glad/glad.h>
#include <utility>
#include <cmath>

enum class TextureFilter {
    Nearest = GL_NEAREST,
    Linear = GL_LINEAR,
};

enum class TextureWrap {
    Repeat = GL_REPEAT,
    MirroredRepeated = GL_MIRRORED_REPEAT,
    ClampToEdge = GL_CLAMP_TO_EDGE,
    ClampToBorder = GL_CLAMP_TO_BORDER
};

enum class TextureFormat {
    R8 = GL_R8,
    RG8 = GL_RG8,
    RGB8 = GL_RGB8,
    RGBA8 = GL_RGBA8,
    SRGB8 = GL_SRGB8,
    SRGB8_A8 = GL_SRGB8_ALPHA8,

    R16F = GL_R16F,
    RG16F = GL_RG16F,
    RGB16F = GL_RGB16F,
    RGBA16F = GL_RGBA16F,
    R32F = GL_R32F,
    RGB32F = GL_RGB32F,

    Depth24 = GL_DEPTH_COMPONENT24,
    Depth32F = GL_DEPTH_COMPONENT32F,
    Depth24Stencil8 = GL_DEPTH24_STENCIL8,
};

static GLenum FormatToChannels(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::R8:
        case TextureFormat::R16F:
        case TextureFormat::R32F:
        case TextureFormat::Depth24:
        case TextureFormat::Depth32F:
            return GL_RED;

        case TextureFormat::RG8:
        case TextureFormat::RG16F:
            return GL_RG;

        case TextureFormat::RGB8:
        case TextureFormat::RGB16F:
        case TextureFormat::RGB32F:
        case TextureFormat::SRGB8:
            return GL_RGB;

        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::SRGB8_A8:
            return GL_RGBA;

        case TextureFormat::Depth24Stencil8:
            return GL_DEPTH_STENCIL;

        default:
            return GL_RGB;
    }
};

static GLenum FormatToDataType(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::R8:
        case TextureFormat::RG8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:
        case TextureFormat::SRGB8:
        case TextureFormat::SRGB8_A8:
            return GL_UNSIGNED_BYTE;

        case TextureFormat::R16F:
        case TextureFormat::RG16F:
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:
        case TextureFormat::R32F:
        case TextureFormat::RGB32F:
            return GL_FLOAT;

        case TextureFormat::Depth24:
        case TextureFormat::Depth32F:
        case TextureFormat::Depth24Stencil8:
            return GL_FLOAT;

        default:
            return GL_UNSIGNED_BYTE;
    }
}
