#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "model.h"
#include "mesh.h"
#include "scene.h"
#include "logger.h"

class Renderer {
public:
    Renderer() = default;
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
        const auto& models = scene.GetModels();

        for (const auto& [handle, model] : models) {
            Draw(model, shader);
        }
    }

private:
    void drawMesh(const Mesh& mesh, Shader& shader) const
    {
        for (const auto& texture : mesh.GetTextures())
        {
            const uint32_t slot = static_cast<uint32_t>(texture.type);
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, texture.id);
        }

        glBindVertexArray(mesh.GetVAO());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.GetIndices().size()),
            GL_UNSIGNED_INT, nullptr);
    }
};