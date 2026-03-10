#include <glad/glad.h>
#include <glm/glm.hpp>

enum class MaterialType{
    Albedo,
    Normal,
    Metallic,
    Roughness,
    Ao, // ambient occlusion
};


struct Material {

};

struct alignas(16) MaterialUBO {
    glm::vec4 albedoFactor;
    float     metallicFactor;
    float     roughnessFactor;
    float     aoStrength;
    float     _pad;
};