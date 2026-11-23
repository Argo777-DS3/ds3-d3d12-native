#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

enum class PrototypeState : std::uint32_t {
    standby = 0,
    observe = 1,
    correct = 2,
    stopping = 3,
    failed = 4,
};

enum class PrototypeAction : std::uint32_t {
    prepare,
    enable,
    disable,
    stop,
};

inline const char* PrototypeStateName(PrototypeState state) {
    switch (state) {
        case PrototypeState::standby: return "STANDBY";
        case PrototypeState::observe: return "OBSERVE";
        case PrototypeState::correct: return "CORRECT";
        case PrototypeState::stopping: return "STOPPING";
        case PrototypeState::failed: return "FAILED";
    }
    return "UNKNOWN";
}

inline bool NextPrototypeState(PrototypeState current, PrototypeAction action,
                               PrototypeState* next) {
    if (!next) return false;
    switch (action) {
        case PrototypeAction::prepare:
            if (current != PrototypeState::standby) return false;
            *next = PrototypeState::observe;
            return true;
        case PrototypeAction::enable:
            if (current != PrototypeState::observe) return false;
            *next = PrototypeState::correct;
            return true;
        case PrototypeAction::disable:
            if (current != PrototypeState::correct) return false;
            *next = PrototypeState::observe;
            return true;
        case PrototypeAction::stop:
            if (current == PrototypeState::stopping) return false;
            *next = PrototypeState::stopping;
            return true;
    }
    return false;
}

struct DistanceFrame {
    std::uint64_t index = 0;
    float delta = 0.0f;
    std::uintptr_t transform = 0;
    float position[3]{};
    bool valid = false;
};

inline bool MatchDistanceFrames(const DistanceFrame& previous,
                                const DistanceFrame& current,
                                std::uint64_t expected_index,
                                std::uintptr_t expected_transform,
                                const float current_position[3],
                                const float old_position[3]) {
    return previous.valid && current.valid &&
           previous.index + 1 == current.index &&
           current.index == expected_index &&
           previous.transform == expected_transform &&
           current.transform == expected_transform &&
           std::memcmp(current.position, current_position, 12) == 0 &&
           std::memcmp(previous.position, old_position, 12) == 0;
}

inline bool NormalizeDistanceTest(float raw, float elapsed, float* output) {
    if (!output || !std::isfinite(raw) || raw < 0.0f ||
        !std::isfinite(elapsed) || elapsed <= 0.0f) {
        return false;
    }
    constexpr float reference_interval = 1.0f / 60.0f;
    const float normalized = raw * (reference_interval / elapsed);
    if (!std::isfinite(normalized)) return false;
    *output = normalized;
    return true;
}

struct ComparisonGuardInput {
    unsigned char opcode[3]{};
    DistanceFrame previous{};
    DistanceFrame current{};
    std::uint64_t expected_index = 0;
    std::uintptr_t expected_transform = 0;
    float owner_position[3]{};
    float getter_position[3]{};
    float old_position[3]{};
    float raw = 0.0f;
};

struct ComparisonDecision {
    float normalized = 0.0f;
    bool native_reduces = false;
    bool normalized_reduces = false;
};

inline bool EvaluateComparison(const ComparisonGuardInput& input,
                               ComparisonDecision* decision) {
    if (!decision) return false;
    constexpr unsigned char expected_opcode[3] = {0x0F, 0x2F, 0xF8};
    if (std::memcmp(input.opcode, expected_opcode, sizeof(expected_opcode)) != 0 ||
        std::memcmp(input.owner_position, input.getter_position, 12) != 0 ||
        !MatchDistanceFrames(input.previous, input.current,
                             input.expected_index, input.expected_transform,
                             input.owner_position, input.old_position) ||
        !NormalizeDistanceTest(input.raw, input.previous.delta,
                               &decision->normalized)) {
        return false;
    }
    decision->native_reduces = input.raw < 1.0f;
    decision->normalized_reduces = decision->normalized < 1.0f;
    return true;
}
