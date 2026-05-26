#pragma once

#include <gl/headers.h>
#include <utility>
#include <span>

class VertexBuffer {
public:
    VertexBuffer() {
        if constexpr (USE_DSA) {
            glCreateBuffers(1, &_id);
        } else {
            glGenBuffers(1, &_id);
        }
    }

    ~VertexBuffer() {
        glDeleteBuffers(1, &_id);
    }

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    VertexBuffer(VertexBuffer&& o) noexcept : _id(std::exchange(o._id, 0)) {}

    VertexBuffer& operator=(VertexBuffer&& o) noexcept {
        if (this != &o) {
            glDeleteBuffers(1, &_id);
            _id = std::exchange(o._id, 0);
        }
        return *this;
    }

    GLuint GetID() const { return _id; }

    // Creates immutable storage and uploads data once
    template <class T> //TODO gonna need fallback tp glBufferData on EMSCRIPTEN
    void SetStorage(std::span<const T> data) {
        if constexpr (USE_DSA) {
            glNamedBufferStorage(_id, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, _id);
            glBufferStorage(GL_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    // Creates mutable storage. Can be called repeatedly to update data and size
    template <class T>
    void SetData(std::span<const T> data) {
        if constexpr (USE_DSA) {
            glNamedBufferData(_id, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, _id);
            glBufferData(GL_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

private:
    GLuint _id = 0;
};
