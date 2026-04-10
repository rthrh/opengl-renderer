#pragma once

#include <chrono>
#include <string_view>
#include "logger.h"

class Stopwatch {
    using Clock = std::chrono::steady_clock; // Better for durations than high_res
    using MS    = std::chrono::duration<double, std::milli>;

public:
    explicit Stopwatch(std::string_view label) : _label(label) {}

    void Start(MS delay = MS{0}, bool oneShot = false) {
        _start   = _last = Clock::now();
        _delay   = delay;
        _oneShot = oneShot;
        _running = true;
        _fired   = false;
    }

    void Stamp(std::string_view stepLabel = "") {
        auto now = Clock::now();
        if (!can_log(now)) return;

        Info("[{}] {}: {:.3f} ms", _label, stepLabel, MS(now - _last).count());
        _last = now;
    }

    void Stop(std::string_view stepLabel = "") {
        auto now = Clock::now();
        if (can_log(now)) {
            Stamp(stepLabel);
            Info("[{}] total: {:.3f} ms", _label, MS(now - _start).count());
        }
        _running = false;
        if (_oneShot) _fired = true;
    }

private:
    bool can_log(Clock::time_point now) const {
        return _running && !_fired && (now - _start) >= _delay;
    }

    std::string_view  _label;
    Clock::time_point _start{}, _last{};
    MS                _delay{0};
    bool              _running{false}, _oneShot{false}, _fired{false};
};
