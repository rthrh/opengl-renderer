#pragma once
#include <glad/glad.h>
#include <iostream>

#include "utils/logger.h"
#include "renderer/shader.h"
#include "gl/render_buffer.h"
#include "gl/frame_buffer.h"
#include "gl/texture.h"

//. TODO refactor
class Bloom {
public:
    Bloom(int scrWidth, int scrHeight) :
        _width(scrWidth), _height(scrHeight)
    {
        // configure (floating point) framebuffers
        _hdrFBO = FrameBuffer({TextureAttachment::Color0, TextureAttachment::Color1});

        // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
        _colorBuffers = {
            Texture2D(_width, _height, TextureFormat::RGBA16F),
            Texture2D(_width, _height, TextureFormat::RGBA16F)
        };

        _colorBuffers[0].SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        _colorBuffers[1].SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        _colorBuffers[0].SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        _colorBuffers[1].SetFilter(TextureFilter::Linear, TextureFilter::Linear);

        _hdrFBO.AttachTexture(TextureAttachment::Color0, _colorBuffers[0].GetID());
        _hdrFBO.AttachTexture(TextureAttachment::Color1, _colorBuffers[1].GetID());

        // create and attach depth buffer (renderbuffer)
        _depthRBO = RenderBuffer(_width, _height, TextureFormat::Depth24);
        _hdrFBO.AttachRenderBuffer(TextureAttachment::Depth, _depthRBO);
        _hdrFBO.Status();

        // ping-pong-framebuffer for blurring
        _pingpongFBO = {
            FrameBuffer({TextureAttachment::Color0}),
            FrameBuffer({TextureAttachment::Color0})
        };
 
        _pingpongColorbuffers = {
            Texture2D(_width, _height, TextureFormat::RGBA16F),
            Texture2D(_width, _height, TextureFormat::RGBA16F)
        };

        _pingpongColorbuffers[0].SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        _pingpongColorbuffers[1].SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        _pingpongColorbuffers[0].SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        _pingpongColorbuffers[1].SetFilter(TextureFilter::Linear, TextureFilter::Linear);
    
        _pingpongFBO[0].AttachTexture(TextureAttachment::Color0, _pingpongColorbuffers[0].GetID());
        _pingpongFBO[1].AttachTexture(TextureAttachment::Color0, _pingpongColorbuffers[1].GetID());
        _pingpongFBO[0].Status();
        _pingpongFBO[1].Status();
    }

    ~Bloom() {
    }

    void Blur(Shader& blurShader, int amount) {
        bool first_iteration = true;
        _horizontal = true;
        //glBindVertexArray(_emptyVAO);
        for (int i = 0; i < 10; i++) {
            this->BindPingPong(_horizontal);
            blurShader.SetInt("horizontal", _horizontal);
            auto& textureUnit = first_iteration ? _colorBuffers[1] : _pingpongColorbuffers[!_horizontal];
            textureUnit.Bind(0);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            _horizontal = !_horizontal;
            if (first_iteration)
                first_iteration = false;
        }
    }

    void BindTextures() {
        _colorBuffers[0].Bind(0);
        _pingpongColorbuffers[!_horizontal].Bind(1);
    }

    void BindPingPong(int index) {
        _pingpongFBO[index].Bind();
    }

    void BindHdrFramebuffer() {
        _hdrFBO.Bind();
    }

    GLuint GetHdrFBO() {
        return _hdrFBO.GetId();
    }

    Bloom(const Bloom&) = delete;
    Bloom& operator=(const Bloom&) = delete;

private:
    int _width, _height;
    bool _horizontal = true;
    FrameBuffer _hdrFBO;
    std::array<Texture2D, 2> _colorBuffers;
    RenderBuffer _depthRBO;
    std::array<FrameBuffer, 2> _pingpongFBO;
    std::array<Texture2D, 2> _pingpongColorbuffers;
};
