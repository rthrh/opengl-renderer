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
#include <unordered_map>

#include "mesh.h"
#include "shader.h"
#include "material.h"
#include "material_buffer.h"
#include "texture_cache.h"

class Model 
{
public:
    // constructor, expects a filepath to a 3D model.
    Model(std::string const &path, std::shared_ptr<MaterialBuffer>& materials, std::shared_ptr<TextureCache>& textureCache);
    Model(Mesh mesh, std::shared_ptr<MaterialBuffer>& materials, std::shared_ptr<TextureCache>& textureCache);

    Model(const Model&)            = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&)                 = default;
    Model& operator=(Model&&)      = default;

    void Translate(glm::vec3 position) {
        m_modelMatrix = glm::translate(m_modelMatrix, std::move(position));
    }

    void Rotate(float radians, glm::vec3 axis) {
        m_modelMatrix = glm::rotate(m_modelMatrix, radians, std::move(axis));
    }

    void Scale(glm::vec3 scale) {
        m_modelMatrix = glm::scale(m_modelMatrix, std::move(scale));
    }

    const glm::vec3 GetWorldPos() const {
        // will return the 4th column of model matrix -> translation part
        return glm::vec3(m_modelMatrix * glm::vec4(0.f, 0.f, 0.f, 1.f));
    }

    const glm::mat4 GetModelMatrix() const {
        return m_modelMatrix;
    }

    const std::vector<Mesh>& GetMeshes() const {
        return m_meshes;
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const &path, std::shared_ptr<MaterialBuffer>& materials);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene);

    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    std::vector<Texture> loadMaterialTextures(aiMaterial *material, aiTextureType type, TextureType typeName);

    std::string m_directory;
    std::vector<Mesh> m_meshes;
    glm::mat4 m_modelMatrix;
    std::shared_ptr<TextureCache> m_textureCache;
};
