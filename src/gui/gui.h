#pragma once
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <gl/headers.h>

#include <GLFW/glfw3.h>

#include "gl/uniform_buffer.h"
#include "renderer/ubo.h"
#include "renderer/asset_cache.h"
#include "renderer/mesh_cache.h"
#include "gui/settings.h"
#include "gui/model_list.h"
#include "gui/light_list.h"
//#include "gui/model_loader_gui.h"
#include "gui/benchmark.h"

class GuiLayer {
public:
    GuiLayer(GLFWwindow* window, const std::filesystem::path& modelsDir, const std::shared_ptr<AssetCache>& assetCache, const std::shared_ptr<MeshCache>& meshCache) :
        _assetCache(assetCache),
        _meshCache(meshCache)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        #ifdef __EMSCRIPTEN__
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            ImGui_ImplOpenGL3_Init("#version 300 es");
        #else
            ImGui_ImplOpenGL3_Init("#version 450");
        #endif
    }

    ~GuiLayer() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void Build(UniformBuffer<ConfigUBO>& configUBO, Scene& scene, float deltaTime, ModelLoader& modelLoader) {
        Stopwatch stopwatch("GuiLayer::Build");

        const float pad = 10.0f;
        float x = pad;
        float y = pad;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
        _settings.Build(configUBO);

        x += 300.0;
        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
        _lightList.Build(scene);

        x += 300.0;
        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
        _modelList.Build(scene, *_meshCache, *_assetCache);


        x += 100.0;
        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
        _benchmark.Build(deltaTime);
    }

    static void SetMouseEnabled(bool value) {
        ImGuiIO& io = ImGui::GetIO();
        if (value)
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        else
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }

    static bool WantCaptureMouse()    { return ImGui::GetIO().WantCaptureMouse; }
    static bool WantCaptureKeyboard() { return ImGui::GetIO().WantCaptureKeyboard; }

private:
    Settings _settings;
    ModelList _modelList;
    LightList _lightList;
    Benchmark _benchmark;
    std::shared_ptr<AssetCache> _assetCache;
    std::shared_ptr<MeshCache> _meshCache;
};
