
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include "material.h"

class MaterialBuffer {
public:
    MaterialBuffer() {
        GLint maxBytes = 0;
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBytes);
        std::cout << "Material buffer max bytes: " << maxBytes << std::endl;
        m_maxMaterials = 512;//static_cast<size_t>(maxBytes) / sizeof(MaterialUBO);
    }

    uint32_t add(MaterialUBO gpu = {}, MaterialSlots slots = {}) {
        assert (m_gpu.size() >= m_maxMaterials);
        uint32_t id = static_cast<uint32_t>(m_gpu.size());
        m_gpu.push_back(gpu);
        m_slots.push_back(slots);
        m_dirty = true;
        return id;
    }

    MaterialUBO&   gpu(uint32_t id)   { m_dirty = true; return m_gpu[id]; }
    MaterialSlots& slots(uint32_t id) { return m_slots[id]; }

    void upload() {
        if (!m_dirty) return;

        size_t bufferSize = m_gpu.size() * sizeof(MaterialUBO);

        if (m_ubo == 0) {
            glGenBuffers(1, &m_ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
            glBufferData(GL_UNIFORM_BUFFER,
                         bufferSize, m_gpu.data(), GL_DYNAMIC_DRAW);
        } else {
            // Partial update — only upload what's there, no realloc
            glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
            glBufferSubData(GL_UNIFORM_BUFFER,
                            0, bufferSize, m_gpu.data());
        }

        m_dirty = false;
    }

    void bind(uint32_t id, GLuint bindingPoint = 0) const {
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_ubo);
        m_slots[id].bind();
    }

    size_t count()       const { return m_gpu.size(); }
    size_t maxMaterials() const { return m_maxMaterials; }
    GLuint ubo()         const { return m_ubo; }

    ~MaterialBuffer() {
        if (m_ubo != 0) glDeleteBuffers(1, &m_ubo);
    }

private:
    std::vector<MaterialUBO>   m_gpu;
    std::vector<MaterialSlots> m_slots;
    GLuint m_ubo{0};
    bool   m_dirty{false};
    size_t m_maxMaterials{512};
};