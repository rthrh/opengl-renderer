#pragma once

#include <gl/headers.h>
#include <utility>

#include "dsa_config.h"

template<typename T>
class UniformBuffer {
public:
    UniformBuffer(GLuint bindSlot) :
        _bindSlot(bindSlot)
    {
        #ifdef USE_GL_DSA
            glCreateBuffers(1, &_ubo);
            glNamedBufferData(_ubo, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, _bindSlot, _ubo);
        #else
            glGenBuffers(1, &_ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, _bindSlot, _ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        #endif
    }

    ~UniformBuffer() {
        glDeleteBuffers(1, &_ubo);
    }

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& o) noexcept :
        _ubo(std::exchange(o._ubo, 0)), _data(std::move(o._data)) {}

    UniformBuffer& operator=(UniformBuffer&& o) {
        if (this != &o) {
            glDeleteBuffers(1, &_ubo);
            _ubo = std::exchange(o._ubo, 0);
            _data = std::move(o._data);
        }
        return *this;
    }

    void Upload() {
        #ifdef USE_GL_DSA
            glNamedBufferSubData(_ubo, 0, sizeof(T), &_data);
        #else
            glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &_data);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        #endif
    }

    T& Data() { return _data; }
    const T& Data() const { return _data; }

    void SetData(T data) {
        _data = std::move(data);
    }

private:
    GLuint _ubo = 0;
    GLuint _bindSlot = 0;
    T _data {};
};
