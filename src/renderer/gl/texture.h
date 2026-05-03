#pragma once

#include <glad/glad.h>
#include <bit>
#include <utility>
#include <span>

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

template<GLenum T> requires Supported<T>
class TextureGL {
public:
    //TextureGL 2D
    TextureGL(int width, int height, TextureFormat internalFormat, const void* data = nullptr, bool genMipMaps = false)
        requires (!Array<T> && !Cube<T>) : _width(width), _height(height), _format(internalFormat)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &_id);
        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;
        glTextureStorage2D(_id, mipLevels, (GLuint)internalFormat, width, height);

        this->Upload(data);
        this->SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            glGenerateTextureMipmap(_id);
        }
    }

    // TextureGL 2D Array
    TextureGL(int width, int height, int layers, TextureFormat internalFormat, bool genMipMaps = false)
        requires (Array<T> && !Cube<T>) : _width(width), _height(height), _layers(layers), _format(internalFormat)
    {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &_id);
        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;
        glTextureStorage3D(_id, mipLevels, (GLuint)internalFormat, width, height, layers);
        this->SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            glGenerateTextureMipmap(_id);
        }
    }

    // TextureGL Cube
    TextureGL(int size, TextureFormat internalFormat, bool genMipMaps = false)
        requires (!Array<T> && Cube<T>) : _width(size), _height(size), _format(internalFormat)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_id);

        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;
        glTextureStorage2D(_id, mipLevels, (GLuint)internalFormat, size, size);

        this->SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            glGenerateTextureMipmap(_id);
        }
    }

    // TextureGL Cube Array
    TextureGL(int size, int layers, TextureFormat internalFormat, bool genMipMaps = false)
        requires (Array<T> && Cube<T>) : _width(size), _height(size), _layers(layers), _format(internalFormat)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &_id);

        const GLsizei mipLevels = genMipMaps ? std::bit_width(static_cast<unsigned>(std::max(_width, _height))) : 1;
        glTextureStorage3D(_id, mipLevels, (GLuint)internalFormat, size, size, layers * 6);

        SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps)
            glGenerateTextureMipmap(_id);
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

    void SetWrap(TextureWrap s, TextureWrap t) const requires (!Cube<T>) {
        glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLint)s);
        glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLint)t);
    }

    void SetWrap(TextureWrap s, TextureWrap t, TextureWrap r) const requires (Cube<T>) {
        glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLint)s);
        glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLint)t);
        glTextureParameteri(_id, GL_TEXTURE_WRAP_R, (GLint)r);
    }

    void SetFilter(TextureFilter min, TextureFilter mag) const {
        glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, (GLint)min);
        glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, (GLint)mag);
    }

    void SetBorderColor(float r, float g, float b, float a) const {
        float color[] = {r, g, b, a};
        glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, color);
    }

    void GenerateMipmap() {
        glGenerateTextureMipmap(_id);
    }

    void Upload(const void* data) const requires (!Array<T> && !Cube<T>) {
        if (data) {
            auto channels = FormatToChannels(_format);
            auto dataType = FormatToDataType(_format);
            glTextureSubImage2D(_id, 0, 0, 0, _width, _height, (GLuint)channels, (GLuint)dataType, data);
        }
    }

    void UploadLayer(const void* data, int layer) const requires (Array<T>) {
        if (data) {
            glTextureSubImage3D(_id, 0, 0, 0, layer, _width, _height, 1,
                FormatToChannels(_format), FormatToDataType(_format), data);
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
    int _layers = 0;
    TextureFormat _format {TextureFormat::RGB8};
};

using Texture2D = TextureGL<GL_TEXTURE_2D>;
using Texture2DArray = TextureGL<GL_TEXTURE_2D_ARRAY>;
using TextureCube = TextureGL<GL_TEXTURE_CUBE_MAP>;
using TextureCubeArray = TextureGL<GL_TEXTURE_CUBE_MAP_ARRAY>;
