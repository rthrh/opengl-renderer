#pragma once

#include <glad/glad.h>
#include <vector>
#include <utility>

#include "utils/logger.h"
#include "renderer/texture_slots.h"
#include "gl/texture.h"


class GBuffer {
public:
    explicit GBuffer(int width, int height) : _width(width), _height(height) {
        glCreateFramebuffers(1, &_fbo);

        _texturePosition = this->createTexture(width, height);
        _textureAlbedo = this->createTexture(width, height);
        _textureNormal = this->createTexture(width, height);
        _textureORM = this->createTexture(width, height);
        _textureEmissive = this->createTexture(width, height);

        _textureDepth = Texture2D(width, height, TextureFormat::Depth24Stencil8);
        glNamedFramebufferTexture(_fbo, GL_DEPTH_STENCIL_ATTACHMENT, _textureDepth.GetID(), 0);

        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0 + 0, _texturePosition.GetID(), 0);
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0 + 1, _textureAlbedo.GetID(), 0);
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0 + 2, _textureNormal.GetID(), 0);
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0 + 3, _textureORM.GetID(), 0);
        glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0 + 4, _textureEmissive.GetID(), 0);

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
    }

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;
    GBuffer(GBuffer&& o) noexcept = default;
    GBuffer& operator=(GBuffer&& o) noexcept = default;

    void BindFramebuffer() {
        //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glViewport(0, 0, _width, _height); // will render white stripe on top without this call
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void BindTextures() const {
        using enum SlotDeferred;
        _texturePosition.Bind(slot(Position));
        _textureAlbedo.Bind(slot(Albedo));
        _textureNormal.Bind(slot(Normal));
        _textureORM.Bind(slot(ORM));
        _textureEmissive.Bind(slot(Emissive));
        _textureDepth.Bind(slot(Depth));
    }

    void BlitFramebuffer(GLuint targetFBO, int targetWidth, int targetHeight) const {
        //glBlitNamedFramebuffer(fbo_src, fbo_dst, src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBlitNamedFramebuffer(_fbo, targetFBO, 0, 0, _width, _height, 0, 0, targetWidth, targetHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST); // TODO NEAREST for depth blits?
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Error("BlitFramebuffer: {}", err);
            throw std::runtime_error("glBlitNamedFramebuffer : " + err);
        }
    }

private:
    Texture2D createTexture(int width, int height, TextureFormat format = TextureFormat::RGB32F) {
        auto texture = Texture2D(width, height, format);
        texture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        return texture;
    }

    GLuint _fbo = 0;
    int _width = 0, _height = 0;
    Texture2D _texturePosition;
    Texture2D _textureAlbedo;
    Texture2D _textureNormal;
    Texture2D _textureORM;
    Texture2D _textureEmissive;
    Texture2D _textureDepth;
};
