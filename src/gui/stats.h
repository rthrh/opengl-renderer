#pragma once
#include <imgui.h>

class Stats {
public:
    void Build(float deltaTime) {
        float fps = 1.0f / deltaTime;

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::Begin("##stats", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.0f", fps);

        ImGui::End();
    }
};
