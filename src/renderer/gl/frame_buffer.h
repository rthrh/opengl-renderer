#pragma once

#include <gl/headers.h>
#include <utility>
#include <algorithm>
#include <ranges>
#include <vector>

#include "gl/render_buffer.h"

#include "utils/logger.h"

#include "dsa_config.h"

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
        #ifdef USE_GL_DSA
            glCreateFramebuffers(1, &_id);
            glNamedFramebufferDrawBuffer(_id, GL_NONE);
            glNamedFramebufferReadBuffer(_id, GL_NONE);
        #else
            glGenFramebuffers(1, &_id);
            glBindFramebuffer(GL_FRAMEBUFFER, _id);
            //glDrawBuffers(GL_NONE);
            const GLenum none = GL_NONE;
            glDrawBuffers(1, &none);
            glReadBuffer(GL_NONE);
        #endif
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
        #ifdef USE_GL_DSA
            glNamedFramebufferTexture(_id, (GLenum)attachment, tex, 0);
            this->updateAttachments(attachment);
        #else
            glBindFramebuffer(GL_FRAMEBUFFER, _id);
            //glFramebufferTexture(GL_FRAMEBUFFER, (GLenum)attachment, tex, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)attachment, GL_TEXTURE_2D, tex, 0);
            this->updateAttachments(attachment);
        #endif
    }

    void AttachTextureLayer(TextureAttachment attachment, GLuint tex, int layer, int mip = 0) const {
        #ifdef USE_GL_DSA
            glNamedFramebufferTextureLayer(_id, (GLenum)attachment, tex, mip, layer);
            this->updateAttachments(attachment);
        #else
            glBindFramebuffer(GL_FRAMEBUFFER, _id);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, (GLenum)attachment, tex, mip, layer);
            this->updateAttachments(attachment);
        #endif
    }

    //TODO verify
    void AttachTextureCubeFace(TextureAttachment attachment, GLuint tex, int face, int mip = 0) const {
        #ifdef USE_GL_DSA
            // DSA path: glNamedFramebufferTextureLayer treats cubemap faces as layers 0..5
            glNamedFramebufferTextureLayer(_id, (GLenum)attachment, tex, mip, face);
            this->updateAttachments(attachment);
        #else
            glBindFramebuffer(GL_FRAMEBUFFER, _id);
            glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, tex, mip);
            this->updateAttachments(attachment);
        #endif
    }

    void AttachRenderBuffer(TextureAttachment attachment, const RenderBuffer& renderBuffer) const {
        #ifdef USE_GL_DSA
            glNamedFramebufferRenderbuffer(_id, (GLenum)attachment, GL_RENDERBUFFER, renderBuffer.GetID());
            this->updateAttachments(attachment);
        #else
            glBindFramebuffer(GL_FRAMEBUFFER, _id);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, (GLenum)attachment, GL_RENDERBUFFER, renderBuffer.GetID());
            this->updateAttachments(attachment);
        #endif
    }

    void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, _id);
    }

    void Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
    }

    static void Blit(GLuint srcFBO, GLuint dstFBO, int srcWidth, int srcHeight, int dstWidth, int dstHeight, GLenum mask) {
        #ifdef USE_GL_DSA
            glBlitNamedFramebuffer(srcFBO, dstFBO, 0, 0, srcWidth, srcHeight, 0, 0, dstWidth, dstHeight, mask, GL_NEAREST);
        #else
            glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFBO);
            glBlitFramebuffer(0, 0, srcWidth, srcHeight, 0, 0, dstWidth, dstHeight, mask, GL_NEAREST);
        #endif

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Error("BlitFramebuffer: {}", err);
            throw std::runtime_error("glBlitNamedFramebuffer : " + std::to_string(err));
        }
    }

    void Status() const {
        #ifdef USE_GL_DSA
            GLenum status = glCheckNamedFramebufferStatus(_id, GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) Error("Framebuffer error: {}", status);
        #else
            glBindFramebuffer(GL_FRAMEBUFFER, _id);
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) Error("Framebuffer error: {}", status);
        #endif
    }

private:
    void updateAttachments(TextureAttachment attachment) const {
        using enum TextureAttachment;
        bool isColor = attachment != Depth && attachment != Stencil && attachment != DepthStencil;
        if (isColor && !std::ranges::contains(_colorAttachments, attachment)) {
            _colorAttachments.emplace_back(attachment);

            #ifdef USE_GL_DSA
                glNamedFramebufferDrawBuffers(_id, _colorAttachments.size(), (GLenum*)_colorAttachments.data());
            #else
                // Assumes the FBO is already bound
                glDrawBuffers(_colorAttachments.size(), (GLenum*)_colorAttachments.data());
            #endif
        }
    }

    GLuint _id = 0;
    mutable std::vector<TextureAttachment> _colorAttachments;
};
