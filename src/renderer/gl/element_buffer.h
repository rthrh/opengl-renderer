#pragma once

#include <gl/headers.h>
#include <utility>
#include <span>
#include "dsa_config.h"

class ElementBuffer {
public:
    ElementBuffer() {
        #ifdef USE_GL_DSA
            glCreateBuffers(1, &_id);
        #else
            glGenBuffers(1, &_id);
        #endif
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

    // Creates immutable storage and uploads data once.
    template <class T>
    void SetStorage(std::span<const T> data) {
        #ifdef USE_GL_DSA
            glNamedBufferStorage(_id, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
        #else
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id);
            #ifdef __EMSCRIPTEN__
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);
            #else
                glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
            #endif
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        #endif
    }

    // Creates mutable storage. Can be called repeatedly to update data and size
    template <class T>
    void SetData(std::span<const T> data) {
        #ifdef USE_GL_DSA
            glNamedBufferData(_id, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);
        #else
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.size_bytes(), data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        #endif
    }

private:
    GLuint _id = 0;
};
