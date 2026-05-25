#pragma once

#include <glad/glad.h>
#include <utility>
#include <span>

class ElementBuffer {
public:
    ElementBuffer() {
        glCreateBuffers(1, &_id);
    }

    ~ElementBuffer() {
        glDeleteBuffers(1, &_id);
    }

    ElementBuffer(const ElementBuffer&) = delete;
    ElementBuffer& operator=(const ElementBuffer&) = delete;

    ElementBuffer(ElementBuffer&& o) noexcept : _id(std::exchange(o._id, 0)) {}

    ElementBuffer& operator=(ElementBuffer&& o) noexcept {
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
