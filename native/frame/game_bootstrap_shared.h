#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cwchar>
namespace ds3native::bootstrap {
constexpr uint32_t signature=0x66313244;
enum class Mode : uint32_t { Reference=0, NativeBootstrap=1 };
// I-0082 (DESIGN 3): a crash record the module fills and the controller serves (the minidump is written out of process).
enum class CrashState : LONG { Idle=0, Requested=1, Taken=2, Written=3, Failed=4 };
enum class CrashKind : uint32_t { None=0, Exception=1, NativeFailure=2, Refusal=3, DeviceRemoved=4, GpuHang=5 };
struct Crash {
    volatile LONG state{};          // CrashState
    volatile LONG dumps{};          // dumps written this run (the module stops asking after 4)
    CrashKind kind{};
    uint32_t code{},thread{};       // exception code or exit code; the thread that asked
    uint64_t exception{};           // EXCEPTION_POINTERS* in the game, 0 when none
    int32_t removed_reason{};       // GetDeviceRemovedReason, S_OK when the device was not lost
    char reason[256]{};
    wchar_t dump[520]{};            // the dump path, written by whoever wrote the dump
};
constexpr uint32_t control_version=5;   // 4: the Reference probe and code capture fields removed (I-0102); 5: bounded_seconds (security audit M1)
struct Control {
    uint32_t magic{},version{},pid{};
    Mode mode{};
    volatile LONG ready{},stop{},closed{},legacy_refused{};
    volatile LONG64 phase_calls{},native_frames{},faults{};
    wchar_t journal[1024]{};
    uint32_t bounded_seconds{};   // the controller's bound (1 to 7200: a runner's run), 0 for a play session (both launchers): the witness files are read only in a bounded run (security audit M1, 2026-11-23)
    Crash crash{};           // version 3 (I-0082)
};
inline void Name(wchar_t (&name)[96],DWORD pid) {
    swprintf_s(name,L"Local\\DS3I66Bootstrap-%lu",pid);
}
}
