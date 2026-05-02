#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

#include "camera.h"
#include "texture_slots.h"
#include "gl/texture.h"

class ShadowMapDirectional {
public:
    ShadowMapDirectional(int size = 2048) :
        _size(size),
        _depthTexture(size, size, TextureFormat::Depth32F)
    {
        glCreateFramebuffers(1, &_fbo);

        // create depth texture
        //_depthTexture = Texture2D(_size, _size, TextureFormat::Depth32F);
        _depthTexture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _depthTexture.SetWrap(TextureWrap::ClampToBorder, TextureWrap::ClampToBorder);
        _depthTexture.SetBorderColor(1, 1, 1, 1);

        glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture.GetID(), 0);
        // don't render any color data
        glNamedFramebufferDrawBuffer(_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(_fbo, GL_NONE);
    }

    ~ShadowMapDirectional() {
        glDeleteFramebuffers(1, &_fbo);
    }

    ShadowMapDirectional(const ShadowMapDirectional&) = delete;
    ShadowMapDirectional& operator=(const ShadowMapDirectional&) = delete;

    void BindFramebuffer() const {
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        _depthTexture.Bind(slot(SlotOther::ShadowDirectional));
    }

private:
    GLuint _fbo = 0;
    //GLuint _depthTexture = 0;
    Texture2D _depthTexture;
    int _size;
};
