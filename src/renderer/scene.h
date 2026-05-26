#pragma once

#include <gl/headers.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
#include "mesh_cache.h"

#include "gl/uniform_buffer.h"
#include "gl/shader_storage_buffer.h"

#include "mesh_cache.h"

class Scene {
public:
    using ModelHandle = uint32_t;

    Scene(const std::shared_ptr<AssetCache>& assetCache) :
        _assetCache(assetCache)
    {

    }
    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    ModelHandle AddDirectionalLight(DirectionalLightUBO light) {
        ModelHandle index = _handleNext;
        _handleNext++;

        _directionalLightUBO.Data() = light;
        _directionalLightUBO.Upload();
        return index;
    }

    DirectionalLightUBO& GetDirectionalLight() {
        return _directionalLightUBO.Data();
    }

    ModelHandle AddPointLight(PointLightBlockGPU light) {
        ModelHandle index = _handleNext;
        _handleNext++;

        _pointLightUBO.Data().Pushback(light);
        _pointLightUBO.Upload();

        // Add debug light marker
        uint32_t defaultMatIndex = _assetCache->AddMaterial(_assetCache->GetDefaultMaterial());
        //Mesh markerMesh(cube_vertices, cube_indices, defaultMatIndex);
        //Model markerModel(std::move(markerMesh));
        //markerModel.SetTranslation(light.GetPosition());
        //markerModel.SetScale({0.1f, 0.1f, 0.1f});
        //AddModel(std::move(markerModel), NoShadow);

        return index;
    }

    PointLightUBO& GetPointLights() {
        return _pointLightUBO.Data();
    }

    ModelHandle AddSpotLight(SpotLightBlockGPU light) {
        ModelHandle index = _handleNext;
        _handleNext++;

        _spotLightUBO.Data().Pushback(light);
        _spotLightUBO.Upload();

        // Add debug light marker
        uint32_t defaultMatIndex = _assetCache->AddMaterial(_assetCache->GetDefaultMaterial());
        //Mesh markerMesh(cube_vertices, cube_indices, defaultMatIndex);
        //Model markerModel(std::move(markerMesh));
        //markerModel.SetTranslation(light.GetPosition());
        //markerModel.SetScale({0.1f, 0.1f, 0.1f});
        //AddModel(std::move(markerModel), NoShadow);

        return index;
    }

    SpotLightUBO& GetSpotLights() {
        return _spotLightUBO.Data();
    }

    ModelHandle AddModel(Model model) {
        ModelHandle handle = _handleNext++;
        _modelMap.insert({handle, std::move(model)});
        return handle;
    }

    // Call at start of each frame to update
    void UploadTransforms(const std::shared_ptr<MeshCache>& meshCache) {
        for (auto& [handle, model] : _modelMap) model.UploadTransforms(meshCache);

    }


private:
    uint32_t _handleNext = 0;
    UniformBuffer<DirectionalLightUBO, 0> _directionalLightUBO;
    UniformBuffer<PointLightUBO, 1> _pointLightUBO;
    UniformBuffer<SpotLightUBO, 2> _spotLightUBO;
    std::shared_ptr<AssetCache> _assetCache;
    std::unordered_map<ModelHandle, Model> _modelMap;
};
