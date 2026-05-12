#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

#include <string>
#include <iostream>
#include <vector>
#include <filesystem>

#include "mesh.h"
#include "asset_cache.h"
#include "model.h"

class ModelLoader
{
public:
    ModelLoader(const std::shared_ptr<AssetCache>& assetCache) : _assetCache{assetCache} {
    }

    ModelLoader(const ModelLoader&) = delete;
    ModelLoader& operator=(const ModelLoader&) = delete;
    ModelLoader(ModelLoader&&) = default;
    ModelLoader& operator=(ModelLoader&&) = default;

    std::optional<Model> Load(const std::filesystem::path& path) {
        std::vector<Mesh> meshes;
        this->loadModel(path.string(), meshes);
        return Model(std::move(meshes));
    }

    Model Load(Mesh mesh) {
        return Model(std::move(mesh));
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(std::string const &path, std::vector<Mesh>& meshes) {
        Assimp::Importer importer;

        // aiProcess_PreTransformVertices will disable animations
        auto flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals| aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices; 
        const aiScene* scene = importer.ReadFile(path, flags);

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) { // if is Not Zero
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        _directory = std::filesystem::path(path).parent_path().string();
        _name = std::filesystem::path(path).stem().string();
        processNode(scene->mRootNode, scene, meshes);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene, std::vector<Mesh>& meshes) {
        for(auto i = 0u; i < node->mNumMeshes; i++) {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.emplace_back(std::move(processMesh(mesh, scene)));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(auto i = 0u; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, meshes);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

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
        aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
        Material meshMaterial = _assetCache->GetDefaultMaterial();

        aiString alphaMode;
        if (aiMaterial->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
            std::string_view mode = alphaMode.C_Str();
            if (mode == "MASK")  meshMaterial.alphaMode = AlphaMode::Mask;
            if (mode == "BLEND") meshMaterial.alphaMode = AlphaMode::Blend;
        }

        float alphaCutoff = 0.0f;
        if (aiMaterial->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS)
            meshMaterial.alphaCutoff = alphaCutoff;

        int doubleSided = 0;
        if (aiMaterial->Get(AI_MATKEY_TWOSIDED, doubleSided) == AI_SUCCESS)
            meshMaterial.doubleSided = doubleSided;

        float metallicFactor = 0.0f, roughnessFactor = 0.0f;
        if (aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS)
            meshMaterial.metallicFactor = metallicFactor;

        if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS)
            meshMaterial.roughnessFactor = roughnessFactor;

        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
            meshMaterial.emissiveFactor = glm::vec4(emissive.r, emissive.g, emissive.b, 1.0f);

        // DEBUG print all materials available
        /*
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            aiColor4D diffuse;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
                Info("Material {} diffuse color: {:.2f} {:.2f} {:.2f}", 
                    mat->GetName().C_Str(), diffuse.r, diffuse.g, diffuse.b);
        }*/

        if (auto texture = loadMaterialTexture(aiMaterial, aiTextureType_BASE_COLOR)) {
            meshMaterial.baseColorTexture = texture;
        }

        aiColor4D c(1.0f, 1.0f, 1.0f, 1.0f);
        if (aiMaterial->Get(AI_MATKEY_BASE_COLOR, c) == AI_SUCCESS) {
            meshMaterial.baseColorFactor = {c.r, c.g, c.b, c.a};
        }

        if (auto texture = loadMaterialTexture(aiMaterial, aiTextureType_NORMALS)) {
            meshMaterial.normalTexture = texture;
        }

        float normalScale = 1.0f;
        if (aiMaterial->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), normalScale) == AI_SUCCESS) {
            meshMaterial.normalScale = normalScale;
        }

        if (auto texture = loadMaterialTexture(aiMaterial, aiTextureType_EMISSIVE)) {
            meshMaterial.emissiveTexture = texture;
        }

        float occlusionStrength = 1.0f;
        if (aiMaterial->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0), occlusionStrength) == AI_SUCCESS) {
            meshMaterial.occlusionStrength = occlusionStrength;
        }

        // Ambient Occlusion - Roughness - Metalness
        if (auto texture = buildORM(aiMaterial)) {
            meshMaterial.ormTexture = texture;
        }

        /*Info("=== Mesh: {} ===", mesh->mName.C_Str());
        for (unsigned int t = 0; t < aiTextureType_UNKNOWN; t++) {
            if (aiMaterial->GetTextureCount((aiTextureType)t) > 0) {
                aiString p;
                aiMaterial->GetTexture((aiTextureType)t, 0, &p);
                Info("  [{}] {} = {}", t, aiTextureTypeToString((aiTextureType)t), p.C_Str());
            }
        }*/

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, meshMaterial);
    }


    GLuint buildORM(aiMaterial* mat) {
        aiString mrPath, aoPath;
        bool hasMR = mat->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS) > 0;
        bool hasAO = mat->GetTextureCount(aiTextureType_LIGHTMAP) > 0;

        if (!hasMR && !hasAO)
            return _assetCache->GetDummyTexture(TextureType::ORM);

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

        // Use metallic-roughness path as key
        std::string fullPath = (std::filesystem::path(_directory) / mrPath.C_Str()).string();
        auto id = _assetCache->Load(fullPath, width, height, TextureFormat::RGB8, orm.data());

        if (mr) stbi_image_free(mr);
        if (ao) stbi_image_free(ao);

        return id;
    }


    GLuint loadMaterialTexture(aiMaterial* mat, aiTextureType aiType) {
        if (mat->GetTextureCount(aiType) == 0) return 0;
        aiString str;
        mat->GetTexture(aiType, 0, &str);
        std::string fullPath = (std::filesystem::path(_directory) / str.C_Str()).string();

        bool gamma = (aiType == aiTextureType_BASE_COLOR || aiType == aiTextureType_EMISSIVE);
        return _assetCache->Load(fullPath, gamma);
    }

    std::string _directory;
    std::string _name;
    std::shared_ptr<AssetCache> _assetCache;
};
