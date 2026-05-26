#pragma once

#include <gl/headers.h>
#include <utility>

template<typename T, int BindSlot>
class ShaderStorageBuffer {
public:
    // Treat SSBO as UBO in WASM
    #ifdef __EMSCRIPTEN__
        static constexpr GLenum BufferTarget = GL_UNIFORM_BUFFER;
    #else
        static constexpr GLenum BufferTarget = GL_SHADER_STORAGE_BUFFER;
    #endif

    ShaderStorageBuffer() {
        #ifdef USE_GL_DSA
            glCreateBuffers(1, &_ssbo);
        #else
            glGenBuffers(1, &_ssbo);
        #endif
        glBindBufferBase(BufferTarget, BindSlot, _ssbo);
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
        this->Upload();
        return index;
    }

    T& Get(uint32_t index) { return _data[index]; }

    // Call manually to update SSBO after editing items via Get()
    void Upload() {
        #ifdef USE_GL_DSA
            glNamedBufferData(_ssbo, _data.size() * sizeof(T), _data.data(), GL_DYNAMIC_DRAW);
        #else
            glBindBuffer(BufferTarget, _ssbo);
            glBufferData(BufferTarget, _data.size() * sizeof(T), _data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(BufferTarget, 0);
        #endif
        glBindBufferBase(BufferTarget, BindSlot, _ssbo);
    }

private:
    GLuint _ssbo = 0;
    std::vector<T> _data;
};
