#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>
#include <unordered_map>
#include <cassert>

#include "model.h"
#include "lights.h"

const int MAX_LIGHTS = 16;
// TODO add spotlights, pointlights, skybox, etc.
class Scene {
    using Handle = uint32_t;

    public:
        Scene() {
            this->InitUBO(m_directionalLightUBO, sizeof(DirectionalLightBlockGPU) * MAX_LIGHTS, 0);
            this->InitUBO(m_pointLightUBO, sizeof(glm::ivec4) + sizeof(PointLightBlockGPU) * MAX_LIGHTS, 1);
            this->InitUBO(m_spotLightUBO, sizeof(glm::ivec4) + sizeof(SpotLightBlockGPU) * MAX_LIGHTS, 2);
        };

        ~Scene() {
            glDeleteBuffers(1, &m_directionalLightUBO);
            glDeleteBuffers(1, &m_pointLightUBO);
            glDeleteBuffers(1, &m_spotLightUBO);
        }

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) noexcept = default;
        Scene& operator=(Scene&&) noexcept = default;

        Handle AddDirectionalLight(DirectionalLightBlockGPU light) {
            Handle index = m_handleNext;
            m_directionalLight = std::move(light);
            m_handleNext++;
            this->UpdateDirectionalLightUBO(m_directionalLightUBO, m_directionalLight.value());
            return index;
        }

        Handle AddPointLight(PointLightBlockGPU light) {
            assert(m_pointLights.size() < MAX_LIGHTS);
            Handle index = m_handleNext;
            m_pointLights.emplace_back(std::move(light));
            m_handleNext++;
            this->UpdatePointLightsUBO(m_pointLightUBO, m_pointLights);
            return index;
        }

        Handle AddSpotLight(SpotLightBlockGPU light) {
            assert(m_spotLights.size() < MAX_LIGHTS);
            Handle index = m_handleNext;
            m_spotLights.emplace_back(std::move(light));
            m_handleNext++;
            this->UpdateSpotLightsUBO(m_spotLightUBO, m_spotLights);
            return index;
        }

        Handle AddModel(Model model) {
            Handle index = m_handleNext;
            m_models.insert({m_handleNext, std::move(model)});
            m_handleNext++;
            return index;
        }

        const std::unordered_map<Handle, Model>& GetModels() const {
            return m_models;
        }

        Model& GetModel(Handle handle) {} // TODO

    private:
        void InitUBO(GLuint& ubo, size_t size, int binding) {
            glGenBuffers(1, &ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        void UpdateDirectionalLightUBO(GLuint ubo, const DirectionalLightBlockGPU& light) {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(DirectionalLightBlockGPU), &light);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        void UpdatePointLightsUBO(GLuint ubo, const std::vector<PointLightBlockGPU>& lights) {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glm::ivec4 countBlock{ static_cast<int>(lights.size()), 0, 0, 0 };
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::ivec4), &countBlock);
            glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::ivec4), sizeof(PointLightBlockGPU) * lights.size(), lights.data());
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        void UpdateSpotLightsUBO(GLuint ubo, const std::vector<SpotLightBlockGPU>& lights) {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glm::ivec4 countBlock{ static_cast<int>(lights.size()), 0, 0, 0 };
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::ivec4), &countBlock);
            glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::ivec4), sizeof(SpotLightBlockGPU) * lights.size(), lights.data());
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        std::unordered_map<Handle, Model> m_models;
        std::optional<DirectionalLightBlockGPU> m_directionalLight;
        std::vector<PointLightBlockGPU> m_pointLights;
        std::vector<SpotLightBlockGPU> m_spotLights;

        uint32_t m_handleNext{0};
        GLuint m_directionalLightUBO{0};
        GLuint m_pointLightUBO{0};
        GLuint m_spotLightUBO{0};
};
