#pragma once

#include <glad/glad.h>
#include <vector>
#include <utility>

#include "utils/logger.h"


enum GBufferPbrTextureType {
    Position,
    Albedo, // RGB8
    Normal, // RGB16F
    ORM, // Occlussion/Rougness/Metallic RGB8
    Emissive, // RGB16F hdr emissive
    Depth, //DEPTH24_STENCIL8
    EnumSize
};


class GBuffer {
public:
    explicit GBuffer(int width, int height) : _width(width), _height(height) {
        glCreateFramebuffers(1, &_fbo);

        constexpr int numTextures = GBufferPbrTextureType::EnumSize;
        _textures.resize(numTextures);

        this->createTexture(width, height, Position);
        this->createTexture(width, height, Albedo);
        this->createTexture(width, height, Normal);
        this->createTexture(width, height, ORM);
        this->createTexture(width, height, Emissive);
        this->createDepthTexture(width, height, Depth);

        std::vector<GLenum> drawBuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
        glNamedFramebufferDrawBuffers(_fbo, drawBuffers.size(), drawBuffers.data());
        GLenum status = glCheckNamedFramebufferStatus(_fbo, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            Error("Framebuffer error: {}", status);
        }
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
    GBuffer(GBuffer&& o) noexcept {
        _fbo = std::exchange(o._fbo, 0);
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
        //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glViewport(0, 0, _width, _height); // will render white stripe on top without this call
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void BindTextures() const {
        for (auto i = 0u; i < _textures.size(); i++)
            glBindTextureUnit(i, _textures[i]);
    }

    void BlitFramebuffer(GLuint targetFBO, int targetWidth, int targetHeight) const {
        //glBlitNamedFramebuffer(fbo_src, fbo_dst, src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBlitNamedFramebuffer(_fbo, targetFBO, 0, 0, _width, _height, 0, 0, targetWidth, targetHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST); // TODO NEAREST for depth blits?
    }

private:
    void createTexture(int width, int height, GLuint slot, GLuint type = GL_RGB32F) {
        auto& tex = _textures[slot];
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        glTextureStorage2D(tex, 1, type, width, height);
        //glTextureSubImage2D(tex, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, nullptr);        
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0 + slot, tex, 0);
    }

    void createDepthTexture(int width, int height, GLuint slot) {
        auto& tex = _textures[slot];
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        glTextureStorage2D(tex, 1, GL_DEPTH_COMPONENT32F, width, height);
        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, tex, 0);
    }

    GLuint _fbo {0};
    std::vector<GLuint> _textures;
    int _width, _height;
};