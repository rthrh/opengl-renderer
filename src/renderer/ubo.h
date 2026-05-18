#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <cassert>

// All UBOs field sizes must exactly match to ubo.glsl !
// Data must be aligned to 16 bits

struct DirectionalLightUBO {
    DirectionalLightUBO() = default;
    DirectionalLightUBO(glm::vec3 direction, float intensity = 1.0f)
        : _direction{direction, 0.0f}, _colorAndIntensity{1.0f, 1.0f, 1.0f, intensity} {}

    DirectionalLightUBO& SetDirection(glm::vec3 direction) {_direction = {direction, 0.0f}; return *this; }
    DirectionalLightUBO& SetColor(uint8_t r, uint8_t g, uint8_t b) { _colorAndIntensity = {r / 255.0f, g / 255.0f, b / 255.0f, _colorAndIntensity.a}; return *this; }
    DirectionalLightUBO& SetIntensity(float intensity) { _colorAndIntensity.a = intensity; return *this; }

    glm::vec3 GetDirection() const { return glm::vec3(_direction); }
    glm::vec3 GetColor() const { return glm::vec3(_colorAndIntensity); }
    float GetIntensity() const { return _colorAndIntensity.w; }

private:
    glm::vec4 _direction;
    glm::vec4 _colorAndIntensity; // rgb, a = intensity
};


struct PointLightBlockGPU {
    PointLightBlockGPU() = default;
    PointLightBlockGPU(glm::vec3 position, float intensity = 1.0f, float range = 10.0f)
        : _positionAndRange{glm::vec4(position, range)}, _colorAndIntensity{1.0f, 1.0f, 1.0f, intensity} {}

    PointLightBlockGPU& SetPosition(glm::vec3 position) { _positionAndRange = {position, _positionAndRange.w}; return *this; }
    PointLightBlockGPU& SetRange(float range) { _positionAndRange.w = range; return *this; }
    PointLightBlockGPU& SetColor(uint8_t r, uint8_t g, uint8_t b) { _colorAndIntensity = {r / 255.0f, g / 255.0f, b / 255.0f, _colorAndIntensity.a}; return *this; }
    PointLightBlockGPU& SetIntensity(float intensity) { _colorAndIntensity.a = intensity; return *this; }

    glm::vec3 GetPosition() const { return glm::vec3(_positionAndRange); }
    float GetRange() const { return _positionAndRange.w; }
    glm::vec3 GetColor() const { return glm::vec3(_colorAndIntensity); }
    float GetIntensity() const { return _colorAndIntensity.w; }

private:
    // data aligned for std140
    glm::vec4 _positionAndRange;
    glm::vec4 _colorAndIntensity;
};


constexpr int MAX_POINT_LIGHTS = 1024;
struct PointLightUBO {
    glm::ivec4 count{0}; // x = count, yzw = padding
    PointLightBlockGPU lights[MAX_POINT_LIGHTS];

    void Pushback(const PointLightBlockGPU& light) {
        assert(count.x < MAX_POINT_LIGHTS);
        lights[count.x++] = light;
    }

    void Remove(int index) {
        int last = count.x - 1;
        if (index != last)
            lights[index] = lights[last];
        lights[last] = PointLightBlockGPU{};
        count.x--;
    }

    PointLightBlockGPU& At(int index) {
        assert(index < count.x);
        return lights[index];
    }

    const PointLightBlockGPU& At(int index) const {
        assert(index < count.x);
        return lights[index];
    }

    int Count() const { return count.x; }
};

struct SpotLightBlockGPU {
    SpotLightBlockGPU() = default;
    SpotLightBlockGPU(glm::vec3 position, glm::vec3 direction, float intensity = 1.0f, float range = 10.0f, float inner = 12.5f, float outer = 15.0f)
        : _position{glm::vec4(position, 1.0f)}, _direction{glm::vec4(direction, 0.0f)}, _colorAndIntensity{1.0f, 1.0f, 1.0f, intensity},
          _range(range), _innerCone{glm::cos(glm::radians(inner))}, _outerCone{glm::cos(glm::radians(outer))} {}

    SpotLightBlockGPU& SetPosition(glm::vec3 position) { _position = {position, 1.0f}; return *this; }
    SpotLightBlockGPU& SetDirection(glm::vec3 direction) { _direction = {direction, 0.0f}; return *this; }
    SpotLightBlockGPU& SetColor(uint8_t r, uint8_t g, uint8_t b) { _colorAndIntensity = {r / 255.0f, g / 255.0f, b / 255.0f, _colorAndIntensity.a}; return *this; }
    SpotLightBlockGPU& SetIntensity(float intensity) { _colorAndIntensity.a = intensity; return *this; }
    SpotLightBlockGPU& SetRange(float range) { _range = range; return *this; }
    SpotLightBlockGPU& SetCone(float inner, float outer) { _innerCone = glm::cos(glm::radians(inner)); _outerCone = glm::cos(glm::radians(outer)); return *this; } // input values in degrees, stored as cosine

    glm::vec3 GetPosition() const { return glm::vec3(_position); }
    glm::vec3 GetDirection() const { return glm::vec3(_direction); }
    glm::vec3 GetColor() const { return glm::vec3(_colorAndIntensity); }
    float GetIntensity() const { return _colorAndIntensity.w; }
    float GetRange() const { return _range; }
    float GetInnerConeDegrees() const { return glm::degrees(glm::acos(_innerCone)); }
    float GetOuterConeDegrees() const { return glm::degrees(glm::acos(_outerCone)); }

private:
    glm::vec4 _position;
    glm::vec4 _direction;
    glm::vec4 _colorAndIntensity;
    float _range;
    float _innerCone;
    float _outerCone;
    float _padding{0.0f};
};

constexpr int MAX_SPOT_LIGHTS = 1024;
struct SpotLightUBO {
    glm::ivec4 count{0}; // x = count, yzw = padding
    SpotLightBlockGPU lights[MAX_SPOT_LIGHTS];

    void Pushback(const SpotLightBlockGPU& light) {
        assert(count.x < MAX_SPOT_LIGHTS);
        lights[count.x++] = light;
    }

    void Remove(int index) {
        int last = count.x - 1;
        if (index != last)
            lights[index] = lights[last];
        lights[last] = SpotLightBlockGPU{};
        count.x--;
    }

    SpotLightBlockGPU& At(int index) {
        assert(index < count.x);
        return lights[index];
    }

    int Count() const { return count.x; }
};

constexpr int MAX_SPOT_SHADOW_CASTERS = 4; //TODO move
struct ShadowMapUBO {
    glm::mat4 dirLightProjMatrix;
    glm::mat4 spotLightProjMatrices[MAX_SPOT_SHADOW_CASTERS];
};

struct ConfigUBO {
    // Bloom + tonemapping
    int bloomEnabled = true;
    float exposure = 1.0f;
    float gamma = 2.2f;
    float bloomStrength = 0.04f;
    float filterRadius = 0.005f;

    // SSAO
    int ssaoEnabled = true;
    float ssaoRadius = 0.5f;
    float ssaoBias = 0.025f;
    int ssaoKernel = 64;

    // Shadows
    int shadowsEnabled = true; // for CPU
    int maxPointShadowCasters = 4;
    float pointShadowBias = 0.05;
    float dirShadowBiasMin = 0.005;
    float dirShadowBiasMax = 0.05;

    int maxSpotShadowCasers = 4;
    float spotShadowBiasMin = 0.0005;
    float spotShadowBiasMax = 0.005;

    // IBL
    float maxReflectionLOD = 4.0f;
    float _pad0;
    float _pad1;
    float _pad2;
};
