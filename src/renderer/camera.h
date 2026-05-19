#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utils/stopwatch.h"

// aligned to std140
struct CameraUBO {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 position;  // w - unused
};


enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera
{
public:
    // constructor with vectors
    Camera(float aspectRatio, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f) :
        _position(position),
        _front(glm::vec3(0.0f, 0.0f, -1.0f)),
        _up(up),
        _worldUp(0.0f, 1.0f, 0.0f),
        _aspectRatio(aspectRatio),
        _yaw(yaw),
        _pitch(pitch),
        _movementSpeed(5.0f),
        _mouseSensitivity(0.1f),
        _zoom(45.0f)
    {
        this->updateCameraVectors();
        this->initUBO();
    }

    ~Camera() {
        if (_ubo) glDeleteBuffers(1, &_ubo);
    }

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) noexcept = default;
    Camera& operator=(Camera&&) noexcept = default;

    // Call every frame
    void UploadUBO() {
        Stopwatch Stopwatch("Camera::UploadUBO");
        CameraUBO data {
            .view = GetViewMatrix(),
            .projection = GetProjectionMatrix(),
            .position = glm::vec4(_position, 0.0f)
        };
        glNamedBufferSubData(_ubo, 0, sizeof(CameraUBO), &data);
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(_position, _position + _front, _up);
    }

    glm::mat4 GetProjectionMatrix() {
        return glm::perspective(glm::radians(this->_zoom), _aspectRatio, 0.1f, 100.0f);
    }

    float GetZoom() {
        return _zoom;
    }

    void SetAspectRatio(float aspectRatio) {
        _aspectRatio = aspectRatio;
    }

    void ProcessKeyboard(CameraMovement direction, float deltaTime)
    {
        float velocity = _movementSpeed * deltaTime;
        if (direction == FORWARD)
            _position += _front * velocity;
        if (direction == BACKWARD)
            _position -= _front * velocity;
        if (direction == LEFT)
            _position -= _right * velocity;
        if (direction == RIGHT)
            _position += _right * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= _mouseSensitivity;
        yoffset *= _mouseSensitivity;

        _yaw   += xoffset;
        _pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (_pitch > 89.0f)
                _pitch = 89.0f;
            if (_pitch < -89.0f)
                _pitch = -89.0f;
        }

        // update _front, _right and _up Vectors using the updated Euler angles
        this->updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        _zoom -= (float)yoffset;
        if (_zoom < 1.0f)
            _zoom = 1.0f;
        if (_zoom > 45.0f)
            _zoom = 45.0f;
    }

    auto& GetPosition() {
        return _position;
    }

private:
    void initUBO() {
        glCreateBuffers(1, &_ubo);
        glNamedBufferData(_ubo, sizeof(CameraUBO), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, _ubo); // binding point 3
    }

    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new _front vector
        glm::vec3 front;
        front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        front.y = sin(glm::radians(_pitch));
        front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        _front = glm::normalize(front);
        // also re-calculate the _right and _up vector
        _right = glm::normalize(glm::cross(_front, _worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        _up    = glm::normalize(glm::cross(_right, _front));
    }

    // camera Attributes
    glm::vec3 _position;
    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _worldUp;
    glm::vec3 _right;
    float _aspectRatio;
    // euler Angles
    float _yaw;
    float _pitch;
    // camera options
    float _movementSpeed;
    float _mouseSensitivity;
    float _zoom;
    GLuint _ubo;
};
