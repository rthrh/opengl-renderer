#pragma once

#include <glad/glad.h>
#include <vector>
#include <utility>

#include "utils/logger.h"


enum GBufferTextureType {
    Position,
    Normal,
    Diffuse,
    Specular,
    Depth,
    EnumSize
};


class GBuffer {
public:
    explicit GBuffer(int width, int height) : _width(width), _height(height) {
        glGenFramebuffers(1, &_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

        constexpr int numTextures = GBufferTextureType::EnumSize;
        _textures.resize(numTextures);
        glGenTextures(_textures.size(), _textures.data());

        this->createTexture(width, height, Position);
        this->createTexture(width, height, Normal);
        this->createTexture(width, height, Diffuse);
        this->createTexture(width, height, Specular);
        this->createDepthTexture(width, height, Depth);

        std::vector<GLenum> drawBuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(drawBuffers.size(), drawBuffers.data());

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            Error("Framebuffer error: {}", status);
        }

        // restore default FBO
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    ~GBuffer() {
        if (_fbo) {
            glDeleteFramebuffers(1, &_fbo);
        }

        if (!_textures.empty()) {
            glDeleteTextures(_textures.size(), _textures.data());
        }
    }

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;
    GBuffer(GBuffer&& o) noexcept : _fbo{std::exchange(o._fbo, 0)} {
        _textures = std::move(o._textures);
        _width = std::exchange(o._width, 0);
        _height = std::exchange(o._height, 0);
    }

    GBuffer& operator=(GBuffer&& o) noexcept {
        if (this == &o) return *this;
        _fbo = std::exchange(o._fbo, 0);
        _width = std::exchange(o._width, 0);
        _height = std::exchange(o._height, 0);
        _textures = std::move(o._textures);
        return *this;
    }

    void BindForWriting() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        //glViewport(0, 0, _width, _height);
    }

    void BindTextures() const {
        for (int i = 0; i < _textures.size(); i++)
            glBindTextureUnit(i, _textures[i]);
    }

    void BlitFramebuffer(GLuint targetFBO, int targetWidth, int targetHeight) const {
        //glBlitNamedFramebuffer(fbo_src, fbo_dst, src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBlitNamedFramebuffer(_fbo, targetFBO, 0, 0, _width, _height, 0, 0, targetWidth, targetHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST); // TODO NEAREST for depth blits?
    }

private:
    void createTexture(int width, int height, GLuint slot) {
        auto& tex = _textures[slot];
        //glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        //glTextureStorage2D(tex, 1, GL_RGB32F, width, height);
        //glTextureSubImage2D(tex, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, nullptr);        
        //glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, _textures[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL); // TODO can save space by using different texture types
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, GL_TEXTURE_2D, _textures[slot], 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void createDepthTexture(int width, int height, GLuint slot) {
        glBindTexture(GL_TEXTURE_2D, _textures[slot]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _textures[slot], 0);
    }

    GLuint _fbo {0};
    std::vector<GLuint> _textures;
    int _width, _height;
};