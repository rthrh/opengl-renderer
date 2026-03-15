#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>
#include <unordered_map>

#include "model.h"
#include "lights.h"


const int MaxLights = 16;
// TODO add spotlights, pointlights, skybox, etc.
class Scene {
    using Handle = uint32_t;

    public:
        Scene() {
            
            glGenBuffers(1, &m_directionalLightUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, m_directionalLightUBO);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(DirectionalLightBlockGPU), nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_directionalLightUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            glGenBuffers(1, &m_pointLightUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, m_pointLightUBO);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightBlockGPU), nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_pointLightUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        };

        ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        Scene(Scene&&) noexcept = default;
        Scene& operator=(Scene&&) noexcept = default;

        Handle AddDirectionalLight(DirectionalLightBlockGPU light) {
            Handle index = m_handleNext;
            //m_directionalLights.insert({m_handleNext, std::move(light)});
            m_directionalLight = std::move(light);
            m_handleNext++;
            this->UpdateDirectionalLightUBO(m_directionalLightUBO, m_directionalLight.value());
            return index;
        }

        Handle AddPointLight(PointLightBlockGPU light) {
            Handle index = m_handleNext;
            //m_pointLights.insert({m_handleNext, std::move(light)});
            m_pointLightsGPU.push_back(std::move(light));
            m_handleNext++;
            this->UpdatePointLightsUBO(m_pointLightUBO, m_pointLightsGPU);
            return index;
        }

        Handle AddSpotLight(SpotLight light) {
            Handle index = m_handleNext;
            m_spotLights.insert({m_handleNext, std::move(light)});
            m_handleNext++;
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

        void UpdateDirectionalLightUBO(GLuint ubo, const DirectionalLightBlockGPU& light) {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(DirectionalLightBlockGPU), &light);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        void UpdatePointLightsUBO(GLuint ubo, const std::vector<PointLightBlockGPU>& lights) {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);

            // ---- Upload count (first 16 bytes) ----
            glm::ivec4 countBlock{ static_cast<int>(lights.size()), 0, 0, 0 };
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::ivec4), &countBlock);

            // ---- Upload light array ----
            glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::ivec4), sizeof(PointLightBlockGPU) * lights.size(), lights.data());

            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        std::unordered_map<Handle, Model> m_models;
        //std::unordered_map<Handle, DirectionalLight> m_directionalLights;
        std::optional<DirectionalLightBlockGPU> m_directionalLight;
        //std::unordered_map<Handle, PointLight> m_pointLights;
        std::unordered_map<Handle, SpotLight> m_spotLights;

        std::vector<PointLightBlockGPU> m_pointLightsGPU;

        uint32_t m_handleNext{0};
        GLuint m_directionalLightUBO{0};
        GLuint m_pointLightUBO{0};
};
