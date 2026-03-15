#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "shader.h"
#include "texture_cache.h"




struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal{};
    glm::vec2 TexCoords{};
    glm::vec3 Tangent{};
};



class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    {
        this->vertices = std::move(vertices);
        this->indices = std::move(indices);
        this->textures = std::move(textures);

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    {
        this->vertices = std::move(vertices);
        this->indices = std::move(indices);
        setupMesh();
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& o) noexcept : m_VAO(o.m_VAO), m_VBO(o.m_VBO), m_EBO(o.m_EBO),
                              vertices(std::move(o.vertices)), indices(std::move(o.indices)), textures(std::move(o.textures)) {
        o.m_VAO = o.m_VBO = o.m_EBO = 0;
    }

    Mesh& operator=(Mesh&& o) noexcept {
        if (this == &o) return *this;
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);

        m_VAO = o.m_VAO; m_VBO = o.m_VBO; m_EBO = o.m_EBO;
        o.m_VAO = o.m_VBO = o.m_EBO = 0;
        vertices = std::move(o.vertices);
        indices  = std::move(o.indices);
        textures = std::move(o.textures);
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
        return textures;
    }

    const std::vector<unsigned int>& GetIndices() const {
        return indices;
    }

private:
    // render data 
    unsigned int m_VAO{0}, m_VBO{0}, m_EBO{0};

    // mesh Data
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

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
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    }
};
