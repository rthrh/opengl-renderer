#include <glad/glad.h>


enum class TextureFilter : GLenum {
    Nearest = GL_NEAREST,
    Linear = GL_LINEAR,
    // TODO add mipmaps
};

enum class TextureWrap : GLenum {
    Repeat = GL_REPEAT,
    MirroredRepeat = GL_MIRRORED_REPEAT,
    ClampToEdge = GL_CLAMP_TO_EDGE,
    ClampToBorder = GL_CLAMP_TO_BORDER
};


class Texture {
public:
    Texture(){}
    ~Texture(){}

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&&) noexcept = default;

private:
    GLint _handle;

};