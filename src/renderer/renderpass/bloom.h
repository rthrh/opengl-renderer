#pragma once

#include <gl/headers.h>

#include "utils/logger.h"
#include "renderer/shader.h"
#include "gl/frame_buffer.h"
#include "gl/texture.h"


class Bloom {
public:
    Bloom(int scrWidth, int scrHeight) :
        _scrWidth(scrWidth), _scrHeight(scrHeight)
    {
        this->Init(scrWidth, scrHeight);
    }

    ~Bloom() = default;

    Bloom(const Bloom&) = delete;
    Bloom& operator=(const Bloom&) = delete;

    void Init(int scrWidth, int scrHeight) {
        _hdrColor = Texture2D(scrWidth, scrHeight, TextureFormat::RGBA16F);
        _hdrColor.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        _hdrColor.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

        _depthRBO = RenderBuffer(scrWidth, scrHeight, TextureFormat::Depth32F);

        _bloomFBO = FrameBuffer();
        _hdrFBO = FrameBuffer();
        _hdrFBO.AttachTexture(TextureAttachment::Color0, _hdrColor.GetID());
        _hdrFBO.AttachRenderBuffer(TextureAttachment::Depth, _depthRBO);
        _hdrFBO.Status();
        this->initMipmaps(_scrWidth, _scrHeight);
    }

    void Resize(int scrWidth, int scrHeight) {
        _scrWidth = scrWidth;
        _scrHeight = scrHeight;
        _mipChain.clear();
        this->Init(scrWidth, scrHeight);
    }

    void BindHdrFramebuffer() {
        _hdrFBO.Bind();
    }

    GLuint GetHdrFBO() {
        return _hdrFBO.GetId();
    }

    void BindTextures() {
        _hdrColor.Bind(0);
        _mipChain[0].Bind(1);
    }

    void RenderUpsamples(Shader& upsampleShader, float filterRadius = 0.005f)
    {
        _bloomFBO.Bind();

        upsampleShader.Activate();
        upsampleShader.SetFloat("filterRadius", filterRadius);
        upsampleShader.SetInt("srcTexture", 0);

        // Enable additive blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        for (int i = (int)_mipChain.size() - 1; i > 0; i--)
        {
            const auto& mip = _mipChain[i];
            const auto& nextMip = _mipChain[i-1];

            // Bind viewport and texture from where to read
            mip.Bind(0);

            // Set framebuffer render target (we write to this texture)
            glViewport(0, 0, nextMip.GetWidth(), nextMip.GetHeight());
            _bloomFBO.AttachTexture(TextureAttachment::Color0, nextMip.GetID());

            // Render screen-filled quad of resolution of current mip
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Disable additive blending
        //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        glUseProgram(0);
    }


    void RenderDownsamples(Shader& downsampleShader)
    {
        _bloomFBO.Bind();

        downsampleShader.Activate();
        downsampleShader.SetInt("mipLevel", 1); // default
        downsampleShader.SetVec2("srcResolution", {_scrWidth, _scrHeight});
        downsampleShader.SetInt("srcTexture", 0);
        if (_karisAverage) {
            downsampleShader.SetInt("mipLevel", 0);
        }

        // Bind srcTexture (HDR color buffer) as initial texture input
        _hdrColor.Bind(0);

        // Progressively downsample through the mip chain
        for (auto i = 0u; i < _mipChain.size(); i++)
        {
            auto& mip = _mipChain[i];
            glViewport(0, 0, mip.GetWidth(), mip.GetHeight());
            _bloomFBO.AttachTexture(TextureAttachment::Color0, mip.GetID());


            // Render screen-filled quad of resolution of current mip
            //renderQuad();
            glDrawArrays(GL_TRIANGLES, 0, 3);

            // Set current mip resolution as srcResolution for next iteration
            downsampleShader.SetVec2("srcResolution", { mip.GetWidth(), mip.GetHeight() });
            // Set current mip as texture input for next iteration
            mip.Bind(0);
            // Disable Karis average for consequent downsamples
            if (i == 0) { downsampleShader.SetInt("mipLevel", 1); }
        }

        glUseProgram(0);
    }

private:
	void initMipmaps(unsigned windowWidth, unsigned windowHeight, const unsigned numBloomMips = 6) { //TODO numBLoomMips to config
        glm::ivec2 mipIntSize((int)windowWidth, (int)windowHeight);
        _mipChain.reserve(numBloomMips);
        for (unsigned int i = 0; i < numBloomMips; i++) {
            mipIntSize /= 2;
            if (mipIntSize.x < 1 || mipIntSize.y < 1) return;
            _mipChain.emplace_back(mipIntSize.x, mipIntSize.y, TextureFormat::R11F_G11F_B10F);
            Texture2D& mip = _mipChain.back();
            mip.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
            mip.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        }
    }

    int _scrWidth, _scrHeight;
    FrameBuffer _hdrFBO;
	FrameBuffer _bloomFBO;
    Texture2D _hdrColor;
    std::vector<Texture2D> _mipChain;
    RenderBuffer _depthRBO;
	bool _karisAverage = true;
};
