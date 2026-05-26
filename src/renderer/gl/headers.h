#pragma once

#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h>
    #include <GLES2/gl2ext.h>
    #include <GLES2/gl2ext.h>
    #define GL_TEXTURE_BORDER_COLOR GL_TEXTURE_BORDER_COLOR_EXT
#else
    #include <glad/glad.h>
#endif
