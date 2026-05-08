#pragma once

#include <glad/glad.h>
#include <gl/texture.h>

class RenderBuffer {
public:
    RenderBuffer() = default;

    RenderBuffer(int width, int height, TextureFormat format) {
        glCreateRenderbuffers(1, &_id);
        glNamedRenderbufferStorage(_id, (GLenum)format, width, height);
    }

    ~RenderBuffer() {
        glDeleteRenderbuffers(1, &_id);
    }

    RenderBuffer(const RenderBuffer&) = delete;
    RenderBuffer& operator=(const RenderBuffer&) = delete;

    RenderBuffer(RenderBuffer&& o) noexcept :
        _id(std::exchange(o._id, 0)) {}

    RenderBuffer& operator=(RenderBuffer&& o) noexcept {
        if (this != &o) {
            glDeleteRenderbuffers(1, &_id);
            _id = std::exchange(o._id, 0);
        }
        return *this;
    }

    GLuint GetID() const { return _id; }

private:
    GLuint _id = 0;
};
