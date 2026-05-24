#pragma once

#include <glad/glad.h>
#include <bit>
#include <utility>
#include <span>

#include "dsa_config.h"

enum class TextureFilter {
    Nearest = GL_NEAREST,
    Linear = GL_LINEAR,
    LinearMipMapLinear = GL_LINEAR_MIPMAP_LINEAR,
    LinearMipMapNearest = GL_LINEAR_MIPMAP_NEAREST
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
    R11F_G11F_B10F = GL_R11F_G11F_B10F,
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
        case TextureFormat::R11F_G11F_B10F:
            return GL_RGB;
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::SRGB8_A8:
            return GL_RGBA;
        case TextureFormat::Depth24Stencil8:
            return GL_DEPTH_STENCIL;
    }
    std::unreachable();
}

static GLenum FormatToDataType(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::R8:
        case TextureFormat::RG8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:
        case TextureFormat::SRGB8:
        case TextureFormat::SRGB8_A8:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::Depth24Stencil8:
            return GL_UNSIGNED_INT_24_8;
        default:
            return GL_FLOAT;
    }
}

template<GLenum T> concept Supported = (T == GL_TEXTURE_2D || T == GL_TEXTURE_2D_ARRAY || T == GL_TEXTURE_CUBE_MAP || T == GL_TEXTURE_CUBE_MAP_ARRAY);
template<GLenum T> concept Array = (T == GL_TEXTURE_2D_ARRAY || T == GL_TEXTURE_CUBE_MAP_ARRAY);
template<GLenum T> concept Cube = (T == GL_TEXTURE_CUBE_MAP || T == GL_TEXTURE_CUBE_MAP_ARRAY);

//TODO refactor data, genMipMaps
template<GLenum T> requires Supported<T>
class TextureGL {
public:
    // Default texture in invalid state
    TextureGL() = default;

    //TextureGL 2D
    TextureGL(int width, int height, TextureFormat internalFormat, const void* data = nullptr, bool genMipMaps = false)
        requires (!Array<T> && !Cube<T>) : _width(width), _height(height), _format(internalFormat)
    {
        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;
        if constexpr (USE_DSA) {
            glCreateTextures(GL_TEXTURE_2D, 1, &_id);
            glTextureStorage2D(_id, mipLevels, (GLuint)internalFormat, width, height);
        } else {
            glGenTextures(1, &_id);
            glBindTexture(GL_TEXTURE_2D, _id);
            glTexStorage2D(GL_TEXTURE_2D, mipLevels, (GLuint)internalFormat, width, height);
        }

        this->Upload(data);
        this->SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            if constexpr (USE_DSA) { glGenerateTextureMipmap(_id);
            } else { glGenerateMipmap(T); }
        }
    }

    // TextureGL 2D Array
    TextureGL(int width, int height, int layers, TextureFormat internalFormat, bool genMipMaps = false)
        requires (Array<T> && !Cube<T>) : _width(width), _height(height), _layers(layers), _format(internalFormat)
    {
        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;
        if constexpr (USE_DSA) {
            glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &_id);
            glTextureStorage3D(_id, mipLevels, (GLuint)internalFormat, width, height, layers);
        } else {
            glGenTextures(1, &_id);
            glBindTexture(GL_TEXTURE_2D_ARRAY, _id);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, (GLuint)internalFormat, width, height, layers);
        }

        this->SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            if constexpr (USE_DSA) { glGenerateTextureMipmap(_id);
            } else { glGenerateMipmap(T); } // TODO might need rebind
        }
    }

    // TextureGL Cube
    TextureGL(int size, TextureFormat internalFormat, bool genMipMaps = false)
        requires (!Array<T> && Cube<T>) : _width(size), _height(size), _format(internalFormat)
    {
        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;

        if constexpr (USE_DSA) {
            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_id);
            glTextureStorage2D(_id, mipLevels, (GLuint)internalFormat, size, size);
        } else {
            glGenTextures(1, &_id);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _id);
            glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipLevels, (GLuint)internalFormat, size, size);
        }

        this->SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            if constexpr (USE_DSA) { glGenerateTextureMipmap(_id);
            } else { glGenerateMipmap(T); }
        }
    }

    // TextureGL Cube Array
    TextureGL(int size, int layers, TextureFormat internalFormat, bool genMipMaps = false)
        requires (Array<T> && Cube<T>) : _width(size), _height(size), _layers(layers), _format(internalFormat)
    {
        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;

        if constexpr (USE_DSA) {
            glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &_id);
            glTextureStorage3D(_id, mipLevels, (GLuint)internalFormat, size, size, layers * 6);
        } else {
            glGenTextures(1, &_id);
            glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, _id);
            glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mipLevels, (GLuint)internalFormat, size, size, layers * 6);
        }

        SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            if constexpr (USE_DSA) { glGenerateTextureMipmap(_id);
            } else { glGenerateMipmap(T); }
        }
    }

    ~TextureGL() {
        this->Destroy();
    }

    TextureGL(const TextureGL&) = delete;
    TextureGL& operator=(const TextureGL&) = delete;
    TextureGL(TextureGL&& o) noexcept {
        _id = std::exchange(o._id, 0);
        _width = std::exchange(o._width, 0);
        _height = std::exchange(o._height, 0);
        _layers = std::exchange(o._layers, 0);
        _format = std::exchange(o._format, TextureFormat::RGB8);
    }

    TextureGL& operator=(TextureGL&& o) {
        if (this != &o) {
            this->Destroy();
            _id = std::exchange(o._id, 0);
            _width = std::exchange(o._width, 0);
            _height = std::exchange(o._height, 0);
            _layers = std::exchange(o._layers, 0);
            _format = std::exchange(o._format, TextureFormat::RGB8);
        }
        return *this;
    }

    // Sets all wraps to the same value at once
    void SetWrap(TextureWrap wraps) const {
        if constexpr (USE_DSA) {
            glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLint)wraps);
            glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLint)wraps);
        } else {
            glBindTexture(T, _id);
            glTexParameteri(T, GL_TEXTURE_WRAP_S, (GLint)wraps);
            glTexParameteri(T, GL_TEXTURE_WRAP_T, (GLint)wraps);
        }

        if constexpr (Cube<T>) {
            if constexpr (USE_DSA) {
                glTextureParameteri(_id, GL_TEXTURE_WRAP_R, (GLint)wraps);
            } else {
                glTexParameteri(T, GL_TEXTURE_WRAP_R, (GLint)wraps);
            }
        }

        if constexpr (!USE_DSA) { glBindTexture(T, 0); }
    }

    void SetWrap(TextureWrap s, TextureWrap t) const requires (!Cube<T>) {
        if constexpr (USE_DSA) {
            glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLint)s);
            glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLint)t);
        } else {
            glBindTexture(T, _id);
            glTexParameteri(T, GL_TEXTURE_WRAP_S, (GLint)s);
            glTexParameteri(T, GL_TEXTURE_WRAP_T, (GLint)t);
            glBindTexture(T, 0);
        }
    }

    void SetWrap(TextureWrap s, TextureWrap t, TextureWrap r) const requires (Cube<T>) {
        if constexpr (USE_DSA) {
            glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLint)s);
            glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLint)t);
            glTextureParameteri(_id, GL_TEXTURE_WRAP_R, (GLint)r);
        } else {
            glBindTexture(T, _id);
            glTexParameteri(T, GL_TEXTURE_WRAP_S, (GLint)s);
            glTexParameteri(T, GL_TEXTURE_WRAP_T, (GLint)t);
            glTexParameteri(T, GL_TEXTURE_WRAP_R, (GLint)r);
            glBindTexture(T, 0);
        }
    }

    // Sets all filters to the same value at once
    void SetFilter(TextureFilter minMag) const {
        SetFilter(minMag, minMag);
    }

    void SetFilter(TextureFilter min, TextureFilter mag) const {
        if constexpr (USE_DSA) {
            glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, (GLint)min);
            glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, (GLint)mag);
        } else {
            glBindTexture(T, _id);
            glTexParameteri(T, GL_TEXTURE_MIN_FILTER, (GLint)min);
            glTexParameteri(T, GL_TEXTURE_MAG_FILTER, (GLint)mag);
            glBindTexture(T, 0);
        }
    }

    void SetBorderColor(float r, float g, float b, float a) const {
        float color[] = {r, g, b, a};
        if constexpr (USE_DSA) {
            glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, color);
        } else {
            glBindTexture(T, _id);
            glTexParameterfv(T, GL_TEXTURE_BORDER_COLOR, color);
            glBindTexture(T, 0);
        }
    }

    void GenerateMipmap() {
        if constexpr (USE_DSA) {
            glGenerateTextureMipmap(_id);
        } else {
            glBindTexture(T, _id);
            glGenerateMipmap(T);
            glBindTexture(T, 0);
        }
    }

    void Upload(const void* data) const requires (!Array<T> && !Cube<T>) {
        if (data) {
            auto channels = FormatToChannels(_format);
            auto dataType = FormatToDataType(_format);

            if constexpr (USE_DSA) {
                glTextureSubImage2D(_id, 0, 0, 0, _width, _height, (GLuint)channels, (GLuint)dataType, data);
            } else {
                glBindTexture(GL_TEXTURE_2D, _id);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, (GLuint)channels, (GLuint)dataType, data);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
    }

    void UploadLayer(const void* data, int layer) const requires (Array<T>) {
        if (data) {
            if constexpr (USE_DSA) {
                glTextureSubImage3D(_id, 0, 0, 0, layer, _width, _height, 1,
                    FormatToChannels(_format), FormatToDataType(_format), data);
            } else {
                glBindTexture(T, _id);
                glTexSubImage3D(T, 0, 0, 0, layer, _width, _height, 1,
                    FormatToChannels(_format), FormatToDataType(_format), data);
                glBindTexture(T, 0);
            }
        }
    }

    void Bind(GLuint slot) const {
        if constexpr (USE_DSA) {
            glBindTextureUnit(slot, _id);
        } else {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(T, _id);
        }
    }

    GLuint GetID() const { return _id; }
    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }

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
    int _layers = 0;
    TextureFormat _format {TextureFormat::RGB8};
};

using Texture2D = TextureGL<GL_TEXTURE_2D>;
using Texture2DArray = TextureGL<GL_TEXTURE_2D_ARRAY>;
using TextureCube = TextureGL<GL_TEXTURE_CUBE_MAP>;
using TextureCubeArray = TextureGL<GL_TEXTURE_CUBE_MAP_ARRAY>;
