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
#include "file_watcher.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// wireframe
bool wireframe = false;
bool uiMode = false;

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
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
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

    // TODO setup input in seperate class
    // Input input;
    // input.addKeyCallback(GLuint key, [](){});
    // input.addFrameCallback()
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to disable mouse and use raw mouse motion to avoid big delta mouse positions when in ui mode
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);


    return window;
}


int main()
{
    auto* window = create_glfw_window(SCR_WIDTH, SCR_HEIGHT, "opengl-model-viewer");

    // build and compile shaders
    //Shader ourShader("shader.vs", "shader.fs");
    std::filesystem::path root = PROJECT_SOURCE_DIR;
    std::filesystem::path vertexPath = root / "src/shaders" / "shader.vert";
    std::filesystem::path fragmentPath = root / "src/shaders" / "phong.frag";
    Shader lightingShader(vertexPath, fragmentPath);

    // set up shader file watcher
    FileWatcher fileWatcher;
    auto fileCallback = [&lightingShader](const std::filesystem::path&){ lightingShader.Reload();};
    fileWatcher.WatchFile(vertexPath, fileCallback);
    fileWatcher.WatchFile(fragmentPath, fileCallback);
    

    Renderer renderer;
    Scene scene;
    auto materialBuffer = std::make_shared<MaterialBuffer>();
    auto textureCache = std::make_shared<TextureCache>();


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
    //lightCubeModel.Translate(lightPos);
    //scene.AddModel(std::move(lightCubeModel));

    PointLightBlockGPU light(lightPos);
    PointLightBlockGPU light2({-10.0f, 0.0f, 0.0f});
    PointLightBlockGPU light3({0.0f, 10.0f, 0.0f});
    light.SetColor({0.0, 0.0, 125.0});
    light2.SetColor({0.0, 125.0, 0.0});
    light3.SetColor({125.0, 0.0, 0.0});
    //scene.AddPointLight(std::move(light));
    //scene.AddPointLight(std::move(light2));
    //scene.AddPointLight(std::move(light3));

    SpotLightBlockGPU spotLight1({0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f});
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
            processInput(window);
        }

        // create gui items
        guiLayer.build(guiData);

        // don't forget to enable shader before setting uniforms
        lightingShader.Activate();
        lightingShader.SetVec4("color", guiData.color);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.SetMat4("projection", projection);
        lightingShader.SetMat4("view", view);
        lightingShader.SetVec3("viewPos", camera.Position);

        // render scene
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
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
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

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
