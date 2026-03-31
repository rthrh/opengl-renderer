#pragma once

#include <filesystem>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <string>
#include <iostream>

class FileWatcher {
public:
    using Callback = std::function<void(const std::filesystem::path&)>;

    FileWatcher(std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
        : m_interval(interval), m_lastCheck(std::chrono::steady_clock::now())
    {}

    // watch a file, call callback when modified
    void WatchFile(const std::filesystem::path& path, Callback callback) {
        m_files[path] = { std::filesystem::last_write_time(path), std::move(callback) };
        std::cout << "Watching file: " << path << std::endl;
    }

    // call this each frame
    void Update() {
        auto now = std::chrono::steady_clock::now();
        if (now - m_lastCheck < m_interval) return;
        m_lastCheck = now;

        for (auto& [path, info] : m_files) {
            auto currentTime = std::filesystem::last_write_time(path);
            if (currentTime != info.lastWriteTime) {
                info.lastWriteTime = currentTime;
                if (info.callback)
                    info.callback(path);
            }
        }
    }

private:
    struct FileInfo {
        std::filesystem::file_time_type lastWriteTime;
        Callback callback;
    };

    std::unordered_map<std::filesystem::path, FileInfo> m_files;
    std::chrono::milliseconds m_interval;
    std::chrono::steady_clock::time_point m_lastCheck;
};