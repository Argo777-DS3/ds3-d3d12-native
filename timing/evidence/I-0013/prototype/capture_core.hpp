#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "control_core.hpp"

namespace i0012 {

enum class CaptureState : std::uint32_t {
    idle = 0,
    recording = 1,
    exporting = 2,
    saved = 3,
    failed = 4,
};

enum class MatrixCell : std::uint32_t {
    fps60_off = 0,
    fps240_off = 1,
    fps240_on = 2,
    fps60_on = 3,
    complete = 4,
};

enum class CaptureCommand : std::uint32_t {
    none = 0,
    start = 1,
    stop = 2,
};

struct MatrixRequirement {
    float fps = 0.0f;
    bool correction = false;
};

struct TrajectorySample {
    std::int64_t qpc = 0;
    std::uint64_t update_index = 0;
    std::uintptr_t player = 0;
    std::uintptr_t transform = 0;
    float delta = 0.0f;
    float x = 0.0f;
    float z = 0.0f;
    float y = 0.0f;
    std::uint32_t valid = 0;
};

constexpr std::size_t kMaximumCaptureSamples = 65536;

inline MatrixRequirement RequirementFor(MatrixCell cell) {
    switch (cell) {
        case MatrixCell::fps60_off: return {60.0f, false};
        case MatrixCell::fps240_off: return {240.0f, false};
        case MatrixCell::fps240_on: return {240.0f, true};
        case MatrixCell::fps60_on: return {60.0f, true};
        case MatrixCell::complete: return {};
    }
    return {};
}

inline bool CaptureSettingsMatch(MatrixCell cell, CadenceMode mode,
                                 float target_fps, bool correction) {
    if (cell == MatrixCell::complete || mode != CadenceMode::numeric ||
        !std::isfinite(target_fps)) return false;
    const MatrixRequirement requirement = RequirementFor(cell);
    return target_fps == requirement.fps &&
           correction == requirement.correction;
}

inline const char* MatrixCellName(MatrixCell cell) {
    switch (cell) {
        case MatrixCell::fps60_off: return "60-off";
        case MatrixCell::fps240_off: return "240-off";
        case MatrixCell::fps240_on: return "240-on";
        case MatrixCell::fps60_on: return "60-on";
        case MatrixCell::complete: return "complete";
    }
    return "unknown";
}

inline const char* CaptureStateName(CaptureState state) {
    switch (state) {
        case CaptureState::idle: return "IDLE";
        case CaptureState::recording: return "RECORDING";
        case CaptureState::exporting: return "EXPORTING";
        case CaptureState::saved: return "SAVED";
        case CaptureState::failed: return "FAILED";
    }
    return "UNKNOWN";
}

}  // namespace i0012
