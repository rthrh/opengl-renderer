#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <filesystem>
#include <functional>
#include <random>

#include "renderer/camera.h"
#include "renderer/model.h"
#include "renderer/shader.h"
#include "renderer/renderer.h"
#include "renderer/shapes.h"
#include "renderer/scene.h"

#include "renderer/asset_cache.h"
#include "renderer/shader_cache.h"
#include "renderer/model_loader.h"

#include "gui/gui.h"
#include "utils/file_watcher.h"
#include "demo.h"
#include "window.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, const std::shared_ptr<Camera>&);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool uiMode = false;


struct AppCallbackData {
    std::shared_ptr<Camera> cameraPtr;
    Renderer* rendererPtr;

    // Mouse callback
    float mouseLastX; //{ SCR_WIDTH / 2.0f };
    float mouseLastY; //{ SCR_HEIGHT / 2.0f };
    bool firstMouse = true;

    // Resize callback
    int newScrWidth;
    int newScrHeight;
};


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        uiMode = !uiMode;
        auto cursor_mode = uiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
        glfwSetInputMode(window, GLFW_CURSOR, cursor_mode);

        GuiLayer::SetMouseEnabled(uiMode);
    }
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window, const std::shared_ptr<Camera>& camera)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(RIGHT, deltaTime);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    auto* data = static_cast<AppCallbackData*>(glfwGetWindowUserPointer(window));
    data->rendererPtr->Resize(width, height);
    data->cameraPtr->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
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
    if (io.WantCaptureMouse || uiMode)
        return;  // ImGui is using the mouse

    data->cameraPtr->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* data = static_cast<AppCallbackData*>(glfwGetWindowUserPointer(window));
    data->cameraPtr->ProcessMouseScroll(static_cast<float>(yoffset));
}


int main()
{
    const unsigned int windowWidth = 1600u;
    const unsigned int windowHeight = 1200u;
    auto window = Window(windowWidth, windowHeight, "opengl-model-viewer");

    // Input callbacks
    glfwSetFramebufferSizeCallback(window.GetHandle(), framebuffer_size_callback);
    glfwSetCursorPosCallback(window.GetHandle(), mouse_callback);
    glfwSetScrollCallback(window.GetHandle(), scroll_callback);
    glfwSetKeyCallback(window.GetHandle(), key_callback);

    // build and compile shaders
    std::filesystem::path root = PROJECT_SOURCE_DIR;
    std::filesystem::path pathShaders = root / "src/shaders";
    ShaderCache shaderCache;
    shaderCache.SetPostBuildHook(InitSamplers);
    shaderCache.LoadDirectory(pathShaders);

    // set up shader file watcher
    FileWatcher fileWatcher;
    auto fileCallback = [&shaderCache](const std::filesystem::path&) { shaderCache.ReloadAll();};
    fileWatcher.WatchDirectory(pathShaders, fileCallback);

    float aspectRatio = (float)windowWidth / (float)windowHeight;
    auto camera = std::make_shared<Camera>(aspectRatio, glm::vec3(0.0f, 0.0f, 3.0f));

    // setup skybox rogland_clear_night_4k newport_loft.hdr
    //std::filesystem::path skyboxPath = root / "resources" / "newport_loft.hdr";
    std::filesystem::path skyboxPath = root / "resources" / "rogland_clear_night_4k.exr";

    auto assetCache = std::make_shared<AssetCache>();
    auto meshCache = std::make_shared<MeshCache>(assetCache);
    Renderer renderer(windowWidth, windowHeight, camera, assetCache, meshCache, shaderCache);
    renderer.LoadSkybox(skyboxPath);


    // App context data for callbacks
    AppCallbackData callbackData {.cameraPtr{camera},
                                  .rendererPtr{&renderer},
                                  .mouseLastX = windowWidth / 2.0f,
                                  .mouseLastY = windowHeight / 2.0f };
    glfwSetWindowUserPointer(window.GetHandle(), &callbackData);

     // init imgui
    std::filesystem::path modelsDirectory = root / ".." / "glTF-Sample-Models/2.0";
    GuiLayer guiLayer(window.GetHandle(), modelsDirectory, assetCache);

    // Scene setup
    ModelLoader modelLoader(assetCache, meshCache);
    //Scene scene(assetCache); setupScene(scene, assetCache, modelLoader);
    //auto scene = setupTestModels(assetCache, modelLoader);
    auto scene = setupSponza(assetCache, modelLoader);

    // restore viewport of screen size // TODO move it somewhere?
    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window.GetHandle(), &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    // render loop
    while (!window.ShouldClose())
    {
        Stopwatch stopwatch("Render loop");
        // poll events
        glfwPollEvents();
        fileWatcher.Update();

        // per-frame time logic
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		guiLayer.BeginFrame();

        // handle input
        if (!guiLayer.WantCaptureMouse()) {

        }
        if (!guiLayer.WantCaptureKeyboard()) {
            // input
            processInput(window.GetHandle(), camera);
        }

        // create gui items
        guiLayer.Build(renderer.GetConfig(), scene, deltaTime, modelLoader);

        // Render scene
        camera->UploadUBO();
        assetCache->UploadMaterials();
        scene.UploadTransforms(meshCache);

        glCullFace(GL_FRONT); // TODO move
        renderer.PassShadowDirectional(scene);
        renderer.PassShadowPoint(scene);
        renderer.PassShadowSpot(scene);
        glCullFace(GL_BACK);

        renderer.PassGeometryBuffer(scene);
        renderer.PassSSAO(scene);
        renderer.PassDeferred(scene);
        renderer.PassForward(scene);

        glDisable(GL_CULL_FACE);
        renderer.PassSkybox();
        glEnable(GL_CULL_FACE);

        renderer.PassNoShadow(scene);
        renderer.PassBloom();
        renderer.PassFXAA();
        Stopwatch stopwatch1("GuiLayer::EndFrame");
        // Renders the ImGUI elements
		guiLayer.EndFrame();
        stopwatch1.Stop();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        Stopwatch stopwatch2("glfwSwapBuffers(window);");
        window.SwapBuffers();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}
