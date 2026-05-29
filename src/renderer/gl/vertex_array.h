#pragma once

#include <gl/headers.h>
#include <utility>

#include "gl/dsa_config.h"

class VertexArray {
public:
    VertexArray() {
        #ifdef USE_GL_DSA
            glCreateVertexArrays(1, &_id);
        #else
            glGenVertexArrays(1, &_id);
        #endif
    }

    ~VertexArray() {
        glDeleteVertexArrays(1, &_id);
    }

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& o) noexcept
        : _id(std::exchange(o._id, 0)),
          _vbo(std::exchange(o._vbo, 0)),
          _stride(std::exchange(o._stride, 0)),
          _divisor(std::exchange(o._divisor, 0)) {}

    VertexArray& operator=(VertexArray&& o) noexcept {
        if (this != &o) {
            glDeleteVertexArrays(1, &_id);
            _id = std::exchange(o._id, 0);
            _vbo = std::exchange(o._vbo, 0);
            _stride = std::exchange(o._stride, 0);
            _divisor = std::exchange(o._divisor, 0);
        }
        return *this;
    }

    GLuint GetID() const { return _id; }

    void SetBindingDivisor(GLuint bindingIndex, GLuint divisor) {
        #ifdef USE_GL_DSA
            glVertexArrayBindingDivisor(_id, bindingIndex, divisor);
        #else
            _divisor = divisor;
        #endif
    }

    void AddAttribute(GLuint bindingIndex, GLuint attribIndex, GLint numValues, GLuint offsetBytes, GLenum type = GL_FLOAT, GLboolean normalized = GL_FALSE) {
        #ifdef USE_GL_DSA
            glEnableVertexArrayAttrib(_id, attribIndex);
            glVertexArrayAttribFormat(_id, attribIndex, numValues, type, normalized, offsetBytes);
            glVertexArrayAttribBinding(_id, attribIndex, bindingIndex);
        #else
            if (_vbo == 0) throw std::runtime_error("AddAttribute called before BindVertexBuffer");
            glBindVertexArray(_id);
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glEnableVertexAttribArray(attribIndex);
            glVertexAttribPointer(attribIndex, numValues, type, normalized, _stride, reinterpret_cast<const void*>(offsetBytes));

            if (_divisor) {
                glVertexAttribDivisor(attribIndex, _divisor);
            }
            glBindVertexArray(0);
        #endif
    }

    void Bind() {
        glBindVertexArray(_id);
    }

    void BindVertexBuffer(GLuint vbo, GLuint bindingIndex, GLint offset, GLsizei stride) {
        #ifdef USE_GL_DSA
            glVertexArrayVertexBuffer(_id, bindingIndex, vbo, offset, stride);
        #else
            _vbo = vbo;
            _stride = stride;
            _divisor = 0; // reset divisor after bind //TODO needed?
        #endif
    }

    void BindElementBuffer(GLuint ebo) {
        #ifdef USE_GL_DSA
            glVertexArrayElementBuffer(_id, ebo);
        #else
            glBindVertexArray(_id);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBindVertexArray(0);
        #endif
    }

private:
    GLuint _id = 0;

    // Non-DSA path separates setting attributes into multiple steps, so
    // these variables need to be kept for later
    GLuint _vbo = 0;
    GLsizei _stride = 0;
    GLuint _divisor = 0;
};
