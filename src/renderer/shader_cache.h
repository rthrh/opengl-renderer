#pragma once

#include <filesystem>
#include <unordered_map>

#include "shader.h"


class ShaderCache {
public:
    explicit ShaderCache() {}
    ~ShaderCache() {}

    ShaderCache(const ShaderCache&)            = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&& other) noexcept = default;
    ShaderCache& operator=(ShaderCache&& other) noexcept = default;

    // Loads all shader files from the provided directory path
    void LoadDirectory(std::filesystem::path& directory, bool recursive = true) {


    }

    void Build(std::string shaderName, std::string vert, std::string frag, std::string geom = "") {

    }

private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> _shaderMap;
};