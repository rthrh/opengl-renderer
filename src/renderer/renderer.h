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
#include "renderer/shadow_map.h"

#include "utils/logger.h"
#include "utils/stopwatch.h"

class Renderer {
public:
    explicit Renderer(int scrWidth, int scrHeight, std::shared_ptr<Camera> camera) :
        _cameraPtr(std::move(camera)),
        _gBuffer(scrWidth, scrHeight),
        _scrWidth(scrWidth), _scrHeight(scrHeight)
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

    // TODO move
    glm::mat4 GetLightSpaceMatrix(const glm::vec3& lightDir, float nearPlane = 1.0f, float farPlane  = 50.0f, float frustumSize = 20.0f) {
        glm::mat4 lightProj = glm::ortho(-frustumSize, frustumSize, -frustumSize, frustumSize, nearPlane, farPlane);
        //glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0)); //TODO
        glm::mat4 lightView = glm::lookAt(-glm::normalize(lightDir) * 20.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        return lightProj * lightView;
    }

    void PassShadow(Scene& scene, Shader& shader, ShadowMap& shadowMap) {

        auto directionalLight = scene.GetDirectionalLight();
        auto lightDir = directionalLight.has_value() ? directionalLight.value().GetDirection() : glm::vec3(0,0,0); //TODO this vector here is wrong
        auto lightSpaceMatrix = this->GetLightSpaceMatrix(lightDir);

        scene.UpdateShadowMapUBO(lightSpaceMatrix);
        shadowMap.BindTexture(7); //TODO remove magic binding slot number
        shadowMap.BindForWriting();
        shader.Activate();

        glEnable(GL_CULL_FACE); // TODO
        glCullFace(GL_FRONT); // TODO

        for (const auto& [handle, model] : scene.GetQueue(Deferred))
            Draw(model, shader);
        for (const auto& [handle, model] : scene.GetQueue(Forward))
            Draw(model, shader);

        glCullFace(GL_BACK);
        glDisable(GL_CULL_FACE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, _scrWidth, _scrHeight);
    }

    void PassGeometryBuffer(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassGeometryBuffer");
        stopwatch.Start();
        _gBuffer.BindForWriting();
        shader.Activate();

        render(scene.GetQueue(Deferred), shader);
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
        render(scene.GetQueue(Forward), shader);
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
    GLuint _emptyVAO {0};
    int _scrWidth, _scrHeight;
};