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
        : _interval(interval), _lastCheck(std::chrono::steady_clock::now())
    {}

    // watch a file, call callback when modified
    void WatchFile(const std::filesystem::path& path, Callback callback) {
        _files[path] = { std::filesystem::last_write_time(path), std::move(callback) };
        //std::cout << "Watching file: " << path << std::endl;
    }

    void WatchDirectory(const std::filesystem::path& dir, Callback callback) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            WatchFile(entry.path(), callback);
        }
    }

    // call this each frame
    void Update() {
        auto now = std::chrono::steady_clock::now();
        if (now - _lastCheck < _interval) return;
        _lastCheck = now;

        for (auto& [path, info] : _files) {
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

    std::unordered_map<std::filesystem::path, FileInfo> _files;
    std::chrono::milliseconds _interval;
    std::chrono::steady_clock::time_point _lastCheck;
};
