#include <stdint.h>
#include <string_view>
#include <unordered_map>
#include <string>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

struct TextureX {

};

using TextureHandle = uint32_t;
const TextureHandle NULL_TEXTURE = 0;

class TextureCache {
public: 
    TextureCache() = default;
    ~TextureCache() {};

    TextureCache(const TextureCache&)            = delete;
    TextureCache& operator=(const TextureCache&) = delete;
    TextureCache(TextureCache&&)                 = default;
    TextureCache& operator=(TextureCache&&)      = default;

    TextureHandle Load(const std::string& path) {
        if (m_pathCache.find(path.data()) == m_pathCache.end()) {
            auto handle = this->load(path);
            m_pathCache.insert({path, handle});
        }
    }

    static TextureHandle load(const std::string& path) {
        std::string filename = std::string(path);

        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }

    std::unordered_map<TextureHandle, TextureX> m_textureCache;
    std::unordered_map<std::string, TextureHandle> m_pathCache;
};
