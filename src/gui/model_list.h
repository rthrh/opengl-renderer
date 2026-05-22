#pragma once
#include <imgui.h>
#include <glm/glm.hpp>
#include "renderer/scene.h"

class ModelList {
public:
    void Build(Scene& scene) {
        ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(420, 10), ImGuiCond_Once);
        ImGui::Begin("Models");
        ImGui::BeginChild("##modellist", ImVec2(0, 0), false, ImGuiWindowFlags_None);


        ImGui::EndChild();
        ImGui::End();
    }

private:
    /*void buildQueue(Scene::RenderQueue& queue) {
        for (auto& [handle, model] : queue) {
            auto label = model.GetName().empty()
                ? "Model #" + std::to_string(handle)
                : model.GetName() + " (#" + std::to_string(handle) + ")";

            if (!ImGui::TreeNode(label.c_str()))
                continue;

            glm::vec3 translation = model.GetTranslation();
            if (ImGui::DragFloat3("Translation", &translation.x, 0.1f))
                model.SetTranslation(translation);

            glm::vec3 scale = model.GetScale();
            if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.001f, 100.0f))
                model.SetScale(scale);

            glm::vec3 euler = model.GetEulerAngles();
            if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f, -360.0f, 360.0f))
                model.SetEulerAngles(euler);

            ImGui::TreePop();
        }
    }*/

};