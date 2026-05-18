#pragma once
#include <glad/glad.h>
#include <iostream>

#include "utils/logger.h"
#include "renderer/shader.h"
#include "gl/render_buffer.h"
#include "gl/frame_buffer.h"
#include "gl/texture.h"



// bloom stuff
struct bloomMip
{
	glm::vec2 size;
	glm::ivec2 intSize;
	unsigned int texture;
};

class bloomFBO
{
public:
	bloomFBO();
	~bloomFBO();
	bool Init(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength);
	void Destroy();
	void BindForWriting();
	const std::vector<bloomMip>& MipChain() const;

private:
	bool mInit;
	unsigned int mFBO;
	std::vector<bloomMip> mMipChain;
};

bloomFBO::bloomFBO() : mInit(false) {}
bloomFBO::~bloomFBO() {}

bool bloomFBO::Init(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength)
{
	if (mInit) return true;

	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

	glm::vec2 mipSize((float)windowWidth, (float)windowHeight);
	glm::ivec2 mipIntSize((int)windowWidth, (int)windowHeight);
	// Safety check
	if (windowWidth > (unsigned int)INT_MAX || windowHeight > (unsigned int)INT_MAX) {
		std::cerr << "Window size conversion overflow - cannot build bloom FBO!" << std::endl;
		return false;
	}

	for (GLuint i = 0; i < mipChainLength; i++)
	{
		bloomMip mip;

		mipSize *= 0.5f;
		mipIntSize /= 2;
		mip.size = mipSize;
		mip.intSize = mipIntSize;

		glGenTextures(1, &mip.texture);
		glBindTexture(GL_TEXTURE_2D, mip.texture);
		// we are downscaling an HDR color buffer, so we need a float texture format
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F,
		             (int)mipSize.x, (int)mipSize.y,
		             0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		std::cout << "Created bloom mip " << mipIntSize.x << 'x' << mipIntSize.y << std::endl;
		mMipChain.emplace_back(mip);
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	                       GL_TEXTURE_2D, mMipChain[0].texture, 0);

	// setup attachments
	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	// check completion status
	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("gbuffer FBO error, status: 0x%x\n", status);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	mInit = true;
	return true;
}

void bloomFBO::Destroy()
{
	for (int i = 0; i < (int)mMipChain.size(); i++) {
		glDeleteTextures(1, &mMipChain[i].texture);
		mMipChain[i].texture = 0;
	}
	glDeleteFramebuffers(1, &mFBO);
	mFBO = 0;
	mInit = false;
}

void bloomFBO::BindForWriting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
}

const std::vector<bloomMip>& bloomFBO::MipChain() const
{
	return mMipChain;
}


//. TODO refactor
class Bloom {
public:
    Bloom(int scrWidth, int scrHeight) :
        _width(scrWidth), _height(scrHeight)
    {
        _hdrColor = Texture2D(scrWidth, scrHeight, TextureFormat::RGBA16F);
        _hdrColor.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        _hdrColor.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        _depthRBO = RenderBuffer(scrWidth, scrHeight, TextureFormat::Depth24);
        _hdrFBO = FrameBuffer({TextureAttachment::Color0});
        _hdrFBO.AttachTexture(TextureAttachment::Color0, _hdrColor.GetID());
        _hdrFBO.AttachRenderBuffer(TextureAttachment::Depth, _depthRBO);
        _hdrFBO.Status();
        this->Init(scrWidth, scrHeight);
    }

    ~Bloom() = default;

    Bloom(const Bloom&) = delete;
    Bloom& operator=(const Bloom&) = delete;

	bool Init(unsigned int windowWidth, unsigned int windowHeight) {
	    if (mInit) return true;
        mSrcViewportSize = glm::ivec2(windowWidth, windowHeight);
        mSrcViewportSizeFloat = glm::vec2((float)windowWidth, (float)windowHeight);

        // Framebuffer
        const unsigned int num_bloom_mips = 6; // TODO: Play around with this value
        bool status = mFBO.Init(windowWidth, windowHeight, num_bloom_mips);
        if (!status) {
            std::cerr << "Failed to initialize bloom FBO - cannot create bloom renderer!\n";
            return false;
        }
        return true;
    }

    void BindHdrFramebuffer() {
        _hdrFBO.Bind();
    }

    GLuint GetHdrFBO() {
        return _hdrFBO.GetId();
    }

    void BindTextures() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _hdrColor.GetID());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, BloomTexture());
    }

    void RenderUpsamples(Shader& mUpsampleShader, float filterRadius = 0.005f)
    {
        mFBO.BindForWriting();
        const std::vector<bloomMip>& mipChain = mFBO.MipChain();

        mUpsampleShader.Activate();
        mUpsampleShader.SetFloat("filterRadius", filterRadius);
        mUpsampleShader.SetInt("srcTexture", 0);

        // Enable additive blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        for (int i = (int)mipChain.size() - 1; i > 0; i--)
        {
            const bloomMip& mip = mipChain[i];
            const bloomMip& nextMip = mipChain[i-1];

            // Bind viewport and texture from where to read
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mip.texture);

            // Set framebuffer render target (we write to this texture)
            glViewport(0, 0, nextMip.size.x, nextMip.size.y);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, nextMip.texture, 0);

            // Render screen-filled quad of resolution of current mip
            //renderQuad();
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Disable additive blending
        //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        glUseProgram(0);
    }


    void RenderDownsamples(Shader& mDownsampleShader, unsigned int srcTexture = 0)
    {
        mFBO.BindForWriting(); //TODO redundant
        const std::vector<bloomMip>& mipChain = mFBO.MipChain();

        mDownsampleShader.Activate();
        mDownsampleShader.SetVec2("srcResolution", mSrcViewportSizeFloat);
        mDownsampleShader.SetInt("srcTexture", 0);
        if (mKarisAverageOnDownsample) {
            mDownsampleShader.SetInt("mipLevel", 0);
        }

        // Bind srcTexture (HDR color buffer) as initial texture input
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _hdrColor.GetID());

        // Progressively downsample through the mip chain
        for (int i = 0; i < (int)mipChain.size(); i++)
        {
            const bloomMip& mip = mipChain[i];
            glViewport(0, 0, mip.size.x, mip.size.y);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, mip.texture, 0);

            // Render screen-filled quad of resolution of current mip
            //renderQuad();
            glDrawArrays(GL_TRIANGLES, 0, 3);

            // Set current mip resolution as srcResolution for next iteration
            mDownsampleShader.SetVec2("srcResolution", mip.size);
            // Set current mip as texture input for next iteration
            glBindTexture(GL_TEXTURE_2D, mip.texture);
            // Disable Karis average for consequent downsamples
            if (i == 0) { mDownsampleShader.SetInt("mipLevel", 1); }
        }

        glUseProgram(0);
    }

    void Destroy()
    {
        mFBO.Destroy();
    }

    GLuint BloomTexture() {
        return mFBO.MipChain()[0].texture;
    }

    GLuint BloomMip_i(int index)
    {
        const std::vector<bloomMip>& mipChain = mFBO.MipChain();
        int size = (int)mipChain.size();
        return mipChain[(index > size-1) ? size-1 : (index < 0) ? 0 : index].texture;
    }



private:
    int _width, _height;
    FrameBuffer _hdrFBO;
	bool mInit = false;
	bloomFBO mFBO;
	glm::ivec2 mSrcViewportSize;
	glm::vec2 mSrcViewportSizeFloat;
    Texture2D _hdrColor;
    RenderBuffer _depthRBO;
	bool mKarisAverageOnDownsample = true;
};
