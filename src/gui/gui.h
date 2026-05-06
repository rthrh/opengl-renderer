#pragma once
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gl/uniform_buffer.h"
#include "renderer/ubo.h"

class GuiLayer {
public:
    GuiLayer(GLFWwindow* window) {
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

    void Build(UniformBuffer<ConfigUBO, 5>& configUBO) {
        ConfigUBO& config = configUBO.Data();
        bool dirty = false;

        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::Begin("Renderer Config");

        if (ImGui::Button("Reset sliders")) {
            config = ConfigUBO{};
            dirty = true;
        }

        ImGui::Separator();

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Bloom & Tonemapping")) {
            dirty |= ImGui::Checkbox("Bloom Enabled", (bool*)&config.bloomEnabled);
            dirty |= ImGui::SliderFloat("Exposure",             &config.exposure,            0.1f, 10.0f);
            dirty |= ImGui::SliderFloat("Gamma",                &config.gamma,               1.0f, 3.0f);
            dirty |= ImGui::SliderFloat("Brightness Threshold", &config.brightnessThreshold, 0.1f, 5.0f);
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("SSAO")) {
            dirty |= ImGui::Checkbox("SSAO Enabled",  (bool*)&config.ssaoEnabled);
            dirty |= ImGui::SliderFloat("Radius",     &config.ssaoRadius, 0.01f, 2.0f);
            dirty |= ImGui::SliderFloat("Bias",       &config.ssaoBias,   0.001f, 0.1f);
            constexpr int kernelValues[] = {8, 16, 32, 64};
            int currentIdx = std::ranges::find(kernelValues, config.ssaoKernel) - std::begin(kernelValues);
            dirty |= ImGui::SliderInt("Kernel Size", &currentIdx, 0, 3, std::to_string(kernelValues[currentIdx]).c_str());
            config.ssaoKernel = kernelValues[currentIdx];
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Shadows")) {
            ImGui::Text("Dir Light Far Plane");
            dirty |= ImGui::SliderFloat("Dir Far Plane",   &config.pointShadowFarPlane,   1.0f, 500.0f);
            ImGui::Separator();
            ImGui::Text("Bias");
            dirty |= ImGui::SliderFloat("Dir Bias Min",  &config.dirShadowBiasMin,  0.0f, 0.1f,  "%.4f");
            dirty |= ImGui::SliderFloat("Dir Bias Max",  &config.dirShadowBiasMax,  0.0f, 0.5f,  "%.4f");
            dirty |= ImGui::SliderFloat("Spot Bias Min", &config.spotShadowBiasMin, 0.0f, 0.01f, "%.5f");
            dirty |= ImGui::SliderFloat("Spot Bias Max", &config.spotShadowBiasMax, 0.0f, 0.1f,  "%.4f");
            dirty |= ImGui::SliderFloat("Point Bias",    &config.pointShadowBias,   0.0f, 0.5f,  "%.4f");
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("IBL")) {
            int lod = static_cast<int>(config.maxReflectionLOD);
            dirty |= ImGui::SliderInt("Max Reflection LOD", &lod, 0, 4);
            config.maxReflectionLOD = static_cast<float>(lod);
        }

        ImGui::End();

        if (dirty)
            configUBO.Upload();
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
};
