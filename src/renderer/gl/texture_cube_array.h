#pragma once

#include <glad/glad.h>
#include <utility>
#include <cmath>
#include <bit>

#include "gl/texture.h"

class TextureCubeArray {
public:
    TextureCubeArray(int size, int layerCount, TextureFormat internalFormat, bool genMipMaps = false)
        : _size(size), _layerCount(layerCount), _format(internalFormat)
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &_id);

        GLsizei mipLevels = genMipMaps ? std::bit_width((unsigned int)_size) : 1;
        glTextureStorage3D(_id, mipLevels, (GLuint)internalFormat, size, size, layerCount * 6);

        SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps)
            glGenerateTextureMipmap(_id);
    }

    ~TextureCubeArray() { Destroy(); }

    TextureCubeArray(const TextureCubeArray&) = delete;
    TextureCubeArray& operator=(const TextureCubeArray&) = delete;

    TextureCubeArray(TextureCubeArray&& o) noexcept {
        _id = std::exchange(o._id, 0);
        _size = std::exchange(o._size, 0);
        _layerCount = std::exchange(o._layerCount, 0);
        _format = std::exchange(o._format, TextureFormat::RGB8);
    }

    TextureCubeArray& operator=(TextureCubeArray&& o) noexcept {
        if (this != &o) {
            Destroy();
            _id = std::exchange(o._id, 0);
            _size = std::exchange(o._size, 0);
            _layerCount = std::exchange(o._layerCount, 0);
            _format = std::exchange(o._format, TextureFormat::RGB8);
        }
        return *this;
    }

    void SetWrap(TextureWrap s, TextureWrap t, TextureWrap r) const {
        glTextureParameteri(_id, GL_TEXTURE_WRAP_S, (GLuint)s);
        glTextureParameteri(_id, GL_TEXTURE_WRAP_T, (GLuint)t);
        glTextureParameteri(_id, GL_TEXTURE_WRAP_R, (GLuint)r);
    }

    void SetFilter(TextureFilter min, TextureFilter mag) const {
        glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, (GLuint)min);
        glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, (GLuint)mag);
    }

    void SetBorderColor(float r, float g, float b, float a) const {
        float color[] = {r, g, b, a};
        glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, color);
    }

    void Bind(GLuint slot) const { glBindTextureUnit(slot, _id); }
    GLuint GetID() const { return _id; }

    void Destroy() {
        if (_id) { glDeleteTextures(1, &_id); _id = 0; }
    }

private:
    GLuint _id = 0;
    int _size = 0;
    int _layerCount = 0;
    TextureFormat _format{TextureFormat::RGB8};
};