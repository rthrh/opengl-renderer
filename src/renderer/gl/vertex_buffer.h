#pragma once

#include <glad/glad.h>
#include <utility>
#include <span>

class VertexBuffer {
public:
    VertexBuffer() {
        glCreateBuffers(1, &_id);
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
    template <class T>
    void SetStorage(std::span<const T> data) {
        glNamedBufferStorage(_id, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
    }

    // Creates mutable storage. Can be called repeatedly to update data and size
    template <class T>
    void SetData(std::span<const T> data) {
        glNamedBufferData(_id, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);
    }

private:
    GLuint _id = 0;
};
