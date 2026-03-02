#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>

#include "mesh.h"
#include "shader.h"

// phong
struct Material {
    float diffuse;
    float specular;
    float shininess;
};

class Model 
{
public:
    // model data 
    std::unordered_map<std::string, Texture> m_loadedTextures;
    std::string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(std::string const &path, bool gamma = false);
    Model(Mesh& mesh, bool gamma = false);

    void Translate(glm::vec3 position) {
        m_modelMatrix = glm::translate(m_modelMatrix, std::move(position));
    }

    void Rotate(float radians, glm::vec3 axis) {
        m_modelMatrix = glm::rotate(m_modelMatrix, radians, std::move(axis));
    }

    void Scale(glm::vec3 scale) {
        m_modelMatrix = glm::scale(m_modelMatrix, std::move(scale));
    }

    glm::vec3 GetWorldPos() {
        // will return the 4th column of model matrix -> translation part
        return glm::vec3(m_modelMatrix * glm::vec4(0.f, 0.f, 0.f, 1.f));
    }

    glm::mat4 GetModelMatrix() {
        return m_modelMatrix;
    }

    std::vector<Mesh> GetMeshes() {
        return m_meshes;
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const &path);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene);

    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    std::vector<Texture> loadMaterialTextures(aiMaterial *material, aiTextureType type, TextureType typeName);
    unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);

    std::vector<Mesh> m_meshes;
    //std::vector<Material> m_materials; //not used yet
    glm::mat4 m_modelMatrix;
};