#pragma once

#include <windows.h>

#include <cstdint>

#include "control_core.hpp"
#include "capture_core.hpp"

namespace i0012 {

constexpr std::uint32_t kStatusVersion = 1;

struct RuntimeStatus {
    std::uint32_t version = kStatusVersion;
    RuntimeState state = RuntimeState::starting;
    CadenceMode cadence_mode = CadenceMode::native;
    ControlResult last_control_result = ControlResult::none;
    bool ready = false;
    bool correction_active = false;
    bool guard_failed = false;
    bool timing_restored = true;
    bool debug_restored = true;
    float target_fps = 0.0f;
    float actual_fps = 0.0f;
    std::uint64_t updates = 0;
    std::uint64_t comparisons = 0;
    std::uint64_t corrections = 0;
    std::uint64_t guard_failures = 0;
    std::uint64_t cadence_transitions = 0;
    std::uint64_t correction_transitions = 0;
    CaptureState capture_state = CaptureState::idle;
    MatrixCell matrix_cell = MatrixCell::fps60_off;
    std::uint64_t capture_samples = 0;
    std::uint64_t capture_invalid = 0;
    std::uint64_t capture_attempt = 0;
    bool capture_overflow = false;
};

bool Start(HMODULE module);
void RequestNumericCadence(float fps);
void RequestNativeCadence();
void RequestCorrection(bool enabled);
void RequestCaptureStart();
void RequestCaptureStop();
void RequestStop();
bool StopAndWait(unsigned long timeout_ms);
RuntimeStatus GetStatus();

}  // namespace i0012
