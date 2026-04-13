#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>
#include <array>
#include "shader.h"
#include <ranges>
constexpr std::array<std::string_view, 4> gShaderExtensions = { ".vert", ".frag", ".geom", ".comp" };



class ShaderCache {
public:
    using ShaderName = std::string;
    using FileName = std::string;

    explicit ShaderCache() = default;
    ~ShaderCache() = default;

    ShaderCache(const ShaderCache&)            = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&& other) noexcept = default;
    ShaderCache& operator=(ShaderCache&& other) noexcept = default;

    // Loads all shader files from the provided directory path
    void LoadDirectory(const std::filesystem::path& dir) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            if (!std::ranges::contains(gShaderExtensions, std::string_view{ext})) continue;

            FileName filename = entry.path().filename().string();
            _files[filename] = entry.path();
        }
    }

    // Builds a shader with a shaderName from the provided shader file names
    std::shared_ptr<Shader> Build(const ShaderName& name, const FileName& vert, const FileName& frag, const FileName& geom = "") {
        auto vertPath = getFile(vert);
        auto fragPath = getFile(frag);
        if (!vertPath || !fragPath) throw std::runtime_error("No vert or frag shader found");

        std::optional<std::filesystem::path> geomPath = "";
        if (!geom.empty()) {
            geomPath = getFile(geom);
            if (!geomPath) throw std::runtime_error("No geometry shader found: " + geom);
        }

        auto shaderPtr = std::make_shared<Shader>(*vertPath, *fragPath, *geomPath);
        _shaders[name] = shaderPtr;
        Info("Shader built: {}, {} + {}", name, vert, frag);
        return shaderPtr;
    }

    std::shared_ptr<Shader> Get(const ShaderName& name) const {
        auto it = _shaders.find(name);
        if (it == _shaders.end()) {
            Error("Shader {} not found", name);
            return nullptr;
        }
        return it->second;
    }

    void ReloadAll() {
        for (auto& [name, shader] : _shaders) {
            shader->Reload();
        }
        Info("Shaders reloaded");
    }

private:
    std::optional<std::filesystem::path> getFile(const FileName& filename) const {
        auto it = _files.find(filename);
        if (it == _files.end()) {
            Error("Shader file name not found: {}", filename);
            return std::nullopt;
        }
        return it->second;
    }

    std::unordered_map<FileName, std::filesystem::path> _files;
    std::unordered_map<ShaderName, std::shared_ptr<Shader>> _shaders;
};