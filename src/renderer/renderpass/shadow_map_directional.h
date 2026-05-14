#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>

#include "camera.h"
#include "texture_slots.h"
#include "gl/texture.h"
#include "gl/frame_buffer.h"

class ShadowMapDirectional {
public:
    ShadowMapDirectional(int size = 2048) :
        _size(size)
    {
        // Create depth texture
        _depthTexture = Texture2D(_size, _size, TextureFormat::Depth32F);
        _depthTexture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _depthTexture.SetWrap(TextureWrap::ClampToBorder, TextureWrap::ClampToBorder);
        _depthTexture.SetBorderColor(1, 1, 1, 1);

        // Attach to framebuffer
        constexpr TextureAttachment attachments[] = {TextureAttachment::Depth};
        _framebuffer = FrameBuffer(attachments);
        _framebuffer.AttachTexture(TextureAttachment::Depth, _depthTexture.GetID());
    }

    ~ShadowMapDirectional() = default;

    ShadowMapDirectional(const ShadowMapDirectional&) = delete;
    ShadowMapDirectional& operator=(const ShadowMapDirectional&) = delete;
    ShadowMapDirectional(ShadowMapDirectional&&) noexcept = default;
    ShadowMapDirectional& operator=(ShadowMapDirectional&&) noexcept = default;

    void BindFramebuffer() const {
        _framebuffer.Bind();
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        _depthTexture.Bind(slot(SlotOther::ShadowDirectional));
    }

private:
    int _size;
    FrameBuffer _framebuffer;
    Texture2D _depthTexture;
};
