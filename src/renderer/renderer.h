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

    void PassGeometryBuffer(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassGeometryBuffer");
        stopwatch.Start();
        _gBuffer.BindForWriting();
        shader.Activate();

        render(scene.GetQueue(Deferred), shader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
        stopwatch.Stop("PassGeometryBuffer");
    }

     void PassDeferred(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassDeferred");
        stopwatch.Start();
        _gBuffer.BindTextures();

        shader.Activate();

        // render empty fullscreen quad
        glDepthMask(GL_FALSE);
        glBindVertexArray(_emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDepthMask(GL_TRUE);

        stopwatch.Stop("PassDeferred");
    }

    void PassForward(Scene& scene, Shader& shader) {
        Stopwatch stopwatch("PassForward");
        stopwatch.Start();
        _gBuffer.BlitFramebuffer(0, _scrWidth, _scrHeight);
        render(scene.GetQueue(Forward), shader);
        stopwatch.Stop("PassForward");
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