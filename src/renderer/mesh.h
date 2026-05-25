#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <utility>
#include <span>

#include "utils/stopwatch.h"
#include "gl/vertex_array.h"
#include "gl/vertex_buffer.h"
#include "gl/element_buffer.h"

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
    Mesh(Mesh&& o) noexcept = default;
    Mesh& operator=(Mesh&& o) noexcept = default;

    ~Mesh() = default;

    GLuint GetVAO() const {
        return _VAO.GetID();
    }

    const std::vector<unsigned int>& GetIndices() const {
        return _indices;
    }

    uint32_t GetMaterialIndex() const { return _materialIndex; }
    GLuint GetIndexCount() const { return _indices.size(); }
    GLsizei GetInstanceCount() const { return _instanceCount; }

    // Uploads instance matrices for this mesh
    void SetInstances(std::span<const glm::mat4> matrices) {
        _instanceCount = static_cast<GLsizei>(matrices.size());
        _instanceVBO.SetData(matrices);
    }

private:
    // Initializes all the buffer objects/arrays
    void setupMesh()
    {
        // Load data into vertex buffers and element buffer
        _VBO.SetStorage<Vertex>(_vertices);
        _EBO.SetStorage<unsigned>(_indices);
        _instanceVBO.SetData<glm::mat4>({});

        // Bind EBO to VAO, set divisor for instance matrix
        _VAO.BindElementBuffer(_EBO.GetID());

        // Bind VBO to VAO at binding index 0
        _VAO.BindVertexBuffer(_VBO.GetID(), 0, 0, sizeof(Vertex));
        _VAO.AddAttribute(0, 0, 3, offsetof(Vertex, Position));
        _VAO.AddAttribute(0, 1, 3, offsetof(Vertex, Normal));
        _VAO.AddAttribute(0, 2, 2, offsetof(Vertex, TexCoords));
        _VAO.AddAttribute(0, 3, 4, offsetof(Vertex, Tangent));

        // Instance VBO matrix - once per instance
        _VAO.BindVertexBuffer(_instanceVBO.GetID(), 1, 0, sizeof(glm::mat4));
        _VAO.SetBindingDivisor(1, 1);
        constexpr int step = sizeof(glm::vec4);
        _VAO.AddAttribute(1, 4, 4, 0 * step);
        _VAO.AddAttribute(1, 5, 4, 1 * step);
        _VAO.AddAttribute(1, 6, 4, 2 * step);
        _VAO.AddAttribute(1, 7, 4, 3 * step);
    }

    VertexArray _VAO;
    VertexBuffer _VBO;
    ElementBuffer _EBO;
    VertexBuffer _instanceVBO;

    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    uint32_t _materialIndex = 0;
    GLsizei _instanceCount = 0;
};
