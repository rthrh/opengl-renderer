#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>

namespace math {

    // Light space matrix for directional light shadows
    glm::mat4 GetDirLightSpaceMatrix(const glm::vec3& lightDir, float nearPlane = 1.0f, float farPlane  = 50.0f, float frustumSize = 20.0f) {
        glm::mat4 lightProj = glm::ortho(-frustumSize, frustumSize, -frustumSize, frustumSize, nearPlane, farPlane);

        // If light points straight down or up, use the Z-axis as the Up vector
        glm::vec3 normDir = glm::normalize(lightDir);
        glm::vec3 up = (std::abs(normDir.y) > 0.999f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 lightPos = -lightDir * 20.0f;
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), up);
        //glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0)); //TODO

        return lightProj * lightView;
    }

    // Shadow matrices for point lights shadows
    std::array<glm::mat4, 6> GetPointShadowMatrices(glm::vec3 lightPos, float nearPlane = 0.1f, float farPlane = 25.0f) {

        constexpr float aspectRatio = 1.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspectRatio, nearPlane, farPlane);
        return {
            shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
    }

    // TODO check validity
    glm::mat4 GetSpotLightSpaceMatrix(glm::vec3 position, glm::vec3 direction, float outerConeAngleDeg, float nearPlane = 0.1f, float farPlane = 25.0f) {
        constexpr float aspect = 1.0f;
        glm::mat4 proj = glm::perspective(glm::radians(outerConeAngleDeg * 2.0f), aspect, nearPlane, farPlane);

        // If light points straight down or up, use the Z-axis as the Up vector
        glm::vec3 normDir = glm::normalize(direction);
        glm::vec3 up = (std::abs(normDir.y) > 0.999f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::lookAt(position, position + normDir, up);

        return proj * view;
    }
}
