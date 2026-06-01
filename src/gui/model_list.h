#pragma once

#include <imgui.h>
#include <glm/glm.hpp>

#include "renderer/scene.h"

class ModelList {
public:
    void Build(Scene& scene) {
        constexpr int kMinWidth = 200;
        ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, 0), ImVec2(FLT_MAX, FLT_MAX));
        if (!ImGui::Begin("Models", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { ImGui::End(); return; }

        ImGui::PushItemWidth(sliderWidth() * 3.0f + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f);

        for (auto& [handle, model] : scene.GetModels()) {
            ImGui::PushID(static_cast<int>(handle));
            std::string label = model.GetName().empty()
                ? "Model " + std::to_string(handle)
                : model.GetName();

            if (ImGui::CollapsingHeader(label.c_str())) {
                // Per-instance transforms. Most models have 1 instance.
                int instanceCount = static_cast<int>(model.GetInstanceCount());
                for (int i = 0; i < instanceCount; i++) {
                    ImGui::PushID(i);
                    if (instanceCount > 1) ImGui::Text("Instance %d", i);

                    glm::vec3 t = model.GetTranslation(i);
                    if (ImGui::DragFloat3("Position", &t.x, 0.05f)) {
                        model.SetTranslation(t, i);
                    }

                    glm::vec3 r = model.GetEulerAngles(i);
                    if (ImGui::DragFloat3("Rotation", &r.x, 0.5f)) {
                        model.SetEulerAngles(r, i);
                    }

                    float scale = model.GetScale(i).x;  // assume uniform
                    if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.001f, 100.f)) {
                        model.SetScale(glm::vec3(scale), i);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::PopID();
        }

        ImGui::PopItemWidth();
        ImGui::End();
    }

private:
    static float sliderWidth() {
        return ImGui::CalcTextSize("1000.000").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    }
};