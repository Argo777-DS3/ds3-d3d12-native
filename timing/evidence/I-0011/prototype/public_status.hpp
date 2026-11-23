#pragma once

#include <cstdint>

constexpr std::uint32_t kRuntimeStatusMagic = 0x49303131u;  // I011
constexpr std::uint32_t kRuntimeStatusVersion = 1;

struct RuntimePublicStatus {
    std::uint32_t magic = kRuntimeStatusMagic;
    std::uint32_t version = kRuntimeStatusVersion;
    std::uint32_t state = 0;
    std::uint32_t cadence_mode = 0;
    std::uint32_t correction_active = 0;
    std::uint32_t overlay_visible = 0;
    std::uint32_t guard_failed = 0;
    std::uint32_t timing_restored = 0;
    std::uint32_t debug_restored = 0;
    std::uint32_t last_control_result = 0;
    float target_fps = 0.0f;
    float actual_fps = 0.0f;
    std::uint64_t updates = 0;
    std::uint64_t comparisons = 0;
    std::uint64_t corrections = 0;
    std::uint64_t guard_failures = 0;
    std::uint64_t cadence_transitions = 0;
    std::uint64_t correction_transitions = 0;
};
