#pragma once

#include <windows.h>

#include <cstdint>

#include "control_core.hpp"

namespace i0014 {

constexpr std::uint32_t kStatusVersion = 2;

struct RuntimeStatus {
    std::uint32_t version = kStatusVersion;
    RuntimeState state = RuntimeState::starting;
    CadenceMode cadence_mode = CadenceMode::native;
    ControlResult last_control_result = ControlResult::none;
    bool ready = false;
    bool ltf_active = false;
    bool trtf_active = false;
    bool guard_failed = false;
    bool timing_restored = true;
    bool debug_restored = true;
    float target_fps = 0.0f;
    float actual_fps = 0.0f;
    std::uint64_t updates = 0;
    std::uint64_t ltf_comparisons = 0;
    std::uint64_t ltf_corrections = 0;
    std::uint64_t trtf_calls = 0;
    std::uint64_t trtf_writes = 0;
    std::uint64_t trtf_native_passthroughs = 0;
    std::uint64_t guard_failures = 0;
    std::uint64_t cadence_transitions = 0;
    std::uint64_t ltf_transitions = 0;
    std::uint64_t trtf_transitions = 0;
    std::uint64_t trtf_state_restorations = 0;
};

bool Start(HMODULE module);
void RequestNumericCadence(float fps);
void RequestNativeCadence();
void RequestLtf(bool enabled);
void RequestTrtf(bool enabled);
void RequestStop();
bool StopAndWait(unsigned long timeout_ms);
RuntimeStatus GetStatus();

}  // namespace i0014
