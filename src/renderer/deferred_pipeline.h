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
    DeferredPipeline(int width, int height, std::shared_ptr<Shader> geometryShader, std::shared_ptr<Shader> lightingShader)
        : _gBuffer(width, height), _geometryShader(std::move(geometryShader)), _lightingShader(std::move(lightingShader))
    {
        glCreateVertexArrays(1, &_quadVAO);
    }

    ~DeferredPipeline() {
        if (_quadVAO) glDeleteVertexArrays(1, &_quadVAO);
    }

    DeferredPipeline(const DeferredPipeline&)            = delete;
    DeferredPipeline& operator=(const DeferredPipeline&) = delete;


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

private:
    void drawMesh(const Mesh& mesh) const {
        for (const auto& texture : mesh.GetTextures()) {
            const auto slot = static_cast<uint32_t>(texture.type);
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, texture.id);
        }
        glBindVertexArray(mesh.GetVAO());
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(mesh.GetIndices().size()),
                       GL_UNSIGNED_INT, nullptr);
    }
   void drawMesh(const Mesh& mesh, Shader& shader) const
    {
        for (const auto& texture : mesh.GetTextures()) {
            const auto slot = static_cast<uint32_t>(texture.type);
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, texture.id);
        }

        glBindVertexArray(mesh.GetVAO());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.GetIndices().size()),
            GL_UNSIGNED_INT, nullptr);
    }
    GLuint _quadVAO;
    GBuffer _gBuffer;
    std::shared_ptr<Shader> _geometryShader;
    std::shared_ptr<Shader> _lightingShader;
};