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
    // constructor, expects a filepath to a 3D model.
    Model(std::string const &path,const std::shared_ptr<TextureCache>& textureCache) : _modelMatrix(1.0f), _textureCache{textureCache} {
        loadModel(path);
    }

    Model(Mesh mesh, const std::shared_ptr<TextureCache>& textureCache) : _modelMatrix(1.0f), _textureCache{textureCache} {
        if (mesh.GetTextures().empty()) {
            auto dummySet = textureCache->GetDummyTextureSet();
            mesh.SetTextures(dummySet);
        }
        _meshes.emplace_back(std::move(mesh));
    }

    Model(const Model&)            = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&)                 = default;
    Model& operator=(Model&&)      = default;

    void SetTranslation(glm::vec3 position) {
        _translation = position;
        _dirty = true;
    }

    void SetRotation(float radians, glm::vec3 axis) {
        _rotation = glm::angleAxis(radians, glm::normalize(axis));
        _dirty = true;
    }

    void SetScale(glm::vec3 scale) {
        _scale = scale;
        _dirty = true;
    }

    glm::vec3 GetWorldPos() const {
        return _translation;
    }

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
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const &path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        _directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            _meshes.emplace_back(std::move(processMesh(mesh, scene)));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            // positions
            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;

            // normals
            if (mesh->HasNormals())
            {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            } else {
                std::cout << "Warn: mesh has no normals" << std::endl;
            }

            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                // take the first set (0) of texture coordinates
                vertex.TexCoords.x = mesh->mTextureCoords[0][i].x; 
                vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
                // tangent
                vertex.Tangent.x = mesh->mTangents[i].x;
                vertex.Tangent.y = mesh->mTangents[i].y;
                vertex.Tangent.z = mesh->mTangents[i].z;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        // TODO
        //auto textureTypes = {TextureType::Albedo, TextureType::Normal, TextureType::Emissive, TextureType::Metallic, TextureType::Roughness, TextureType::AO};
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    

        // 1. diffuse map
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::Albedo);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, TextureType::Normal);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 3. emissive maps
        std::vector<Texture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, TextureType::Emissive);
        textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
        // 4. metallic maps
        std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, TextureType::Metallic);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
        // 5. roughness maps
        std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::Roughness);
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
        // 6. ambient occlusion maps
        std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, TextureType::AO);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
        Info("diffuseMaps: {}, normalMaps: {}, emissiveMaps: {}, metallicMaps: {}, roughnessMaps: {}, aoMaps: {}",
            diffuseMaps.size(), normalMaps.size(), emissiveMaps.size(), metallicMaps.size(), roughnessMaps.size(), aoMaps.size());

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType aiType, TextureType type) {
        std::vector<Texture> result;
        for (unsigned int i = 0; i < mat->GetTextureCount(aiType); i++) {
            aiString str;
            mat->GetTexture(aiType, i, &str);
            std::string fullPath = _directory + '/' + str.C_Str();

            bool gamma = (type == TextureType::Albedo || type == TextureType::Emissive);
            uint32_t id = _textureCache->load(fullPath, type, gamma);
            result.push_back(Texture{ id, type, fullPath });
        }
        return result;
    }

    std::string _directory;
    std::vector<Mesh> _meshes;

    mutable glm::mat4 _modelMatrix{1.0f};
    glm::vec3 _translation{0.0f, 0.0f, 0.0f};
    glm::quat _rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 _scale{1.0f, 1.0f, 1.0f};
    mutable bool _dirty{true};

    std::shared_ptr<TextureCache> _textureCache;
};
