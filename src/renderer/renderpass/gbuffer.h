#pragma once

#include <gl/headers.h>
#include <vector>
#include <utility>

#include "utils/logger.h"
#include "renderer/texture_slots.h"
#include "gl/texture.h"
#include "gl/frame_buffer.h"

class GBuffer {
public:
    explicit GBuffer(int scrWidth, int scrHeight) :
        _scrWidth(scrWidth),
        _scrHeight(scrHeight),
        _framebuffer()
    {
        Init(scrWidth, scrHeight);
    }

    ~GBuffer() = default;

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;
    GBuffer(GBuffer&& o) noexcept = default;
    GBuffer& operator=(GBuffer&& o) noexcept = default;

    void Init(int scrWidth, int scrHeight) {
        using enum TextureAttachment;
        _textureAlbedo = this->createTexture(scrWidth, scrHeight, TextureFormat::RGBA8);
        _textureNormal = this->createTexture(scrWidth, scrHeight, TextureFormat::RGBA16F);
        _textureORM = this->createTexture(scrWidth, scrHeight, TextureFormat::RGBA8);
        _textureEmissive = this->createTexture(scrWidth, scrHeight, TextureFormat::RGBA16F);

        _textureDepth = Texture2D(scrWidth, scrHeight, TextureFormat::Depth32F);
        _textureDepth.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _textureDepth.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

        _framebuffer.AttachTexture(Color0, _textureAlbedo.GetID());
        _framebuffer.AttachTexture(Color1, _textureNormal.GetID());
        _framebuffer.AttachTexture(Color2, _textureORM.GetID());
        _framebuffer.AttachTexture(Color3, _textureEmissive.GetID());
        _framebuffer.AttachTexture(Depth, _textureDepth.GetID());

        _framebuffer.Status();
    }

    void Resize(int scrWidth, int scrHeight) {
        _scrWidth = scrWidth;
        _scrHeight = scrHeight;
        this->Init(scrWidth, scrHeight);
    }

    void BindFramebuffer() {
        _framebuffer.Bind();
        glViewport(0, 0, _scrWidth, _scrHeight); // will render white stripe on top without this call
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void BindTextures() const {
        using enum TextureSlot;
        _textureAlbedo.Bind(slot(Albedo));
        _textureNormal.Bind(slot(Normal));
        _textureORM.Bind(slot(ORM));
        _textureEmissive.Bind(slot(Emissive));
        _textureDepth.Bind(slot(Depth));
    }

    void BlitFramebuffer(GLuint targetFBO, int targetWidth, int targetHeight) const {
        FrameBuffer::Blit(_framebuffer.GetId(), targetFBO, _scrWidth, _scrHeight, targetWidth, targetHeight, GL_DEPTH_BUFFER_BIT);
    }

private:
    Texture2D createTexture(int scrWidth, int scrHeight, TextureFormat format) {
        auto texture = Texture2D(scrWidth, scrHeight, format);
        texture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        return texture;
    }

    int _scrWidth = 0, _scrHeight = 0;
    FrameBuffer _framebuffer;
    Texture2D _textureAlbedo;
    Texture2D _textureNormal;
    Texture2D _textureORM;
    Texture2D _textureEmissive;
    Texture2D _textureDepth;
};
