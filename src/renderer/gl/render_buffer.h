#pragma once

#include <glad/glad.h>
#include <gl/texture.h>

class RenderBuffer {
public:
    RenderBuffer() = default;

    RenderBuffer(int width, int height, TextureFormat format) {
        _format = format;
        glCreateRenderbuffers(1, &_id);
        glNamedRenderbufferStorage(_id, (GLenum)format, width, height);
    }

    ~RenderBuffer() {
        glDeleteRenderbuffers(1, &_id);
    }

    RenderBuffer(const RenderBuffer&) = delete;
    RenderBuffer& operator=(const RenderBuffer&) = delete;

    RenderBuffer(RenderBuffer&& o) noexcept :
        _id(std::exchange(o._id, 0)),
        _format(o._format) {}

    RenderBuffer& operator=(RenderBuffer&& o) noexcept {
        if (this != &o) {
            glDeleteRenderbuffers(1, &_id);
            _id = std::exchange(o._id, 0);
            _format = o._format;
        }
        return *this;
    }

    GLuint GetID() const { return _id; }

    void Resize(int width, int height) {
        glNamedRenderbufferStorage(_id, (GLenum)_format, width, height);
    }

private:
    GLuint _id = 0;
    TextureFormat _format;
};
