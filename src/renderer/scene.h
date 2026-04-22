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
#include "gl/uniform_buffer.h"

enum RenderQueueType {
    Forward,
    Deferred
};

const int MAX_LIGHTS = 16;
class Scene {
public:
    using Handle = uint32_t;
    using RenderQueue = std::unordered_map<Handle, Model>;

    Scene() {
        //this->InitUBO(_directionalLightUBO, sizeof(DirectionalLightUBO) * MAX_LIGHTS, 0);
        //this->InitUBO(_pointLightUBO, sizeof(glm::ivec4) + sizeof(PointLightBlockGPU) * MAX_LIGHTS, 1);
        //this->InitUBO(_spotLightUBO, sizeof(glm::ivec4) + sizeof(SpotLightBlockGPU) * MAX_LIGHTS, 2);
        this->InitUBO(_shadowMapUBO, sizeof(ShadowMapUBO), 4);
    };

    ~Scene() {
        //glDeleteBuffers(1, &_directionalLightUBO);
        //glDeleteBuffers(1, &_pointLightUBO);
        //glDeleteBuffers(1, &_spotLightUBO);
        glDeleteBuffers(1, &_shadowMapUBO);
    }

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    Handle AddDirectionalLight(DirectionalLightUBO light) {
        Handle index = _handleNext;
        _handleNext++;

        _directionalLightUBO.Data() = light;
        _directionalLightUBO.Upload();
        return index;
    }

    DirectionalLightUBO& GetDirectionalLight() {
        return _directionalLightUBO.Data();
    }

    Handle AddPointLight(PointLightBlockGPU light) {
        Handle index = _handleNext;
        _handleNext++;

        _pointLightUBO.Data().Pushback(light);
        _pointLightUBO.Upload();
        return index;
    }

    PointLightUBO GetPointLights() {
        return _pointLightUBO.Data();
    }

    Handle AddSpotLight(SpotLightBlockGPU light) {
        Handle index = _handleNext;
        _handleNext++;

        _spotLightUBO.Data().Pushback(light);
        _spotLightUBO.Upload();
        return index;
    }

    SpotLightUBO& GetSpotLights() {
        return _spotLightUBO.Data();
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
    void UpdateShadowMapUBO(const ShadowMapUBO lightSpaceMatrix) {
        _lightSpaceMatrices = lightSpaceMatrix;
        glBindBuffer(GL_UNIFORM_BUFFER, _shadowMapUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShadowMapUBO), &_lightSpaceMatrices);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ShadowMapUBO GetLightSpaceMatrices() {
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

    RenderQueue _forwardQueue;
    RenderQueue _deferredQueue;
    RenderQueue _noShadowQueue; //TODO rename?

    ShadowMapUBO _lightSpaceMatrices;

    uint32_t _handleNext{0};
    GLuint _shadowMapUBO{0};

    UniformBuffer<DirectionalLightUBO, 0> _directionalLightUBO {};
    UniformBuffer<PointLightUBO, 1> _pointLightUBO {};
    UniformBuffer<SpotLightUBO, 2> _spotLightUBO {};
};
