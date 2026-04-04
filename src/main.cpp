#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <filesystem>
#include <functional>

#include "renderer/camera.h"
#include "renderer/model.h"
#include "renderer/shader.h"
#include "renderer/renderer.h"
#include "renderer/light_cube.h"
#include "renderer/scene.h"

#include "renderer/material.h"
#include "renderer/texture_cache.h"

#include "gui.h"
#include "input.h"
#include "utils/file_watcher.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, const std::shared_ptr<Camera>&);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// wireframe
bool wireframe = false;
bool uiMode = false;


struct CallbackData {
    std::shared_ptr<Camera> cameraPtr;
    float lastX { SCR_WIDTH / 2.0f };
    float lastY { SCR_HEIGHT / 2.0f };
    bool firstMouse = true;
};


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        wireframe = !wireframe;
        if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        uiMode = !uiMode;
        auto cursor_mode = uiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
        glfwSetInputMode(window, GLFW_CURSOR, cursor_mode);

        GuiLayer::setMouseEnabled(uiMode);
    }
}


GLFWwindow* create_glfw_window(int width, int height, const char* name)
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    //glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // maximize window
    GLFWwindow* window = glfwCreateWindow(width, height, name, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // input callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to disable mouse and use raw mouse motion to avoid big delta mouse positions when in ui mode
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);


    return window;
}


int main()
{
    auto* window = create_glfw_window(SCR_WIDTH, SCR_HEIGHT, "opengl-model-viewer");

    // build and compile shaders
    //Shader ourShader("shader.vs", "shader.fs");
    std::filesystem::path root = PROJECT_SOURCE_DIR;
    std::filesystem::path pathShaders = root / "src/shaders";
    std::filesystem::path vertexPath = pathShaders / "shader.vert";
    std::filesystem::path fragmentPath = pathShaders / "pbr.frag";
    Shader lightingShader(vertexPath, fragmentPath);

    std::filesystem::path gBufferVertPath = pathShaders / "gBuffer.vert";
    std::filesystem::path gBufferFragPath = pathShaders / "gBuffer.frag";
    Shader gBufferShader(gBufferVertPath, gBufferFragPath);

    // set up shader file watcher
    FileWatcher fileWatcher;
    auto fileCallback = [&lightingShader](const std::filesystem::path&) { lightingShader.Reload();};
    fileWatcher.WatchDirectory(pathShaders, fileCallback);
    
    auto camera = std::make_shared<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));
    Renderer renderer(camera);
    Scene scene;
    auto materialBuffer = std::make_shared<MaterialBuffer>();
    auto textureCache = std::make_shared<TextureCache>();

    // App context data for callbacks
    CallbackData callbackData {.cameraPtr{camera}};
    glfwSetWindowUserPointer(window, &callbackData);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);
    //Model ourModel(FileSystem::getPath("resources/backpack/backpack.obj"));
    //Model ourModel(FileSystem::getPath("resources/barrack/Models/Obj/Barrack.obj"));
    std::filesystem::path modelPath = root / "resources" / "99-intergalactic_spaceship-obj/Intergalactic_Spaceship-(Wavefront).obj";
    auto absPath = std::filesystem::absolute(modelPath);
    Model ourModel(absPath.string(), materialBuffer, textureCache);

    // light cube model
    Mesh lightCubeMesh(cube_vertices, cube_indices);
    Model lightCubeModel(std::move(lightCubeMesh), materialBuffer, textureCache);
    glm::vec3 lightPos = {10.0f, 0.0f, 0.0f};
    Mesh floorMesh(floor_vertices, floor_indices);
    Model floorModel(std::move(floorMesh), materialBuffer, textureCache);
    floorModel.Scale({10.0f, 1.0f, 10.0f});   // make it big
    floorModel.Translate({0.0f, -2.0f, 0.0f}); // push it below the model
    scene.AddModel(std::move(floorModel));

    PointLightBlockGPU light(lightPos);
    PointLightBlockGPU light2({-10.0f, 0.0f, 0.0f});
    PointLightBlockGPU light3({0.0f, 10.0f, 0.0f});
    light.SetColor({0.0, 0.0, 125.0});
    light2.SetColor({0.0, 125.0, 0.0});
    light3.SetColor({125.0, 0.0, 0.0});
    scene.AddPointLight(std::move(light2));
    DirectionalLightBlockGPU dirLight({-1.0, -1.0, 0.0});
    scene.AddDirectionalLight(std::move(dirLight));

    SpotLightBlockGPU spotLight1({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, -1.0f});
    spotLight1.SetColor({0.0, 0.0, 125.0});
    scene.AddSpotLight(std::move(spotLight1));

    // init imgui
    GuiLayer guiLayer(window);
    GuiData guiData {
        .color = glm::vec4{0.6f, 0.5f, 0.4f, 0.3f}
    };

    scene.AddModel(std::move(ourModel));
    //DirectionalLightBlockGPU lightDir{{1.0f, 1.0f, 0.0f}};
    //scene.AddDirectionalLight(lightDir);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
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

        // Tell OpenGL a new frame is about to begin
		guiLayer.beginFrame();

        // handle input
        if (!guiLayer.wantCaptureMouse()) {

        }
        if (!guiLayer.wantCaptureKeyboard()) {
            // input
            processInput(window, camera);
        }

        // create gui items
        guiLayer.build(guiData);

        // don't forget to enable shader before setting uniforms
        lightingShader.Activate();
        lightingShader.SetVec4("color", guiData.color);

        // view/projection transformations
        glm::mat4 projection = camera->GetProjectionMatrix();
        glm::mat4 view = camera->GetViewMatrix();
        lightingShader.SetMat4("projection", projection);
        lightingShader.SetMat4("view", view);
        lightingShader.SetVec3("viewPos", camera->Position);


        // TODO g-buffer stuff
        gBufferShader.Activate();
        renderer.PassGeometry(scene, gBufferShader);
        // render scene
        lightingShader.Activate();
        renderer.Render(scene, lightingShader);

        // Renders the ImGUI elements
		guiLayer.endFrame();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
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
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || uiMode)
        return;  // ImGui is using the mouse

    auto* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (data->firstMouse)
    {
        data->lastX = xpos;
        data->lastY = ypos;
        data->firstMouse = false;
    }

    float xoffset = xpos - data->lastX;
    float yoffset = data->lastY - ypos; // reversed since y-coordinates go from bottom to top

    data->lastX = xpos;
    data->lastY = ypos;

    data->cameraPtr->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
    data->cameraPtr->ProcessMouseScroll(static_cast<float>(yoffset));
}
