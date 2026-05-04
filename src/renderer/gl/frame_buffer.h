#pragma once

#include <glad/glad.h>
#include <utility>
#include "utils/logger.h"
#include <algorithm>
#include <ranges>

enum class TextureAttachment : GLenum {
    Color0 = GL_COLOR_ATTACHMENT0,
    Color1 = GL_COLOR_ATTACHMENT1,
    Color2 = GL_COLOR_ATTACHMENT2,
    Color3 = GL_COLOR_ATTACHMENT3,
    Color4 = GL_COLOR_ATTACHMENT4,
    Depth = GL_DEPTH_ATTACHMENT,
    Stencil = GL_STENCIL_ATTACHMENT,
    DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
};

class FrameBuffer {
public:
    FrameBuffer(std::initializer_list<TextureAttachment> attachments = {}) {
        glCreateFramebuffers(1, &_id);

        bool hasColor = std::ranges::any_of(attachments, [](auto a) {
            using enum TextureAttachment;
            return a != Depth && a != Stencil && a != DepthStencil;
        });

        if (hasColor)
            glNamedFramebufferDrawBuffers(_id, attachments.size(), (GLenum*)attachments.begin());
        else {
            glNamedFramebufferDrawBuffer(_id, GL_NONE);
            glNamedFramebufferReadBuffer(_id, GL_NONE);
        }

        /*GLenum status = glCheckNamedFramebufferStatus(_id, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            Error("Framebuffer error: {}", status);
        }*/
    }

    ~FrameBuffer() {
        glDeleteFramebuffers(1, &_id);
    }

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    FrameBuffer(FrameBuffer&& o) noexcept {
        _id = std::exchange(o._id, 0);
    }

    FrameBuffer& operator=(FrameBuffer&& o) {
        if (this != &o) {
            glDeleteFramebuffers(1, &_id);
            _id = std::exchange(o._id, 0);
        }
        return *this;
    }

    GLuint GetId() const {
        return _id;
    }

    void AttachTexture(TextureAttachment attachment, GLuint tex) const {
        glNamedFramebufferTexture(_id, (GLenum)attachment, tex, 0);
    }

    void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, _id);
    }

    void Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
    }

    void Status() const {
        GLenum status = glCheckNamedFramebufferStatus(_id, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) Error("Framebuffer error: {}", status);
    }

private:
    GLuint _id = 0;
};
