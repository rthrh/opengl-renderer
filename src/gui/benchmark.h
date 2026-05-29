#pragma once
#include <imgui.h>
#include "utils/stopwatch.h"

class Benchmark {
public:
    void Build(float deltaTime) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
        ImGui::Begin("Benchmark");

        ImGui::Text("FPS: %.0f", 1.0f / deltaTime);
        ImGui::Separator();

        double totalMs = 0.0;
        for (auto& [name, entry] : Stopwatch::GetRegistry().entries) {
            totalMs += entry.ms;
            ImGui::Text("%-24s  cur: %7.1f us  avg: %7.1f us",
                name.c_str(),
                entry.ms    * 1000.0,
                entry.msAvg * 1000.0);

            constexpr int height = 4;
            ImGui::ProgressBar(static_cast<float>(entry.ms / 8.33), ImVec2(-1, height), "");
        }

        ImGui::Separator();
        ImGui::End();
    }
};
