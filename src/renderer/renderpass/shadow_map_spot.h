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

class ShadowMapSpot {
public:
    ShadowMapSpot(int size = 512, int maxShadowCasters = 4) :
        _size(size)
    {
        // Create depth texture
        _depthTexture = Texture2DArray(size, size, maxShadowCasters, TextureFormat::Depth32F);
        _depthTexture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _depthTexture.SetWrap(TextureWrap::ClampToBorder, TextureWrap::ClampToBorder);
        _depthTexture.SetBorderColor(1, 1, 1, 1);

        constexpr TextureAttachment attachments[] = {TextureAttachment::Depth};
        _framebuffer = FrameBuffer(attachments);
    }

    ~ShadowMapSpot() = default;

    ShadowMapSpot(const ShadowMapSpot&) = delete;
    ShadowMapSpot& operator=(const ShadowMapSpot&) = delete;
    ShadowMapSpot(ShadowMapSpot&&) noexcept = default;
    ShadowMapSpot& operator=(ShadowMapSpot&&) noexcept = default;

    void BindFramebufferLayer(int lightIndex) const {
        _framebuffer.AttachTextureLayer(TextureAttachment::Depth, _depthTexture.GetID(), lightIndex);
        _framebuffer.Bind();
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        _depthTexture.Bind(slot(SlotOther::ShadowSpot));
    }

private:
    int _size;
    Texture2DArray _depthTexture;
    FrameBuffer _framebuffer;
};
