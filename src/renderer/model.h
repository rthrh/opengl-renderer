#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <iostream>
#include <vector>

#include "mesh.h"
#include "texture_cache.h"

class Model
{
public:
    Model(Mesh mesh, std::string name = "") :
        _name(std::move(name)),
        _meshes()
    {
        _meshes.emplace_back(std::move(mesh));
    }

    Model(std::vector<Mesh> meshes, std::string name = "") :
        _name(std::move(name)),
        _meshes(std::move(meshes))
    {
    }

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    const std::string& GetName() const { return _name; }

    void SetTranslation(glm::vec3 position) {
        _translation = position;
        _dirty = true;
    }

    glm::vec3 GetTranslation() const { return _translation; }

    // x - pitch, y - yaw, z - roll, CCW
    void SetEulerAngles(glm::vec3 degrees) {
        _eulerAngles = degrees;
        _rotation = glm::quat(glm::radians(degrees)); // convert for internal use
        _dirty = true;
    }

    glm::vec3 GetEulerAngles() const {
        return _eulerAngles;
    }

    void SetScale(glm::vec3 scale) {
        _scale = scale;
        _dirty = true;
    }

    glm::vec3 GetScale() const { return _scale; }

    glm::vec3 GetWorldPos() const { return _translation; }

    glm::mat4 GetModelMatrix() const {
        if (_dirty) {
            _modelMatrix = glm::translate(glm::mat4(1.0f), _translation)
                         * glm::mat4_cast(_rotation)
                         * glm::scale(glm::mat4(1.0f), _scale);
            _dirty = false;
        }
        return _modelMatrix;
    }

    const std::vector<Mesh>& GetMeshes() const {
        return _meshes;
    }

private:
    std::string _name;
    std::vector<Mesh> _meshes;

    mutable glm::mat4 _modelMatrix {1.0f};
    glm::vec3 _translation {0.0f, 0.0f, 0.0f};
    glm::quat _rotation {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 _scale {1.0f, 1.0f, 1.0f};
    glm::vec3 _eulerAngles {0.0f, 0.0f, 0.0f};
    mutable bool _dirty{true};

};
