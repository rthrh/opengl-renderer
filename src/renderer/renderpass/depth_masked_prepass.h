#pragma once
#include <glad/glad.h>

#include "utils/logger.h"
#include "renderer/shader.h"
#include "gl/frame_buffer.h"
#include "gl/render_buffer.h"
//#include "gl/texture.h"


class DepthMaskedPrepass {
public:
    DepthMaskedPrepass(int scrWidth, int scrHeight) :
        _scrWidth(scrWidth),
        _scrHeight(scrHeight)
    {
        this->Init(scrWidth, scrHeight);
    }

    ~DepthMaskedPrepass() = default;
    DepthMaskedPrepass(const DepthMaskedPrepass&) = delete;
    DepthMaskedPrepass& operator=(const DepthMaskedPrepass&) = delete;

    void Init(int scrWidth, int scrHeight) {
        _depthRBO = RenderBuffer(_scrWidth, _scrHeight, TextureFormat::Depth32F);
        _FBO = FrameBuffer();
        _FBO.AttachRenderBuffer(TextureAttachment::Depth, _depthRBO);
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

    GLuint GetFBO() const {
        return _FBO.GetId();
    }

private:
    int _scrWidth;
    int _scrHeight;
    FrameBuffer _FBO;
    RenderBuffer _depthRBO;
};
