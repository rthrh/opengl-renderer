#pragma once

#include <glad/glad.h>
#include <vector>
#include <utility>

#include "utils/logger.h"
#include "renderer/texture_slots.h"
#include "gl/texture.h"
#include "gl/frame_buffer.h"

class GBuffer {
public:
    explicit GBuffer(int width, int height) : _width(width), _height(height) {
        using enum TextureAttachment; // TODO add check if we attach to wrong attachments
        TextureAttachment attachments[] = { Color0, Color1, Color2, Color3 };
        _framebuffer = FrameBuffer(attachments);

        _textureAlbedo = this->createTexture(width, height, TextureFormat::RGBA8);
        _textureNormal = this->createTexture(width, height, TextureFormat::RGB16F);
        _textureORM = this->createTexture(width, height, TextureFormat::RGBA8);
        _textureEmissive = this->createTexture(width, height, TextureFormat::RGB16F);
        _textureDepth = Texture2D(width, height, TextureFormat::Depth24Stencil8);

        _framebuffer.AttachTexture(Color0, _textureAlbedo.GetID());
        _framebuffer.AttachTexture(Color1, _textureNormal.GetID());
        _framebuffer.AttachTexture(Color2, _textureORM.GetID());
        _framebuffer.AttachTexture(Color3, _textureEmissive.GetID());
        _framebuffer.AttachTexture(DepthStencil, _textureDepth.GetID());

        _framebuffer.Status();
    }

    ~GBuffer() = default;

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;
    GBuffer(GBuffer&& o) noexcept = default;
    GBuffer& operator=(GBuffer&& o) noexcept = default;

    void BindFramebuffer() {
        _framebuffer.Bind();
        glViewport(0, 0, _width, _height); // will render white stripe on top without this call
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void BindTextures() const {
        using enum SlotDeferred;
        _textureAlbedo.Bind(slot(Albedo));
        _textureNormal.Bind(slot(Normal));
        _textureORM.Bind(slot(ORM));
        _textureEmissive.Bind(slot(Emissive));
        _textureDepth.Bind(slot(Depth));
    }

    void BlitFramebuffer(GLuint targetFBO, int targetWidth, int targetHeight) const {
        FrameBuffer::Blit(_framebuffer.GetId(), targetFBO, _width, _height, targetWidth, targetHeight, GL_DEPTH_BUFFER_BIT);
    }

private:
    Texture2D createTexture(int width, int height, TextureFormat format = TextureFormat::RGB32F) {
        auto texture = Texture2D(width, height, format);
        texture.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        return texture;
    }

    int _width = 0, _height = 0;
    FrameBuffer _framebuffer;
    Texture2D _textureAlbedo;
    Texture2D _textureNormal;
    Texture2D _textureORM;
    Texture2D _textureEmissive;
    Texture2D _textureDepth;
};
