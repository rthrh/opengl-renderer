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

#include "renderer/texture_cache.h"
#include "renderer/shader_cache.h"
#include "renderer/model_loader.h"

#include "gui/gui.h"
#include "utils/file_watcher.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, const std::shared_ptr<Camera>&);
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// wireframe
bool wireframe = false;
bool uiMode = false;


struct MouseCallbackData {
    std::shared_ptr<Camera> cameraPtr;
    float lastX; //{ SCR_WIDTH / 2.0f };
    float lastY; //{ SCR_HEIGHT / 2.0f };
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

        GuiLayer::SetMouseEnabled(uiMode);
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

    // Init debug context and callback
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    if (glfwExtensionSupported("GL_KHR_debug")) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        Info("KHR_debug enabled");
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // fixes seams artifacts from skybox shape

    return window;
}


void setupScene(Scene& scene, const std::shared_ptr<TextureCache>& textureCache, ModelLoader& modelLoader) {
    std::filesystem::path root = PROJECT_SOURCE_DIR;
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    std::filesystem::path modelPath = root / ".." / "glTF-Sample-Models/2.0" / "EnvironmentTest/glTF-IBL/EnvironmentTest.gltf";
    std::filesystem::path modelPath2 = root / ".." / "glTF-Sample-Models/2.0" / "DamagedHelmet/glTF/DamagedHelmet.gltf";
    //std::filesystem::path modelPath = root / "resources" / "99-intergalactic_spaceship-obj/Intergalactic_Spaceship-(Wavefront).obj";

    auto absPath = std::filesystem::absolute(modelPath);
    auto ourModel = modelLoader.Load(absPath);

    scene.AddModel(std::move(*ourModel), Deferred);

    auto absPath2 = std::filesystem::absolute(modelPath2);
    auto ourModel2 = modelLoader.Load(absPath2);

    scene.AddModel(std::move(*ourModel2), Deferred);

    // floor model
    Mesh floorMesh(floor_vertices, floor_indices);
    auto floorModel = modelLoader.Load((std::move(floorMesh)));
    floorModel.SetTranslation({0.0f, -2.0f, 0.0f});
    floorModel.SetScale({50.0f, 1.0f, 50.0f});

    scene.AddModel(std::move(floorModel));


    DirectionalLightUBO dirLight({-1.0, -1.0, 0.0});
    auto light1 = PointLightBlockGPU({0,10,0}).SetColor(0, 125, 255).SetRange(50);
    auto light2 = PointLightBlockGPU({0,10,-10}).SetColor(0, 255, 125).SetRange(50);
    auto light3 = PointLightBlockGPU({0,2,10}).SetColor(255, 125, 0).SetRange(50);

    auto spotLight1 = SpotLightBlockGPU({0, 3, 6}, {0, -0.5, -1}).SetColor(0, 0, 255).SetRange(25.0).SetIntensity(10);
    auto spotLight2 = SpotLightBlockGPU({0, 10, 0}, {0, -1.0, 0}).SetColor(125, 0, 0).SetRange(25.0).SetIntensity(10);
    //auto spotLight2 = SpotLightBlockGPU({0, 10, 0}, {0, -1.0, 0.1}).SetColor(125, 0, 0).SetRange(25.0).SetIntensity(10);

    scene.AddDirectionalLight(std::move(dirLight));
    scene.AddPointLight(std::move(light1));
    scene.AddPointLight(std::move(light2));
    scene.AddPointLight(std::move(light3));
    scene.AddSpotLight(std::move(spotLight1));
    scene.AddSpotLight(std::move(spotLight2));

}

void setupScene1k(Scene& scene, std::shared_ptr<TextureCache> textureCache, ModelLoader& modelLoader) {
    std::mt19937 rng(42); // fixed seed for reproducibility
    std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
    std::uniform_real_distribution<float> heightDist(0.5f, 15.0f);
    std::uniform_int_distribution<int> colorDist(50, 255);
    std::uniform_real_distribution<float> rangeDist(5.0f, 30.0f);
    std::uniform_real_distribution<float> intensityDist(1.0f, 20.0f);

    std::filesystem::path root = PROJECT_SOURCE_DIR;
    //std::filesystem::path modelPath = root / "resources" / "barrack/Models/Obj/Barrack.obj";
    //std::filesystem::path modelPath = root / "resources" / "backpack/backpack.obj";
    std::filesystem::path modelPath = root / ".." / "glTF-Sample-Models/2.0" / "DamagedHelmet/glTF/DamagedHelmet.gltf";
    //std::filesystem::path modelPath = root / "resources" / "99-intergalactic_spaceship-obj/Intergalactic_Spaceship-(Wavefront).obj";
    auto absPath = std::filesystem::absolute(modelPath);
    auto ourModel = modelLoader.Load(absPath);
    scene.AddModel(std::move(*ourModel), Deferred);

    // floor model
    Mesh floorMesh(floor_vertices, floor_indices);
    auto floorModel = modelLoader.Load((std::move(floorMesh)));
    floorModel.SetTranslation({0.0f, -2.0f, 0.0f});
    floorModel.SetScale({50.0f, 1.0f, 50.0f});
    scene.AddModel(std::move(floorModel));

    for (int i = 0; i < 1000; i++) {
        glm::vec3 pos = { posDist(rng), heightDist(rng), posDist(rng)
        };

        auto light = PointLightBlockGPU(pos)
            .SetColor(colorDist(rng), colorDist(rng), colorDist(rng))
            .SetRange(rangeDist(rng))
            .SetIntensity(intensityDist(rng));

        scene.AddPointLight(std::move(light));
    }
}

int main()
{
    const unsigned int windowWidth = 1600u;
    const unsigned int windowHeight = 1200u;
    auto* window = create_glfw_window(windowWidth, windowHeight, "opengl-model-viewer");

    // build and compile shaders
    std::filesystem::path root = PROJECT_SOURCE_DIR;
    std::filesystem::path pathShaders = root / "src/shaders";
    ShaderCache shaderCache;
    shaderCache.LoadDirectory(pathShaders);

    auto deferredLightShader = shaderCache.Build("deferred", "quad.vert", "deferred_pbr.frag");
    auto gBufferShader = shaderCache.Build("gBuffer", "gBuffer.vert", "gBuffer.frag");
    auto debugShader = shaderCache.Build("deferred_debug", "quad.vert", "deferred_pbr.frag");
    auto forwardShader = shaderCache.Build("forward", "forward.vert", "forward_pbr.frag");
    auto phongShader = shaderCache.Build("phong_forward", "forward.vert", "forward_phong.frag");

    auto equirectShader = shaderCache.Build("equirect", "equirect_to_cubemap.vert", "equirect_to_cubemap.frag");
    auto skyboxShader = shaderCache.Build("skybox", "skybox.vert", "skybox.frag");

    auto shadowDirShader = shaderCache.Build("shadow_directional", "shadow_directional.vert", "depth.frag");
    auto shadowPointShader = shaderCache.Build("shadow_point", "shadow_point.vert", "shadow_point.frag", "shadow_point.geom");
    auto shadowSpotShader = shaderCache.Build("shadow_spot", "shadow_spot.vert", "depth.frag");

    auto blurShader = shaderCache.Build("blur", "quad.vert", "blur.frag");
    auto bloomShader = shaderCache.Build("bloom", "quad.vert", "bloom.frag");

    auto irradianceShader = shaderCache.Build("irradiance", "irradiance.vert", "irradiance.frag");
    auto prefilterShader = shaderCache.Build("prefilter", "irradiance.vert", "prefilter.frag");
    auto brdfShader = shaderCache.Build("brdf", "brdf.vert", "brdf.frag");

    auto ssaoShader = shaderCache.Build("ssao", "quad.vert", "ssao.frag");
    auto ssaoBlurShader = shaderCache.Build("ssao_blur", "quad.vert", "ssao_blur.frag");

    auto unlitShader = shaderCache.Build("unlit", "unlit.vert", "unlit.frag"); // debug light cubes

    // set up shader file watcher
    FileWatcher fileWatcher;
    auto fileCallback = [&shaderCache](const std::filesystem::path&) { shaderCache.ReloadAll();};
    fileWatcher.WatchDirectory(pathShaders, fileCallback);

    float aspectRatio = (float)windowWidth / (float)windowHeight;
    auto camera = std::make_shared<Camera>(aspectRatio, glm::vec3(0.0f, 0.0f, 3.0f));

    // setup skybox rogland_clear_night_4k newport_loft.hdr
    //std::filesystem::path skyboxPath = root / "resources" / "newport_loft.hdr";
    std::filesystem::path skyboxPath = root / "resources" / "rogland_clear_night_4k.exr";
    auto skybox = std::make_shared<Skybox>(skyboxPath, *equirectShader, *irradianceShader, *prefilterShader, *brdfShader);


    Renderer renderer(windowWidth, windowHeight, camera, skybox);
    auto textureCache = std::make_shared<TextureCache>();

    UniformBuffer<ConfigUBO, 5> configUBO; // TODO who should own?
    configUBO.Upload();

    // App context data for callbacks
    MouseCallbackData callbackData {.cameraPtr{camera}, .lastX = windowWidth / 2.0f, .lastY = windowHeight / 2.0f };
    glfwSetWindowUserPointer(window, &callbackData);

     // init imgui
    std::filesystem::path modelsDirectory = root / ".." / "glTF-Sample-Models/2.0";
    GuiLayer guiLayer(window, modelsDirectory, textureCache);

    // Scene setup
    ModelLoader modelLoader(textureCache);
    Scene scene(textureCache);
    setupScene1k(scene, textureCache, modelLoader);

    // restore viewport of screen size // TODO move it somewhere?
    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);


    glm::mat4 projection = glm::perspective(glm::radians(camera->GetZoom()), (float)scrWidth / (float)scrHeight, 0.1f, 100.0f);
    //pbrShader.use();
    //pbrShader.setMat4("projection", projection);
    skyboxShader->Activate();
    skyboxShader->SetMat4("projection", projection);

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

		guiLayer.BeginFrame();

        // handle input
        if (!guiLayer.WantCaptureMouse()) {

        }
        if (!guiLayer.WantCaptureKeyboard()) {
            // input
            processInput(window, camera);
        }

        // create gui items
        guiLayer.Build(configUBO, scene, deltaTime, modelLoader);

        // Render scene
        camera->UploadUBO();

        renderer.PassShadowDirectional(scene, *shadowDirShader);
        renderer.PassShadowPoint(scene, *shadowPointShader, configUBO.Data());
        renderer.PassShadowSpot(scene, *shadowSpotShader, configUBO.Data());
        renderer.PassGeometryBuffer(scene, *gBufferShader);
        renderer.PassSSAO(scene, *ssaoShader, *ssaoBlurShader);
        renderer.PassDeferred(scene, *deferredLightShader);
        renderer.PassForward(scene, *forwardShader);
        renderer.PassSkybox(*skybox, *skyboxShader);
        renderer.PassNoShadow(scene, *unlitShader);
        renderer.PassBloom(*blurShader, *bloomShader);
        // Renders the ImGUI elements
		guiLayer.EndFrame();

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
    auto* data = static_cast<MouseCallbackData*>(glfwGetWindowUserPointer(window));
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

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || uiMode)
        return;  // ImGui is using the mouse

    data->cameraPtr->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* data = static_cast<MouseCallbackData*>(glfwGetWindowUserPointer(window));
    data->cameraPtr->ProcessMouseScroll(static_cast<float>(yoffset));
}

void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            unsigned int id,
                            GLenum severity,
                            GLsizei length,
                            const char *message,
                            const void *userParam)
{
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
}
