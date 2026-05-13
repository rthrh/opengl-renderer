#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <utility>

#include "utils/stopwatch.h"

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
        _VAO(std::exchange(o._VAO, 0)),
        _VBO(std::exchange(o._VBO, 0)),
        _EBO(std::exchange(o._EBO, 0)),
        _vertices(std::move(o._vertices)),
        _indices(std::move(o._indices)),
        _materialIndex(o._materialIndex)
    {}

    Mesh& operator=(Mesh&& o) noexcept {
        if (this == &o) return *this;
        glDeleteVertexArrays(1, &_VAO);
        glDeleteBuffers(1, &_VBO);
        glDeleteBuffers(1, &_EBO);
        _VAO = std::exchange(o._VAO, 0);
        _VBO = std::exchange(o._VBO, 0);
        _EBO = std::exchange(o._EBO, 0);
        _vertices = std::move(o._vertices);
        _indices  = std::move(o._indices);
        _materialIndex  = o._materialIndex;
        return *this;
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &_VAO);
        glDeleteBuffers(1, &_VBO);
        glDeleteBuffers(1, &_EBO);
    }

    unsigned int GetVAO() const
    {
        return _VAO;
    }

    const std::vector<unsigned int>& GetIndices() const {
        return _indices;
    }

    uint32_t GetMaterialIndex() const { return _materialIndex; }
    GLuint GetIndexCount() const { return _indices.size(); }

private:
    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        Stopwatch stopwatch("setupMesh");

        // create buffers/arrays
        glCreateVertexArrays(1, &_VAO);
        glCreateBuffers(1, &_VBO);
        glCreateBuffers(1, &_EBO);

        // load data into vertex buffers
        glNamedBufferStorage(_VBO, _vertices.size() * sizeof(Vertex), _vertices.data(), GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferStorage(_EBO, _indices.size() * sizeof(unsigned int), _indices.data(), GL_DYNAMIC_STORAGE_BIT);

        // bind EBO to VAO
        glVertexArrayElementBuffer(_VAO, _EBO);

        // bind VBO to VAO at binding index 0
        glVertexArrayVertexBuffer(_VAO, 0, _VBO, 0, sizeof(Vertex));

        // set the vertex attributes
        // vertex positions
        glEnableVertexArrayAttrib(_VAO, 0);
        glVertexArrayAttribFormat(_VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Position));
        glVertexArrayAttribBinding(_VAO, 0, 0);
        // vertex normals
        glEnableVertexArrayAttrib(_VAO, 1);
        glVertexArrayAttribFormat(_VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal));
        glVertexArrayAttribBinding(_VAO, 1, 0);
        // vertex texture coords
        glEnableVertexArrayAttrib(_VAO, 2);
        glVertexArrayAttribFormat(_VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords));
        glVertexArrayAttribBinding(_VAO, 2, 0);
        // vertex tangent
        glEnableVertexArrayAttrib(_VAO, 3);
        glVertexArrayAttribFormat(_VAO, 3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent));
        glVertexArrayAttribBinding(_VAO, 3, 0);

        stopwatch.Stop();

    }

    // render data 
    GLuint _VAO{0}, _VBO{0}, _EBO{0};

    // mesh Data
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    uint32_t _materialIndex;
};
