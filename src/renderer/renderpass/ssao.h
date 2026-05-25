#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <random>
#include <iostream>
#include "shader.h"
#include "texture_slots.h"
#include "gl/texture.h"
#include "gl/frame_buffer.h"
#include "gl/vertex_array.h"

class SSAO {
public:
    SSAO(int scrWidth, int scrHeight) :
        _scrWidth(scrWidth), _scrHeight(scrHeight),
        _noiseTexture(generateNoiseTexture()),
        _ssaoKernel(generateSampleKernel()),
        _ssaoFBO(),
        _ssaoBlurFBO()
    {
        this->Init(scrWidth, scrHeight);
    }

    ~SSAO() {
    }

    SSAO(const SSAO&) = delete;
    SSAO& operator=(const SSAO&) = delete;
    SSAO(SSAO&&) noexcept = default;
    SSAO& operator=(SSAO&&) noexcept = default;

    void Init(int scrWidth, int scrHeight) {
        //TODO check wrap
        _ssaoColorBuffer = Texture2D(scrWidth, scrHeight, TextureFormat::R8);
        _ssaoColorBufferBlur = Texture2D(scrWidth, scrHeight, TextureFormat::R8);

        _ssaoColorBuffer.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _ssaoColorBufferBlur.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);

        _ssaoFBO.AttachTexture(TextureAttachment::Color0, _ssaoColorBuffer.GetID());
        _ssaoBlurFBO.AttachTexture(TextureAttachment::Color0, _ssaoColorBufferBlur.GetID());
    }

    void Resize(int scrWidth, int scrHeight) {
        _scrWidth = scrWidth;
        _scrHeight = scrHeight;
        this->Init(scrWidth, scrHeight);
    }

    void Run(Shader& shaderSSAO) {
        _ssaoFBO.Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAO.Activate();
        // Send kernel + rotation
        //TODO this can be UBO
        shaderSSAO.SetVec2("noiseScale", glm::vec2(_scrWidth / 4.0f, _scrHeight / 4.0f));
        for (unsigned int i = 0; i < 64; ++i)
            shaderSSAO.SetVec3("samples[" + std::to_string(i) + "]", _ssaoKernel[i]);

        _noiseTexture.Bind(slot(TextureSlot::SSAO));

        // render quad
        _emptyVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        _ssaoFBO.Unbind();
    }

    void Blur(Shader& shaderBlurSSAO) {
        _ssaoBlurFBO.Bind();
        glClear(GL_COLOR_BUFFER_BIT);
        shaderBlurSSAO.Activate();
        _ssaoColorBuffer.Bind(slot(TextureSlot::SSAO));

        // render quad
        _emptyVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        _ssaoBlurFBO.Unbind();
    }

    void BindSSAOTexture() const {
        _ssaoColorBufferBlur.Bind(slot(TextureSlot::SSAO));
    }

private:
    std::vector<glm::vec3> generateSampleKernel(int size = 64) { //TODO size unused
        // generate sample kernel
        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
        std::default_random_engine generator;
        std::vector<glm::vec3> ssaoKernel;
        for (unsigned int i = 0; i < 64; ++i) {
            glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
            sample = glm::normalize(sample);
            sample *= randomFloats(generator);
            float scale = float(i) / 64.0f;

            // scale samples s.t. they're more aligned to center of kernel
            //scale = glm::mix(0.1f, 1.0f, scale * scale); TODO replace?
            scale = ourLerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            ssaoKernel.push_back(sample);
        }
        return ssaoKernel;
    }

    float ourLerp(float a, float b, float f) {
        return a + f * (b - a);
    }

    Texture2D generateNoiseTexture() {
        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
        std::default_random_engine generator;
        std::vector<glm::vec3> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++) {
            glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
            ssaoNoise.push_back(noise);
        }

        Texture2D noise(4, 4, TextureFormat::RGB16F, ssaoNoise.data());
        noise.SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        noise.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        return noise;
    }

    int _scrWidth = 0, _scrHeight = 0;
    VertexArray _emptyVAO;
    Texture2D _ssaoColorBuffer;
    Texture2D _ssaoColorBufferBlur;
    Texture2D _noiseTexture;
    std::vector<glm::vec3> _ssaoKernel;
    FrameBuffer _ssaoFBO;
    FrameBuffer _ssaoBlurFBO;
};
