#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace i0012 {

enum class RuntimeState : std::uint32_t {
    starting = 0,
    standby = 1,
    native = 2,
    active = 3,
    stopping = 4,
    stopped = 5,
    failed = 6,
};

enum class CadenceMode : std::uint32_t {
    native = 0,
    numeric = 1,
    unlimited = 2,
};

enum class ControlResult : std::uint32_t {
    none = 0,
    queued = 1,
    applied = 2,
    invalid = 3,
    unsupported = 4,
    failed = 5,
};

struct CadenceRequest {
    CadenceMode mode = CadenceMode::native;
    float fps = 60.0f;
};

struct TimingWritePlan {
    bool supported = false;
    bool use_override = false;
    float fps = 0.0f;
    unsigned char enable = 0;
};

struct TimingPair {
    float fps = 0.0f;
    unsigned char enable = 0;
};

constexpr float kMinimumFps = 30.0f;
constexpr float kMaximumFps = 360.0f;
constexpr std::array<int, 7> kFpsPresets = {
    30, 60, 90, 120, 144, 165, 240,
};

inline float ClampSliderFps(float value) {
    if (!std::isfinite(value)) return kMinimumFps;
    return std::clamp(std::round(value), kMinimumFps, kMaximumFps);
}

inline bool IsNumericFps(float value) {
    return std::isfinite(value) && value >= kMinimumFps &&
           value <= kMaximumFps;
}

inline TimingWritePlan ResolveCadenceRequest(const CadenceRequest& request,
                                             float original_fps,
                                             unsigned char original_enable) {
    switch (request.mode) {
        case CadenceMode::native:
            return {true, false, original_fps, original_enable};
        case CadenceMode::numeric:
            if (!IsNumericFps(request.fps)) return {};
            return {true, true, request.fps, 1};
        case CadenceMode::unlimited:
            return {false, false, original_fps, original_enable};
    }
    return {};
}

inline RuntimeState StateForCadence(const CadenceRequest& request) {
    return request.mode == CadenceMode::numeric
        ? RuntimeState::active
        : RuntimeState::native;
}

struct FpsWindow {
    std::int64_t first_qpc = 0;
    std::uint32_t frames = 0;
    float last_fps = 0.0f;

    bool Add(std::int64_t qpc, std::int64_t frequency) {
        if (qpc <= 0 || frequency <= 0) return false;
        if (first_qpc == 0 || qpc <= first_qpc) {
            first_qpc = qpc;
            frames = 0;
            return false;
        }
        ++frames;
        const std::int64_t elapsed = qpc - first_qpc;
        if (elapsed < frequency / 2) return false;
        last_fps = static_cast<float>(
            static_cast<double>(frames) * static_cast<double>(frequency) /
            static_cast<double>(elapsed));
        first_qpc = qpc;
        frames = 0;
        return std::isfinite(last_fps) && last_fps >= 0.0f;
    }
};

inline const char* RuntimeStateName(RuntimeState state) {
    switch (state) {
        case RuntimeState::starting: return "STARTING";
        case RuntimeState::standby: return "STANDBY";
        case RuntimeState::native: return "NATIVE";
        case RuntimeState::active: return "ACTIVE";
        case RuntimeState::stopping: return "STOPPING";
        case RuntimeState::stopped: return "STOPPED";
        case RuntimeState::failed: return "FAILED";
    }
    return "UNKNOWN";
}

}  // namespace i0012
