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
#include <utility>

#include "model.h"
#include "ubo.h"

enum RenderQueueType {
    Forward,
    Deferred
};

const int MAX_LIGHTS = 16;
// TODO add spotlights, pointlights, skybox, etc.
class Scene {
public:
    using Handle = uint32_t;
    using RenderQueue = std::unordered_map<Handle, Model>;

    Scene() {
        this->InitUBO(_directionalLightUBO, sizeof(DirectionalLightBlockGPU) * MAX_LIGHTS, 0);
        this->InitUBO(_pointLightUBO, sizeof(glm::ivec4) + sizeof(PointLightBlockGPU) * MAX_LIGHTS, 1);
        this->InitUBO(_spotLightUBO, sizeof(glm::ivec4) + sizeof(SpotLightBlockGPU) * MAX_LIGHTS, 2);
        this->InitUBO(_shadowMapUBO, sizeof(ShadowMapBlockGPU), 4);
    };

    ~Scene() {
        glDeleteBuffers(1, &_directionalLightUBO);
        glDeleteBuffers(1, &_pointLightUBO);
        glDeleteBuffers(1, &_spotLightUBO);
        glDeleteBuffers(1, &_shadowMapUBO);
    }

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    Handle AddDirectionalLight(DirectionalLightBlockGPU light) {
        Handle index = _handleNext;
        _directionalLight = std::move(light);
        _handleNext++;
        this->UpdateDirectionalLightUBO(_directionalLightUBO, _directionalLight.value());
        return index;
    }

    std::optional<DirectionalLightBlockGPU>& GetDirectionalLight() {
        return _directionalLight;
    }

    Handle AddPointLight(PointLightBlockGPU light) {
        assert(_pointLights.size() < MAX_LIGHTS);
        Handle index = _handleNext;
        _pointLights.emplace_back(std::move(light));
        _handleNext++;
        this->UpdatePointLightsUBO(_pointLightUBO, _pointLights);
        return index;
    }

    std::vector<PointLightBlockGPU>& GetPointLights() {
        return _pointLights;
    }

    Handle AddSpotLight(SpotLightBlockGPU light) {
        assert(_spotLights.size() < MAX_LIGHTS);
        Handle index = _handleNext;
        _spotLights.emplace_back(std::move(light));
        _handleNext++;
        this->UpdateSpotLightsUBO(_spotLightUBO, _spotLights);
        return index;
    }

    std::vector<SpotLightBlockGPU>& GetSpotLights() {
        return _spotLights;
    }

    Handle AddModel(Model model, RenderQueueType queue = Deferred) {
        Handle index = _handleNext;
        switch (queue)
        {
            case Forward:
                _forwardQueue.insert({_handleNext, std::move(model)});
                break;
            case Deferred:
                _deferredQueue.insert({_handleNext, std::move(model)});
                break;
        }

        _handleNext++;
        return index;
    }

    const RenderQueue& GetQueue(RenderQueueType queue) const {
        switch (queue) {
            case Forward:
                return _forwardQueue;
            case Deferred:
                return _deferredQueue;
        }
        std::unreachable();
    }

    //TODO rename to Upload? remove param?
    void UpdateShadowMapUBO(const ShadowMapBlockGPU lightSpaceMatrix) {
        _lightSpaceMatrices = lightSpaceMatrix;
        glBindBuffer(GL_UNIFORM_BUFFER, _shadowMapUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShadowMapBlockGPU), &_lightSpaceMatrices);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ShadowMapBlockGPU GetLightSpaceMatrices() {
        return _lightSpaceMatrices;
    };

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

    RenderQueue _forwardQueue;
    RenderQueue _deferredQueue;

    std::optional<DirectionalLightBlockGPU> _directionalLight;
    std::vector<PointLightBlockGPU> _pointLights;
    std::vector<SpotLightBlockGPU> _spotLights;
    ShadowMapBlockGPU _lightSpaceMatrices;

    uint32_t _handleNext{0};
    GLuint _directionalLightUBO{0};
    GLuint _pointLightUBO{0};
    GLuint _spotLightUBO{0};
    GLuint _shadowMapUBO{0};
};
