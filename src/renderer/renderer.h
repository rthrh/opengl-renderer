#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "renderer/model.h"
#include "renderer/mesh.h"
#include "renderer/scene.h"
#include "renderer/gbuffer.h"
#include "renderer/camera.h"
#include "renderer/skybox.h"
#include "renderer/shadow_map_directional.h"
#include "renderer/shadow_map_point.h"
#include "renderer/shadow_map_spot.h"
#include "renderer/texture_slots.h"
#include "renderer/math.h"
#include "renderer/bloom.h"
#include "renderer/ssao.h"

#include "utils/logger.h"
#include "utils/stopwatch.h"

class Renderer {
public:
    explicit Renderer(int scrWidth, int scrHeight, std::shared_ptr<Camera> camera, const std::shared_ptr<Skybox> skybox) :
        _cameraPtr(std::move(camera)),
        _gBuffer(scrWidth, scrHeight),
        _scrWidth(scrWidth),
        _scrHeight(scrHeight),
        _shadowMapDirectional(),
        _shadowMapPoint(),
        _shadowMapSpot(),
        _bloom(scrWidth, scrHeight),
        _ssao(scrWidth, scrHeight),
        _skybox(skybox)
    {
        glCreateVertexArrays(1, &_emptyVAO);
    }

    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    void Draw(const Model& model, Shader& shader) const {
        shader.Activate();
        shader.SetMat4("model", model.GetModelMatrix());

        auto& meshes = model.GetMeshes();
        for (auto& mesh : meshes) {
            this->drawMesh(mesh, shader);
        }
    }

    void PassShadowDirectional(Scene& scene, Shader& shader) {
        auto directionalLight = scene.GetDirectionalLight();
        auto lightDir = directionalLight.GetDirection(); // TODO no fallback if no dir light present
        auto lightSpaceMatrix = math::GetDirLightSpaceMatrix(lightDir);

        _shadowMapUBO.Data().dirLightProjMatrix = lightSpaceMatrix;
        _shadowMapUBO.Upload();
        _shadowMapDirectional.BindFramebuffer();
        _shadowMapDirectional.BindTexture();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader);
        this->render(scene.GetQueue(Forward), shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowPoint(Scene& scene, Shader& shader, const ConfigUBO& config) {
        const auto& pointLights = scene.GetPointLights();

        _shadowMapPoint.BindFramebuffer();
        _shadowMapPoint.BindTexture();
        shader.Activate();

        const int count = std::min(pointLights.Count(), MAX_POINT_SHADOW_CASTERS);
        for (int i = 0; i < count; i++) {
            auto lightPos = pointLights.At(i).GetPosition();
            auto shadowMatrices = math::GetPointShadowMatrices(lightPos, 0.1, config.pointShadowFarPlane);

            shader.SetInt("lightIndex", i);
            shader.SetVec3("lightPos", lightPos);
            for (auto face = 0u; face < 6; ++face) {
                shader.SetMat4("shadowMatrices[" + std::to_string(face) + "]", shadowMatrices[face]);
            }
            this->render(scene.GetQueue(Deferred), shader);
            this->render(scene.GetQueue(Forward), shader);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowSpot(Scene& scene, Shader& shader, const ConfigUBO& config) {
        auto& spotLights = scene.GetSpotLights();
        auto& ubo = _shadowMapUBO.Data();
    
        _shadowMapSpot.BindTexture();
        shader.Activate();
        int count = std::min(spotLights.Count(), MAX_SPOT_SHADOW_CASTERS);
        for (int i = 0; i < count; i++) {
            auto& light = spotLights.At(i);
            auto lightSpaceMatrix = math::GetSpotLightSpaceMatrix(light.GetPosition(), light.GetDirection(), light.GetOuterConeDegrees(), 0.1f, config.pointShadowFarPlane);

            ubo.spotLightProjMatrices[i] = lightSpaceMatrix;
            _shadowMapSpot.BindFramebufferLayer(i);

            shader.SetMat4("spotLightSpaceMatrix", lightSpaceMatrix);
            this->render(scene.GetQueue(Deferred), shader);
            this->render(scene.GetQueue(Forward), shader);
        }

        _shadowMapUBO.Upload();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassGeometryBuffer(Scene& scene, Shader& shader) {
        _gBuffer.BindFramebuffer();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
        glViewport(0, 0, _scrWidth, _scrHeight); // restore viewport
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);
    }

    void PassSSAO(Scene& scene, Shader& shaderSSAO, Shader& shaderBlur) {
        _gBuffer.BindTextures();
        _ssao.Run(shaderSSAO);
        _ssao.Blur(shaderBlur);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassDeferred(Scene& scene, Shader& shader) {
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
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);
        _bloom.BindHdrFramebuffer();
        this->render(scene.GetQueue(Forward), shader);
    }

    void PassNoShadow(Scene& scene, Shader& unlitShader) {
        _bloom.BindHdrFramebuffer();
        unlitShader.Activate();
        this->render(scene.GetQueue(NoShadow), unlitShader);
    }

    void PassSkybox(Skybox& skybox, Shader& skyboxShader) {
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
    void render(const Scene::RenderQueue& renderQueue, Shader& shader) const {
        for (const auto& [handle, model] : renderQueue) {
            Draw(model, shader);
        }
    }

    void drawMesh(const Mesh& mesh, Shader& shader) const
    {
        for (const auto& texture : mesh.GetTextures())
        {
            const auto slot = static_cast<uint32_t>(texture.type);
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, texture.id);
        }

        glBindVertexArray(mesh.GetVAO());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.GetIndices().size()),
            GL_UNSIGNED_INT, nullptr);
    }

    std::shared_ptr<Camera> _cameraPtr;
    GBuffer _gBuffer;
    GLuint _emptyVAO = 0;
    int _scrWidth, _scrHeight;
    float _aspectRatio = 0;

    ShadowMapDirectional _shadowMapDirectional;
    ShadowMapPoint _shadowMapPoint;
    ShadowMapSpot _shadowMapSpot;
    UniformBuffer<ShadowMapUBO, 4> _shadowMapUBO {};
    Bloom _bloom;
    SSAO _ssao;
    std::shared_ptr<Skybox> _skybox;
};
