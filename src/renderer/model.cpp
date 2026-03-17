#include <stb_image.h>

#include <memory>

#include "model.h"


// constructor, expects a filepath to a 3D model.
Model::Model(std::string const &path, const std::shared_ptr<MaterialBuffer>& materials, const std::shared_ptr<TextureCache>& textureCache) : m_modelMatrix(1.0f), m_textureCache{textureCache}
{
    loadModel(path, materials);
}

Model::Model(Mesh mesh, const std::shared_ptr<MaterialBuffer>& materials, const std::shared_ptr<TextureCache>& textureCache) : m_modelMatrix(1.0f), m_textureCache{textureCache} {

    if (mesh.GetTextures().empty()) {
        auto dummySet = textureCache->GetDummyTextureSet();
        mesh.SetTextures(dummySet);
    }
    m_meshes.emplace_back(std::move(mesh));
}


// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
void Model::loadModel(std::string const &path, const std::shared_ptr<MaterialBuffer>& materials)
{
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // retrieve the directory path of the filepath
    m_directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
}

// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
void Model::processNode(aiNode *node, const aiScene *scene)
{
    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.emplace_back(std::move(processMesh(mesh, scene)));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }

}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // walk through each of the mesh's vertices
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
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    

    // 1. diffuse map aiTextureType_BASE_COLOR
    std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::Albedo);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. normal maps aiTextureType_NORMAL_CAMERA
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, TextureType::Normal);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 3. emissive maps     aiTextureType_EMISSION_COLOR
    std::vector<Texture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, TextureType::Emissive);
    textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
    // 4. metallic maps aiTextureType_METALNESS
    std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, TextureType::Metallic);
    textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
    // 5. roughness maps aiTextureType_DIFFUSE_ROUGHNESS
    std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::Roughness);
    textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
    // 6. ambient occlusion maps aiTextureType_AMBIENT_OCCLUSION
    std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, TextureType::AO);
    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices, textures);
}

// checks all material textures of a given type and loads the textures if they're not loaded yet.
// the required info is returned as a Texture struct.
std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType aiType, TextureType type) {
    std::vector<Texture> result;
    for (unsigned int i = 0; i < mat->GetTextureCount(aiType); i++) {
        aiString str;
        mat->GetTexture(aiType, i, &str);
        std::string fullPath = m_directory + '/' + str.C_Str();

        bool gamma = (type == TextureType::Albedo || type == TextureType::Emissive);
        uint32_t id = m_textureCache->load(fullPath, type, gamma);
        result.push_back(Texture{ id, type, fullPath });
    }
    return result;
}

