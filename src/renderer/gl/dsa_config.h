#pragma once
#include <gl/headers.h>

namespace GL {
inline void BindTextureUnit(GLuint unit, GLuint texture) {
    #ifdef USE_GL_DSA
        glBindTextureUnit(unit, texture);
    #else
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
    #endif
}
} // GL
