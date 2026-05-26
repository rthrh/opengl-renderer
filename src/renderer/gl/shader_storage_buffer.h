#pragma once

#include <gl/headers.h>
#include <utility>
//TODO no nondsa path
template<typename T, int BindSlot>
class ShaderStorageBuffer {
public:
    ShaderStorageBuffer() {
        glCreateBuffers(1, &_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BindSlot, _ssbo);
    }

    ~ShaderStorageBuffer() {
        glDeleteBuffers(1, &_ssbo);
    }

    ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer(ShaderStorageBuffer&& o) noexcept :
        _ssbo(std::exchange(o._ssbo, 0)), _data(std::move(o._data)) {}

    ShaderStorageBuffer& operator=(ShaderStorageBuffer&& o) {
        if (this != &o) {
            glDeleteBuffers(1, &_ssbo);
            _ssbo = std::exchange(o._ssbo, 0);
            _data = std::move(o._data);
        }
        return *this;
    }

    uint32_t Pushback(T item ) {
        uint32_t index = _data.size();
        _data.emplace_back(std::move(item));
        glNamedBufferData(_ssbo, _data.size() * sizeof(T), _data.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BindSlot, _ssbo);
        return index;
    }

    T& Get(uint32_t index) { return _data[index]; }

    // Call manually to update SSBO after editing items via Get()
    void Upload() {
        glNamedBufferData(_ssbo, _data.size() * sizeof(T), _data.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BindSlot, _ssbo);
    }

private:
    GLuint _ssbo = 0;
    std::vector<T> _data;
};
