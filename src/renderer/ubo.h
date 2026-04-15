#pragma once

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
        : _direction{direction, 0.0f}, _colorAndIntensity{color, intensity} {}

    DirectionalLightBlockGPU& SetDirection(glm::vec3 direction) {_direction = {direction, 0.0f}; return *this; }
    DirectionalLightBlockGPU& SetColor(glm::vec3 color) { _colorAndIntensity = {color, _colorAndIntensity.a}; return *this; }
    DirectionalLightBlockGPU& SetIntensity(float intensity) { _colorAndIntensity.a = intensity; return *this; }

    // Returning vec4 as vec3 should strip w and return xyz
    glm::vec3 GetDirection() const { return _direction; }
    glm::vec3 GetColor() const { return _colorAndIntensity; }
    float GetIntensity() const { return _colorAndIntensity.w; }

private:
    glm::vec4 _direction;
    glm::vec4 _colorAndIntensity; // rgb, a = intensity
};


struct PointLightBlockGPU {
    explicit PointLightBlockGPU(glm::vec3 position, glm::vec3 color = {1.0f, 1.0f, 1.0f}, float intensity = 1.0f, float range = 10.0f)
        : _positionAndRange{glm::vec4(position, range)}, _colorAndIntensity{glm::vec4(color, intensity)} {}

    PointLightBlockGPU& SetPosition(glm::vec3 position) { _positionAndRange = {position, _positionAndRange.w}; return *this; }
    PointLightBlockGPU& SetRange(float range) { _positionAndRange.w = range; return *this; }
    PointLightBlockGPU& SetColor(glm::vec3 color) { _colorAndIntensity = {color, _colorAndIntensity.a}; return *this; }
    PointLightBlockGPU& SetIntensity(float intensity) { _colorAndIntensity.a = intensity; return *this; }

    glm::vec3 GetPosition() const { return _positionAndRange; }
    float GetRange() const { return _positionAndRange.w; }
    glm::vec3 GetColor() const { return _colorAndIntensity; }
    float GetIntensity() const { return _colorAndIntensity.w; }

private:
    // data aligned for std140
    glm::vec4 _positionAndRange;
    glm::vec4 _colorAndIntensity;
};


struct SpotLightBlockGPU {
    explicit SpotLightBlockGPU(glm::vec3 position, glm::vec3 direction, glm::vec3 color = {1.0f, 1.0f, 1.0f}, float intensity = 1.0f, float range = 10.0f, float inner = 12.5f, float outer = 15.0f)
        : _position{glm::vec4(position, 1.0f)}, _direction{glm::vec4(direction, 0.0f)}, _colorAndIntensity{glm::vec4(color, intensity)},
          _range(range), _innerCone{glm::cos(glm::radians(inner))}, _outerCone{glm::cos(glm::radians(outer))} {}

    SpotLightBlockGPU& SetPosition(glm::vec3 position) { _position = {position, 1.0f}; return *this; }
    SpotLightBlockGPU& SetDirection(glm::vec3 direction) { _direction = {direction, 0.0f}; return *this; }
    SpotLightBlockGPU& SetColor(glm::vec3 color) { _colorAndIntensity = {color, _colorAndIntensity.a}; return *this; }
    SpotLightBlockGPU& SetIntensity(float intensity) { _colorAndIntensity.a = intensity; return *this; }
    SpotLightBlockGPU& SetRange(float range) { _range = range; return *this; }
    SpotLightBlockGPU& SetCone(float inner, float outer) { _innerCone = glm::cos(glm::radians(inner)); _outerCone = glm::cos(glm::radians(outer)); return *this; } // input values in degrees, stored as cosine
 
    glm::vec3 GetPosition() const { return _position; }
    glm::vec3 GetDirection() const { return _direction; }
    glm::vec3 GetColor() const { return _colorAndIntensity; }
    float GetIntensity() const { return _colorAndIntensity.w; }
    float GetRange() const { return _range; }
    float GetInnerCone() const { return _innerCone; }
    float GetOuterCone() const { return glm::degrees(glm::acos(_outerCone)); }

private:
    glm::vec4 _position;
    glm::vec4 _direction;
    glm::vec4 _colorAndIntensity;
    float _range;
    float _innerCone;
    float _outerCone;
    float _padding{0.0f};
};

const int MAX_SPOT_SHADOW_CASTERS = 4; //TODO move
struct ShadowMapBlockGPU {
    glm::mat4 dirLightProjMatrix;
    glm::mat4 spotLightProjMatrices[MAX_SPOT_SHADOW_CASTERS];
};
