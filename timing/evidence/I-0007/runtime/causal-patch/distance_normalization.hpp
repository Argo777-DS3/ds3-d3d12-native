#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
struct DistanceFrame {
    unsigned long long index = 0;
    float delta = 0.0f;
    uintptr_t transform = 0;
    float position[3]{};
    bool valid = false;
};
inline bool MatchDistanceFrames(const DistanceFrame& previous, const DistanceFrame& current,
    unsigned long long index, uintptr_t owner, const float current_position[3], const float old_position[3]) {
    return previous.valid && current.valid && previous.index + 1 == current.index &&
        current.index == index && previous.transform == owner && current.transform == owner &&
        std::memcmp(current.position, current_position, 12) == 0 &&
        std::memcmp(previous.position, old_position, 12) == 0;
}
inline bool NormalizeDistanceTest(float raw, float elapsed, float* output) {
    if (!output || !std::isfinite(raw) || raw < 0.0f || !std::isfinite(elapsed) || elapsed <= 0.0f) return false;
    constexpr float reference_interval = 1.0f / 60.0f;
    const float scale = reference_interval / elapsed;
    const float normalized = raw * scale;
    if (!std::isfinite(normalized)) return false;
    *output = normalized;
    return true;
}
