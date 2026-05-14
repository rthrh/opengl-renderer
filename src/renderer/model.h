#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <iostream>
#include <vector>

#include "mesh.h"
#include "asset_cache.h"

struct Transform {
    glm::vec3 translation{0.f};
    glm::vec3 eulerAngles{0.f};
    glm::vec3 scale{1.f, 1.f, 1.f};
};

class Model
{
public:
    Model(Mesh mesh, std::string name = "") :
        _name(std::move(name)),
        _meshes()
    {
        _meshes.emplace_back(std::move(mesh));
        this->AddInstance(Transform{}); //TODO better init
    }

    Model(std::vector<Mesh> meshes, std::string name = "") :
        _name(std::move(name)),
        _meshes(std::move(meshes))
    {
        this->AddInstance(Transform{}); //TODO better init
    }

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    const std::string& GetName() const { return _name; }

    void AddInstance(Transform transform) {
        _instances.emplace_back(std::move(transform));
        _dirty = true;
    }

    void SetInstances(std::vector<Transform> transforms) {
        _instances = std::move(transforms);
        _dirty = true;
    }

    void SetTranslation(glm::vec3 position, unsigned instanceID = 0) {
        _instances[instanceID].translation = position;
        _dirty = true;
    }

    glm::vec3 GetTranslation(unsigned instanceID = 0) const {
        return _instances[instanceID].translation;
    }

    // x - pitch, y - yaw, z - roll, CCW
    void SetEulerAngles(glm::vec3 degrees, unsigned instanceID = 0) {
        _instances[instanceID].eulerAngles = degrees;
        _dirty = true;
    }

    glm::vec3 GetEulerAngles(unsigned instanceID = 0) const {
        return _instances[instanceID].eulerAngles;
    }

    void SetScale(glm::vec3 scale, unsigned instanceID = 0) {
        _instances[instanceID].scale = scale;
        _dirty = true;
    }

    glm::vec3 GetScale(unsigned instanceID = 0) const {
        return _instances[instanceID].scale;
    }

    glm::vec3 GetWorldPos(unsigned instanceID = 0) const {
        return _instances[instanceID].translation;
    }

    const std::vector<Mesh>& GetMeshes() const {
        return _meshes;
    }

    void UploadTransforms() {
        if (!_dirty) return;
        std::vector<glm::mat4> matrices;
        matrices.reserve(_instances.size());
        for (auto& inst : _instances) {
            glm::quat rotation = glm::quat(glm::radians(inst.eulerAngles));
            matrices.push_back(
                glm::translate(glm::mat4(1.f), inst.translation)
                * glm::mat4_cast(rotation)
                * glm::scale(glm::mat4(1.f), inst.scale)
            );
        }
        for (auto& mesh : _meshes)
            mesh.SetInstances(matrices);
        _dirty = false;
    }

private:
    std::string _name;
    std::vector<Mesh> _meshes;
    std::vector<Transform> _instances;

    bool _dirty{true};
};
