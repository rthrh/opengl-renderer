#pragma once

#include <gl/headers.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <stb_image.h>

#include "utils/logger.h"
#include "material.h"
#include "gl/texture.h"
#include "gl/shader_storage_buffer.h"

// TODO get rid of
enum class TextureType {
    Albedo = 0,
    Normal = 1,
    Emissive = 2,
    ORM = 3,
};

class AssetCache {
public:
    AssetCache() {
        // Initialize dummy 1x1 fallback textures in case real ones are missing
        uint8_t white[]      = {255, 255, 255, 255};
        uint8_t black[]      = {0, 0, 0, 0};
        uint8_t flatNormal[] = {128, 128, 255, 255};
        uint8_t ormDefault[] = {255, 255, 0, 255}; // TODO find good default values

        auto albedoDummy = Texture2D(1, 1, TextureFormat::RGBA8, white);
        auto normalDummy = Texture2D(1, 1, TextureFormat::RGBA8, flatNormal);
        auto emissiveDummy = Texture2D(1, 1, TextureFormat::RGBA8, black);

        auto ormDummy = Texture2D(1, 1, TextureFormat::RGBA8, ormDefault);

        _dummyTextures.emplace_back(albedoDummy.GetID());
        _dummyTextures.emplace_back(normalDummy.GetID());
        _dummyTextures.emplace_back(emissiveDummy.GetID());
        _dummyTextures.emplace_back(ormDummy.GetID());

        _textures.emplace_back(std::move(albedoDummy));
        _textures.emplace_back(std::move(normalDummy));
        _textures.emplace_back(std::move(emissiveDummy));
        _textures.emplace_back(std::move(ormDummy));
    }

    ~AssetCache() = default;


    uint32_t AddMaterial(Material material) {
        return _materialSSBO.Pushback(std::move(material));
    }

    Material& GetMaterial(uint32_t index) {
        return _materialSSBO.Get(index);
    }

    Material GetDefaultMaterial() {
        Material material;
        material.baseColorTexture = this->GetDummyTexture(TextureType::Albedo);
        material.normalTexture    = this->GetDummyTexture(TextureType::Normal);
        material.emissiveTexture  = this->GetDummyTexture(TextureType::Emissive);
        material.ormTexture       = this->GetDummyTexture(TextureType::ORM);
        return material;
    }

    void UploadMaterials() {
        Stopwatch stopwatch("AssetCache::UploadMaterials");
        _materialSSBO.Upload();
    }

    // Creates new texture from given data and dimensions
    GLuint Load(const std::string& path, int width, int height, TextureFormat format, const void* data) {
        auto absPath = std::filesystem::absolute(path).string();
        if (auto it = _pathToId.find(absPath); it != _pathToId.end())
            return it->second;

        auto texture = Texture2D(width, height, format, data, true);
        texture.SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        texture.SetFilter(TextureFilter::LinearMipMapLinear, TextureFilter::Linear);

        auto id = texture.GetID();
        _pathToId[absPath] = id;
        _textures.emplace_back(std::move(texture));

        return id;
    }


    // Returns existing texture id if path was already loaded.
    GLuint Load(const std::string& path, bool gammaCorrect = false) {
        auto absPath = std::filesystem::absolute(path).string();
        if (auto it = _pathToId.find(absPath); it != _pathToId.end())
            return it->second;

        auto texture = upload(absPath, gammaCorrect);
        if (!texture) {
            return 0; //TODO better handling
        }

        auto id = texture->GetID();
        _pathToId[absPath] = id;
        _textures.emplace_back(std::move(*texture));

        return id;
    }

    GLuint GetDummyTexture(TextureType type) const {
        return _dummyTextures[static_cast<int>(type)];
    }

private:
    static std::optional<Texture2D> upload(const std::string& path, bool gammaCorrect) {
        int width, height, channels;
        //stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            Error("AssetCache: failed to load {}", path);
            return std::nullopt;
        }

        TextureFormat dataFormat;
        if (channels == 1) {
            dataFormat = TextureFormat::R8;
        } else if (channels == 3) {
            dataFormat = gammaCorrect ? TextureFormat::SRGB8 : TextureFormat::RGB8;
        } else {
            dataFormat = gammaCorrect ? TextureFormat::SRGB8_A8 : TextureFormat::RGBA8;
        }

        auto texture = Texture2D(width, height, dataFormat, data, true);
        texture.SetWrap(TextureWrap::Repeat, TextureWrap::Repeat);
        texture.SetFilter(TextureFilter::LinearMipMapLinear, TextureFilter::Linear);

        stbi_image_free(data);
        return texture;
    }

    std::unordered_map<std::string, GLuint> _pathToId;
    std::vector<Texture2D> _textures;
    ShaderStorageBuffer<Material, 0> _materialSSBO;

    std::vector<GLuint> _dummyTextures;
};
