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
    ShadowMapSpot(int size = 2048) : 
        _size(size)
    {
        glCreateFramebuffers(1, &_fbo);

        // create depth texture
        glCreateTextures(GL_TEXTURE_2D, 1, &_depthMap);
        glTextureStorage2D(_depthMap, 1, GL_DEPTH_COMPONENT32F, _size, _size);
        glTextureParameteri(_depthMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_depthMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(_depthMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(_depthMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTextureParameterfv(_depthMap, GL_TEXTURE_BORDER_COLOR, borderColor);

        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthMap, 0);
        // don't render any color data
        glNamedFramebufferDrawBuffer(_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(_fbo, GL_NONE);
    }
    ~ShadowMapSpot() {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_depthMap);
    }

    ShadowMapSpot(const ShadowMapSpot&) = delete;
    ShadowMapSpot& operator=(const ShadowMapSpot&) = delete;

    void BindFramebuffer() const {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        glBindTextureUnit(slot(SlotOther::ShadowSpot), _depthMap);
    }

private:
    GLuint _depthMap = 0;
    GLuint _fbo = 0;
    GLuint _planeVAO = 0;
    int _size;
};