#pragma once

#include <gl/headers.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <functional>
#include <random>

#include "renderer/camera.h"
#include "renderer/shader.h"
#include "renderer/renderer.h"
#include "renderer/scene.h"
#include "renderer/asset_cache.h"
#include "renderer/shader_cache.h"
#include "renderer/model_loader.h"

#include "gui/gui.h"
#include "utils/file_watcher.h"
#include "demo.h"
#include "window.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

struct AppCallbackData {
    std::shared_ptr<Camera> cameraPtr;
    Renderer* rendererPtr;
    bool uiMode = false;

    // Mouse callback
    float mouseLastX; //{ SCR_WIDTH / 2.0f };
    float mouseLastY; //{ SCR_HEIGHT / 2.0f };
    bool firstMouse = true;

    // Resize callback
    int newScrWidth;
    int newScrHeight;
};


class App {
public:
    App(int windowWidth, int windowHeight) :
        _window(windowWidth, windowHeight, "opengl-renderer-demo"),
        _assetCache(std::make_shared<AssetCache>()),
        _meshCache(std::make_shared<MeshCache>(_assetCache)),
        _modelLoader(_assetCache, _meshCache)
    {
        // Input callbacks
        glfwSetFramebufferSizeCallback(_window.GetHandle(), framebuffer_size_callback);
        glfwSetCursorPosCallback(_window.GetHandle(), mouse_callback);
        glfwSetScrollCallback(_window.GetHandle(), scroll_callback);
        glfwSetKeyCallback(_window.GetHandle(), key_callback);
        // build and compile shaders
        std::filesystem::path root = PROJECT_SOURCE_DIR;
        std::filesystem::path pathShaders = root / "src/shaders";
        _shaderCache.SetPostBuildHook(InitBindings);
        _shaderCache.LoadDirectory(pathShaders);

        // Set up shader file watcher
        auto fileCallback = [this](const std::filesystem::path&) { _shaderCache.ReloadAll();};
        _fileWatcher.WatchDirectory(pathShaders, fileCallback);

        float aspectRatio = (float)windowWidth / (float)windowHeight;
        _camera = std::make_shared<Camera>(aspectRatio, glm::vec3(0.0f, 0.0f, 3.0f));

        _renderer = std::make_unique<Renderer>(windowWidth, windowHeight, _camera, _assetCache, _meshCache, _shaderCache);

        //std::filesystem::path skyboxPath = root / "resources" / "newport_loft.hdr";
        std::filesystem::path skyboxPath = root / "resources" / "rogland_clear_night_1k.exr";
        _renderer->LoadSkybox(skyboxPath);
        // App context data for callbacks
        _callbackData = {.cameraPtr{_camera},
                         .rendererPtr{_renderer.get()},
                         .mouseLastX = windowWidth / 2.0f,
                         .mouseLastY = windowHeight / 2.0f };
        glfwSetWindowUserPointer(_window.GetHandle(), &_callbackData);

        // Init imgui
        std::filesystem::path modelsDirectory = root / ".." / "glTF-Sample-Models/2.0";
        _guiLayer = std::make_unique<GuiLayer>(_window.GetHandle(), modelsDirectory, _assetCache);
        // Scene setup
        //_scene = std::make_unique<Scene>(setupTestModels(_assetCache, _modelLoader));
        //_scene = std::make_unique<Scene>(setupSponza(_assetCache, _modelLoader));
        _scene = std::make_unique<Scene>(setupLocal(_assetCache, _modelLoader));

    }

    // Render loop
    void Run() {
        #ifdef __EMSCRIPTEN__
            emscripten_set_main_loop_arg(
                [](void* self) { static_cast<App*>(self)->Frame(); },
                this,
                10,   // 0 = use requestAnimationFrame
                1    // simulate infinite loop
            );
        #else
            while (!_window.ShouldClose()) {
                Frame();
            }
        #endif
    }

    void Frame() {
        Stopwatch stopwatch("Render loop");

        // poll events
        glfwPollEvents();

        #ifndef __EMSCRIPTEN__
            _fileWatcher.Update();
        #endif

        // per-frame time logic
        auto currentFrame = static_cast<float>(glfwGetTime());
        _deltaTime = currentFrame - _lastFrame;
        _lastFrame = currentFrame;

        // Begin frame
        _guiLayer->BeginFrame();

        // Handle input
        if (!_guiLayer->WantCaptureMouse()) { }
        if (!_guiLayer->WantCaptureKeyboard()) {
            processInput(_window.GetHandle(), _camera);
        }

        // Create GUI
        _guiLayer->Build(_renderer->GetConfig(), *_scene, _deltaTime, _modelLoader);

        // Render scene
        _renderer->RenderFrame(*_scene);

        // Renders the ImGUI elements
        _guiLayer->EndFrame();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        Stopwatch stopwatch2("glfwSwapBuffers(_window);");
        _window.SwapBuffers();

    }

private:
    // process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
    void processInput(GLFWwindow *window, const std::shared_ptr<Camera>& camera)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            _camera->ProcessKeyboard(FORWARD, _deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            _camera->ProcessKeyboard(BACKWARD, _deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            _camera->ProcessKeyboard(LEFT, _deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            _camera->ProcessKeyboard(RIGHT, _deltaTime);
    }

    inline static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* data = static_cast<AppCallbackData*>(glfwGetWindowUserPointer(window));
        auto& uiMode = data->uiMode;
        if (key == GLFW_KEY_G && action == GLFW_PRESS) {
            uiMode = !uiMode;
            auto cursor_mode = uiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
            glfwSetInputMode(window, GLFW_CURSOR, cursor_mode);

            GuiLayer::SetMouseEnabled(uiMode);
        }
    }

    // glfw: whenever the _window size changed (by OS or user resize) this callback function executes
    inline static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        // make sure the viewport matches the new _window dimensions; note that width and
        // height will be significantly larger than specified on retina displays.
        auto* data = static_cast<AppCallbackData*>(glfwGetWindowUserPointer(window));
        data->rendererPtr->Resize(width, height);
        data->cameraPtr->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
        glViewport(0, 0, width, height);
    }

    // glfw: whenever the mouse moves, this callback is called
    inline static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
    {
        auto* data = static_cast<AppCallbackData*>(glfwGetWindowUserPointer(window));
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (data->firstMouse)
        {
            data->mouseLastX = xpos;
            data->mouseLastY = ypos;
            data->firstMouse = false;
        }

        float xoffset = xpos - data->mouseLastX;
        float yoffset = data->mouseLastY - ypos; // reversed since y-coordinates go from bottom to top

        data->mouseLastX = xpos;
        data->mouseLastY = ypos;

        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || data->uiMode)
            return;  // ImGui is using the mouse

        data->cameraPtr->ProcessMouseMovement(xoffset, yoffset);
    }

    // glfw: whenever the mouse scroll wheel scrolls, this callback is called
    inline static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto* data = static_cast<AppCallbackData*>(glfwGetWindowUserPointer(window));
        data->cameraPtr->ProcessMouseScroll(static_cast<float>(yoffset));
    }

    Window _window;
    ShaderCache _shaderCache;
    FileWatcher _fileWatcher;
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<AssetCache> _assetCache;
    std::shared_ptr<MeshCache> _meshCache;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<GuiLayer> _guiLayer;
    ModelLoader _modelLoader;
    std::unique_ptr<Scene> _scene;
    AppCallbackData _callbackData;

    // timing
    float _deltaTime = 0.0f;
    float _lastFrame = 0.0f;
};