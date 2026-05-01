#pragma once

#include <glad/glad.h>
#include <utility>
#include <cmath>

#include "gl/texture.h"

class TextureCube {
public:
    TextureCube(int size, TextureFormat internalFormat, bool genMipMaps = false) : _size(size), _format(internalFormat) {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_id);

        GLsizei mipLevels = genMipMaps ? std::floor(std::log2(_size)) + 1 : 1;
        glTextureStorage2D(_id, mipLevels, (GLuint)internalFormat, size, size);

        this->SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        this->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        if (genMipMaps) {
            glGenerateTextureMipmap(_id);
        }
    }

    ~TextureCube() {
        this->Destroy();
    }

    TextureCube(const TextureCube&) = delete;
    TextureCube& operator=(const TextureCube&) = delete;
    TextureCube(TextureCube&& o) noexcept {
        _id = std::exchange(o._id, 0);
        _size = std::exchange(o._size, 0);
        _format = std::exchange(o._format, TextureFormat::RGB8);
    }

    TextureCube& operator=(TextureCube&& o) {
        if (this != &o) {
            this->Destroy();
            _id = std::exchange(o._id, 0);
            _size = std::exchange(o._size, 0);
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
    int _size = 0;
    TextureFormat _format {TextureFormat::RGB8};
};
