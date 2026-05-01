#pragma once

#include <glad/glad.h>
#include <utility>
#include <cmath>

#include "texture.h"

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
