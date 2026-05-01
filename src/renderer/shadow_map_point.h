#pragma once

#include <glad/glad.h>
#include <string>
#include <stdexcept>

#include "texture_slots.h"
#include "gl/texture.h"


class ShadowMapPoint {
public:
    explicit ShadowMapPoint(int size = 2048, int maxShadowCasters = 4) : _size(size) {
        // create depth cubemap texture
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &_depthCubemapArray);
        glTextureStorage3D(_depthCubemapArray, 1, GL_DEPTH_COMPONENT32F,
                           size, size, 6 * maxShadowCasters);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // attach depth texture as FBO's depth buffer
        glCreateFramebuffers(1, &_fbo);
        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthCubemapArray, 0);
        glNamedFramebufferDrawBuffer(_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(_fbo, GL_NONE);

        GLenum status = glCheckNamedFramebufferStatus(_fbo, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("ShadowMapPoint FBO incomplete: " + std::to_string(status));
        }
    }

    ~ShadowMapPoint() {
        glDeleteFramebuffers(1, &_fbo);
        glDeleteTextures(1, &_depthCubemapArray);
    }

    ShadowMapPoint(const ShadowMapPoint&) = delete;
    ShadowMapPoint& operator=(const ShadowMapPoint&) = delete;

    void BindFramebuffer() const {
        glViewport(0, 0, _size, _size);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        glBindTextureUnit(slot(SlotOther::ShadowPoint), _depthCubemapArray);
    }
private:
    GLuint _fbo = 0;
    GLuint _depthCubemapArray = 0;
    int _size;
};
