#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <utility>
#include <span>

#include "utils/stopwatch.h"
#include "gl/vertex_array.h"

// TODO GL_INT_2_10_10_10_REV packing
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal{};
    glm::vec2 TexCoords{};
    glm::vec4 Tangent{}; // xyz = tangent, w = handness sign
};


class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, uint32_t materialIndex)
        : _vertices(std::move(vertices)), _indices(std::move(indices)), _materialIndex(materialIndex)
    {
        setupMesh();
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& o) noexcept:
        _VAO(std::move(o._VAO)),
        _VBO(std::exchange(o._VBO, 0)),
        _EBO(std::exchange(o._EBO, 0)),
        _instanceVBO(std::exchange(o._instanceVBO, 0)),
        _vertices(std::move(o._vertices)),
        _indices(std::move(o._indices)),
        _materialIndex(o._materialIndex),
        _instanceCount(o._instanceCount)
    {}

    Mesh& operator=(Mesh&& o) noexcept {
        if (this == &o) return *this;

        glDeleteBuffers(1, &_VBO);
        glDeleteBuffers(1, &_EBO);
        glDeleteBuffers(1, &_instanceVBO);
        _VAO = std::move(o._VAO);
        _VBO = std::exchange(o._VBO, 0);
        _EBO = std::exchange(o._EBO, 0);
        _instanceVBO = std::exchange(o._instanceVBO, 0);
        _vertices = std::move(o._vertices);
        _indices  = std::move(o._indices);
        _materialIndex  = o._materialIndex;
        _instanceCount  = o._instanceCount;

        return *this;
    }

    ~Mesh() {
        glDeleteBuffers(1, &_VBO);
        glDeleteBuffers(1, &_EBO);
        glDeleteBuffers(1, &_instanceVBO);
    }

    GLuint GetVAO() const
    {
        return _VAO.GetID();
    }

    const std::vector<unsigned int>& GetIndices() const {
        return _indices;
    }

    uint32_t GetMaterialIndex() const { return _materialIndex; }
    GLuint GetIndexCount() const { return _indices.size(); }
    GLsizei GetInstanceCount() const { return _instanceCount; }

    void SetInstances(std::span<const glm::mat4> matrices) {
        _instanceCount = static_cast<GLsizei>(matrices.size());
        glNamedBufferData(_instanceVBO, matrices.size_bytes(), matrices.data(), GL_DYNAMIC_DRAW);
    }

private:
    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glCreateBuffers(1, &_VBO);
        glCreateBuffers(1, &_EBO);
        glCreateBuffers(1, &_instanceVBO);
        _VAO.SetBindingDivisor(1, 1);

        // load data into vertex buffers
        glNamedBufferStorage(_VBO, _vertices.size() * sizeof(Vertex), _vertices.data(), GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferStorage(_EBO, _indices.size() * sizeof(unsigned int), _indices.data(), GL_DYNAMIC_STORAGE_BIT);

        // bind EBO to VAO
        _VAO.BindElementBuffer(_EBO);

        // bind VBO to VAO at binding index 0
        _VAO.BindVertexBuffer(_VBO, 0, 0, sizeof(Vertex));
        _VAO.BindVertexBuffer(_instanceVBO, 1, 0, sizeof(glm::mat4));

        // set the vertex attributes
        _VAO.AddAttribute(0, 0, 3, offsetof(Vertex, Position));
        _VAO.AddAttribute(0, 1, 3, offsetof(Vertex, Normal));
        _VAO.AddAttribute(0, 2, 2, offsetof(Vertex, TexCoords));
        _VAO.AddAttribute(0, 3, 4, offsetof(Vertex, Tangent));

        // instance matrix - once per instance
        int step = sizeof(glm::vec4);
        _VAO.AddAttribute(1, 4, 4, 0 * step);
        _VAO.AddAttribute(1, 5, 4, 1 * step);
        _VAO.AddAttribute(1, 6, 4, 2 * step);
        _VAO.AddAttribute(1, 7, 4, 3 * step);
    }

    // render data
    VertexArray _VAO;
    GLuint _VBO = 0, _EBO = 0;
    GLuint _instanceVBO = 0;
    // mesh Data
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    uint32_t _materialIndex = 0;
    GLsizei _instanceCount = 0;
};
