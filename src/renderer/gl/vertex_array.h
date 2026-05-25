#pragma once

#include <glad/glad.h>
#include <utility>

class VertexArray {
public:
    VertexArray()
    {
        glCreateVertexArrays(1, &_id);
    }

    ~VertexArray() {
        glDeleteVertexArrays(1, &_id);
    }

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& o) noexcept : _id(std::exchange(o._id, 0)) {}

    VertexArray& operator=(VertexArray&& o) noexcept {
        if (this != &o) {
            glDeleteVertexArrays(1, &_id);
            _id = std::exchange(o._id, 0);
        }
        return *this;
    }

    GLuint GetID() const { return _id; }

    void SetBindingDivisor(GLuint bindingIndex, GLuint divisor) {
        glVertexArrayBindingDivisor(_id, bindingIndex, divisor);
    }

    void AddAttribute(GLuint bindingIndex, GLuint attribIndex, GLint numValues, GLuint offsetBytes, GLenum type = GL_FLOAT, GLboolean normalized = GL_FALSE) {
        glEnableVertexArrayAttrib(_id, attribIndex);
        glVertexArrayAttribFormat(_id, attribIndex, numValues, type, normalized, offsetBytes);
        glVertexArrayAttribBinding(_id, attribIndex, bindingIndex);
    }

    void BindVertexBuffer(GLuint vbo, GLuint bindingIndex, GLint offset, GLsizei stride) {
        glVertexArrayVertexBuffer(_id, bindingIndex, vbo, offset, stride);
    }

    void BindElementBuffer(GLuint ebo) {
        glVertexArrayElementBuffer(_id, ebo);
    }

private:
    GLuint _id = 0;
};
