#pragma once

#include <chrono>
#include <string>
#include <vector>
#include "utils/logger.h"

class Stopwatch {
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    explicit Stopwatch(std::string label)
        : _label(std::move(label))
        , _start(Clock::now())
        , _last(_start)
    {}

    void Stamp(const std::string& stepLabel = "") {
        auto now   = Clock::now();
        auto delta = toMs(now - _last);
        _last      = now;
        Info("[{}] {}: {:.3f} ms", _label, stepLabel, delta);
    }

    void Stop(const std::string& stepLabel = "") {
        this->Stamp(stepLabel);
        auto total = toMs(Clock::now() - _start);
        Info("[{}] total: {:.3f} ms", _label, total);
    }

private:
    double toMs(std::chrono::nanoseconds d) {
        return std::chrono::duration<double, std::milli>(d).count();
    }

    std::string _label;
    TimePoint   _start;
    TimePoint   _last;
};
