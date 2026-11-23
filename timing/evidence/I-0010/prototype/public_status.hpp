#pragma once

#include <cstdint>

constexpr std::uint32_t kPrototypeStatusMagic = 0x49303130u;  // I010
constexpr std::uint32_t kPrototypeStatusVersion = 1;

struct PrototypePublicStatus {
    std::uint32_t magic = kPrototypeStatusMagic;
    std::uint32_t version = kPrototypeStatusVersion;
    std::uint32_t state = 0;
    std::uint32_t timing_override_active = 0;
    std::uint32_t correction_active = 0;
    std::uint32_t guard_failed = 0;
    std::uint32_t debug_restored = 0;
    std::uint32_t timing_restored = 0;
    std::uint32_t last_control_success = 0;
    std::uint32_t reserved = 0;
    float requested_fps = 240.0f;
    float measured_fps = 0.0f;
    std::uint64_t updates = 0;
    std::uint64_t arms = 0;
    std::uint64_t disarms = 0;
    std::uint64_t comparisons = 0;
    std::uint64_t recurrence_exits = 0;
    std::uint64_t commits = 0;
    std::uint64_t corrections = 0;
    std::uint64_t native_reduce_corrected = 0;
    std::uint64_t normalized_reduce_retained = 0;
    std::uint64_t guard_failures = 0;
    std::uint64_t central_delta_checks = 0;
    std::uint64_t central_delta_mismatches = 0;
    std::uint64_t animation_samples = 0;
    std::uint64_t animation_changes = 0;
};
