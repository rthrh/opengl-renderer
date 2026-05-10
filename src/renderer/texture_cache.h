#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <stb_image.h>

#include "gl/texture.h"
#include "utils/logger.h"

// TODO get rid of
enum class TextureType {
    Albedo = 0,
    Normal = 1,
    Emissive = 2,
    ORM = 3,
};

struct TextureHandle {
    GLuint id;
    TextureType type;
    std::string path;
};

class TextureCache {
public:
    TextureCache() {
        // Initialize dummy 1x1 fallback textures in case real ones are missing
        uint8_t white[]      = {255, 255, 255, 255};
        uint8_t black[]      = {0, 0, 0, 0};
        uint8_t flatNormal[] = {128, 128, 255, 255};
        uint8_t ormDefault[] = {255, 255, 0, 255}; // TODO find good default values

        auto whiteDummy = Texture2D(1, 1, TextureFormat::RGBA8, white);
        auto blackDummy = Texture2D(1, 1, TextureFormat::RGBA8, black);
        auto normalDummy = Texture2D(1, 1, TextureFormat::RGBA8, flatNormal);
        auto ormDummy = Texture2D(1, 1, TextureFormat::RGBA8, ormDefault);

        _dummyTextures.emplace_back(TextureHandle{.id = whiteDummy.GetID(), .type = TextureType::Albedo});
        _dummyTextures.emplace_back(TextureHandle{.id = normalDummy.GetID(), .type = TextureType::Normal});
        _dummyTextures.emplace_back(TextureHandle{.id = blackDummy.GetID(), .type = TextureType::Emissive});
        _dummyTextures.emplace_back(TextureHandle{.id = ormDummy.GetID(), .type = TextureType::ORM});

        _textures.emplace_back(std::move(whiteDummy));
        _textures.emplace_back(std::move(blackDummy));
        _textures.emplace_back(std::move(normalDummy));
        _textures.emplace_back(std::move(ormDummy));
    }

    ~TextureCache() = default;


    // Returns existing texture id if path was already loaded.
    uint32_t load(const std::string& path, TextureType type, bool gammaCorrect = false) {
        auto absPath = std::filesystem::absolute(path).string();
        if (auto it = _cache.find(absPath); it != _cache.end())
            return it->second.id;

        auto texture = upload(absPath, gammaCorrect);
        if (!texture) {
            return 0; //TODO better handling
        }

        auto id = texture->GetID();
        _cache[absPath] = TextureHandle{ id, type, absPath };
        _textures.emplace_back(std::move(*texture));

        return id;
    }

    TextureHandle GetDummyTexture(TextureType type) const {
        return _dummyTextures[static_cast<int>(type)];
    }

private:
    static std::optional<Texture2D> upload(const std::string& path, bool gammaCorrect) {
        int width, height, channels;
        //stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            Error("TextureCache: failed to load {}", path);
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

    std::unordered_map<std::string, TextureHandle> _cache;
    std::vector<Texture2D> _textures;
    std::vector<TextureHandle> _dummyTextures;
};
