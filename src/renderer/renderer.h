#include <glad/glad.h>

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

#include "utils/logger.h"
#include "utils/stopwatch.h"

class Renderer {
public:
    explicit Renderer(int scrWidth, int scrHeight, std::shared_ptr<Camera> camera, std::shared_ptr<AssetCache>& assetCache, std::shared_ptr<MeshCache>& meshCache, ShaderCache& shaderCache) :
        _cameraPtr(std::move(camera)),
        _configUBO(),
        _gBuffer(scrWidth, scrHeight),
        _scrWidth(scrWidth),
        _scrHeight(scrHeight),
        _shadowMapDirectional(),
        _shadowMapPoint(),
        _shadowMapSpot(),
        _bloom(scrWidth, scrHeight),
        _ssao(scrWidth, scrHeight),
        _fxaa(scrWidth, scrHeight),
        _skybox(2048),
        _assetCache(assetCache),
        _meshCache(meshCache)
    {
        _shadowDirShader = shaderCache.Build("shadow_directional", "shadow_directional.vert", "depth.frag");
        _shadowPointShader = shaderCache.Build("shadow_point", "shadow_point.vert", "shadow_point.frag");
        _shadowSpotShader = shaderCache.Build("shadow_spot", "shadow_spot.vert", "depth.frag");

        _deferredLightShader = shaderCache.Build("deferred", "quad.vert", "deferred_pbr.frag");
        _gBufferShader = shaderCache.Build("gBuffer", "gBuffer.vert", "gBuffer.frag");
        _forwardShader = shaderCache.Build("forward", "forward.vert", "forward_pbr.frag");
        //_phongShader = shaderCache.Build("phong_forward", "forward.vert", "forward_phong.frag");

        _equirectShader = shaderCache.Build("equirect", "equirect_to_cubemap.vert", "equirect_to_cubemap.frag");
        _skyboxShader = shaderCache.Build("skybox", "skybox.vert", "skybox.frag");
        _irradianceShader = shaderCache.Build("irradiance", "irradiance.vert", "irradiance.frag");
        _prefilterShader = shaderCache.Build("prefilter", "irradiance.vert", "prefilter.frag");
        _brdfShader = shaderCache.Build("brdf", "brdf.vert", "brdf.frag");

        _downsampleShader = shaderCache.Build("downsample", "quad.vert", "downsample.frag");
        _upsampleShader = shaderCache.Build("upsample", "quad.vert", "upsample.frag");
        _bloomFinalShader = shaderCache.Build("bloomFinal", "quad.vert", "bloom_final.frag");

        _ssaoShader = shaderCache.Build("ssao", "quad.vert", "ssao.frag");
        _ssaoBlurShader = shaderCache.Build("ssao_blur", "quad.vert", "ssao_blur.frag");

        _fxaaShader = shaderCache.Build("fxaa", "quad.vert", "fxaa.frag");

        _unlitShader = shaderCache.Build("unlit", "unlit.vert", "unlit.frag"); // debug light cubes

        this->UploadConfig();

        // Init GL state
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
        _skybox.LoadTexture(skyboxPath, *_equirectShader, *_irradianceShader, *_prefilterShader, *_brdfShader);
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
        _bloom.Resize(scrWidth, scrHeight);
        _ssao.Resize(scrWidth, scrHeight);
        _fxaa.Resize(scrWidth, scrHeight);
    }

    void DepthMaskedPrepassOpaque(Scene& scene, Shader& depthMaskedShader) {
        Stopwatch stopwatch("DepthPrepassOpaque");
        depthMaskedShader.Activate();

        this->render(_meshCache->GetQueue(Masked), depthMaskedShader, true);

    }

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
        _shadowDirShader->Activate();

        this->render(_meshCache->GetQueue(Opaque), *_shadowDirShader, true);
        //this->render(scene.GetQueue(Blend), shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowPoint(Scene& scene) {
        Stopwatch stopwatch("PassShadowPoint");
        if (!_configUBO.Data().pointShadowsEnabled) return;

        const auto& pointLights = scene.GetPointLights();
        _shadowMapPoint.BindTexture();
        _shadowPointShader->Activate();

        const int count = std::min(pointLights.Count(), _configUBO.Data().maxPointShadowCasters);
        for (int i = 0; i < count; i++) {
            auto lightPos = pointLights.At(i).GetPosition();
            float farPlane = pointLights.At(i).GetRange();
            _shadowPointShader->SetFloat("farPlane", farPlane);
            auto shadowMatrices = math::GetPointShadowMatrices(lightPos, 0.1f, farPlane);

            _shadowPointShader->SetVec3("lightPos", lightPos);
            for (int face = 0; face < 6; face++) {
                _shadowMapPoint.BindFramebufferFace(i, face);
                _shadowPointShader->SetMat4("lightSpaceMatrix", shadowMatrices[face]);
                this->render(_meshCache->GetQueue(Opaque), *_shadowPointShader, true);
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
        _shadowSpotShader->Activate();
        int count = std::min(spotLights.Count(), _configUBO.Data().maxSpotShadowCasers);
        for (int i = 0; i < count; i++) {
            auto& light = spotLights.At(i);
            float farPlane = light.GetRange();
            auto lightSpaceMatrix = math::GetSpotLightSpaceMatrix(light.GetPosition(), light.GetDirection(), light.GetOuterConeDegrees(), 0.1f, farPlane);

            ubo.spotLightProjMatrices[i] = lightSpaceMatrix;
            _shadowMapSpot.BindFramebufferLayer(i);

            _shadowSpotShader->SetMat4("spotLightSpaceMatrix", lightSpaceMatrix);
            this->render(_meshCache->GetQueue(Opaque), *_shadowSpotShader, true);
            //this->render(scene.GetQueue(Blend), shader);
        }

        _shadowMapUBO.Upload();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassGeometryBuffer(Scene& scene) {
        Stopwatch stopwatch("PassGeometryBuffer");
        _gBuffer.BindFramebuffer();
        _gBufferShader->Activate();

        // Render opaque, then masked meshes
        this->render(_meshCache->GetQueue(Opaque), *_gBufferShader);
        this->render(_meshCache->GetQueue(Masked), *_gBufferShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
        glViewport(0, 0, _scrWidth, _scrHeight); // restore viewport
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);
    }

    void PassSSAO(Scene& scene) {
        Stopwatch stopwatch("PassSSAO");
        _gBuffer.BindTextures();
        _ssao.Run(*_ssaoShader);
        _ssao.Blur(*_ssaoBlurShader);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassDeferred(Scene& scene) {
        Stopwatch stopwatch("PassDeferred");
        _gBuffer.BindTextures();
        _ssao.BindSSAOTexture();
        _skybox.BindTexturesIBL();
        _bloom.BindHdrFramebuffer();
        glClear(GL_COLOR_BUFFER_BIT); // clear color (removes artifacts when rendering closer than z-near)

        _deferredLightShader->Activate();
        // render empty fullscreen quad
        //glDepthMask(GL_FALSE);
        _emptyVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        //glDepthMask(GL_TRUE);
    }

    void PassForward(Scene& scene) {
        Stopwatch stopwatch("PassForward");
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);
        _bloom.BindHdrFramebuffer();
        _forwardShader->Activate();

        // Render blend meshes
        _forwardShader->SetBool("blendPass", true);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        this->render(_meshCache->GetQueue(Blend), *_forwardShader);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void PassNoShadow(Scene& scene) {
        Stopwatch stopwatch("PassNoShadow");
        _bloom.BindHdrFramebuffer();
        _unlitShader->Activate();
        this->render(_meshCache->GetQueue(NoShadow), *_unlitShader);
    }

    void PassSkybox() {
        Stopwatch stopwatch("PassSkybox");
        _bloom.BindHdrFramebuffer();

        glDepthFunc(GL_LEQUAL);  // pass when depth equals 1.0
        glDepthMask(GL_FALSE);   // disable depth buffer writes
        auto view = _cameraPtr->GetViewMatrix();
        auto projection = _cameraPtr->GetProjectionMatrix();
        _skybox.Draw(*_skyboxShader, view, projection);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // restore default
    }

    void PassBloom() {
        Stopwatch stopwatch("PassBloom");
        _emptyVAO.Bind();
        _bloom.RenderDownsamples(*_downsampleShader);
        _bloom.RenderUpsamples(*_upsampleShader);
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        _fxaa.BindFramebuffer();
        glViewport(0, 0, _scrWidth, _scrHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        _bloomFinalShader->Activate();
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

        _fxaaShader->Activate();
        _fxaaShader->SetVec2("resolution", glm::vec2(_scrWidth, _scrHeight));

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

private:
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


    std::shared_ptr<Shader> _shadowDirShader;
    std::shared_ptr<Shader> _shadowPointShader;
    std::shared_ptr<Shader> _shadowSpotShader;

    std::shared_ptr<Shader> _deferredLightShader;
    std::shared_ptr<Shader> _gBufferShader;
    std::shared_ptr<Shader> _forwardShader;

    std::shared_ptr<Shader> _equirectShader;
    std::shared_ptr<Shader> _skyboxShader;
    std::shared_ptr<Shader> _irradianceShader;
    std::shared_ptr<Shader> _prefilterShader;
    std::shared_ptr<Shader> _brdfShader;

    std::shared_ptr<Shader> _downsampleShader;
    std::shared_ptr<Shader> _upsampleShader;
    std::shared_ptr<Shader> _bloomFinalShader;

    std::shared_ptr<Shader> _ssaoShader;
    std::shared_ptr<Shader> _ssaoBlurShader;
    std::shared_ptr<Shader> _fxaaShader;
    std::shared_ptr<Shader> _unlitShader;
};
