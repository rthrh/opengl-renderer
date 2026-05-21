#pragma once
#include <glad/glad.h>

#include "utils/logger.h"
#include "renderer/shader.h"
#include "gl/frame_buffer.h"
#include "gl/texture.h"


class FXAA {
public:
    FXAA(int scrWidth, int scrHeight) :
        _scrWidth(scrWidth),
        _scrHeight(scrHeight)
    {
        this->Init(scrWidth, scrHeight);
    }

    ~FXAA() = default;
    FXAA(const FXAA&) = delete;
    FXAA& operator=(const FXAA&) = delete;

    void Init(int scrWidth, int scrHeight) {
        _texture = Texture2D(scrWidth, scrHeight, TextureFormat::RGBA8);
        _FBO = FrameBuffer();
        _texture.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        _texture.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        _FBO.AttachTexture(TextureAttachment::Color0, _texture.GetID());
        _FBO.Status();
    }

    void Resize(int scrWidth, int scrHeight) {
        _scrWidth = scrWidth;
        _scrHeight = scrHeight;
        this->Init(scrWidth, scrHeight);
    }

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
    int _scrWidth;
    int _scrHeight;
    Texture2D _texture;
    FrameBuffer _FBO;
};
