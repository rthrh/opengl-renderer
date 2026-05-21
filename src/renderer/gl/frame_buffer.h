#pragma once

#include <glad/glad.h>
#include <utility>
#include "utils/logger.h"
#include <algorithm>
#include <ranges>
#include "gl/render_buffer.h"
#include <ranges>
#include <vector>

enum class TextureAttachment : GLenum {
    Color0 = GL_COLOR_ATTACHMENT0,
    Color1 = GL_COLOR_ATTACHMENT1,
    Color2 = GL_COLOR_ATTACHMENT2,
    Color3 = GL_COLOR_ATTACHMENT3,
    Color4 = GL_COLOR_ATTACHMENT4,
    Depth  = GL_DEPTH_ATTACHMENT,
    Stencil = GL_STENCIL_ATTACHMENT,
    DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
};

class FrameBuffer {
public:
    FrameBuffer() {
        glCreateFramebuffers(1, &_id);
        glNamedFramebufferDrawBuffer(_id, GL_NONE);
        glNamedFramebufferReadBuffer(_id, GL_NONE);
    }

    ~FrameBuffer() {
        glDeleteFramebuffers(1, &_id);
    }

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    FrameBuffer(FrameBuffer&& o) noexcept {
        _id = std::exchange(o._id, 0);
        _colorAttachments = std::move(o._colorAttachments);
    }

    FrameBuffer& operator=(FrameBuffer&& o) {
        if (this != &o) {
            glDeleteFramebuffers(1, &_id);
            _id = std::exchange(o._id, 0);
            _colorAttachments = std::move(o._colorAttachments);
        }
        return *this;
    }

    GLuint GetId() const {
        return _id;
    }

    void AttachTexture(TextureAttachment attachment, GLuint tex) const {
        glNamedFramebufferTexture(_id, (GLenum)attachment, tex, 0);
        this->updateAttachments(attachment);
    }

    void AttachTextureLayer(TextureAttachment attachment, GLuint tex, int layer, int mip = 0) const {
        glNamedFramebufferTextureLayer(_id, (GLenum)attachment, tex, mip, layer);
        this->updateAttachments(attachment);
    }

    void AttachRenderBuffer(TextureAttachment attachment, const RenderBuffer& renderBuffer) const {
        glNamedFramebufferRenderbuffer(_id, (GLenum)attachment, GL_RENDERBUFFER, renderBuffer.GetID());
        this->updateAttachments(attachment);
    }

    void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, _id);
    }

    void Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
    }

    static void Blit(GLuint srcFBO, GLuint dstFBO, int srcWidth, int srcHeight, int dstWidth, int dstHeight, GLenum mask) {
        glBlitNamedFramebuffer(srcFBO, dstFBO, 0, 0, srcWidth, srcHeight, 0, 0, dstWidth, dstHeight, mask, GL_NEAREST);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Error("BlitFramebuffer: {}", err);
            throw std::runtime_error("glBlitNamedFramebuffer : " + std::to_string(err));
        }
    }

    void Status() const {
        GLenum status = glCheckNamedFramebufferStatus(_id, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) Error("Framebuffer error: {}", status);
    }

private:
    void updateAttachments(TextureAttachment attachment) const {
        using enum TextureAttachment;
        bool isColor = attachment != Depth && attachment != Stencil && attachment != DepthStencil;
        if (isColor && !std::ranges::contains(_colorAttachments, attachment)) {
            _colorAttachments.emplace_back(attachment);
            glNamedFramebufferDrawBuffers(_id, _colorAttachments.size(), (GLenum*)_colorAttachments.data());
        }
    }

    GLuint _id = 0;
    mutable std::vector<TextureAttachment> _colorAttachments;
};
