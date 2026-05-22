#pragma once
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gl/uniform_buffer.h"
#include "renderer/ubo.h"
#include "gui/settings.h"
#include "gui/model_list.h"
//#include "gui/model_loader_gui.h"
#include "gui/benchmark.h"

class GuiLayer {
public:
    GuiLayer(GLFWwindow* window, const std::filesystem::path& modelsDir, const std::shared_ptr<AssetCache>& assetCache)
        //: _modelLoader(modelsDir, assetCache)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 450");
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
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

    void Build(UniformBuffer<ConfigUBO, 5>& configUBO, Scene& scene, float deltaTime, ModelLoader& modelLoader) {
        Stopwatch stopwatch("GuiLayer::Build");
        _settings.Build(configUBO);
        _modelList.Build(scene);
        //_modelLoader.Build(scene, modelLoader);
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
    //ModelLoaderGUI _modelLoader;
    Benchmark _benchmark;
};
