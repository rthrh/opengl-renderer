#pragma once
#include <glad/glad.h>


class TextureGL {
public:
    TextureGL() {

    }

    ~TextureGL() {
    }

    TextureGL(const TextureGL&) = delete;
    TextureGL& operator=(const TextureGL&) = delete;
    TextureGL(TextureGL&& o) noexcept {}

    TextureGL& operator=(TextureGL&& o) {
        if (this != &o) {

        }
        return *this;
    }



private:
    GLuint _ubo = 0;
    //T _data {};
};
