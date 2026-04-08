#pragma once

#include <glad/glad.h>
#include "renderer/shader.h"
#include "renderer/scene.h"
#include "renderer/camera.h"

class ForwardPipeline {
public:
    explicit ForwardPipeline(std::shared_ptr<Shader> shader)
        : _shader(std::move(shader))
    {}

    ForwardPipeline(const ForwardPipeline&)            = delete;
    ForwardPipeline& operator=(const ForwardPipeline&) = delete;
    ForwardPipeline(ForwardPipeline&& other) noexcept = default;
    ForwardPipeline& operator=(ForwardPipeline&& other) noexcept = default;

    void PassForward(const Scene& scene, const std::shared_ptr<Camera>& camera) {
        // TODO do transparency
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        _shader->Activate();
        _shader->SetMat4("view",       camera->GetViewMatrix());
        _shader->SetMat4("projection", camera->GetProjectionMatrix());
        _shader->SetVec3("viewPos",    camera->Position);

        for (const auto& [handle, model] : scene.GetQueue(RenderQueueType::Forward)) {
            _shader->SetMat4("model", model.GetModelMatrix());
            for (auto& mesh : model.GetMeshes())
                drawMesh(mesh);
        }

        //glDisable(GL_BLEND);
    }

    void ReloadShaders() { _shader->Reload(); }

private:
    void drawMesh(const Mesh& mesh) const {
        for (const auto& texture : mesh.GetTextures()) {
            const auto slot = static_cast<uint32_t>(texture.type);
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, texture.id);
        }
        glBindVertexArray(mesh.GetVAO());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.GetIndices().size()), GL_UNSIGNED_INT, nullptr);
    }

    std::shared_ptr<Shader> _shader;
};