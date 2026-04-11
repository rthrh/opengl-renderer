#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct CountBlockGPU {
    explicit CountBlockGPU (int count) : m_count(count) {}

private:
    glm::ivec4 m_count;
};

struct DirectionalLightBlockGPU {
    explicit DirectionalLightBlockGPU(glm::vec3 direction, glm::vec3 color = {1.0f, 1.0f, 1.0f}, float intensity = 1.0f)
        : m_direction{direction, 0.0f}, m_colorAndIntensity{color, intensity} {}

    DirectionalLightBlockGPU& SetDirection(glm::vec3 direction) {m_direction = {direction, 0.0f}; return *this; }
    DirectionalLightBlockGPU& SetColor(glm::vec3 color) { m_colorAndIntensity = {color, m_colorAndIntensity.a}; return *this; }
    DirectionalLightBlockGPU& SetIntensity(float intensity) { m_colorAndIntensity.a = intensity; return *this; }

private:
    glm::vec4 m_direction;
    glm::vec4 m_colorAndIntensity; // rgb, a = intensity
};


struct PointLightBlockGPU {
    explicit PointLightBlockGPU(glm::vec3 position, glm::vec3 color = {1.0f, 1.0f, 1.0f}, float intensity = 1.0f, float range = 10.0f)
        : m_positionAndRange{glm::vec4(position, range)}, m_colorAndIntensity{glm::vec4(color, intensity)} {}

    PointLightBlockGPU& SetPosition(glm::vec3 position) { m_positionAndRange = {position, m_positionAndRange.w}; return *this; }
    PointLightBlockGPU& SetRange(float range) { m_positionAndRange.w = range; return *this; }
    PointLightBlockGPU& SetColor(glm::vec3 color) { m_colorAndIntensity = {color, m_colorAndIntensity.a}; return *this; }
    PointLightBlockGPU& SetIntensity(float intensity) { m_colorAndIntensity.a = intensity; return *this; }

private:
    // data aligned for std140
    glm::vec4 m_positionAndRange;
    glm::vec4 m_colorAndIntensity;
};


struct SpotLightBlockGPU {
    explicit SpotLightBlockGPU(glm::vec3 position, glm::vec3 direction, glm::vec3 color = {1.0f, 1.0f, 1.0f}, float intensity = 1.0f, float range = 10.0f, float inner = 12.5f, float outer = 15.0f)
        : m_position{glm::vec4(position, 1.0f)}, m_direction{glm::vec4(direction, 0.0f)}, m_colorAndIntensity{glm::vec4(color, intensity)},
          m_range(range), m_innerCone{glm::cos(glm::radians(inner))}, m_outerCone{glm::cos(glm::radians(outer))} {}

    SpotLightBlockGPU& SetPosition(glm::vec3 position) { m_position = {position, 1.0f}; return *this; }
    SpotLightBlockGPU& SetDirection(glm::vec3 direction) {m_direction = {direction, 0.0f}; return *this; }
    SpotLightBlockGPU& SetColor(glm::vec3 color) { m_colorAndIntensity = {color, m_colorAndIntensity.a}; return *this; }
    SpotLightBlockGPU& SetIntensity(float intensity) { m_colorAndIntensity.a = intensity; return *this; }
    SpotLightBlockGPU& SetRange(float range) { m_range = range; return *this; }
    SpotLightBlockGPU& SetCone(float inner, float outer) { m_innerCone = glm::cos(glm::radians(inner)); m_outerCone = glm::cos(glm::radians(outer)); return *this; } // input values in degrees, stored as cosine

private:
    glm::vec4 m_position;
    glm::vec4 m_direction;
    glm::vec4 m_colorAndIntensity;
    float m_range;
    float m_innerCone;
    float m_outerCone;
    float m_padding_{0.0f};
};


