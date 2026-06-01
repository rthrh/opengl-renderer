#pragma once

#include <imgui.h>
#include <ranges>
#include "gl/uniform_buffer.h"
#include "renderer/ubo.h"

class Settings {
public:
    void Build(UniformBuffer<ConfigUBO, 5>& configUBO) {
        ConfigUBO& config = configUBO.Data();
        bool dirty = false;

        constexpr int kMinWidth = 200;
        ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, 0), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("Renderer Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PushItemWidth(kSliderWidth);

        if (ImGui::Button("Reset sliders")) {
            config = ConfigUBO{};
            dirty = true;
        }
        ImGui::Separator();

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Debug")) {
            dirty |= ImGui::Checkbox("Light Cubes on/off", (bool*)&config.lightCubesEnabled);
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Bloom & Tonemapping")) {
            dirty |= ImGui::Checkbox("Bloom Enabled", (bool*)&config.bloomEnabled);
            dirty |= ImGui::SliderFloat("Exposure",       &config.exposure,      0.1f, 10.0f);
            dirty |= ImGui::SliderFloat("Gamma",          &config.gamma,         1.0f, 3.0f);
            dirty |= ImGui::SliderFloat("Bloom Strength", &config.bloomStrength, 0.0f, 1.0f);
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("Shadows")) {
            dirty |= ImGui::Checkbox("Directional Shadows", (bool*)&config.dirShadowsEnabled);
            dirty |= ImGui::Checkbox("Point Shadows",       (bool*)&config.pointShadowsEnabled);
            dirty |= ImGui::Checkbox("Spot Shadows",        (bool*)&config.spotShadowsEnabled);
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("SSAO")) {
            dirty |= ImGui::Checkbox("SSAO Enabled", (bool*)&config.ssaoEnabled);
            dirty |= ImGui::SliderFloat("Radius",    &config.ssaoRadius, 0.01f, 2.0f);
            dirty |= ImGui::SliderFloat("Bias",      &config.ssaoBias,   0.001f, 0.1f);
            constexpr int kernelValues[] = {8, 16, 32, 64};
            int currentIdx = std::ranges::find(kernelValues, config.ssaoKernel) - std::begin(kernelValues);
            dirty |= ImGui::SliderInt("Kernel Size", &currentIdx, 0, 3, std::to_string(kernelValues[currentIdx]).c_str());
            config.ssaoKernel = kernelValues[currentIdx];
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("FXAA")) {
            dirty |= ImGui::Checkbox("FXAA Enabled",          (bool*)&config.fxaaEnable);
            dirty |= ImGui::SliderFloat("Edge Threshold Min", &config.fxaaEdgeThresholdMin, 0.0312f, 0.0833f, "%.4f");
            dirty |= ImGui::SliderFloat("Edge Threshold Max", &config.fxaaEdgeThresholdMax, 0.063f,  0.333f,  "%.3f");
            dirty |= ImGui::SliderFloat("Subpixel Quality",   &config.fxaaSubpixelQuality,  0.0f,    1.0f,    "%.2f");
        }

        ImGui::PopItemWidth();
        ImGui::End();

        if (dirty)
            configUBO.Upload();
    }

private:
    static constexpr float kSliderWidth = 120.0f;
};