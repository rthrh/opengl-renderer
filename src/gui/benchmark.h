#pragma once
#include <imgui.h>
#include "utils/stopwatch.h"

class Benchmark {
public:
    void Build(float deltaTime) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
        ImGui::Begin("Benchmark");

        ImGui::Text("FPS: %.0f", 1.0f / deltaTime);
        ImGui::Separator();

        double totalMs = 0.0;
        for (auto& [name, entry] : Stopwatch::GetRegistry().entries) {
            totalMs += entry.ms;
            ImGui::Text("%-24s  cur: %7.1f us  avg: %7.1f us  min: %7.1f us  max: %7.1f us",
                name.c_str(),
                entry.ms    * 1000.0,
                entry.msAvg * 1000.0,
                entry.msMin * 1000.0,
                entry.msMax * 1000.0);
            ImGui::ProgressBar(static_cast<float>(entry.ms / 33.3), ImVec2(-1, 0), "");
        }

        ImGui::Separator();
        ImGui::Text("%-24s  cur: %7.1f us", "Total", totalMs * 1000.0);

        ImGui::End();
    }
};