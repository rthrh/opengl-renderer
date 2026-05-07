#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <random>
#include <iostream>
#include "shader.h"
#include "texture_slots.h"

class SSAO {
public:
    SSAO(int scrWidth, int scrHeight) : 
        _scrWidth(scrWidth), _scrHeight(scrHeight),
        _ssaoColorBuffer(scrWidth, scrHeight, TextureFormat::R8),
        _ssaoColorBufferBlur(scrWidth, scrHeight, TextureFormat::R8),
        _noiseTexture(generateNoiseTexture())
    {
        //TODO check wrap
        _ssaoColorBuffer.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
        _ssaoColorBufferBlur.SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);

        glCreateFramebuffers(1, &_ssaoFBO);
        glNamedFramebufferTexture(_ssaoFBO, GL_COLOR_ATTACHMENT0, _ssaoColorBuffer.GetID(), 0);
        if (glCheckNamedFramebufferStatus(_ssaoFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) Error("SSAO FBO incomplete");

        glCreateFramebuffers(1, &_ssaoBlurFBO);
        glNamedFramebufferTexture(_ssaoBlurFBO, GL_COLOR_ATTACHMENT0, _ssaoColorBufferBlur.GetID(), 0);
        if (glCheckNamedFramebufferStatus(_ssaoBlurFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) Error("SSAO Blur FBO incomplete");

        _ssaoKernel = generateSampleKernel();

        glCreateVertexArrays(1, &_emptyVAO);

    }

    ~SSAO() {
        glDeleteFramebuffers(1, &_ssaoFBO);
        glDeleteFramebuffers(1, &_ssaoBlurFBO);
        glDeleteVertexArrays(1, &_emptyVAO);

    }

    SSAO(const SSAO&) = delete;
    SSAO& operator=(const SSAO&) = delete;

    // 2. generate SSAO texture
    void Run(Shader& shaderSSAO) {
        glBindFramebuffer(GL_FRAMEBUFFER, _ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderSSAO.Activate();
        // Send kernel + rotation 
        shaderSSAO.SetVec2("noiseScale", glm::vec2(_scrWidth / 4.0f, _scrHeight / 4.0f));
        for (unsigned int i = 0; i < 64; ++i)
            shaderSSAO.SetVec3("samples[" + std::to_string(i) + "]", _ssaoKernel[i]);

        _noiseTexture.Bind(slot(SlotOther::SSAO));

        // render quad
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 3. blur SSAO texture to remove noise
    void Blur(Shader& shaderBlurSSAO) {
        glBindFramebuffer(GL_FRAMEBUFFER, _ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        shaderBlurSSAO.Activate();
        _ssaoColorBuffer.Bind(slot(SlotOther::SSAO));

        // render quad
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void BindSSAOTexture() const {
        _ssaoColorBufferBlur.Bind(slot(SlotOther::SSAO));
    }

private:
    std::vector<glm::vec3> generateSampleKernel(int size = 64) { //TODO size unused
        // generate sample kernel
        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
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
        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
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
    GLuint _ssaoFBO = 0;
    GLuint _ssaoBlurFBO = 0;
    GLuint _emptyVAO = 0;
    Texture2D _ssaoColorBuffer;
    Texture2D _ssaoColorBufferBlur;
    Texture2D _noiseTexture;
    std::vector<glm::vec3> _ssaoKernel;
};
