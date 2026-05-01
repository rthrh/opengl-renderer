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


class Texture2D {
public:
    Texture2D(int width, int height, TextureFormat internalFormat, const void* data = nullptr, bool genMipMaps = false) : _width(width), _height(height), _format(internalFormat) {
        glCreateTextures(GL_TEXTURE_2D, 1, &_id);
        GLsizei mipLevels = genMipMaps ? std::floor(std::log2(std::max(width, height))) + 1 : 1;
        glTextureStorage2D(_id, mipLevels, (GLuint)internalFormat, width, height);

        this->Upload(data);
        this->SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            glGenerateTextureMipmap(_id);
        }
    }

    ~Texture2D() {
        this->Destroy();
    }

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& o) noexcept {
        _id = std::exchange(o._id, 0);
        _width = std::exchange(o._width, 0);
        _height = std::exchange(o._height, 0);
        _format = std::exchange(o._format, TextureFormat::RGB8);
    }

    Texture2D& operator=(Texture2D&& o) {
        if (this != &o) {
            this->Destroy();
            _id = std::exchange(o._id, 0);
            _width = std::exchange(o._width, 0);
            _height = std::exchange(o._height, 0);
            _format = std::exchange(o._format, TextureFormat::RGB8);
        }
        return *this;
    }

    void SetWrap(TextureWrap wrapS, TextureWrap wrapT) const {
        glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLuint)wrapS);
        glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLuint)wrapT);
    }

    void SetFilter(TextureFilter minFilter, TextureFilter magFilter) const {
        glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, (GLuint)minFilter);
        glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, (GLuint)magFilter);
    }

    void SetBorderColor(float r, float g, float b, float a) const {
        float color[] = {r, g, b, a};
        glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, color);
    }

    void Upload(const void* data) const {
        if (data) {
            auto channels = FormatToChannels(_format);
            auto dataType = FormatToDataType(_format);
            glTextureSubImage2D(_id, 0, 0, 0, _width, _height, (GLuint)channels, (GLuint)dataType, data);
        }
    }

    void Bind(GLuint slot) const {
        glBindTextureUnit(slot, _id);
    }

    GLuint GetID() const {
        return _id;
    }

    void Destroy() {
        if (_id) {
            glDeleteTextures(1, &_id);
            _id = 0;
        }
    }

private:
    GLuint _id = 0;
    int _width = 0;
    int _height = 0;
    TextureFormat _format {TextureFormat::RGB8};
};
