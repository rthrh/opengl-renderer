#pragma once

#include <glad/glad.h>
#include <string>
#include <stdexcept>

#include "texture_slots.h"
#include "gl/texture.h"

class ShadowMapPoint {
public:
    explicit ShadowMapPoint(int size = 2048, int maxShadowCasters = 4) :
        _size(size),
        _depthTexture(size, maxShadowCasters, TextureFormat::Depth32F)
    {
        _depthTexture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _depthTexture.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        // attach depth texture as FBO's depth buffer
        glCreateFramebuffers(1, &_fbo);
        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture.GetID(), 0);
        glNamedFramebufferDrawBuffer(_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(_fbo, GL_NONE);

        GLenum status = glCheckNamedFramebufferStatus(_fbo, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("ShadowMapPoint FBO incomplete: " + std::to_string(status));
        }
    }

    ~ShadowMapPoint() {
        glDeleteFramebuffers(1, &_fbo);
    }

    ShadowMapPoint(const ShadowMapPoint&) = delete;
    ShadowMapPoint& operator=(const ShadowMapPoint&) = delete;

    void BindFramebuffer() const {
        glViewport(0, 0, _size, _size);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        glBindTextureUnit(slot(SlotOther::ShadowPoint), _depthTexture.GetID());
    }
private:
    GLuint _fbo = 0;
    TextureCubeArray _depthTexture;
    int _size;
};
