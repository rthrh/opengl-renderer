#pragma once

#include <gl/headers.h>
#include <string>
#include <stdexcept>

#include "texture_slots.h"
#include "gl/texture.h"
#include "gl/frame_buffer.h"

class ShadowMapPoint {
public:
    static constexpr int maxShadowCasters = 4;

    explicit ShadowMapPoint(int size = 512) :
        _size(size)
    {
        for (int i = 0; i < maxShadowCasters; i++) {
            _cubes[i] = TextureCube(size, TextureFormat::Depth32F);
            _cubes[i].SetFilter(TextureFilter::Nearest);
            _cubes[i].SetWrap(TextureWrap::ClampToEdge);
        }
    }

    ~ShadowMapPoint() = default;

    ShadowMapPoint(const ShadowMapPoint&) = delete;
    ShadowMapPoint& operator=(const ShadowMapPoint&) = delete;
    ShadowMapPoint(ShadowMapPoint&&) noexcept = default;
    ShadowMapPoint& operator=(ShadowMapPoint&&) noexcept = default;

    void BindFramebufferFace(int lightIndex, int face) const {
        _framebuffer.Bind();
        //_framebuffer.AttachTextureLayer(TextureAttachment::Depth, _cubes[lightIndex].GetID(), lightIndex * 6 + face);
        _framebuffer.AttachTextureCubeFace(TextureAttachment::Depth, _cubes[lightIndex].GetID(), face);
        glViewport(0, 0, _size, _size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void BindTextures() const {
        _cubes[0].Bind(slot(TextureSlot::ShadowPoint0));
        _cubes[1].Bind(slot(TextureSlot::ShadowPoint1));
        _cubes[2].Bind(slot(TextureSlot::ShadowPoint2));
        _cubes[3].Bind(slot(TextureSlot::ShadowPoint3));
    }

private:
    int _size;
    std::array<TextureCube, maxShadowCasters> _cubes;
    FrameBuffer _framebuffer;
};
