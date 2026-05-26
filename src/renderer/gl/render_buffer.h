#pragma once

#include <gl/headers.h>
#include <utility>

#include "gl/texture.h"
#include "gl/dsa_config.h"

class RenderBuffer {
public:
    RenderBuffer() = default;

    RenderBuffer(int width, int height, TextureFormat format) :
        _format(format)
    {
        #ifdef USE_GL_DSA
            glCreateRenderbuffers(1, &_id);
            glNamedRenderbufferStorage(_id, (GLenum)format, width, height);
        #else
            glGenRenderbuffers(1, &_id);
            glBindRenderbuffer(GL_RENDERBUFFER, _id);
            glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)format, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        #endif
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
        #ifdef USE_GL_DSA
            glNamedRenderbufferStorage(_id, (GLenum)_format, width, height);
        #else
            glBindRenderbuffer(GL_RENDERBUFFER, _id);
            glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)_format, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        #endif
    }

private:
    GLuint _id = 0;
    TextureFormat _format;
};
