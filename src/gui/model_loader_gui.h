#pragma once
#include <imgui.h>
#include <filesystem>
#include <vector>
#include <string>
#include <ranges>
#include "renderer/scene.h"
#include "renderer/model.h"
#include "texture_cache.h"
#include "renderer/model_loader.h"
#include "utils/logger.h"

namespace fs = std::filesystem;

class ModelLoaderGUI {
public:
    ModelLoaderGUI(const fs::path& rootDir, const std::shared_ptr<TextureCache>& textureCache)
        : _textureCache(textureCache) {
        scan(rootDir);
    }

    void Build(Scene& scene, ModelLoader& loader) {
        ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(680, 10), ImGuiCond_Once);
        ImGui::Begin("Model Loader");
        ImGui::BeginChild("##modelfiles", ImVec2(0, 0), false);

        for (auto& [displayName, fullPath] : _models) {
            if (ImGui::Selectable(displayName.c_str())) {
                auto model = loader.Load(fullPath);
                if (model) {
                    scene.AddModel(std::move(*model), Deferred);
                } else {
                    Warn("Error loading model: {}", fullPath.string());
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", fullPath.string().c_str());
        }

        ImGui::EndChild();
        ImGui::End();
    }

private:
    void scan(const fs::path& dir) {
        if (!fs::exists(dir)) return;
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (entry.path().extension() == ".gltf")
                _models.emplace_back(fs::relative(entry.path(), dir).string(), entry.path());
        }
        std::ranges::sort(_models, {}, [](const auto& p) { return p.first; });
    }

    std::shared_ptr<TextureCache> _textureCache;
    std::vector<std::pair<std::string, fs::path>> _models;
};