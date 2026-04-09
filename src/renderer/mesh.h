#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "texture_cache.h"
#include "utils/stopwatch.h"
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal{};
    glm::vec2 TexCoords{};
    glm::vec3 Tangent{};
};


class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures = {})
        : _vertices(std::move(vertices)), _indices(std::move(indices)), _textures(std::move(textures))
    {
        setupMesh();
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& o) noexcept : _VAO(o._VAO), _VBO(o._VBO), _EBO(o._EBO),
                              _vertices(std::move(o._vertices)), _indices(std::move(o._indices)), _textures(std::move(o._textures)) {
        o._VAO = o._VBO = o._EBO = 0;
    }

    Mesh& operator=(Mesh&& o) noexcept {
        if (this == &o) return *this;
        glDeleteVertexArrays(1, &_VAO);
        glDeleteBuffers(1, &_VBO);
        glDeleteBuffers(1, &_EBO);

        _VAO = o._VAO; _VBO = o._VBO; _EBO = o._EBO;
        o._VAO = o._VBO = o._EBO = 0;
        _vertices = std::move(o._vertices);
        _indices  = std::move(o._indices);
        _textures = std::move(o._textures);
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

    const std::vector<Texture>& GetTextures() const {
        return _textures;
    }

    void SetTextures(const std::vector<Texture>& textures) {
        _textures = textures;
    }

    const std::vector<unsigned int>& GetIndices() const {
        return _indices;
    }


private:
    // render data 
    unsigned int _VAO{0}, _VBO{0}, _EBO{0};

    // mesh Data
    std::vector<Vertex>       _vertices;
    std::vector<unsigned int> _indices;
    std::vector<Texture>      _textures;

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
        glVertexArrayAttribFormat(_VAO, 2, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords));
        glVertexArrayAttribBinding(_VAO, 2, 0);
        // vertex tangent
        glEnableVertexArrayAttrib(_VAO, 3);
        glVertexArrayAttribFormat(_VAO, 3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent));
        glVertexArrayAttribBinding(_VAO, 3, 0);

        stopwatch.Stop();

    }
};
