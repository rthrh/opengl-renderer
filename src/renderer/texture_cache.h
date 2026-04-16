#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <stb_image.h>


enum class TextureType {
    Albedo = 0,
    Normal = 1,
    Emissive = 2,
    Metallic = 3,
    Roughness = 4,
    AO = 5,
    Shadow = 6
};

struct Texture {
    GLuint id;
    TextureType type;
    std::string path;
};

class TextureCache {
public:
    TextureCache() {
        initDummies();
    }

    ~TextureCache() {
        for (auto& [path, tex] : _cache)
            glDeleteTextures(1, &tex.id);
    }


    // Returns existing texture id if path was already loaded.
    uint32_t load(const std::string& path, TextureType type, bool gammaCorrect = false) {
        if (auto it = _cache.find(path); it != _cache.end())
            return it->second.id;

        uint32_t id = upload(path, gammaCorrect);
        _cache[path] = Texture{ id, type, path };
        return id;
    }

    bool     has(const std::string& path) const { return _cache.contains(path); }
    Texture  get(const std::string& path) const { return _cache.at(path); }
    size_t   count()                      const { return _cache.size(); }

    Texture GetDummyTexture(TextureType type) {
        return _dummyTextures[static_cast<int>(type)];
    }


    // todo move it somewhere
    std::vector<Texture> GetDummyTextureSet() {
        return std::vector<Texture>(&_dummyTextures[0], &_dummyTextures[6]); // TODO check if textures up to AO are returned and make it more robust
    }

private:
    void initDummies() {
        uint8_t white[]      = {255, 255, 255, 255};
        uint8_t black[]      = {0, 0, 0, 0};
        uint8_t flatNormal[] = {128, 128, 255, 255};
        _whiteDummy = createDummy(white);
        _blackDummy = createDummy(black);
        _normalDummy = createDummy(flatNormal);

        _dummyTextures.emplace_back(Texture{.id = _whiteDummy, .type = TextureType::Albedo});
        _dummyTextures.emplace_back(Texture{.id = _normalDummy, .type = TextureType::Normal});
        _dummyTextures.emplace_back(Texture{.id = _blackDummy, .type = TextureType::Emissive});
        _dummyTextures.emplace_back(Texture{.id = _whiteDummy, .type = TextureType::Metallic});
        _dummyTextures.emplace_back(Texture{.id = _whiteDummy, .type = TextureType::Roughness});
        _dummyTextures.emplace_back(Texture{.id = _whiteDummy, .type = TextureType::AO});
        _dummyTextures.emplace_back(Texture{.id = _blackDummy, .type = TextureType::Shadow});
    }

    static uint32_t upload(const std::string& path, bool gammaCorrect) {
        int width, height, channels;
        //stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            std::cerr << "TextureCache: failed to load " << path << "\n";
            return 0;
        }

        GLenum internalFormat, dataFormat;
        if (channels == 1) {
            internalFormat = dataFormat = GL_RED;
        } else if (channels == 3) {
            internalFormat = gammaCorrect ? GL_SRGB : GL_RGB;
            dataFormat     = GL_RGB;
        } else {
            internalFormat = gammaCorrect ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat     = GL_RGBA;
        }

        uint32_t id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                     dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return id;
    }

    static GLuint createDummy(uint8_t rgba[4]) {
        GLuint tex;
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        glTextureStorage2D(tex, 1, GL_RGBA8, 1, 1);
        glTextureSubImage2D(tex, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        return tex;
    }

    std::unordered_map<std::string, Texture> _cache;
    std::vector<Texture> _dummyTextures;
    GLuint _whiteDummy = 0, _blackDummy = 0, _normalDummy = 0;
};
