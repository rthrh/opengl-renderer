#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>

class Stopwatch {
    using Clock = std::chrono::steady_clock;
    using MS    = std::chrono::duration<double, std::milli>;
public:
    static constexpr int HISTORY = 120;

    struct Entry {
        double ms{0.0};
        double msMin{1e9};
        double msMax{0.0};
        double msAvg{0.0};
        int    samples{0};

        std::array<double, HISTORY> history{};
        int historyIndex{0};

        void Record(double sample) {
            ms    = sample;
            msMin = std::min(msMin, sample);
            msMax = std::max(msMax, sample);
            samples++;

            history[historyIndex] = sample;
            historyIndex = (historyIndex + 1) % HISTORY;

            int count = std::min(samples, HISTORY);
            double sum = 0.0;
            for (int i = 0; i < count; i++) sum += history[i];
            msAvg = sum / count;
        }
    };

    struct Registry {
        std::vector<std::pair<std::string, Entry>> entries;
        std::unordered_map<std::string, size_t>    index;

        Entry& operator[](const std::string& name) {
            auto it = index.find(name);
            if (it == index.end()) {
                index[name] = entries.size();
                return entries.emplace_back(name, Entry{}).second;
            }
            return entries[it->second].second;
        }
    };

    static Registry& GetRegistry() {
        static Registry registry;
        return registry;
    }

    explicit Stopwatch(std::string label)
        : _label(std::move(label)), _start(Clock::now()) {}

    ~Stopwatch() {
        if (!_stopped)
            record(MS(Clock::now() - _start).count());
    }

    void Stop() {
        if (!_stopped) {
            record(MS(Clock::now() - _start).count());
            _stopped = true;
        }
    }

private:
    void record(double ms) {
        GetRegistry()[_label].Record(ms);
    }

    std::string       _label;
    Clock::time_point _start;
    bool              _stopped{false};
};
