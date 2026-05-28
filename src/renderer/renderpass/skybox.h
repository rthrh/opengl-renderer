#pragma once

#include <gl/headers.h>
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
#include "gl/vertex_array.h"
#include "gl/vertex_buffer.h"

class Skybox {
public:
    Skybox(GLsizei cubeSize = 2048) :
        _cubeSize(cubeSize),
        _captureFBO(),
        _captureRBO(cubeSize, cubeSize, TextureFormat::Depth32F),
        _cubeVBO(),
        _envCubemap(cubeSize, TextureFormat::RGBA16F, true),
        _irradianceCubemap(initIrradianceMap()),
        _prefilteredCubemap(initPrefilteredMap()),
        _brdfLUT(initBrdfLUT())
    {
        _captureFBO.AttachRenderBuffer(TextureAttachment::Depth, _captureRBO);

        _envCubemap.SetFilter(TextureFilter::LinearMipMapLinear, TextureFilter::Linear);
        _envCubemap.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

        this->initCube();
        this->setupCaptureViews();
    }

    //TODO FBO, RBO, HDR texture can be removed after init
    ~Skybox() = default;

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    Skybox(Skybox&&) noexcept = default;
    Skybox& operator=(Skybox&&) noexcept = default;

    void LoadTexture(const std::filesystem::path& path, Shader& equirectShader, Shader& irradianceShader, Shader& prefilterShader, Shader& brdfShader) {
        _hdrTexture = initTextureHDR(path);
        this->equirectToEnvMap(equirectShader);
        _envCubemap.GenerateMipmap();

        this->bakeIrradianceMap(irradianceShader);
        this->bakePrefilteredMap(prefilterShader);
        this->bakeBrdfLUT(brdfShader);
    }

    void Draw(Shader& skyboxShader, glm::mat4 view, glm::mat4 projection) {
        skyboxShader.Activate();
        skyboxShader.SetMat4("view", view);
        skyboxShader.SetMat4("projection", projection);

        _envCubemap.Bind(slot(TextureSlot::Skybox));
        renderCube();
    }

    void BindTexturesIBL() const {
        _irradianceCubemap.Bind(slot(TextureSlot::Irradiance));
        _prefilteredCubemap.Bind(slot(TextureSlot::PrefilterEnv));
        _brdfLUT.Bind(slot(TextureSlot::BrdfLUT));
    }

private:
    void setupCaptureViews() {
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

        // Flip vertically - swap rows from the top and bottom halves moving inward
        const int rowSize = width * nrComponents;
        std::ranges::for_each(std::views::iota(0, height / 2), [=](int i) {
            std::ranges::swap_ranges(
                std::span(data + i * rowSize, rowSize),
                std::span(data + (height - 1 - i) * rowSize, rowSize)
            );
        });

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
        _cubeVBO.SetStorage(std::span<const float>(cube_map_vertices));

        constexpr int bindingIndex = 0;
        _cubeVAO.BindVertexBuffer(_cubeVBO.GetID(), bindingIndex, 0, 8 * sizeof(float));
        _cubeVAO.AddAttribute(bindingIndex, 0, 3, 0);
        _cubeVAO.AddAttribute(bindingIndex, 1, 3, 3 * sizeof(float));
        _cubeVAO.AddAttribute(bindingIndex, 2, 2, 6 * sizeof(float));
    }

    // Converts equirectangular map to cube map
    void equirectToEnvMap(Shader& equirectShader) {
        equirectShader.Activate();
        equirectShader.SetMat4("projection", _captureProjection);
        _hdrTexture.Bind(0);

        glViewport(0, 0, _cubeSize, _cubeSize);
        _captureFBO.Bind();
        for (unsigned i = 0; i < 6; ++i)
        {
            equirectShader.SetMat4("view", _captureViews[i]);
            _captureFBO.AttachTextureCubeFace(TextureAttachment::Color0, _envCubemap.GetID(), i);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            this->renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderCube() {
        _cubeVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 cubemap triangles
    }

    TextureCube initIrradianceMap() {
        constexpr int size = 32; // scaled down
        TextureCube irradiance(size, TextureFormat::RGBA16F);
        irradiance.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        irradiance.SetFilter(TextureFilter::Linear, TextureFilter::Linear);
        return irradiance;
    }

    void bakeIrradianceMap(Shader& irradianceShader) {
        // Resize capture RBO to 32x32
        _captureRBO.Resize(32, 32);

        irradianceShader.Activate();
        irradianceShader.SetMat4("projection", _captureProjection);
        _envCubemap.Bind(slot(TextureSlot::Skybox));

        glViewport(0, 0, 32, 32);
        _captureFBO.Bind();
        for (unsigned i = 0; i < 6; i++) {
            irradianceShader.SetMat4("view", _captureViews[i]);
            _captureFBO.AttachTextureCubeFace(TextureAttachment::Color0, _irradianceCubemap.GetID(), i);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    TextureCube initPrefilteredMap() {
        constexpr int size = 128;
        TextureCube prefiltered(size, TextureFormat::RGBA16F, true);
        prefiltered.SetWrap(TextureWrap::ClampToEdge, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
        prefiltered.SetFilter(TextureFilter::LinearMipMapLinear, TextureFilter::Linear);
        return prefiltered;
    }

    void bakePrefilteredMap(Shader& prefilterShader) {
        prefilterShader.Activate();
        prefilterShader.SetMat4("projection", _captureProjection);

        _envCubemap.Bind(slot(TextureSlot::Skybox));
        _captureFBO.Bind();

        unsigned maxMipLevels = 5;
        for (unsigned mip = 0; mip < maxMipLevels; ++mip)
        {
            // Resize framebuffer according to mip-level size.
            unsigned mipWidth  = static_cast<unsigned>(128 * std::pow(0.5, mip));
            unsigned mipHeight = static_cast<unsigned>(128 * std::pow(0.5, mip));
            _captureRBO.Resize(mipWidth, mipHeight);

            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader.SetFloat("roughness", roughness);
            for (unsigned i = 0; i < 6; ++i)
            {
                prefilterShader.SetMat4("view", _captureViews[i]);
                _captureFBO.AttachTextureCubeFace(TextureAttachment::Color0, _prefilteredCubemap.GetID(), i, mip);
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
        _emptyVAO.Bind();

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLsizei _cubeSize = 0;
    FrameBuffer _captureFBO;
    RenderBuffer _captureRBO;
    VertexBuffer _cubeVBO;

    VertexArray _cubeVAO;
    VertexArray _emptyVAO;

    TextureCube _envCubemap;
    Texture2D _hdrTexture;
    TextureCube _irradianceCubemap;
    TextureCube _prefilteredCubemap;
    Texture2D _brdfLUT;

    std::array<glm::mat4, 6> _captureViews;
    glm::mat4 _captureProjection;
};
