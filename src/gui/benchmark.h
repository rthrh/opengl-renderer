#pragma once

#include <imgui.h>
#include "utils/stopwatch.h"

class Benchmark {
public:
    void Build(float deltaTime) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
        ImGui::Begin("Benchmark CPU", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        const float fps = 1.0f / deltaTime;
        const float frameBudgetMs = deltaTime * 1000.0f;
        ImGui::Text("FPS: %.0f  (budget: %.2f ms)", fps, frameBudgetMs);
        ImGui::Separator();

        double totalMs = 0.0;
        for (auto& [name, entry] : Stopwatch::GetRegistry().entries) {
            totalMs += entry.ms;
            ImGui::Text("%-18s %6.1f us",
                        name.c_str(),
                        entry.msAvg * 1000.0);
            constexpr int height = 4;
            ImGui::ProgressBar(static_cast<float>(entry.ms / frameBudgetMs), ImVec2(-1, height), "");
        }
        ImGui::Separator();
        ImGui::End();
    }
};