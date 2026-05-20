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
            dirty |= ImGui::SliderFloat("Exposure",          &config.exposure,   0.1f, 10.0f);
            dirty |= ImGui::SliderFloat("Gamma",             &config.gamma,      1.0f, 3.0f);
            dirty |= ImGui::SliderFloat("Bloom Strength", &config.bloomStrength, 0.0f, 1.0f);
            //dirty |= ImGui::Checkbox("Karis Average", (bool*)&config.bloomKarisMipLevel);
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
            dirty |= ImGui::Checkbox("Shadows Enabled", (bool*)&config.shadowsEnabled);
            dirty |= ImGui::SliderInt("Max Point Casters", &config.maxPointShadowCasters, 0, 4);
            dirty |= ImGui::SliderInt("Max Spot Casters",  &config.maxSpotShadowCasers,   0, 4);
            dirty |= ImGui::SliderFloat("Dir Bias Min",    &config.dirShadowBiasMin,  0.0f, 0.1f,  "%.4f");
            dirty |= ImGui::SliderFloat("Dir Bias Max",    &config.dirShadowBiasMax,  0.0f, 0.5f,  "%.4f");
            dirty |= ImGui::SliderFloat("Point Bias",      &config.pointShadowBias,   0.0f, 0.5f,  "%.4f");
            dirty |= ImGui::SliderFloat("Spot Bias Min",   &config.spotShadowBiasMin, 0.0f, 0.01f, "%.5f");
            dirty |= ImGui::SliderFloat("Spot Bias Max",   &config.spotShadowBiasMax, 0.0f, 0.1f,  "%.4f");
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::CollapsingHeader("FXAA")) {
            dirty |= ImGui::Checkbox("FXAA Enabled",   (bool*)&config.fxaaEnable);
            dirty |= ImGui::SliderFloat("Edge Threshold Min", &config.fxaaEdgeThresholdMin, 0.0312f, 0.0833f, "%.4f");
            dirty |= ImGui::SliderFloat("Edge Threshold Max", &config.fxaaEdgeThresholdMax, 0.063f,  0.333f,  "%.3f");
            dirty |= ImGui::SliderFloat("Subpixel Quality",   &config.fxaaSubpixelQuality,  0.0f,    1.0f,    "%.2f");
            dirty |= ImGui::SliderInt("Iterations",           &config.fxaaIterations,       3,       12);
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
};