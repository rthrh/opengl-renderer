#pragma once

#include <gl/headers.h>

#include <GLFW/glfw3.h>
#include <stdexcept>

#include "utils/logger.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/html5_webgl.h>
#endif

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);

class Window {
public:
    Window(int width, int height, const char* name) {
        // Initialize and configure
        glfwInit();
        #ifdef __EMSCRIPTEN__
            // WebGL2 uses OpenGL ES 3.0
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        #else
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        #endif
        // Window creation
        //glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // maximize _window
        _window = glfwCreateWindow(width, height, name, NULL, NULL);
        if (_window == NULL) {
            std::cout << "Failed to create GLFW _window" << std::endl;
            glfwTerminate();
        }

        glfwMakeContextCurrent(_window);



        // glad: load all OpenGL function pointers
        #ifndef __EMSCRIPTEN__
            // Disable mouse cursor
            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
                throw std::runtime_error("Failed to initialize GLAD");
            }
            this->setupDebugContex();
        #else
            EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_get_current_context();
            emscripten_webgl_enable_extension(ctx, "EXT_color_buffer_float");
            emscripten_webgl_enable_extension(ctx, "OES_texture_float_linear");
        #endif

        glfwSwapInterval(1);
        //glfwSwapInterval(0); // Disable vsync
    }

    ~Window() {
        if (_window) glfwDestroyWindow(_window);
        glfwTerminate();
    }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept = delete;
    Window& operator=(Window&&) noexcept = delete;

    GLFWwindow* GetHandle() const { return _window; }
    bool ShouldClose() const { return glfwWindowShouldClose(_window); }
    void SwapBuffers() const { glfwSwapBuffers(_window); }

private:
    void setupDebugContex() {
        #ifndef __EMSCRIPTEN__
            // Init debug context and callback
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
            if (glfwExtensionSupported("GL_KHR_debug")) {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(glDebugOutput, nullptr);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
                Info("KHR_debug enabled");
            }
        #endif
    }

    GLFWwindow* _window = nullptr;
};

// Debug output callback
void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            unsigned int id,
                            GLenum severity,
                            GLsizei length,
                            const char *message,
                            const void *userParam)
{
#ifndef __EMSCRIPTEN__
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " <<  message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
#endif
}
