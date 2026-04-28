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
        _scrWidth(scrWidth), _scrHeight(scrHeight)
    {
        init(scrWidth, scrHeight);
        _ssaoKernel = generateSampleKernel();
        generateNoiseTexture();

        glCreateVertexArrays(1, &_emptyVAO);

    }

    ~SSAO() {
        glDeleteFramebuffers(1, &_ssaoFBO);
        glDeleteFramebuffers(1, &_ssaoBlurFBO);
        glDeleteTextures(1, &_ssaoColorBuffer);
        glDeleteTextures(1, &_ssaoColorBufferBlur);
        glDeleteTextures(1, &_noiseTexture);
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

        glBindTextureUnit(slot(SlotOther::SSAO), _noiseTexture);

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
        glBindTextureUnit(slot(SlotOther::SSAO), _ssaoColorBuffer);

        // render quad
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void BindSSAOTexture() const {
        glBindTextureUnit(slot(SlotOther::SSAO), _ssaoColorBufferBlur);
    }

private:

    void init(int scrWidth, int scrHeight) {
        // also create framebuffer to hold SSAO processing stage 
        // -----------------------------------------------------
        glGenFramebuffers(1, &_ssaoFBO);  glGenFramebuffers(1, &_ssaoBlurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, _ssaoFBO);
        // SSAO color buffer
        glGenTextures(1, &_ssaoColorBuffer);
        glBindTexture(GL_TEXTURE_2D, _ssaoColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, scrWidth, scrHeight, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _ssaoColorBuffer, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SSAO Framebuffer not complete!" << std::endl;
        // and blur stage
        glBindFramebuffer(GL_FRAMEBUFFER, _ssaoBlurFBO);
        glGenTextures(1, &_ssaoColorBufferBlur);
        glBindTexture(GL_TEXTURE_2D, _ssaoColorBufferBlur);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, scrWidth, scrHeight, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _ssaoColorBufferBlur, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::vector<glm::vec3> generateSampleKernel(int size = 64) { //TODO size unused
        // generate sample kernel
        // ----------------------
        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
        std::default_random_engine generator;
        std::vector<glm::vec3> ssaoKernel;
        for (unsigned int i = 0; i < 64; ++i) {
            glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
            sample = glm::normalize(sample);
            sample *= randomFloats(generator);
            float scale = float(i) / 64.0f;

            // scale samples s.t. they're more aligned to center of kernel
            //scale = glm::mix(0.1f, 1.0f, scale * scale); todo check this replacement
            scale = ourLerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            ssaoKernel.push_back(sample);
        }
        return ssaoKernel;
    }

    float ourLerp(float a, float b, float f) {
        return a + f * (b - a);
    }

    void generateNoiseTexture() {
        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
        std::default_random_engine generator;
        std::vector<glm::vec3> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++) {
            glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
            ssaoNoise.push_back(noise);
        }
        glCreateTextures(GL_TEXTURE_2D, 1, &_noiseTexture);
        glTextureStorage2D(_noiseTexture, 1, GL_RGB16F, 4, 4);
        glTextureSubImage2D(_noiseTexture, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, ssaoNoise.data());
        glTextureParameteri(_noiseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(_noiseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(_noiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_noiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    int _scrWidth = 0, _scrHeight = 0;
    GLuint _ssaoFBO = 0;
    GLuint _ssaoBlurFBO = 0;
    GLuint _ssaoColorBuffer = 0;
    GLuint _ssaoColorBufferBlur = 0;
    GLuint _noiseTexture = 0;
    GLuint _emptyVAO = 0;
    std::vector<glm::vec3> _ssaoKernel;
};
