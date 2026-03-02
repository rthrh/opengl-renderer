#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


struct DirectionalLightBlockGPU {
    DirectionalLightBlockGPU(glm::vec3 direction, glm::vec3 color = {1.0f, 1.0f, 1.0f}, float intensity = 1.0f) : 
    m_direction{direction, 0.0f},
    m_colorAndIntensity{color, intensity} {}
private:
    glm::vec4 m_direction;
    glm::vec4 m_colorAndIntensity; // rgb, a = intensity
};



struct PointLightBlockGPU {
    public:
        PointLightBlockGPU(glm::vec3 position, glm::vec3 color = {1.0f, 1.0f, 1.0f}, float intensity = 1.0f, float range = 10.0f, float constant = 1.0f, float linear = 0.09f, float quadratic = 0.032f) :
            m_position{glm::vec4(position, 0.0f)},
            m_colorAndIntensity{glm::vec4(color, intensity)},
            m_range{range},
            m_constant{constant},
            m_linear{linear},
            m_quadratic{quadratic}
        {
        }
    private:
        // data aligned for std120
        glm::vec4 m_position;
        glm::vec4 m_colorAndIntensity;
        float m_range{10.0f}; // cut off range
        float m_constant{1.0f};
        float m_linear{0.09f};
        float m_quadratic{0.032f};    
};

struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f}; // normalized
    glm::vec3 color{1.0f};
    float intensity{1.0f};

    float range{15.0f};      // Effective radius
    float innerCone{0.9f};   // Cosine of inner angle
    float outerCone{0.85f};  // Cosine of outer angle
};

struct SpotLightBlockGPU {
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 colorAndIntensity;
    float range;
    float innerCone;
    float outerCone;
    float padding_{0.0f};
};