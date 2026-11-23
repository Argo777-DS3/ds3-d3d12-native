#pragma once

#include <Windows.h>
#include <cstddef>
#include <cstdint>

namespace ds3native::frame::window {

// Observed DS3 image-relative global locations. These are CPU data locations;
// reading them does not create or call a graphics interface.
inline constexpr uintptr_t kWindowServiceRva = 0x00489F118;
inline constexpr uintptr_t kWidthRva = 0x0046601E8;
inline constexpr uintptr_t kHeightRva = 0x0046601EC;
inline constexpr size_t kServiceHwndOffset = 0x08;
inline constexpr size_t kServiceChildOffset = 0x38;

struct WindowSnapshot {
    uintptr_t service{};
    uintptr_t child{};
    HWND hwnd{};
    DWORD window_thread_id{};
    DWORD snapshot_thread_id{};
    uint32_t configured_width{};
    uint32_t configured_height{};
    uint32_t client_width{};
    uint32_t client_height{};
};

// Optional byte reader is provided for deterministic neutral controls. A null
// reader uses ReadProcessMemory on the current process. It is not an engine
// lease/publication check and does not imply native backend readiness.
using ReadBytes = bool (*)(uintptr_t address, void* destination, size_t bytes,
                           void* context) noexcept;

bool SnapshotWindow(const void* image, uintptr_t expected_service,
                    WindowSnapshot& output, ReadBytes reader = nullptr,
                    void* reader_context = nullptr) noexcept;

} // namespace ds3native::frame::window
