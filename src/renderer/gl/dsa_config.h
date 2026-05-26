#pragma once
#include <gl/headers.h>

#ifdef USE_GL_DSA
    inline constexpr bool USE_DSA = true;
#else
    inline constexpr bool USE_DSA = false;
#endif

namespace GL {
inline void BindTextureUnit(GLuint unit, GLuint texture) {
    if constexpr (USE_DSA) {
        glBindTextureUnit(unit, texture);
    } else {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
    }
}
} // GL
