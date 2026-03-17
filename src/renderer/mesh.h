#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "texture_cache.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal{};
    glm::vec2 TexCoords{};
    glm::vec3 Tangent{};
};


class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures = {})
        : m_vertices(std::move(vertices)), m_indices(std::move(indices)), m_textures(std::move(textures))
    {
        setupMesh();
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& o) noexcept : m_VAO(o.m_VAO), m_VBO(o.m_VBO), m_EBO(o.m_EBO),
                              m_vertices(std::move(o.m_vertices)), m_indices(std::move(o.m_indices)), m_textures(std::move(o.m_textures)) {
        o.m_VAO = o.m_VBO = o.m_EBO = 0;
    }

    Mesh& operator=(Mesh&& o) noexcept {
        if (this == &o) return *this;
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);

        m_VAO = o.m_VAO; m_VBO = o.m_VBO; m_EBO = o.m_EBO;
        o.m_VAO = o.m_VBO = o.m_EBO = 0;
        m_vertices = std::move(o.m_vertices);
        m_indices  = std::move(o.m_indices);
        m_textures = std::move(o.m_textures);
        return *this;
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    unsigned int GetVAO() const
    {
        return m_VAO;
    }

    const std::vector<Texture>& GetTextures() const {
        return m_textures;
    }

    void SetTextures(const std::vector<Texture>& textures) {
        m_textures = textures;
    }

    const std::vector<unsigned int>& GetIndices() const {
        return m_indices;
    }


private:
    // render data 
    unsigned int m_VAO{0}, m_VBO{0}, m_EBO{0};

    // mesh Data
    std::vector<Vertex>       m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<Texture>      m_textures;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // unbind VAO
        glBindVertexArray(0);

    }
};
