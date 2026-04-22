#pragma once
#include <chrono>
#include <string>
#include "logger.h"

class Stopwatch {
    using Clock = std::chrono::steady_clock;
    using MS    = std::chrono::duration<double, std::milli>;
public:
    explicit Stopwatch(std::string label) : _label(std::move(label)) {}

    void Start(float delay = 0.0f, bool oneShot = false) {
        _start   = _last = Clock::now();
        _delay   = MS{delay};
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
            Info("[{}] total: {:.3f} ms", _label, MS(now - _start).count());
        }
        _running = false;
        if (_oneShot) _fired = true;
    }

private:
    bool can_log(Clock::time_point now) const {
        return _running && !_fired && (now - _start) >= _delay;
    }

    std::string       _label;  // owns the string
    Clock::time_point _start{}, _last{};
    MS                _delay{0};
    bool              _running{false}, _oneShot{false}, _fired{false};
};