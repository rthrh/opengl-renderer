#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>
#include <tinyexr.h>

#include <vector>
#include <string>
#include <ranges>
#include <span>

#include "renderer/shader.h"
#include "utils/logger.h"
#include "renderer/shapes.h"
#include "renderer/texture_slots.h"

class Skybox {
public:
    Skybox(std::filesystem::path path, Shader& equirectShader, Shader& irradianceShader, Shader& prefilterShader, Shader& brdfShader, GLsizei cubeSize = 2048) :
        _cubeSize(cubeSize)
    {
        initBuffers();
        initTextureHDR(std::move(path));
        initCubemap();
        initIrradianceMap();
        initPrefilteredMap();
        initBrdfLUT();
        initCube();

        equirectToEnvMap(equirectShader);
        glGenerateTextureMipmap(_envCubemap);
        bakeIrradianceMap(irradianceShader);
        bakePrefilteredMap(prefilterShader);
        bakeBrdfLUT(brdfShader);
    }

    ~Skybox() {
        //TODO FBO, RBO, HDR texture can be removed after init
        glDeleteTextures(1, &_envCubemap);
        glDeleteTextures(1, &_hdrTexture);
        glDeleteFramebuffers(1, &_captureFBO);
        glDeleteRenderbuffers(1, &_captureRBO);
        glDeleteVertexArrays(1, &_cubeVAO);
        glDeleteBuffers(1, &_cubeVBO);
        glDeleteVertexArrays(1, &_emptyVAO);
        glDeleteTextures(1, &_irradianceCubemap);
        glDeleteTextures(1, &_prefilteredCubemap);
        glDeleteTextures(1, &_brdfLUT);
    }

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void Draw(Shader& skyboxShader, glm::mat4 view, glm::mat4 projection) {
        skyboxShader.Activate();
        skyboxShader.SetMat4("view", view);
        skyboxShader.SetMat4("projection", projection);

        glBindTextureUnit(slot(SlotOther::Skybox), _envCubemap);
        renderCube();
    }

    void BindTexturesIBL() const {
        glBindTextureUnit(slot(SlotOther::Irradiance), _irradianceCubemap);
        glBindTextureUnit(slot(SlotOther::PrefilterEnv), _prefilteredCubemap);
        glBindTextureUnit(slot(SlotOther::BrdfLUT), _brdfLUT);
    }

private:
    // Initializes capture FBO and RBO
    void initBuffers() {
        glCreateFramebuffers(1, &_captureFBO);
        glCreateRenderbuffers(1, &_captureRBO);
        glNamedRenderbufferStorage(_captureRBO, GL_DEPTH_COMPONENT24, _cubeSize, _cubeSize);
        glNamedFramebufferRenderbuffer(_captureFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _captureRBO);
    }

    void initTextureHDR(const std::filesystem::path& path) {
        int width, height, nrComponents;
        float *data = nullptr;

        if (path.extension() == ".exr") {
            const char* err = nullptr;
            int ret = LoadEXR(&data, &width, &height, path.string().c_str(), &err);
            if (ret != TINYEXR_SUCCESS) {
                std::string errMsg = err ? err : "unknown";
                FreeEXRErrorMessage(err);
                throw std::runtime_error("Failed to load EXR image: " + errMsg);
            }
            nrComponents = 4;
        } else {
            //stbi_set_flip_vertically_on_load(true);
            data = stbi_loadf(path.string().c_str(), &width, &height, &nrComponents, 0);
            if (!data)
                throw std::runtime_error("Failed to load HDR image: " + path.string());
        }

        GLenum storageFormat = (nrComponents == 4) ? GL_RGBA16F : GL_RGB16F;
        GLenum subImageFormat = (nrComponents == 4) ? GL_RGBA : GL_RGB;

        // Flip vertically TODO move it to function
        const int rowSize = width * nrComponents;
        auto rows = std::span(data, height * rowSize) | std::views::chunk(rowSize);

        // Zip the top half and bottom half together, then swap their contents
        for (auto [top, bottom] : std::views::zip(rows | std::views::take(height / 2), rows | std::views::reverse)) {
            std::ranges::swap_ranges(top, bottom);
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &_hdrTexture);
        glTextureStorage2D(_hdrTexture, 1, storageFormat, width, height);
        glTextureSubImage2D(_hdrTexture, 0, 0, 0, width, height, subImageFormat, GL_FLOAT, data);
        glTextureParameteri(_hdrTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_hdrTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_hdrTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_hdrTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (path.extension() == ".exr") {
            free(data);
        } else {
            stbi_image_free(data);
        }
    }

    // setup cubemap to render to and attach to framebuffer
    void initCubemap() {
        int mipLevels = static_cast<int>(std::floor(std::log2(_cubeSize))) + 1;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_envCubemap);
        glTextureStorage2D(_envCubemap, mipLevels, GL_RGB16F, _cubeSize, _cubeSize);
        glTextureParameteri(_envCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_envCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_envCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_envCubemap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(_envCubemap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Init cube geometry
    void initCube() {
        glCreateVertexArrays(1, &_emptyVAO);
        glCreateVertexArrays(1, &_cubeVAO);
        glCreateBuffers(1, &_cubeVBO);
        glNamedBufferStorage(_cubeVBO, sizeof(cube_map_vertices), cube_map_vertices, 0);

        glVertexArrayVertexBuffer(_cubeVAO, 0, _cubeVBO, 0, 8 * sizeof(float));

        glVertexArrayAttribFormat(_cubeVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribFormat(_cubeVAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
        glVertexArrayAttribFormat(_cubeVAO, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));

        glVertexArrayAttribBinding(_cubeVAO, 0, 0);
        glVertexArrayAttribBinding(_cubeVAO, 1, 0);
        glVertexArrayAttribBinding(_cubeVAO, 2, 0);

        glEnableVertexArrayAttrib(_cubeVAO, 0);
        glEnableVertexArrayAttrib(_cubeVAO, 1);
        glEnableVertexArrayAttrib(_cubeVAO, 2);
    }

    // Converts equirectangular map to cube map
    void equirectToEnvMap(Shader& equirectShader) {
        // TODO these are only used once
        _captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        _captureViews =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        equirectShader.Activate();
        equirectShader.SetMat4("projection", _captureProjection);
        glBindTextureUnit(0, _hdrTexture); // TODO set binding to enum

        glViewport(0, 0, _cubeSize, _cubeSize);
        glBindFramebuffer(GL_FRAMEBUFFER, _captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            equirectShader.SetMat4("view", _captureViews[i]);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _envCubemap, 0, i);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            this->renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderCube() {
        glBindVertexArray(_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 cubemap triangles
    }

    void initIrradianceMap() {
        constexpr int size = 32; // scaled down
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_irradianceCubemap);
        glTextureStorage2D(_irradianceCubemap, 1, GL_RGB16F, size, size);
        glTextureParameteri(_irradianceCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_irradianceCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_irradianceCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_irradianceCubemap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_irradianceCubemap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void bakeIrradianceMap(Shader& irradianceShader) {
        // resize capture RBO to 32x32
        glNamedRenderbufferStorage(_captureRBO, GL_DEPTH_COMPONENT24, 32, 32);

        irradianceShader.Activate();
        irradianceShader.SetMat4("projection", _captureProjection);
        glBindTextureUnit(slot(SlotOther::Skybox), _envCubemap);

        glViewport(0, 0, 32, 32);
        glBindFramebuffer(GL_FRAMEBUFFER, _captureFBO);
        for (unsigned int i = 0; i < 6; i++) {
            irradianceShader.SetMat4("view", _captureViews[i]);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _irradianceCubemap, 0, i);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void initPrefilteredMap() {
        constexpr int size = 128;
        constexpr int mipLevels = 5;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_prefilteredCubemap);
        glTextureStorage2D(_prefilteredCubemap, mipLevels, GL_RGB16F, size, size);
        glTextureParameteri(_prefilteredCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_prefilteredCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_prefilteredCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_prefilteredCubemap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(_prefilteredCubemap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void bakePrefilteredMap(Shader& prefilterShader) {
        prefilterShader.Activate();
        prefilterShader.SetMat4("projection", _captureProjection);

        glBindTextureUnit(slot(SlotOther::Skybox), _envCubemap);
        glBindFramebuffer(GL_FRAMEBUFFER, _captureFBO);

        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            glNamedRenderbufferStorage(_captureRBO, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader.SetFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                prefilterShader.SetMat4("view", _captureViews[i]);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _prefilteredCubemap, mip, i);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                renderCube();
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void initBrdfLUT() {
        glCreateTextures(GL_TEXTURE_2D, 1, &_brdfLUT);
        glTextureStorage2D(_brdfLUT, 1, GL_RG16F, 512, 512);
        glTextureParameteri(_brdfLUT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_brdfLUT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_brdfLUT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_brdfLUT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void bakeBrdfLUT(Shader& brdfShader) {
        constexpr int size = 512;

        GLuint lutFBO, lutRBO;
        glCreateFramebuffers(1, &lutFBO);
        glCreateRenderbuffers(1, &lutRBO);
        glNamedRenderbufferStorage(lutRBO, GL_DEPTH_COMPONENT24, size, size);
        glNamedFramebufferRenderbuffer(lutFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, lutRBO);
        glNamedFramebufferTexture(lutFBO, GL_COLOR_ATTACHMENT0, _brdfLUT, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, lutFBO);

        GLenum status = glCheckNamedFramebufferStatus(lutFBO, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) Error("BRDF LUT FBO incomplete: {}", status);

        glViewport(0, 0, size, size);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        brdfShader.Activate();
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &lutFBO);
        glDeleteRenderbuffers(1, &lutRBO);
    }

    GLuint _captureFBO = 0;
    GLuint _captureRBO = 0;
    GLuint _envCubemap = 0;
    GLuint _hdrTexture = 0;
    GLuint _cubeVAO = 0;
    GLuint _cubeVBO = 0;
    GLsizei _cubeSize = 0;
    GLuint _emptyVAO = 0;
    GLuint _irradianceCubemap = 0;
    GLuint _prefilteredCubemap = 0;
    GLuint _brdfLUT = 0;

    std::array<glm::mat4, 6> _captureViews;
    glm::mat4 _captureProjection;
};
