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
#include "renderer/texture_slots.h"
#include "renderer/math.h"

#include "utils/logger.h"
#include "utils/stopwatch.h"

class Renderer {
public:
    explicit Renderer(int scrWidth, int scrHeight, std::shared_ptr<Camera> camera) :
        _cameraPtr(std::move(camera)),
        _gBuffer(scrWidth, scrHeight),
        _scrWidth(scrWidth), _scrHeight(scrHeight), _aspectRatio(static_cast<float>(scrWidth) / scrHeight)
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

    void PassShadow(Scene& scene, Shader& shader, ShadowMapDirectional& shadowMap) {

        auto directionalLight = scene.GetDirectionalLight();
        auto lightDir = directionalLight.has_value() ? directionalLight.value().GetDirection() : glm::vec3(0,0,0); //TODO this vector here is wrong
        auto lightSpaceMatrix = math::GetLightSpaceMatrix(lightDir);

        scene.UpdateShadowMapUBO(lightSpaceMatrix);
        shadowMap.BindFramebuffer();
        shadowMap.BindTexture();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader);
        this->render(scene.GetQueue(Forward), shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassPointShadow(Scene& scene, Shader& shader, ShadowMapPoint& shadowMap) {
        auto pointLights = scene.GetPointLights();
        auto lightPos = pointLights[0].GetPosition();
        auto shadowMatrices = math::GetShadowMatrices(lightPos);
        const float farPlane = 25.0f;

        shadowMap.BindFramebuffer();
        shadowMap.BindTexture();

        shader.Activate(); // simpleDepthShader
        for (unsigned int i = 0; i < 6; ++i) {
            shader.SetMat4("shadowMatrices[" + std::to_string(i) + "]", shadowMatrices[i]);
        }
        shader.SetFloat("farPlane", farPlane);
        shader.SetVec3("lightPos", lightPos);
        this->render(scene.GetQueue(Deferred), shader);
        this->render(scene.GetQueue(Forward), shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassGeometryBuffer(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassGeometryBuffer");
        stopwatch.Start();
        _gBuffer.BindFramebuffer();
        shader.Activate();

        this->render(scene.GetQueue(Deferred), shader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
        glViewport(0, 0, _scrWidth, _scrHeight); // restore viewport
        _gBuffer.BlitFramebuffer(0, _scrWidth, _scrHeight);

        stopwatch.Stop("PassGeometryBuffer");
    }

     void PassDeferred(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassDeferred");
        stopwatch.Start();
        _gBuffer.BindTextures();

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
        stopwatch.Start();
        _gBuffer.BlitFramebuffer(0, _scrWidth, _scrHeight);
        this->render(scene.GetQueue(Forward), shader);
        stopwatch.Stop("PassForward");
    }

    void PassSkybox(Skybox& skybox, Shader& skyboxShader) {
        glDepthFunc(GL_LEQUAL);  // pass when depth equals 1.0
        glDepthMask(GL_FALSE);   // disable depth buffer writes
        auto view = _cameraPtr->GetViewMatrix();
        auto projection = _cameraPtr->GetProjectionMatrix();
        skybox.Draw(skyboxShader, view, projection);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS); // restore default
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
};