#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "model.h"
#include "mesh.h"
#include "scene.h"
#include "gbuffer.h"
#include "camera.h"
#include "renderpass/skybox.h"
#include "renderpass/shadow_map_directional.h"
#include "renderpass/shadow_map_point.h"
#include "renderpass/shadow_map_spot.h"
#include "renderpass/bloom.h"
#include "renderpass/ssao.h"

#include "texture_slots.h"
#include "math.h"
#include "asset_cache.h"

#include "utils/logger.h"
#include "utils/stopwatch.h"

class Renderer {
public:
    explicit Renderer(int scrWidth, int scrHeight, std::shared_ptr<Camera> camera, const std::shared_ptr<Skybox> skybox, std::shared_ptr<AssetCache>& assetCache) :
        _cameraPtr(std::move(camera)),
        _gBuffer(scrWidth, scrHeight),
        _scrWidth(scrWidth),
        _scrHeight(scrHeight),
        _shadowMapDirectional(),
        _shadowMapPoint(),
        _shadowMapSpot(),
        _bloom(scrWidth, scrHeight),
        _ssao(scrWidth, scrHeight),
        _skybox(skybox),
        _assetCache(assetCache)
    {
        glCreateVertexArrays(1, &_emptyVAO);
    }

    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    void Draw(Model& model, Shader& shader, bool depthOnly = false) const {
        model.UploadTransforms();
        auto& meshes = model.GetMeshes();
        for (auto& mesh : meshes) {
            this->drawMesh(mesh, shader, depthOnly);
        }
    }

    void PassShadowDirectional(Scene& scene, Shader& shader, const ConfigUBO& config) {
        Stopwatch stopwatch("PassShadowDirectional");
        if (!config.shadowsEnabled) return;

        auto directionalLight = scene.GetDirectionalLight();
        auto lightDir = directionalLight.GetDirection(); // TODO no fallback if no dir light present
        auto lightSpaceMatrix = math::GetDirLightSpaceMatrix(lightDir);

        _shadowMapUBO.Data().dirLightProjMatrix = lightSpaceMatrix;
        _shadowMapUBO.Upload();
        _shadowMapDirectional.BindFramebuffer();
        _shadowMapDirectional.BindTexture();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader, true);
        //this->render(scene.GetQueue(Forward), shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowPoint(Scene& scene, Shader& shader, const ConfigUBO& config) {
        Stopwatch stopwatch("PassShadowPoint");
        if (!config.shadowsEnabled) return;

        const auto& pointLights = scene.GetPointLights();
        _shadowMapPoint.BindTexture();
        shader.Activate();

        const int count = std::min(pointLights.Count(), config.maxPointShadowCasters);
        for (int i = 0; i < count; i++) {
            auto lightPos = pointLights.At(i).GetPosition();
            float farPlane = pointLights.At(i).GetRange();
            shader.SetFloat("farPlane", farPlane);
            auto shadowMatrices = math::GetPointShadowMatrices(lightPos, 0.1f, farPlane);

            shader.SetVec3("lightPos", lightPos);
            for (int face = 0; face < 6; face++) {
                _shadowMapPoint.BindFramebufferFace(i, face);
                shader.SetMat4("lightSpaceMatrix", shadowMatrices[face]);
                this->render(scene.GetQueue(Deferred), shader, true);
                //this->render(scene.GetQueue(Forward), shader);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowSpot(Scene& scene, Shader& shader, const ConfigUBO& config) {
        Stopwatch stopwatch("PassShadowSpot");
        if (!config.shadowsEnabled) return;

        auto& spotLights = scene.GetSpotLights();
        auto& ubo = _shadowMapUBO.Data();

        _shadowMapSpot.BindTexture();
        shader.Activate();
        int count = std::min(spotLights.Count(), config.maxSpotShadowCasers);
        for (int i = 0; i < count; i++) {
            auto& light = spotLights.At(i);
            float farPlane = light.GetRange();
            auto lightSpaceMatrix = math::GetSpotLightSpaceMatrix(light.GetPosition(), light.GetDirection(), light.GetOuterConeDegrees(), 0.1f, farPlane);

            ubo.spotLightProjMatrices[i] = lightSpaceMatrix;
            _shadowMapSpot.BindFramebufferLayer(i);

            shader.SetMat4("spotLightSpaceMatrix", lightSpaceMatrix);
            this->render(scene.GetQueue(Deferred), shader, true);
            //this->render(scene.GetQueue(Forward), shader);
        }

        _shadowMapUBO.Upload();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassGeometryBuffer(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassGeometryBuffer");
        _gBuffer.BindFramebuffer();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
        glViewport(0, 0, _scrWidth, _scrHeight); // restore viewport
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);
    }

    void PassSSAO(Scene& scene, Shader& shaderSSAO, Shader& shaderBlur) {
        Stopwatch stopwatch("PassSSAO");
        _gBuffer.BindTextures();
        _ssao.Run(shaderSSAO);
        _ssao.Blur(shaderBlur);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassDeferred(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassDeferred");
        _gBuffer.BindTextures();
        _ssao.BindSSAOTexture();
        _skybox->BindTexturesIBL();
        _bloom.BindHdrFramebuffer();
        glClear(GL_COLOR_BUFFER_BIT); // clear color (removes artifacts when rendering closer than z-near)

        shader.Activate();
        // render empty fullscreen quad
        //glDepthMask(GL_FALSE);
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        //glDepthMask(GL_TRUE);
    }

    void PassForward(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassForward");
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);
        _bloom.BindHdrFramebuffer();
        shader.Activate();

        // Render opaque/masked meshes first
        shader.SetBool("blendPass", false);
        this->render(scene.GetQueue(Forward), shader);

        // Render blend meshes
        shader.SetBool("blendPass", true);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        this->render(scene.GetQueue(Forward), shader);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void PassNoShadow(Scene& scene, Shader& unlitShader) {
        Stopwatch stopwatch("PassNoShadow");
        _bloom.BindHdrFramebuffer();
        unlitShader.Activate();
        this->render(scene.GetQueue(NoShadow), unlitShader);
    }

    void PassSkybox(Skybox& skybox, Shader& skyboxShader) {
        Stopwatch stopwatch("PassSkybox");
        _bloom.BindHdrFramebuffer();

        glDepthFunc(GL_LEQUAL);  // pass when depth equals 1.0
        glDepthMask(GL_FALSE);   // disable depth buffer writes
        auto view = _cameraPtr->GetViewMatrix();
        auto projection = _cameraPtr->GetProjectionMatrix();
        skybox.Draw(skyboxShader, view, projection);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // restore default
    }

    void PassBloom(Shader& blurShader, Shader& bloomShader) {
        Stopwatch stopwatch("PassBloom");
        // Blur bright fragments with two-pass Gaussian Blur
        blurShader.Activate();
        _bloom.Blur(blurShader, 20);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Render to quad, apply HDR tonemapping in bloom shader
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomShader.Activate();
        _bloom.BindTextures();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

private:
    void render(Scene::RenderQueue& renderQueue, Shader& shader, bool depthOnly = false) {
        for (auto& [handle, model] : renderQueue) {
            Draw(model, shader, depthOnly);
        }
    }

    void drawMesh(const Mesh& mesh, Shader& shader, bool depthOnly) const
    {
        if (!depthOnly) {
            const Material& mat = _assetCache->GetMaterial(mesh.GetMaterialIndex());
            glBindTextureUnit(slot(SlotGeometry::Albedo),   mat.baseColorTexture);
            glBindTextureUnit(slot(SlotGeometry::Normal),   mat.normalTexture);
            glBindTextureUnit(slot(SlotGeometry::Emissive), mat.emissiveTexture);
            glBindTextureUnit(slot(SlotGeometry::ORM),      mat.ormTexture);
            shader.SetInt("materialIndex", mesh.GetMaterialIndex());
        }

        glBindVertexArray(mesh.GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mesh.GetIndexCount()), GL_UNSIGNED_INT, nullptr, mesh.GetInstanceCount());
    }

    std::shared_ptr<Camera> _cameraPtr;
    GBuffer _gBuffer;
    GLuint _emptyVAO = 0;
    int _scrWidth, _scrHeight;
    float _aspectRatio = 0;

    ShadowMapDirectional _shadowMapDirectional;
    ShadowMapPoint _shadowMapPoint;
    ShadowMapSpot _shadowMapSpot;
    UniformBuffer<ShadowMapUBO, 4> _shadowMapUBO;
    Bloom _bloom;
    SSAO _ssao;
    std::shared_ptr<Skybox> _skybox;
    std::shared_ptr<AssetCache> _assetCache;
};
