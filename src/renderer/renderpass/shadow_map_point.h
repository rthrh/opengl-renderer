#pragma once

#include <glad/glad.h>
#include <string>
#include <stdexcept>

#include "texture_slots.h"
#include "gl/texture.h"
#include "gl/frame_buffer.h"

class ShadowMapPoint {
public:
    explicit ShadowMapPoint(int size = 512, int maxShadowCasters = 4) :
        _size(size)
    {
        // Create depth texture
        _depthTexture = TextureCubeArray(size, maxShadowCasters, TextureFormat::Depth32F);
        _depthTexture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _depthTexture.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

        // Attach to framebuffer
        constexpr TextureAttachment attachments[] = {TextureAttachment::Depth};
        _framebuffer = FrameBuffer(attachments);
        _framebuffer.AttachTexture(TextureAttachment::Depth, _depthTexture.GetID());
    }

    ~ShadowMapPoint() = default;

    ShadowMapPoint(const ShadowMapPoint&) = delete;
    ShadowMapPoint& operator=(const ShadowMapPoint&) = delete;
    ShadowMapPoint(ShadowMapPoint&&) noexcept = default;
    ShadowMapPoint& operator=(ShadowMapPoint&&) noexcept = default;

    void BindFramebufferFace(int lightIndex, int face) const {
        _framebuffer.Bind();
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture.GetID(), 0, lightIndex * 6 + face);
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTexture() const {
        _depthTexture.Bind(slot(SlotOther::ShadowPoint));
    }

private:
    int _size;
    TextureCubeArray _depthTexture;
    FrameBuffer _framebuffer;
};
