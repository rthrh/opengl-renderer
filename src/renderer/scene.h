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
#include <memory>

#include "model.h"
#include "ubo.h"
#include "asset_cache.h"
#include "shapes.h"

#include "gl/uniform_buffer.h"
#include "gl/shader_storage_buffer.h"

enum RenderQueueType {
    Masked, // Alpha masked + alpha cut off
    Opaque, // Opaque
    Blend, // Blend rendered, sorted TODO
    NoShadow // Debug unlit
};

class Scene {
public:
    using Handle = uint32_t;
    using RenderQueue = std::unordered_map<Handle, Model>;

    Scene(const std::shared_ptr<AssetCache>& assetCache) :
        _assetCache(assetCache)
    {

    }
    ~Scene() = default;

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

        // Add debug light marker
        uint32_t defaultMatIndex = _assetCache->AddMaterial(_assetCache->GetDefaultMaterial());
        Mesh markerMesh(cube_vertices, cube_indices, defaultMatIndex);
        Model markerModel(std::move(markerMesh));
        markerModel.SetTranslation(light.GetPosition());
        markerModel.SetScale({0.1f, 0.1f, 0.1f});
        AddModel(std::move(markerModel), NoShadow);

        return index;
    }

    PointLightUBO& GetPointLights() {
        return _pointLightUBO.Data();
    }

    Handle AddSpotLight(SpotLightBlockGPU light) {
        Handle index = _handleNext;
        _handleNext++;

        _spotLightUBO.Data().Pushback(light);
        _spotLightUBO.Upload();

        // Add debug light marker
        uint32_t defaultMatIndex = _assetCache->AddMaterial(_assetCache->GetDefaultMaterial());
        Mesh markerMesh(cube_vertices, cube_indices, defaultMatIndex);
        Model markerModel(std::move(markerMesh));
        markerModel.SetTranslation(light.GetPosition());
        markerModel.SetScale({0.1f, 0.1f, 0.1f});
        AddModel(std::move(markerModel), NoShadow);

        return index;
    }

    SpotLightUBO& GetSpotLights() {
        return _spotLightUBO.Data();
    }

    Handle AddModel(Model model, RenderQueueType queue = Opaque) {
        Handle index = _handleNext;
        switch (queue) {
            case Masked:
                _maskedQueue.insert({_handleNext, std::move(model)});
                break;
            case Opaque:
                _opaqueQueue.insert({_handleNext, std::move(model)});
                break;
            case Blend:
                _blendQueue.insert({_handleNext, std::move(model)});
                break;
            case NoShadow:
                _noShadowQueue.insert({_handleNext, std::move(model)});
                break;
        }

        _handleNext++;
        return index;
    }

    RenderQueue& GetQueue(RenderQueueType queue) {
        switch (queue) {
            case Masked:
                return _maskedQueue;
            case Opaque:
                return _opaqueQueue;
            case Blend:
                return _blendQueue;
            case NoShadow:
                return _noShadowQueue;
        }
        std::unreachable();
    }

private:
    RenderQueue _maskedQueue;
    RenderQueue _blendQueue;
    RenderQueue _opaqueQueue;
    RenderQueue _noShadowQueue;

    uint32_t _handleNext{0};

    UniformBuffer<DirectionalLightUBO, 0> _directionalLightUBO;
    UniformBuffer<PointLightUBO, 1> _pointLightUBO;
    UniformBuffer<SpotLightUBO, 2> _spotLightUBO;

    std::shared_ptr<AssetCache> _assetCache;
};
