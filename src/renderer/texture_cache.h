#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include "stb_image.h"


enum class TextureType
{
    Albedo,
    Normal,
    Emissive,
    Metallic,
    Roughness,
    AO
};

struct Texture {
    uint32_t id;
    TextureType type;
    std::string path;
};

class TextureCache {
public:
    // Returns existing texture id if path was already loaded.
    uint32_t load(const std::string& path, TextureType type, bool gammaCorrect = false) {
        if (auto it = m_cache.find(path); it != m_cache.end())
            return it->second.id;

        uint32_t id = upload(path, gammaCorrect);
        m_cache[path] = Texture{ id, type, path };
        return id;
    }

    bool     has(const std::string& path) const { return m_cache.contains(path); }
    Texture  get(const std::string& path) const { return m_cache.at(path); }
    size_t   count()                      const { return m_cache.size(); }

    ~TextureCache() {
        for (auto& [path, tex] : m_cache)
            glDeleteTextures(1, &tex.id);
    }

private:
    std::unordered_map<std::string, Texture> m_cache;

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
};