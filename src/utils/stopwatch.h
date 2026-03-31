#include <chrono>
#include <string>
#include <format>

#include "utils/logger.h"

class Stopwatch {
public:
    Stopwatch() : _start(std::chrono::high_resolution_clock::now()), _last(_start) {}

    void Reset() {
        _start = std::chrono::high_resolution_clock::now();
        _last  = _start;
    }

    void Tick() {
        auto now   = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<double, std::micro>(now - _last).count();
        auto total = std::chrono::duration<double, std::micro>(now - _start).count();
        _last      = now;
        Info("[Tick] — delta: {:.3f} ms | total: {:.3f} us", delta, total);
    }

    void Start() {
        _measure_start = std::chrono::high_resolution_clock::now();
    }

    void Stop() {
        auto delta = std::chrono::duration<double, std::micro>(std::chrono::high_resolution_clock::now() - _measure_start).count();
        Info("[Stop] — {:.6f} us", delta);
    }

private:
    std::chrono::high_resolution_clock::time_point _start;
    std::chrono::high_resolution_clock::time_point _last;
    std::chrono::high_resolution_clock::time_point _measure_start;
};