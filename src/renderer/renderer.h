#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
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

#include "utils/logger.h"
#include "utils/stopwatch.h"

class Renderer {
public:
    explicit Renderer(int scrWidth, int scrHeight, std::shared_ptr<Camera> camera) :
        _cameraPtr(std::move(camera)),
        _gBuffer(scrWidth, scrHeight),
        _scrWidth(scrWidth),
        _scrHeight(scrHeight),
        _shadowMapDirectional(),
        _shadowMapPoint(),
        _shadowMapSpot(),
        _bloom(scrWidth, scrHeight)
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
        auto lightDir = directionalLight.has_value() ? directionalLight.value().GetDirection() : glm::vec3(0,0,0); //TODO this vector here is wrong
        auto lightSpaceMatrix = math::GetLightSpaceMatrix(lightDir);

        auto lsm = scene.GetLightSpaceMatrices();
        lsm.dirLightProjMatrix = lightSpaceMatrix;
        scene.UpdateShadowMapUBO(lsm);
        _shadowMapDirectional.BindFramebuffer();
        _shadowMapDirectional.BindTexture();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader);
        this->render(scene.GetQueue(Forward), shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassPointShadow(Scene& scene, Shader& shader) {
        const float farPlane = 25.0f;
        const auto& pointLights = scene.GetPointLights();
        int count = pointLights.size();

        _shadowMapPoint.BindFramebuffer();
        _shadowMapPoint.BindTexture();
        shader.Activate(); // simpleDepthShader
        shader.SetFloat("farPlane", farPlane);
        for (int i = 0; i < count; i++) {
            auto lightPos = pointLights[i].GetPosition();
            auto shadowMatrices = math::GetShadowMatrices(lightPos);

            shader.SetInt("lightIndex", i);
            shader.SetVec3("lightPos", lightPos);
            for (unsigned int face = 0; face < 6; ++face) {
                shader.SetMat4("shadowMatrices[" + std::to_string(face) + "]", shadowMatrices[face]);
            }
            this->render(scene.GetQueue(Deferred), shader);
            this->render(scene.GetQueue(Forward), shader);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassShadowSpot(Scene& scene, Shader& shader) {
        auto& spotLights = scene.GetSpotLights();
        auto lsm = scene.GetLightSpaceMatrices(); //TODO remove this call
        _shadowMapSpot.BindTexture();
        shader.Activate();
        int count = spotLights.size(); // TODO check for MAX SHADOW CASTERS
        for (int i = 0; i < count; i++) {
            auto& light = spotLights[i];
            auto lightSpaceMatrix = math::GetSpotLightSpaceMatrix(light.GetPosition(), light.GetDirection(), light.GetOuterCone());

            lsm.spotLightProjMatrices[i] = lightSpaceMatrix;
            _shadowMapSpot.BindFramebufferLayer(i);

            shader.SetMat4("spotLightSpaceMatrix", lightSpaceMatrix);
            this->render(scene.GetQueue(Deferred), shader);
            this->render(scene.GetQueue(Forward), shader);
        }

        scene.UpdateShadowMapUBO(lsm);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassGeometryBuffer(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassGeometryBuffer");
        //stopwatch.Start();
        _gBuffer.BindFramebuffer();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
        glViewport(0, 0, _scrWidth, _scrHeight); // restore viewport
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);

        stopwatch.Stop("PassGeometryBuffer");
    }

     void PassDeferred(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassDeferred");
        //stopwatch.Start();
        _gBuffer.BindTextures();
        _bloom.BindHdrFramebuffer();
        glClear(GL_COLOR_BUFFER_BIT); // clear color (removes artifacts when rendering closer than z-near)

        shader.Activate();
        shader.SetFloat("farPlane", 25.0f); // TODO move it!!!
        // render empty fullscreen quad
        //glDepthMask(GL_FALSE);
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        //glDepthMask(GL_TRUE);

        stopwatch.Stop("PassDeferred");
    }

    void PassForward(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassForward");
        //stopwatch.Start();
        _gBuffer.BlitFramebuffer(_bloom.GetHdrFBO(), _scrWidth, _scrHeight);
        _bloom.BindHdrFramebuffer();
        this->render(scene.GetQueue(Forward), shader);
        stopwatch.Stop("PassForward");
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

        int bloom = 1; float exposure = 1.0;
        bloomShader.SetFloat("exposure", exposure);
        bloomShader.SetInt("scene", 0);
        bloomShader.SetInt("bloomBlur", 1);
        bloomShader.SetInt("bloom", 1);
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
    Bloom _bloom;

};