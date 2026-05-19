#pragma once
#include <glad/glad.h>

#include "utils/logger.h"
#include "renderer/shader.h"
#include "gl/frame_buffer.h"
#include "gl/texture.h"


class FXAA {
public:
    FXAA(int scrWidth, int scrHeight) :
        _width(scrWidth),
        _height(scrHeight),
        _texture(scrWidth, scrHeight, TextureFormat::RGBA8),
        _FBO({TextureAttachment::Color0})
    {
        _texture.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        _texture.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        _FBO.AttachTexture(TextureAttachment::Color0, _texture.GetID());
        _FBO.Status();
    }

    ~FXAA() = default;
    FXAA(const FXAA&) = delete;
    FXAA& operator=(const FXAA&) = delete;

    void BindFramebuffer() const {
        _FBO.Bind();
    }

    void BindTexture(int slot) {
        _texture.Bind(slot);
    }

    GLuint GetFBO() const {
        return _FBO.GetId();
    }

private:
    int _width;
    int _height;
    Texture2D _texture;
    FrameBuffer _FBO;
};
