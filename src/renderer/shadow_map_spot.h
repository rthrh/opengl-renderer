#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

#include "camera.h"
#include "texture_slots.h"

class ShadowMapSpot {
public:
    ShadowMapSpot(int size = 2048, int maxShadowCasters = 4) :
        _size(size), _maxShadowCasters(maxShadowCasters)
    {
        glCreateFramebuffers(1, &_fbo);

        // create depth texture
        glCreateTextures(GL_TEXTURE_2D, 1, &_depthTexture);
        glTextureStorage2D(_depthTexture, 1, GL_DEPTH_COMPONENT32F, _size, _size);
        glTextureParameteri(_depthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_depthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(_depthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(_depthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTextureParameterfv(_depthTexture, GL_TEXTURE_BORDER_COLOR, borderColor);

        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture, 0);
        // don't render any color data
        glNamedFramebufferDrawBuffer(_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(_fbo, GL_NONE);
    }

    ~ShadowMapSpot() {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_depthTexture);
    }

    ShadowMapSpot(const ShadowMapSpot&) = delete;
    ShadowMapSpot& operator=(const ShadowMapSpot&) = delete;

    void BindFramebuffer() const {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        glBindTextureUnit(slot(SlotOther::ShadowSpot), _depthTexture);
    }

private:
    GLuint _fbo = 0;
    GLuint _depthTexture = 0;
    int _size;
    int _maxShadowCasters; // TODO unused yet
};
