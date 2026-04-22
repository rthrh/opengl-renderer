#pragma once
#include <glad/glad.h>

template<typename T, int BindSlot>
class UniformBuffer {
public:
    UniformBuffer() {
        glGenBuffers(1, &_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, BindSlot, _ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
        glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &_data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    T& Data() { return _data; }
    const T& Data() const { return _data; }

private:
    GLuint _ubo = 0;
    T _data {};
};
