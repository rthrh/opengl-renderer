#pragma once

#ifdef USE_GL_DSA
    inline constexpr bool USE_DSA = true;
#else
    inline constexpr bool USE_DSA = false;
#endif
