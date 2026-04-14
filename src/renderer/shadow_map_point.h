#pragma once

#include <glad/glad.h>
#include <string>
#include <stdexcept>

#include "renderer/texture_slots.h"

constexpr int MAX_POINT_SHADOWS = 4;

class ShadowMapPoint {
public:
    explicit ShadowMapPoint(int size = 1024) : _size(size) {
        // create depth cubemap texture
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &_depthCubemapArray);
        glTextureStorage3D(_depthCubemapArray, 1, GL_DEPTH_COMPONENT32F,
                           size, size, 6 * MAX_POINT_SHADOWS);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_depthCubemapArray, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // attach depth texture as FBO's depth buffer
        glCreateFramebuffers(1, &_depthMapFBO);
        glNamedFramebufferTexture(_depthMapFBO, GL_DEPTH_ATTACHMENT, _depthCubemapArray, 0);
        glNamedFramebufferDrawBuffer(_depthMapFBO, GL_NONE);
        glNamedFramebufferReadBuffer(_depthMapFBO, GL_NONE);

        GLenum status = glCheckNamedFramebufferStatus(_depthMapFBO, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("ShadowMapPoint FBO incomplete: " + std::to_string(status));
        }
    }

    ~ShadowMapPoint() {
        glDeleteFramebuffers(1, &_depthMapFBO);
        glDeleteTextures(1, &_depthCubemapArray);
    }

    ShadowMapPoint(const ShadowMapPoint&) = delete;
    ShadowMapPoint& operator=(const ShadowMapPoint&) = delete;

    void BindFramebuffer() const {
        glViewport(0, 0, _size, _size);
        glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        glBindTextureUnit(slot(SlotOther::ShadowPoint), _depthCubemapArray);
    }
private:
    GLuint _depthMapFBO = 0;
    GLuint _depthCubemapArray = 0;
    int _size = 0;
};