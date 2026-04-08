#pragma once

#include <glad/glad.h>
#include "renderer/gbuffer.h"
#include "renderer/shader.h"
#include "renderer/scene.h"
#include "renderer/camera.h"
#include "renderer/render_queue.h"

#include <memory>


class DeferredPipeline {
public:
    explicit DeferredPipeline(int width, int height, std::shared_ptr<Shader> geometryShader, std::shared_ptr<Shader> lightingShader)
        : _gBuffer(width, height), _geometryShader(std::move(geometryShader)), _lightingShader(std::move(lightingShader))
    {
        glCreateVertexArrays(1, &_quadVAO);
    }

    ~DeferredPipeline() {
        if (_quadVAO) glDeleteVertexArrays(1, &_quadVAO);
    }

    DeferredPipeline(const DeferredPipeline&)            = delete;
    DeferredPipeline& operator=(const DeferredPipeline&) = delete;
    DeferredPipeline(DeferredPipeline&& other) noexcept = default;
    DeferredPipeline& operator=(DeferredPipeline&& other) noexcept = default;

    void PassGeometry(const Scene& scene, const std::shared_ptr<Camera>& camera) {
        _gBuffer.BindForWriting();
        _geometryShader->Activate();
        _geometryShader->SetMat4("view", camera->GetViewMatrix());
        _geometryShader->SetMat4("projection", camera->GetProjectionMatrix());

        for (const auto& [handle, model] : scene.GetQueue(RenderQueueType::Deferred)) {
            _geometryShader->SetMat4("model", model.GetModelMatrix());
            for (auto& mesh : model.GetMeshes())
                drawMesh(mesh);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // restore default FBO
    }

    void PassLighting(const Camera& camera, int width, int height) {
        glViewport(0, 0, width, height);

        _gBuffer.BindTextures();

        _lightingShader->Activate();
        _lightingShader->SetVec3("viewPos", camera.Position);

        glBindVertexArray(_quadVAO); // TODO need to bind empty VAO?
        glDepthMask(GL_FALSE); // depth buffer was already filled in geometry pass
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDepthMask(GL_TRUE);
    }

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

    GLuint _quadVAO {0};
    GBuffer _gBuffer;
    std::shared_ptr<Shader> _geometryShader;
    std::shared_ptr<Shader> _lightingShader;
};