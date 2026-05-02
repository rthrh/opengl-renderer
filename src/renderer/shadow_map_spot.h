#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

#include "camera.h"
#include "texture_slots.h"
#include "gl/texture.h"

class ShadowMapSpot {
public:
    ShadowMapSpot(int size = 2048, int maxShadowCasters = 4) :
        _size(size), _depthTexture(size, size, maxShadowCasters, TextureFormat::Depth32F)
    {
        _depthTexture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _depthTexture.SetWrap(TextureWrap::ClampToBorder, TextureWrap::ClampToBorder);
        _depthTexture.SetBorderColor(1, 1, 1, 1);

        glCreateFramebuffers(1, &_fbo);

        // don't render any color data
        glNamedFramebufferDrawBuffer(_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(_fbo, GL_NONE);
    }

    ~ShadowMapSpot() {
        glDeleteFramebuffers(1, &_fbo);
    }

    ShadowMapSpot(const ShadowMapSpot&) = delete;
    ShadowMapSpot& operator=(const ShadowMapSpot&) = delete;

    void BindFramebufferLayer(int lightIndex) const {
        glNamedFramebufferTextureLayer(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture.GetID(), 0, lightIndex);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        glBindTextureUnit(slot(SlotOther::ShadowSpot), _depthTexture.GetID());
    }

private:
    GLuint _fbo = 0;
    int _size;
    Texture2DArray _depthTexture;
};
