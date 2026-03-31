#pragma once
#include <format>
#include <iostream>
#include <string_view>

namespace {
enum class LogLevel { Info, Warn, Error };

template<typename... Args>
void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
    std::string msg = std::format(fmt, std::forward<Args>(args)...);

    switch (level) {
        case LogLevel::Info:  std::cout << "[INFO]  " << msg << "\n"; break;
        case LogLevel::Warn:  std::cout << "[WARN]  " << msg << "\n"; break;
        case LogLevel::Error: std::cerr << "[ERROR] " << msg << "\n"; break;
    }
}
};

template<typename... Args> void Info (std::format_string<Args...> fmt, Args&&... args) { Log(LogLevel::Info,  fmt, std::forward<Args>(args)...); }
template<typename... Args> void Warn (std::format_string<Args...> fmt, Args&&... args) { Log(LogLevel::Warn,  fmt, std::forward<Args>(args)...); }
template<typename... Args> void Error(std::format_string<Args...> fmt, Args&&... args) { Log(LogLevel::Error, fmt, std::forward<Args>(args)...); }