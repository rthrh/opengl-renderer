#pragma once

#include <imgui.h>
#include <glm/glm.hpp>
#include <cmath>
#include <string>

#include "renderer/scene.h"

class LightList {
public:
    void Build(Scene& scene) {
        if (!ImGui::Begin("Lights")) { ImGui::End(); return; }

        if (ImGui::CollapsingHeader("Directional", ImGuiTreeNodeFlags_DefaultOpen)) {
            buildDirectional(scene.GetDirectionalLight());
        }
        if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            buildPointLights(scene.GetPointLights());
        }
        if (ImGui::CollapsingHeader("Spot Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            buildSpotLights(scene.GetSpotLights());
        }

        ImGui::End();
        scene.UploadLights();
    }

private:
    void buildDirectional(DirectionalLightUBO& dir) {
        editDirectionYawPitch(dir);
        editColor(dir);
        editIntensity(dir);
    }

    void buildPointLights(PointLightUBO& lights) {
        for (int i = 0; i < lights.Count(); i++) {
            ImGui::PushID(i);
            if (ImGui::TreeNode(("Point " + std::to_string(i)).c_str())) {
                auto& light = lights.At(i);
                editPosition(light);
                editColor(light);
                editIntensity(light);
                editRange(light);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    void buildSpotLights(SpotLightUBO& lights) {
        for (int i = 0; i < lights.Count(); i++) {
            ImGui::PushID(i);
            if (ImGui::TreeNode(("Spot " + std::to_string(i)).c_str())) {
                auto& light = lights.At(i);
                editPosition(light);
                editColor(light);
                editIntensity(light);
                editRange(light);
                editDirectionYawPitch(light);

                float inner = light.GetInnerConeDegrees();
                float outer = light.GetOuterConeDegrees();
                if (ImGui::DragFloat("Inner cone", &inner, 0.5f, 0.f, 90.f)) light.SetCone(inner, outer);
                if (ImGui::DragFloat("Outer cone", &outer, 0.5f, 0.f, 90.f)) light.SetCone(inner, outer);

                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    template<typename L>
    void editDirectionYawPitch(L& light) {
        glm::vec3 d = light.GetDirection();
        float pitch = glm::degrees(std::asin(glm::clamp(d.y, -1.0f, 1.0f)));
        float yaw   = glm::degrees(std::atan2(d.z, d.x));

        bool changed = false;
        if (ImGui::DragFloat("Yaw",   &yaw,   1.0f, -180.f, 180.f)) changed = true;
        if (ImGui::DragFloat("Pitch", &pitch, 1.0f,  -90.f,  90.f)) changed = true;

        if (changed) {
            float p = glm::radians(pitch);
            float y = glm::radians(yaw);
            float cosP = std::cos(p);
            light.SetDirection(glm::vec3(cosP * std::cos(y), std::sin(p), cosP * std::sin(y)));
        }
    }

    template<typename L> void editPosition(L& light) { glm::vec3 v = light.GetPosition(); if (ImGui::DragFloat3("Position", &v.x, 0.05f)) light.SetPosition(v); }
    template<typename L> void editColor(L& light)    { glm::vec3 v = light.GetColor();    if (ImGui::ColorEdit3("Color",    &v.x))         light.SetColor(v); }
    template<typename L> void editIntensity(L& light){ float v = light.GetIntensity();    if (ImGui::DragFloat("Intensity", &v, 0.1f, 0.f, 1000.f)) light.SetIntensity(v); }
    template<typename L> void editRange(L& light)    { float v = light.GetRange();        if (ImGui::DragFloat("Range",     &v, 0.1f, 0.01f, 1000.f)) light.SetRange(v); }
};
