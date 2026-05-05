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

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

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
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const &path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) { // if is Not Zero
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        _directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene) {
        for(auto i = 0u; i < node->mNumMeshes; i++) {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            _meshes.emplace_back(std::move(processMesh(mesh, scene)));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(auto i = 0u; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<TextureHandle> textures;

        for (auto i = 0u; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

            if (mesh->HasNormals()) {
                vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
            } else {
                Warn("Mesh has no normals");
            }

            if (mesh->mTextureCoords[0]) { // does the mesh contain texture coordinates?
                // take the first set (0) of texture coordinates
                vertex.TexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

                // tangent + calculate handness sign for mirrored geometry
                glm::vec3 T = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                glm::vec3 B = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
                glm::vec3 N = vertex.Normal;

                float sign = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;
                vertex.Tangent = glm::vec4(T, sign);
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }
        // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(auto i = 0u; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(auto j = 0u; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        // TODO
        //auto textureTypes = {TextureType::Albedo, TextureType::Normal, TextureType::Emissive, TextureType::Metallic, TextureType::Roughness, TextureType::AO};
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. diffuse map
        std::vector<TextureHandle> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::Albedo);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. normal maps
        std::vector<TextureHandle> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, TextureType::Normal);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 3. emissive maps
        std::vector<TextureHandle> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, TextureType::Emissive);
        textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
        // 4. Ambient Occlusion - Roughness - Metalness
        TextureHandle orm = buildORM(material);
        textures.push_back(orm);
        Info("diffuseMaps: {}, normalMaps: {}, emissiveMaps: {}",
            diffuseMaps.size(), normalMaps.size(), emissiveMaps.size());

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    TextureHandle buildORM(aiMaterial* mat) {
        aiString mrPath, aoPath;
        bool hasMR = mat->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS) > 0;
        bool hasAO = mat->GetTextureCount(aiTextureType_LIGHTMAP) > 0;

        if (!hasMR && !hasAO)
            return TextureHandle{0, TextureType::ORM, ""};

        int width = 0, height = 0, channels = 0;

        unsigned char* mr = nullptr; // metalness-roughness
        unsigned char* ao = nullptr; // ambient occlusion

        if (hasMR) {
            mat->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &mrPath);
            std::string p = _directory + '/' + mrPath.C_Str();
            mr = stbi_load(p.c_str(), &width, &height, &channels, 0);
        }

        if (hasAO) {
            mat->GetTexture(aiTextureType_LIGHTMAP, 0, &aoPath);
            std::string p = _directory + '/' + aoPath.C_Str();
            ao = stbi_load(p.c_str(), &width, &height, &channels, 0);
        }

        // Combine ao and metal-roughness textures
        std::vector<unsigned char> orm(width * height * 3);
        for (int i = 0; i < width * height; i++) {
            orm[i * 3 + 0] = ao ? ao[i * channels] : 255;
            orm[i * 3 + 1] = mr ? mr[i * channels + 1] : 255;
            orm[i * 3 + 2] = mr ? mr[i * channels + 2] : 0;
        }

        uint32_t id;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, GL_RGB8, width, height);
        glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, orm.data());
        glGenerateTextureMipmap(id);

        if (mr) stbi_image_free(mr);
        if (ao) stbi_image_free(ao);

        return TextureHandle{ id, TextureType::ORM, "ORM" };
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a TextureHandle struct.
    std::vector<TextureHandle> loadMaterialTextures(aiMaterial* mat, aiTextureType aiType, TextureType type) {
        std::vector<TextureHandle> result;
        for (auto i = 0u; i < mat->GetTextureCount(aiType); i++) {
            aiString str;
            mat->GetTexture(aiType, i, &str);
            std::string fullPath = _directory + '/' + str.C_Str();

            bool gamma = (type == TextureType::Albedo || type == TextureType::Emissive);
            uint32_t id = _textureCache->load(fullPath, type, gamma);
            result.push_back(TextureHandle{ id, type, fullPath });
        }
        return result;
    }

    std::string _directory;
    std::vector<Mesh> _meshes;

    mutable glm::mat4 _modelMatrix {1.0f};
    glm::vec3 _translation {0.0f, 0.0f, 0.0f};
    glm::quat _rotation {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 _scale {1.0f, 1.0f, 1.0f};
    glm::vec3 _eulerAngles {0.0f, 0.0f, 0.0f};
    mutable bool _dirty{true};

    std::shared_ptr<TextureCache> _textureCache;
};
