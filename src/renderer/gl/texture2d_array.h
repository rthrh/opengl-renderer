#pragma once

#include <glad/glad.h>
#include <utility>
#include <cmath>

#include "texture.h"

class Texture2DArray {
public:
    Texture2DArray(int width, int height, int layers, TextureFormat internalFormat, bool genMipMaps = false)
        : _width(width), _height(height), _layers(layers), _format(internalFormat) {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &_id);
        GLsizei mipLevels = genMipMaps ? std::floor(std::log2(std::max(width, height))) + 1 : 1;
        glTextureStorage3D(_id, mipLevels, (GLuint)internalFormat, width, height, layers);
        SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps)
            glGenerateTextureMipmap(_id);
    }

    ~Texture2DArray() { Destroy(); }

    Texture2DArray(const Texture2DArray&) = delete;
    Texture2DArray& operator=(const Texture2DArray&) = delete;

    Texture2DArray(Texture2DArray&& o) noexcept {
        _id = std::exchange(o._id, 0);
        _width = std::exchange(o._width, 0);
        _height = std::exchange(o._height, 0);
        _layers = std::exchange(o._layers, 0);
        _format = std::exchange(o._format, TextureFormat::RGB8);
    }

    Texture2DArray& operator=(Texture2DArray&& o) noexcept {
        if (this != &o) {
            Destroy();
            _id = std::exchange(o._id, 0);
            _width = std::exchange(o._width, 0);
            _height = std::exchange(o._height, 0);
            _layers = std::exchange(o._layers, 0);
            _format = std::exchange(o._format, TextureFormat::RGB8);
        }
        return *this;
    }

    void SetWrap(TextureWrap s, TextureWrap t) const {
        glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLuint)s);
        glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLuint)t);
    }

    void SetFilter(TextureFilter min, TextureFilter mag) const {
        glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, (GLuint)min);
        glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, (GLuint)mag);
    }

    void SetBorderColor(float r, float g, float b, float a) const {
        float color[] = {r, g, b, a};
        glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, color);
    }

    void UploadLayer(const void* data, int layer) const {
        if (data)
            glTextureSubImage3D(_id, 0, 0, 0, layer, _width, _height, 1,
                FormatToChannels(_format), FormatToDataType(_format), data);
    }

    void Bind(GLuint slot) const { glBindTextureUnit(slot, _id); }
    GLuint GetID() const { return _id; }

    void Destroy() {
        if (_id) { glDeleteTextures(1, &_id); _id = 0; }
    }

private:
    GLuint _id = 0;
    int _width = 0;
    int _height = 0;
    int _layers = 0;
    TextureFormat _format{TextureFormat::RGB8};
};
