#pragma once

#include "gl/headers.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "renderpass/gbuffer.h"
#include "renderpass/skybox.h"
#include "renderpass/shadow_map_directional.h"
#include "renderpass/shadow_map_point.h"
#include "renderpass/shadow_map_spot.h"
#include "renderpass/bloom.h"
#include "renderpass/ssao.h"
#include "renderpass/fxaa.h"
#include "gl/dsa_config.h"

#include "model.h"
#include "mesh.h"
#include "scene.h"
#include "camera.h"
#include "texture_slots.h"
#include "math_matrix.h"
#include "asset_cache.h"
#include "mesh_cache.h"
#include "shader_cache.h"
#include "shader_set.h"

#include "utils/logger.h"
#include "utils/stopwatch.h"

class Renderer {
public:
    explicit Renderer(int scrWidth, int scrHeight, std::shared_ptr<Camera> camera, std::shared_ptr<AssetCache>& assetCache, std::shared_ptr<MeshCache>& meshCache, ShaderCache& shaderCache) :
        _shaders(shaderCache),
        _cameraPtr(std::move(camera)),
        _configUBO(),
        _gBuffer(scrWidth, scrHeight),
        _scrWidth(scrWidth),
        _scrHeight(scrHeight),
        _shadowMapDirectional(),
        _shadowMapPoint(),
        _shadowMapSpot(),
        _bloom(scrWidth, scrHeight, _gBuffer.GetDepthTextureID()),
        _ssao(scrWidth, scrHeight),
        _fxaa(scrWidth, scrHeight),
        _skybox(2048),
        _assetCache(assetCache),
        _meshCache(meshCache)
    {
        this->UploadConfig();

        // Init GL state
        #ifndef __EMSCRIPTEN__
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // Fixes skybox seams
        #endif
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE); // Cull back faces
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    }

    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    // Loads skybox HDR texture from skyboxPath
    void LoadSkybox(const std::filesystem::path& skyboxPath) {
        glDisable(GL_CULL_FACE);
        _skybox.LoadTexture(skyboxPath, *_shaders.equirect, *_shaders.irradiance, *_shaders.prefilter, *_shaders.brdf);
        glEnable(GL_CULL_FACE);
    }

    // Returns GPU config. Call UploadConfig after editing
    UniformBuffer<ConfigUBO, 5>& GetConfig() {
        return _configUBO; // TODO should return raw ConfigUBO
    }

    // Uploads current ConfigUBO to GPU
    void UploadConfig() {
        _configUBO.Upload();
    }

    void Draw(Mesh& mesh, Shader& shader, bool depthOnly = false) const {
        this->drawMesh(mesh, shader, depthOnly);
    }

    void Resize(int scrWidth, int scrHeight) {
        _scrWidth = scrWidth;
        _scrHeight = scrHeight;
        _gBuffer.Resize(scrWidth, scrHeight);
        _bloom.Resize(scrWidth, scrHeight, _gBuffer.GetDepthTextureID());
        _ssao.Resize(scrWidth, scrHeight);
        _fxaa.Resize(scrWidth, scrHeight);
    }

    // Call in main loop to render frame
    void RenderFrame(Scene& scene) {
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _cameraPtr->UploadUBO();
        _assetCache->UploadMaterials();
        scene.UploadTransforms(_meshCache);

        glCullFace(GL_FRONT);
        this->PassShadowDirectional(scene);
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassShadowDirectional: {:x}", e);
        this->PassShadowPoint(scene);
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassShadowPoint: {:x}", e);

        this->PassShadowSpot(scene);
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassShadowSpot: {:x}", e);

        glCullFace(GL_BACK);

        this->PassGeometryBuffer(scene);
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassGeometryBuffer: {:x}", e);

        this->PassSSAO(scene);
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassSSAO: {:x}", e);
        this->PassDeferred(scene);
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassDeferred: {:x}", e);

        this->PassForward(scene);
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassForward: {:x}", e);

        glDisable(GL_CULL_FACE);
        this->PassSkybox();
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassSkybox: {:x}", e);

        glEnable(GL_CULL_FACE);

        this->PassNoShadow(scene);
        this->PassBloom();
        //if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassBloom: {:x}", e);

        this->PassFXAA();
        if (GLenum e = glGetError(); e != GL_NO_ERROR) Info("Error after PassFXAA: {:x}", e);
    }

private:

    void PassShadowDirectional(Scene& scene) {
        Stopwatch stopwatch("PassShadowDirectional");
        if (!_configUBO.Data().dirShadowsEnabled) return;

        auto directionalLight = scene.GetDirectionalLight();
        auto lightDir = directionalLight.GetDirection(); // TODO no fallback if no dir light present
        auto lightSpaceMatrix = math::GetDirLightSpaceMatrix(lightDir);

        _shadowMapUBO.Data().dirLightProjMatrix = lightSpaceMatrix;
        _shadowMapUBO.Upload();
        _shadowMapDirectional.BindFramebuffer();
        _shadowMapDirectional.BindTexture();
        _shaders.shadowDir->Activate();

        this->render(_meshCache->GetQueue(Opaque), *_shaders.shadowDir, true);
        //this->render(scene.GetQueue(Blend), shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowPoint(Scene& scene) {
        Stopwatch stopwatch("PassShadowPoint");
        if (!_configUBO.Data().pointShadowsEnabled) return;

        const auto& pointLights = scene.GetPointLights();
        _shadowMapPoint.BindTextures();
        _shaders.shadowPoint->Activate();

        const int count = std::min({pointLights.Count(), _configUBO.Data().maxPointShadowCasters, _shadowMapPoint.maxShadowCasters});
        for (int i = 0; i < count; i++) {
            auto lightPos = pointLights.At(i).GetPosition();
            float farPlane = pointLights.At(i).GetRange();
            _shaders.shadowPoint->SetFloat("farPlane", farPlane);
            auto shadowMatrices = math::GetPointShadowMatrices(lightPos, 0.1f, farPlane);

            _shaders.shadowPoint->SetVec3("lightPos", lightPos);
            for (int face = 0; face < 6; face++) {
                _shadowMapPoint.BindFramebufferFace(i, face);
                _shaders.shadowPoint->SetMat4("lightSpaceMatrix", shadowMatrices[face]);
                this->render(_meshCache->GetQueue(Opaque), *_shaders.shadowPoint, true);
                //this->render(scene.GetQueue(Blend), shader);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowSpot(Scene& scene) {
        Stopwatch stopwatch("PassShadowSpot");
        if (!_configUBO.Data().spotShadowsEnabled) return;

        auto& spotLights = scene.GetSpotLights();
        auto& ubo = _shadowMapUBO.Data();

        _shadowMapSpot.BindTexture();
        _shaders.shadowSpot->Activate();
        int count = std::min(spotLights.Count(), _configUBO.Data().maxSpotShadowCasers);
        for (int i = 0; i < count; i++) {
            auto& light = spotLights.At(i);
            float farPlane = light.GetRange();
            auto lightSpaceMatrix = math::GetSpotLightSpaceMatrix(light.GetPosition(), light.GetDirection(), light.GetOuterConeDegrees(), 0.1f, farPlane);

            ubo.spotLightProjMatrices[i] = lightSpaceMatrix;
            _shadowMapSpot.BindFramebufferLayer(i);

            _shaders.shadowSpot->SetMat4("spotLightSpaceMatrix", lightSpaceMatrix);
            this->render(_meshCache->GetQueue(Opaque), *_shaders.shadowSpot, true);
            //this->render(scene.GetQueue(Blend), shader);
        }

        _shadowMapUBO.Upload();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassGeometryBuffer(Scene& scene) {
        Stopwatch stopwatch("PassGeometryBuffer");
        _gBuffer.BindFramebuffer();
        _shaders.gBuffer->Activate();

        // Render opaque, then masked meshes
        this->render(_meshCache->GetQueue(Opaque), *_shaders.gBuffer);
        this->render(_meshCache->GetQueue(Masked), *_shaders.gBuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
        glViewport(0, 0, _scrWidth, _scrHeight); // restore viewport
    }

    void PassSSAO(Scene& scene) {
        Stopwatch stopwatch("PassSSAO");
        _gBuffer.BindTextures();
        _ssao.Run(*_shaders.ssao);
        _ssao.Blur(*_shaders.ssaoBlur);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassDeferred(Scene& scene) {
        Stopwatch stopwatch("PassDeferred");
        _gBuffer.BindTextures();
        _ssao.BindSSAOTexture();
        _skybox.BindTexturesIBL();
        _bloom.BindHdrFramebuffer();
        glClear(GL_COLOR_BUFFER_BIT);

        _shaders.deferredLight->Activate();
        // render empty fullscreen quad
        glDepthMask(GL_FALSE); // depth is shared via texture, not blit, so depth writes are disabled
        _emptyVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDepthMask(GL_TRUE);
    }

    void PassForward(Scene& scene) {
        Stopwatch stopwatch("PassForward");
        _bloom.BindHdrFramebuffer();
        _shaders.forward->Activate();

        // Render blend meshes
        _shaders.forward->SetBool("blendPass", true);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        this->render(_meshCache->GetQueue(Blend), *_shaders.forward);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void PassNoShadow(Scene& scene) {
        Stopwatch stopwatch("PassNoShadow");
        _bloom.BindHdrFramebuffer();
        _shaders.unlit->Activate();
        this->render(_meshCache->GetQueue(NoShadow), *_shaders.unlit);
    }

    void PassSkybox() {
        Stopwatch stopwatch("PassSkybox");
        _bloom.BindHdrFramebuffer();

        glDepthFunc(GL_LEQUAL);  // pass when depth equals 1.0
        glDepthMask(GL_FALSE);   // disable depth buffer writes
        auto view = _cameraPtr->GetViewMatrix();
        auto projection = _cameraPtr->GetProjectionMatrix();
        _skybox.Draw(*_shaders.skybox, view, projection);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // restore default
    }

    void PassBloom() {
        Stopwatch stopwatch("PassBloom");
        _emptyVAO.Bind();
        _bloom.RenderDownsamples(*_shaders.bloomDownsample);
        _bloom.RenderUpsamples(*_shaders.bloomUpsample);
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        _fxaa.BindFramebuffer();
        glViewport(0, 0, _scrWidth, _scrHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        _shaders.bloomFinal->Activate();
        _bloom.BindTextures();

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void PassFXAA() {
        Stopwatch stopwatch("FXAA");
        _emptyVAO.Bind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        _fxaa.BindTexture(0);

        _shaders.fxaa->Activate();
        _shaders.fxaa->SetVec2("resolution", glm::vec2(_scrWidth, _scrHeight));

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void render(MeshQueue& meshQueue, Shader& shader, bool depthOnly = false) {
        for (auto& [handle, mesh] : meshQueue) {
            Draw(mesh, shader, depthOnly);
        }
    }

    void drawMesh(const Mesh& mesh, Shader& shader, bool depthOnly) const
    {
        if (!depthOnly) {
            const Material& mat = _assetCache->GetMaterial(mesh.GetMaterialIndex());
            GL::BindTextureUnit(slot(TextureSlot::Albedo), mat.baseColorTexture);
            GL::BindTextureUnit(slot(TextureSlot::Normal), mat.normalTexture);
            GL::BindTextureUnit(slot(TextureSlot::ORM), mat.ormTexture);
            GL::BindTextureUnit(slot(TextureSlot::Emissive), mat.emissiveTexture);

            shader.SetInt("materialIndex", mesh.GetMaterialIndex());
        }

        glBindVertexArray(mesh.GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mesh.GetIndexCount()), GL_UNSIGNED_INT, nullptr, mesh.GetInstanceCount());
    }

    ShaderSet _shaders;
    std::shared_ptr<Camera> _cameraPtr;
    UniformBuffer<ConfigUBO, 5> _configUBO;

    GBuffer _gBuffer;
    VertexArray _emptyVAO;
    int _scrWidth, _scrHeight;
    float _aspectRatio = 0;

    ShadowMapDirectional _shadowMapDirectional;
    ShadowMapPoint _shadowMapPoint;
    ShadowMapSpot _shadowMapSpot;
    UniformBuffer<ShadowMapUBO, 4> _shadowMapUBO;
    Bloom _bloom;
    SSAO _ssao;
    FXAA _fxaa;
    Skybox _skybox;
    std::shared_ptr<AssetCache> _assetCache;
    std::shared_ptr<MeshCache> _meshCache;
};
