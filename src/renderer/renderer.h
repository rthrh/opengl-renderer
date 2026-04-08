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

class Renderer {
public:
    explicit Renderer(std::shared_ptr<Camera> camera) :
        _cameraPtr(std::move(camera)),
        _gBuffer(SSCR_WIDTH, SSCR_HEIGHT) {} //TODO remove SCR_WIDTH and HEIGHT

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

    void Render(Scene& scene, Shader& shader) const {
        const auto& deferred = scene.GetQueue(Deferred);
        for (const auto& [handle, model] : deferred) {
            Draw(model, shader);
        }

    }

    void PassGeometry(Scene& scene, Shader& shader) {
        _gBuffer.BindForWriting();
        shader.Activate();
        shader.SetMat4("view", _cameraPtr->GetViewMatrix());
        shader.SetMat4("projection", _cameraPtr->GetProjectionMatrix());

        Render(scene, shader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
    }

    void BindGBuffer() {
        _gBuffer.BindTextures();
    }

private:
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
};