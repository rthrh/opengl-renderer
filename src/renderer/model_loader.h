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
#include <filesystem>

#include "mesh.h"
#include "texture_cache.h"
#include "model.h"

// TODO refactor
class ModelLoader
{
public:
    ModelLoader(const std::shared_ptr<TextureCache>& textureCache) : _textureCache{textureCache} {
    }

    ModelLoader(const ModelLoader&) = delete;
    ModelLoader& operator=(const ModelLoader&) = delete;
    ModelLoader(ModelLoader&&) = default;
    ModelLoader& operator=(ModelLoader&&) = default;

    std::optional<Model> Load(std::filesystem::path& path) {
        std::vector<Mesh> meshes;
        this->loadModel(path.string(), meshes);
        return Model(std::move(meshes));
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
        // DEBUG print all materials available
        /*
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            aiColor4D diffuse;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
                Info("Material {} diffuse color: {:.2f} {:.2f} {:.2f}", 
                    mat->GetName().C_Str(), diffuse.r, diffuse.g, diffuse.b);
        }*/

        if (auto t = loadMaterialTexture(material, aiTextureType_BASE_COLOR, TextureType::Albedo))
            textures.push_back(*t);
        else {
            aiColor4D c(1.0f, 1.0f, 1.0f, 1.0f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, c);
            textures.push_back({_textureCache->CreateColor(c.r, c.g, c.b, c.a), TextureType::Albedo, "color"});
        }

        if (auto t = loadMaterialTexture(material, aiTextureType_NORMALS, TextureType::Normal)) {
            textures.push_back(*t);
        }
        else {
            textures.push_back(_textureCache->GetDummyTexture(TextureType::Normal));
        }

        if (auto t = loadMaterialTexture(material, aiTextureType_EMISSIVE, TextureType::Emissive)) {
            textures.push_back(*t);
        }
        else {
            textures.push_back(_textureCache->GetDummyTexture(TextureType::Emissive));
        }

        // 4. Ambient Occlusion - Roughness - Metalness
        TextureHandle orm = buildORM(material);
        textures.push_back(orm);
        Info("Textures num: {}", textures.size());

        /*Info("=== Mesh: {} ===", mesh->mName.C_Str());
        for (unsigned int t = 0; t < aiTextureType_UNKNOWN; t++) {
            if (material->GetTextureCount((aiTextureType)t) > 0) {
                aiString p;
                material->GetTexture((aiTextureType)t, 0, &p);
                Info("  [{}] {} = {}", t, aiTextureTypeToString((aiTextureType)t), p.C_Str());
            }
        }*/

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


    std::optional<TextureHandle> loadMaterialTexture(aiMaterial* mat, aiTextureType aiType, TextureType type) {
        if (mat->GetTextureCount(aiType) == 0) return std::nullopt;
        aiString str;
        mat->GetTexture(aiType, 0, &str);
        std::string fullPath = (std::filesystem::path(_directory) / str.C_Str()).string();
        bool gamma = (type == TextureType::Albedo || type == TextureType::Emissive);
        uint32_t id = _textureCache->load(fullPath, type, gamma);
        return TextureHandle{id, type, fullPath};
    }

    std::string _directory;
    std::string _name;
    std::shared_ptr<TextureCache> _textureCache;
};
