#pragma once

#include <gl/headers.h>
#include <utility>

template<typename T, int BindSlot>
class ShaderStorageBuffer {
public:
    // Treat SSBO as UBO in WASM
    #ifdef __EMSCRIPTEN__
        static constexpr GLenum BufferTarget = GL_UNIFORM_BUFFER;
        static constexpr bool AddPadding = true; // UBO needs to have full size always allocated though
        static constexpr int MaxCount = 256; // should match material.glsl GLES path
    #else
        static constexpr GLenum BufferTarget = GL_SHADER_STORAGE_BUFFER;
        static constexpr bool AddPadding = false;
        static constexpr int MaxCount = 0; // unused
    #endif

    ShaderStorageBuffer() {
        #ifdef USE_GL_DSA
            glCreateBuffers(1, &_ssbo);
        #else
            glGenBuffers(1, &_ssbo);
        #endif
        glBindBufferBase(BufferTarget, BindSlot, _ssbo);
        this->Upload();
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
        if constexpr (AddPadding) {
            // UBO path needs padding to MaxCount
            std::vector<T> padded(MaxCount);
            std::copy(_data.begin(), _data.begin() + std::min<size_t>(_data.size(), MaxCount), padded.begin());
            glBindBuffer(BufferTarget, _ssbo);
            glBufferData(BufferTarget, MaxCount * sizeof(T), padded.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(BufferTarget, 0);
        } else {
            glBindBuffer(BufferTarget, _ssbo);
            glBufferData(BufferTarget, _data.size() * sizeof(T), _data.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(BufferTarget, 0);
        }
        #endif
        glBindBufferBase(BufferTarget, BindSlot, _ssbo);
    }

private:
    GLuint _ssbo = 0;
    std::vector<T> _data;
};
