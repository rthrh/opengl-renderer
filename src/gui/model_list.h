#pragma once

#include <imgui.h>
#include <glm/glm.hpp>
#include <string>

#include "renderer/scene.h"
#include "renderer/asset_cache.h"
#include "renderer/mesh_cache.h"
#include "renderer/material.h"

class ModelList {
public:
    void Build(Scene& scene, MeshCache& meshCache, AssetCache& assetCache) {
        constexpr int kMinWidth = 280;
        ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, 0), ImVec2(FLT_MAX, FLT_MAX));
        if (!ImGui::Begin("Models", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { ImGui::End(); return; }

        ImGui::PushItemWidth(sliderWidth() * 3.0f + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f);

        for (auto& [handle, model] : scene.GetModels()) {
            ImGui::PushID(handle);
            std::string label = model.GetName().empty()
                ? "Model " + std::to_string(handle)
                : model.GetName();

            if (ImGui::CollapsingHeader(label.c_str())) {
                buildInstances(model);
                buildMeshes(model, meshCache, assetCache);
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

    void buildInstances(Model& model) {
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

            float scale = model.GetScale(i).x;
            if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.001f, 100.f)) {
                model.SetScale(glm::vec3(scale), i);
            }
            ImGui::PopID();
        }
    }

    void buildMeshes(Model& model, MeshCache& meshCache, AssetCache& assetCache) {
        constexpr int kMeshLabelMinWidth = 8;
        const auto& meshHandles = model.GetMeshes();
        if (meshHandles.empty()) return;

        if (ImGui::TreeNode("Meshes")) {
            for (size_t i = 0; i < meshHandles.size(); i++) {
                ImGui::PushID(static_cast<int>(i));

                Mesh* mesh = meshCache.FindMesh(meshHandles[i]);
                if (!mesh) {
                    ImGui::Text("Mesh %zu: <missing>", i);
                    ImGui::PopID();
                    continue;
                }

                uint32_t matIdx = mesh->GetMaterialIndex();
                Material& material = assetCache.GetMaterial(matIdx);

                char meshLabel[32];
                std::snprintf(meshLabel, sizeof(meshLabel), "%-*s", kMeshLabelMinWidth,
                              ("Mesh " + std::to_string(i)).c_str());
                bool open = ImGui::TreeNodeEx(meshLabel, ImGuiTreeNodeFlags_AllowOverlap);

                // Inline texture thumbnails next to mesh name
                ImGui::SameLine();
                inlineTexture(material.baseColorTexture, "Base Color");
                ImGui::SameLine();
                inlineTexture(material.normalTexture, "Normal");
                ImGui::SameLine();
                inlineTexture(material.ormTexture, "ORM");
                ImGui::SameLine();
                inlineTexture(material.emissiveTexture, "Emissive");

                if (open) {
                    ImGui::Text("Material #%u", matIdx);
                    editMaterial(material);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }

void editMaterial(Material& m) {
        // Base color
        ImGui::PushItemWidth(sliderWidth() * 4.0f);
        ImGui::ColorEdit4("Base Color", &m.baseColorFactor.x);
        ImGui::PopItemWidth();

        // Emissive (RGB; alpha unused)
        ImGui::PushItemWidth(sliderWidth() * 3.0f);
        ImGui::ColorEdit3("Emissive", &m.emissiveFactor.x);
        ImGui::PopItemWidth();

        // PBR scalars
        ImGui::PushItemWidth(sliderWidth());
        ImGui::DragFloat("Metallic ", &m.metallicFactor, 0.01f, 0.0f, 1.0f);
        ImGui::SameLine();
        ImGui::DragFloat("Roughness", &m.roughnessFactor, 0.01f, 0.0f, 1.0f);

        ImGui::DragFloat("Occlusion", &m.occlusionStrength, 0.01f, 0.0f, 1.0f);
        ImGui::SameLine();
        ImGui::DragFloat("Normal Scale", &m.normalScale, 0.01f, 0.0f, 5.0f);
        ImGui::PopItemWidth();
    }

    void inlineTexture(GLuint tex, const char* label) {
        constexpr float kSize = 20.0f;
        if (tex == 0) {
            ImGui::Dummy(ImVec2(kSize, kSize));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s: (none)", label);
            return;
        }
        ImGui::Image(tex, ImVec2(kSize, kSize));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Image(tex, ImVec2(128, 128));
            ImGui::Text("%s", label);
            ImGui::EndTooltip();
        }
    }
};