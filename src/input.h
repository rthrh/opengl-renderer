#pragma once

#include <functional>
#include <map>
#include <stdexcept>
#include <format>
#include <GLFW/glfw3.h>

enum class Action
{
    CameraMoveForward,    // Move camera forward
    CameraMoveBackward,   // Move camera backward
    CameraMoveLeft,       // Strafe left
    CameraMoveRight,      // Strafe right
    CameraMoveUp,         // Camera up
    CameraMoveDown,       // Camera Down
    CameraTurnLeft,       // Yaw left
    CameraTurnRight,      // Yaw right
    CameraLookUp,         // Pitch up
    CameraLookDown,       // Pitch down
    CameraZoomIn,         // Zoom in
    CameraZoomOut,        // Zoom out
    UiMode,
    WireframeMode,
};


class InputHandler {
    public:
        InputHandler(GLFWwindow* window) : m_window(window)
        
        {
            glfwSetWindowUserPointer(window, this);

            glfwSetKeyCallback(window, pressKeyCallback);
        }

        void bindKey(Action action, int key) {
            
        }

        // for event like options toggling
        void setPressKeyCallback(const Action action, const std::function<void()>& callback) {
            m_pressKeyCallbacks.insert({action, callback});
        }

        // for events checked every frame like camera movement
        void setHoldKeyCallback(const Action action, std::function<void()> callback) {
            m_holdKeyCallbacks.insert({action, std::move(callback)});

        }

        void setCursorCallback(std::function<void(double, double)> callback) {
            glfwSetCursorPosCallback(m_window, cursorCallback);
            m_cursorCallback = callback;
        }

        void setScrollCallback(std::function<void(double, double)> callback)  {
            glfwSetScrollCallback(m_window, scrollCallback);
            m_scrollCallback = callback;
        }

        void execute(const Action action) {
            if (m_pressKeyCallbacks.find(action) != m_pressKeyCallbacks.end()) {
                auto callback = m_pressKeyCallbacks[action];
                callback();
            } else {
                auto message = std::format("Action callback for {} not found", static_cast<int>(action));
            }
            
        }

        // call on each rendering frame
        void process() {
            for (const auto& [action, callback] : m_holdKeyCallbacks) {
                auto key = m_holdKeyBinds[action];
                if (glfwGetKey(m_window, key) == GLFW_PRESS) {
                    m_holdKeyCallbacks[action]();
                }
            }
        }

    private:
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
            auto* self = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
            self->m_scrollCallback(xoffset, yoffset);
        }

        static void cursorCallback(GLFWwindow* window, double xoffset, double yoffset) {
            auto* self = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
            self->m_cursorCallback(xoffset, yoffset);
        }

        static void pressKeyCallback(GLFWwindow* window, int pressedKey, int scancode, int action, int mods) {
            auto* self = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
            for (const auto& [boundAction, boundKey] : self->m_pressKeyBinds) {
                if (pressedKey == boundKey && action == GLFW_PRESS) {
                        self->m_pressKeyCallbacks[boundAction]();
                }
            }
        }

        GLFWwindow* m_window;
        std::map<Action, std::function<void()>> m_pressKeyCallbacks;
        std::map<Action, std::function<void()>> m_holdKeyCallbacks;
        std::map<Action, int> m_pressKeyBinds;
        std::map<Action, int> m_holdKeyBinds;

        std::function<void(double, double)> m_cursorCallback;
        std::function<void(double, double)> m_scrollCallback;
};