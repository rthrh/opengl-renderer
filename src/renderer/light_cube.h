#include <vector>

#include "mesh.h"

std::vector<Vertex> cube_vertices {
    // Back face
    {{-0.5f, -0.5f, -0.5f}},
    {{ 0.5f, -0.5f, -0.5f}},
    {{ 0.5f,  0.5f, -0.5f}},
    {{-0.5f,  0.5f, -0.5f}},

    // Front face
    {{-0.5f, -0.5f,  0.5f}},
    {{ 0.5f, -0.5f,  0.5f}},
    {{ 0.5f,  0.5f,  0.5f}},
    {{-0.5f,  0.5f,  0.5f}}
};

std::vector<unsigned int> cube_indices {
    0, 1, 2,
    2, 3, 0,

    4, 5, 6,
    6, 7, 4,

    4, 7, 3,
    3, 0, 4,

    1, 5, 6,
    6, 2, 1,

    4, 5, 1,
    1, 0, 4,

    3, 2, 6,
    6, 7, 3
};

inline std::vector<Vertex> floor_vertices = {
    { {-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
    { { 1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
    { { 1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
    { {-1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
};

inline std::vector<unsigned int> floor_indices = {
    0, 1, 2,
    2, 3, 0
};