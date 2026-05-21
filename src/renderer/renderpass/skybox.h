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
#include "gl/texture.h"
#include "gl/render_buffer.h"
#include "gl/frame_buffer.h"

class Skybox {
public:
    Skybox(std::filesystem::path path, Shader& equirectShader, Shader& irradianceShader, Shader& prefilterShader, Shader& brdfShader, GLsizei cubeSize = 2048) :
        _cubeSize(cubeSize),
        _envCubemap(cubeSize, TextureFormat::RGB16F, true),
        _hdrTexture(initTextureHDR(std::move(path))),
        _irradianceCubemap(initIrradianceMap()),
        _prefilteredCubemap(initPrefilteredMap()),
        _brdfLUT(initBrdfLUT())
    {
        initBuffers();
        _envCubemap.SetFilter(TextureFilter::LinearMipMapLinear, TextureFilter::Linear);
        _envCubemap.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

        initCube();

        equirectToEnvMap(equirectShader);
        _envCubemap.GenerateMipmap();

        bakeIrradianceMap(irradianceShader);
        bakePrefilteredMap(prefilterShader);
        bakeBrdfLUT(brdfShader);
    }

    ~Skybox() {
        //TODO FBO, RBO, HDR texture can be removed after init
        glDeleteVertexArrays(1, &_cubeVAO);
        glDeleteBuffers(1, &_cubeVBO);
        glDeleteVertexArrays(1, &_emptyVAO);
    }

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void Draw(Shader& skyboxShader, glm::mat4 view, glm::mat4 projection) {
        skyboxShader.Activate();
        skyboxShader.SetMat4("view", view);
        skyboxShader.SetMat4("projection", projection);

        _envCubemap.Bind(slot(SlotOther::Skybox));
        renderCube();
    }

    void BindTexturesIBL() const {
        _irradianceCubemap.Bind(slot(SlotOther::Irradiance));
        _prefilteredCubemap.Bind(slot(SlotOther::PrefilterEnv));
        _brdfLUT.Bind(slot(SlotOther::BrdfLUT));
    }

private:
    // Initializes capture FBO and RBO
    void initBuffers() {
        _captureFBO = FrameBuffer();
        _captureRBO = RenderBuffer(_cubeSize, _cubeSize, TextureFormat::Depth32F);
        _captureFBO.AttachRenderBuffer(TextureAttachment::Depth, _captureRBO);
    }

    Texture2D initTextureHDR(const std::filesystem::path& path) {
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
            data = stbi_loadf(path.string().c_str(), &width, &height, &nrComponents, 0);
            if (!data)
                throw std::runtime_error("Failed to load HDR image: " + path.string());
        }

        TextureFormat storageFormat = (nrComponents == 4) ? TextureFormat::RGBA16F : TextureFormat::RGB16F;

        // Flip vertically
        const int rowSize = width * nrComponents;
        auto rows = std::span(data, height * rowSize) | std::views::chunk(rowSize);

        // Zip the top half and bottom half together, then swap their contents
        for (auto [top, bottom] : std::views::zip(rows | std::views::take(height / 2), rows | std::views::reverse)) {
            std::ranges::swap_ranges(top, bottom);
        }

        Texture2D hdrTexture(width, height, storageFormat, data);
        hdrTexture.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        hdrTexture.SetFilter(TextureFilter::Linear, TextureFilter::Linear);

        if (path.extension() == ".exr") {
            free(data);
        } else {
            stbi_image_free(data);
        }

        return hdrTexture;
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
        _hdrTexture.Bind(0);

        glViewport(0, 0, _cubeSize, _cubeSize);
        _captureFBO.Bind();
        for (unsigned int i = 0; i < 6; ++i)
        {
            equirectShader.SetMat4("view", _captureViews[i]);
            _captureFBO.AttachTextureLayer(TextureAttachment::Color0, _envCubemap.GetID(), i);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            this->renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderCube() {
        glBindVertexArray(_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 cubemap triangles
    }

    TextureCube initIrradianceMap() {
        constexpr int size = 32; // scaled down
        TextureCube irradiance(size, TextureFormat::RGB16F);
        irradiance.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        irradiance.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        return irradiance;
    }

    void bakeIrradianceMap(Shader& irradianceShader) {
        // resize capture RBO to 32x32
        _captureRBO.Resize(32, 32);

        irradianceShader.Activate();
        irradianceShader.SetMat4("projection", _captureProjection);
        _envCubemap.Bind(slot(SlotOther::Skybox));

        glViewport(0, 0, 32, 32);
        _captureFBO.Bind();
        for (unsigned int i = 0; i < 6; i++) {
            irradianceShader.SetMat4("view", _captureViews[i]);
            _captureFBO.AttachTextureLayer(TextureAttachment::Color0, _irradianceCubemap.GetID(), i);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    TextureCube initPrefilteredMap() {
        constexpr int size = 128;
        TextureCube prefiltered(size, TextureFormat::RGB16F, true);
        prefiltered.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        prefiltered.SetFilter(TextureFilter::LinearMipMapLinear, TextureFilter::Linear);
        return prefiltered;
    }

    void bakePrefilteredMap(Shader& prefilterShader) {
        prefilterShader.Activate();
        prefilterShader.SetMat4("projection", _captureProjection);

        _envCubemap.Bind(slot(SlotOther::Skybox));
        _captureFBO.Bind();

        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            _captureRBO.Resize(mipWidth, mipHeight);

            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader.SetFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                prefilterShader.SetMat4("view", _captureViews[i]);
                _captureFBO.AttachTextureLayer(TextureAttachment::Color0, _prefilteredCubemap.GetID(), i, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                renderCube();
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Texture2D initBrdfLUT() {
        Texture2D brdf(512, 512, TextureFormat::RG16F);
        brdf.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        brdf.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        return brdf;
    }

    void bakeBrdfLUT(Shader& brdfShader) {
        constexpr int size = 512;

        auto lutFBO = FrameBuffer();
        auto lutRBO = RenderBuffer(size, size, TextureFormat::Depth32F);
        lutFBO.Bind();
        lutFBO.AttachRenderBuffer(TextureAttachment::Depth, lutRBO);
        lutFBO.AttachTexture(TextureAttachment::Color0, _brdfLUT.GetID());
        lutFBO.Status();

        glViewport(0, 0, size, size);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        brdfShader.Activate();
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLsizei _cubeSize = 0;
    FrameBuffer _captureFBO;
    RenderBuffer _captureRBO;
    GLuint _cubeVAO = 0;
    GLuint _cubeVBO = 0;

    GLuint _emptyVAO = 0;
    TextureCube _envCubemap;
    Texture2D _hdrTexture;
    TextureCube _irradianceCubemap;
    TextureCube _prefilteredCubemap;
    Texture2D _brdfLUT;

    std::array<glm::mat4, 6> _captureViews;
    glm::mat4 _captureProjection;
};
