#pragma once

#include <imgui.h>
#include <glm/glm.hpp>
#include <cmath>
#include <cstdio>
#include <string>

#include "renderer/scene.h"

class LightList {
public:
    void Build(Scene& scene) {
        constexpr int kMinWidth = 200;
        ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, 0), ImVec2(FLT_MAX, FLT_MAX));
        if (!ImGui::Begin("Lights", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { ImGui::End(); return; }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, kSliderHeight));

        if (ImGui::CollapsingHeader("Directional", ImGuiTreeNodeFlags_DefaultOpen)) {
            buildDirectional(scene.GetDirectionalLight());
        }
        if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            buildPointLights(scene.GetPointLights());
        }
        if (ImGui::CollapsingHeader("Spot Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            buildSpotLights(scene.GetSpotLights());
        }

        ImGui::PopStyleVar();
        ImGui::End();
        scene.UploadLights();
    }

private:
    static constexpr float kSliderHeight = 2.0f;

    // Width of a slider, sized to fit a typical numeric value with some headroom.
    static float sliderWidth() {
        return ImGui::CalcTextSize("1000.000").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    }

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
                editIntensityAndRange(light);
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
                editIntensityAndRange(light);
                editDirectionYawPitch(light);

                float inner = light.GetInnerConeDegrees();
                float outer = light.GetOuterConeDegrees();
                ImGui::PushItemWidth(sliderWidth());
                if (ImGui::DragFloat("Inner cone", &inner, 0.5f, 0.f, 90.f)) light.SetCone(inner, outer);
                ImGui::SameLine();
                if (ImGui::DragFloat("Outer cone", &outer, 0.5f, 0.f, 90.f)) light.SetCone(inner, outer);
                ImGui::PopItemWidth();

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
        ImGui::PushItemWidth(sliderWidth());
        if (ImGui::DragFloat("Yaw",   &yaw,   1.0f, -180.f, 180.f)) changed = true;
        ImGui::SameLine();
        if (ImGui::DragFloat("Pitch", &pitch, 1.0f,  -90.f,  90.f)) changed = true;
        ImGui::PopItemWidth();

        if (changed) {
            float p = glm::radians(pitch);
            float y = glm::radians(yaw);
            float cosP = std::cos(p);
            light.SetDirection(glm::vec3(cosP * std::cos(y), std::sin(p), cosP * std::sin(y)));
        }
    }

    template<typename L>
    void editPosition(L& light) {
        glm::vec3 v = light.GetPosition();
        ImGui::PushItemWidth(sliderWidth() * 3.0f + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f);
        if (ImGui::DragFloat3("Position", &v.x, 0.05f)) light.SetPosition(v);
        ImGui::PopItemWidth();
    }

    template<typename L>
    void editColor(L& light) {
        glm::vec3 v = light.GetColor();
        ImGui::PushItemWidth(sliderWidth() * 3.0f);
        if (ImGui::ColorEdit3("Color", &v.x)) light.SetColor(v);
        ImGui::PopItemWidth();
    }

    template<typename L>
    void editIntensity(L& light) {
        float intensity = light.GetIntensity();
        ImGui::PushItemWidth(sliderWidth());
        if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.f, 1000.f)) light.SetIntensity(intensity);
        ImGui::PopItemWidth();
    }

    template<typename L>
    void editIntensityAndRange(L& light) {
        ImGui::PushItemWidth(sliderWidth());

        float intensity = light.GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.f, 1000.f)) light.SetIntensity(intensity);

        ImGui::SameLine();
        float range = light.GetRange();
        if (ImGui::DragFloat("Range", &range, 0.1f, 0.01f, 1000.f)) light.SetRange(range);

        ImGui::PopItemWidth();
    }
};
