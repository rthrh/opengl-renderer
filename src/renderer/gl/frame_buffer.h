#pragma once

#include <glad/glad.h>
#include <utility>

class FrameBuffer {
public:
    FrameBuffer() {
        glCreateFramebuffers(1, &_id);
    }

    ~FrameBuffer() {
        glDeleteFramebuffers(1, &_id);
    }

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    FrameBuffer(FrameBuffer&& o) noexcept {
        _id = std::exchange(o._id, 0);
    }

    FrameBuffer& operator=(FrameBuffer&& o) {
        if (this != &o) {
            _id = std::exchange(o._id, 0);
        }
        return *this;
    }

    GLuint GetId() {
        return _id;
    }

private:
    GLuint _id;
};
