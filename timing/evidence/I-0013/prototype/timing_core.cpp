#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <intrin.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <new>

#include "MinHook.h"
#include "control_core.hpp"
#include "timing_core.hpp"
#include "../../I-0010/prototype/prototype_core.hpp"
#include "../../I-0010/prototype/debug_config.hpp"

namespace {

using namespace i0012;

constexpr std::uintptr_t kRvaBroadUpdate = 0x00D57900;
constexpr std::uintptr_t kRvaSpatialBoundary = 0x02344730;
constexpr std::uintptr_t kRvaCentralGlobal = 0x0489DD10;
constexpr std::uintptr_t kRvaWorldChrManGlobal = 0x0477FDB8;
constexpr std::uintptr_t kRvaComparison = 0x005C73D8;
constexpr std::uintptr_t kRvaRecurrenceExit = 0x005C7435;
constexpr std::uintptr_t kCallerArm = 0x00A1429C;
constexpr std::uintptr_t kCallerDisarm = 0x00DE50EC;

std::uintptr_t g_image = 0;
LARGE_INTEGER g_frequency{};
std::atomic<RuntimeState> g_state{RuntimeState::starting};
std::atomic<CadenceMode> g_cadence_mode{CadenceMode::native};
std::atomic<float> g_target_fps{0.0f};
std::atomic<float> g_actual_fps{0.0f};
std::atomic<bool> g_correction_active{false};
std::atomic<bool> g_guard_failed{false};
std::atomic<bool> g_ready{false};
std::atomic<bool> g_started{false};
std::atomic<bool> g_stop_requested{false};
std::atomic<ControlResult> g_last_control{ControlResult::none};

std::atomic<std::uint64_t> g_updates{0};
std::atomic<std::uint64_t> g_comparisons{0};
std::atomic<std::uint64_t> g_corrections{0};
std::atomic<std::uint64_t> g_guard_failures{0};
std::atomic<std::uint64_t> g_cadence_transitions{0};
std::atomic<std::uint64_t> g_correction_transitions{0};
std::atomic<bool> g_timing_restored{true};
std::atomic<bool> g_debug_restored{true};

std::atomic<std::uint64_t> g_cadence_request_generation{0};
std::atomic<std::uint64_t> g_cadence_applied_generation{0};
std::atomic<CadenceMode> g_requested_mode{CadenceMode::native};
std::atomic<float> g_requested_fps{60.0f};
std::atomic<std::uint64_t> g_correction_request_generation{0};
std::atomic<std::uint64_t> g_correction_applied_generation{0};
std::atomic<bool> g_requested_correction{false};

std::atomic<CaptureState> g_capture_state{CaptureState::idle};
std::atomic<MatrixCell> g_matrix_cell{MatrixCell::fps60_off};
std::atomic<CaptureCommand> g_capture_command{CaptureCommand::none};
std::atomic<std::uint64_t> g_capture_request_generation{0};
std::atomic<std::uint64_t> g_capture_applied_generation{0};
std::atomic<std::uint64_t> g_capture_sample_count{0};
std::atomic<std::uint64_t> g_capture_invalid_count{0};
std::atomic<std::uint64_t> g_capture_attempt{0};
std::atomic<bool> g_capture_overflow{false};
std::array<TrajectorySample, kMaximumCaptureSamples> g_capture_samples{};
std::uintptr_t g_capture_player = 0;
std::uintptr_t g_capture_transform = 0;
MatrixCell g_active_capture_cell = MatrixCell::fps60_off;
float g_capture_target_fps = 0.0f;
bool g_capture_correction = false;
wchar_t g_capture_session_dir[MAX_PATH * 4]{};
bool g_capture_storage_ready = false;

HANDLE g_stop_event = nullptr;
HANDLE g_runtime_thread = nullptr;

CRITICAL_SECTION g_timing_lock{};

bool g_original_timing_valid = false;
bool g_timing_modified = false;
std::uintptr_t g_timing_object = 0;
TimingPair g_original_timing{};
FpsWindow g_fps_window{};

class CriticalSectionGuard {
public:
    explicit CriticalSectionGuard(CRITICAL_SECTION* section)
        : section_(section) {
        EnterCriticalSection(section_);
    }
    ~CriticalSectionGuard() { LeaveCriticalSection(section_); }
    CriticalSectionGuard(const CriticalSectionGuard&) = delete;
    CriticalSectionGuard& operator=(const CriticalSectionGuard&) = delete;

private:
    CRITICAL_SECTION* section_;
};

thread_local DistanceFrame g_distance_previous{};
thread_local DistanceFrame g_distance_current{};

using FnBroadUpdate = void(__fastcall*)(void*, void*);
using FnSpatialBoundary = void(__fastcall*)(void*, void*);
FnBroadUpdate g_original_broad_update = nullptr;
FnSpatialBoundary g_original_spatial_boundary = nullptr;

struct PlayerSnapshot {
    std::uintptr_t player = 0;
    std::uintptr_t transform = 0;
    float position[3]{};
    bool position_valid = false;
};

enum class WatchCommand { none, arm, disarm, stop };

struct DebugRegisterState {
    DWORD64 dr0 = 0;
    DWORD64 dr1 = 0;
    DWORD64 dr2 = 0;
    DWORD64 dr3 = 0;
    DWORD64 dr6 = 0;
    DWORD64 dr7 = 0;
};

CRITICAL_SECTION g_watch_lock{};
HANDLE g_watch_request = nullptr;
HANDLE g_watch_done = nullptr;
HANDLE g_watch_thread = nullptr;
PVOID g_watch_veh = nullptr;
WatchCommand g_watch_command = WatchCommand::none;
DWORD g_watch_target_tid = 0;
std::uintptr_t g_watch_addresses[3]{};
DebugRegisterState g_watch_original{};
bool g_watch_original_valid = false;
std::atomic<int> g_watch_result{0};
std::atomic<bool> g_watch_active{false};
std::atomic<DWORD> g_watch_active_tid{0};
std::uintptr_t g_watch_shared_context = 0;
std::uintptr_t g_watch_transform_base = 0;
std::uint64_t g_watch_update_index = 0;
unsigned int g_watch_hits = 0;

long long Qpc() {
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}

template <typename T>
bool SafeRead(std::uintptr_t address, T* value) {
    if (!address || !value) return false;
    __try {
        *value = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <typename T>
bool SafeWrite(std::uintptr_t address, const T& value) {
    if (!address) return false;
    __try {
        *reinterpret_cast<T*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SafeCopy(std::uintptr_t address, void* destination, size_t size) {
    if (!address || !destination) return false;
    __try {
        std::memcpy(destination, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::memset(destination, 0, size);
        return false;
    }
}

bool ReadPointer(std::uintptr_t address, std::uintptr_t* value) {
    return SafeRead(address, value) && *value != 0;
}

bool SameFloatBits(float left, float right) {
    return std::memcmp(&left, &right, sizeof(float)) == 0;
}

void LogEvent(const char*, ControlResult result) {
    if (result != ControlResult::none) {
        g_last_control.store(result, std::memory_order_release);
    }
}

void LogTransition(const char*, const TimingPair&, const TimingPair&,
                   ControlResult) {}

void FailClosed(const char* reason) {
    const bool first = !g_guard_failed.exchange(true, std::memory_order_acq_rel);
    g_guard_failures.fetch_add(1, std::memory_order_relaxed);
    g_state.store(RuntimeState::failed, std::memory_order_release);
    if (first) LogEvent(reason, ControlResult::failed);
    if (g_stop_event) SetEvent(g_stop_event);
}

PlayerSnapshot ReadPlayerSnapshot() {
    PlayerSnapshot snapshot{};
    std::uintptr_t world = 0;
    if (!ReadPointer(g_image + kRvaWorldChrManGlobal, &world)) return snapshot;
    ReadPointer(world + 0x80, &snapshot.player);
    std::uintptr_t list = 0;
    if (ReadPointer(world + 0x40, &list) &&
        ReadPointer(list + 0x28, &snapshot.transform)) {
        snapshot.position_valid =
            SafeRead(snapshot.transform + 0x80, &snapshot.position[0]) &&
            SafeRead(snapshot.transform + 0x84, &snapshot.position[1]) &&
            SafeRead(snapshot.transform + 0x88, &snapshot.position[2]);
    }
    return snapshot;
}

bool InitializeCaptureStorage() {
    wchar_t local_app_data[MAX_PATH * 4]{};
    const DWORD length = GetEnvironmentVariableW(
        L"LOCALAPPDATA", local_app_data, _countof(local_app_data));
    if (length == 0 || length >= _countof(local_app_data)) return false;
    wchar_t root[MAX_PATH * 4]{};
    _snwprintf_s(root, _countof(root), _TRUNCATE,
                 L"%s\\DS3TerrainCapture", local_app_data);
    if (!CreateDirectoryW(root, nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS) return false;
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    _snwprintf_s(
        g_capture_session_dir, _countof(g_capture_session_dir), _TRUNCATE,
        L"%s\\%04u%02u%02uT%02u%02u%02uZ-pid%lu", root,
        utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond,
        static_cast<unsigned long>(GetCurrentProcessId()));
    if (!CreateDirectoryW(g_capture_session_dir, nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS) return false;
    wchar_t info_path[MAX_PATH * 4]{};
    _snwprintf_s(info_path, _countof(info_path), _TRUNCATE,
                 L"%s\\session-info.txt", g_capture_session_dir);
    FILE* info = nullptr;
    if (_wfopen_s(&info, info_path, L"wb") != 0 || !info) return false;
    std::fprintf(
        info,
        "investigation=I-0013\n"
        "game=DarkSoulsIII.exe 1.15.2.0 / BuildID 10167187\n"
        "executable_sha256=EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C\n"
        "operating_condition=standard windowed mode only\n"
        "qpc_frequency=%lld\n"
        "coordinate_order=x,z_vertical,y\n"
        "matrix=60-off,240-off,240-on,60-on-optional\n"
        "maximum_samples=%zu\n",
        static_cast<long long>(g_frequency.QuadPart),
        kMaximumCaptureSamples);
    return std::fclose(info) == 0;
}

const wchar_t* MatrixCellFileName(MatrixCell cell) {
    switch (cell) {
        case MatrixCell::fps60_off: return L"60-off";
        case MatrixCell::fps240_off: return L"240-off";
        case MatrixCell::fps240_on: return L"240-on";
        case MatrixCell::fps60_on: return L"60-on";
        case MatrixCell::complete: return L"complete";
    }
    return L"unknown";
}

bool ExportCapture(bool completed_by_operator) {
    const std::uint64_t raw_count =
        g_capture_sample_count.load(std::memory_order_acquire);
    const std::size_t count = static_cast<std::size_t>(
        raw_count > kMaximumCaptureSamples ? kMaximumCaptureSamples : raw_count);
    const bool overflow = g_capture_overflow.load(std::memory_order_acquire);
    const std::uint64_t attempt =
        g_capture_attempt.load(std::memory_order_acquire);
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(
        path, _countof(path), _TRUNCATE, L"%s\\%s-attempt-%llu%s.csv",
        g_capture_session_dir, MatrixCellFileName(g_active_capture_cell),
        static_cast<unsigned long long>(attempt),
        completed_by_operator ? L"" : L"-partial");
    FILE* file = nullptr;
    if (!g_capture_storage_ready ||
        _wfopen_s(&file, path, L"wb") != 0 || !file) {
        g_capture_state.store(CaptureState::failed, std::memory_order_release);
        return false;
    }
    std::fprintf(
        file,
        "qpc,update_index,player,transform,delta,x,z,y,valid,target_fps,correction\n");
    for (std::size_t index = 0; index < count; ++index) {
        const TrajectorySample& sample = g_capture_samples[index];
        std::fprintf(
            file, "%lld,%llu,0x%llX,0x%llX,%.9g,%.9g,%.9g,%.9g,%u,%.0f,%u\n",
            static_cast<long long>(sample.qpc),
            static_cast<unsigned long long>(sample.update_index),
            static_cast<unsigned long long>(sample.player),
            static_cast<unsigned long long>(sample.transform), sample.delta,
            sample.x, sample.z, sample.y, sample.valid,
            g_capture_target_fps, g_capture_correction ? 1u : 0u);
    }
    const bool write_ok = std::fclose(file) == 0;
    if (write_ok && completed_by_operator && !overflow && count >= 2) {
        const auto next = static_cast<MatrixCell>(
            static_cast<std::uint32_t>(g_active_capture_cell) + 1u);
        g_matrix_cell.store(next, std::memory_order_release);
        g_capture_state.store(CaptureState::saved, std::memory_order_release);
        return true;
    }
    g_capture_state.store(CaptureState::failed, std::memory_order_release);
    return false;
}

void QueueCapture(CaptureCommand command) {
    g_capture_command.store(command, std::memory_order_relaxed);
    g_capture_request_generation.fetch_add(1, std::memory_order_release);
}

void ProcessPendingCaptureControl() {
    const std::uint64_t generation =
        g_capture_request_generation.load(std::memory_order_acquire);
    if (generation ==
        g_capture_applied_generation.load(std::memory_order_relaxed)) return;
    const CaptureCommand command =
        g_capture_command.load(std::memory_order_relaxed);
    const CaptureState state =
        g_capture_state.load(std::memory_order_acquire);
    bool applied = false;
    if (command == CaptureCommand::start &&
        (state == CaptureState::idle || state == CaptureState::saved ||
         state == CaptureState::failed)) {
        const MatrixCell cell = g_matrix_cell.load(std::memory_order_acquire);
        const CadenceMode mode = g_cadence_mode.load(std::memory_order_acquire);
        const float target = g_target_fps.load(std::memory_order_acquire);
        const bool correction =
            g_correction_active.load(std::memory_order_acquire);
        const PlayerSnapshot snapshot = ReadPlayerSnapshot();
        if (g_capture_storage_ready && snapshot.position_valid &&
            snapshot.player != 0 && snapshot.transform != 0 &&
            CaptureSettingsMatch(cell, mode, target, correction)) {
            g_active_capture_cell = cell;
            g_capture_target_fps = target;
            g_capture_correction = correction;
            g_capture_player = snapshot.player;
            g_capture_transform = snapshot.transform;
            g_capture_sample_count.store(0, std::memory_order_relaxed);
            g_capture_invalid_count.store(0, std::memory_order_relaxed);
            g_capture_overflow.store(false, std::memory_order_relaxed);
            g_capture_attempt.fetch_add(1, std::memory_order_relaxed);
            g_capture_state.store(CaptureState::recording,
                                  std::memory_order_release);
            applied = true;
        }
    } else if (command == CaptureCommand::stop &&
               state == CaptureState::recording) {
        g_capture_state.store(CaptureState::exporting,
                              std::memory_order_release);
        applied = true;
    }
    g_last_control.store(applied ? ControlResult::applied
                                 : ControlResult::invalid,
                         std::memory_order_release);
    g_capture_applied_generation.store(generation,
                                       std::memory_order_release);
}

void AppendCaptureSample(std::uint64_t update_index, float delta,
                         bool delta_valid) {
    if (g_capture_state.load(std::memory_order_acquire) !=
        CaptureState::recording) return;
    const std::uint64_t index =
        g_capture_sample_count.fetch_add(1, std::memory_order_relaxed);
    if (index >= kMaximumCaptureSamples) {
        g_capture_overflow.store(true, std::memory_order_release);
        g_capture_state.store(CaptureState::exporting,
                              std::memory_order_release);
        return;
    }
    const PlayerSnapshot snapshot = ReadPlayerSnapshot();
    const bool valid = delta_valid && snapshot.position_valid &&
        snapshot.player == g_capture_player &&
        snapshot.transform == g_capture_transform;
    TrajectorySample sample{};
    sample.qpc = Qpc();
    sample.update_index = update_index;
    sample.player = snapshot.player;
    sample.transform = snapshot.transform;
    sample.delta = delta;
    sample.x = snapshot.position[0];
    sample.z = snapshot.position[1];
    sample.y = snapshot.position[2];
    sample.valid = valid ? 1u : 0u;
    g_capture_samples[static_cast<std::size_t>(index)] = sample;
    if (!valid) {
        g_capture_invalid_count.fetch_add(1, std::memory_order_relaxed);
    }
}

void QueueCadence(CadenceMode mode, float fps) {
    g_requested_fps.store(fps, std::memory_order_relaxed);
    g_requested_mode.store(mode, std::memory_order_relaxed);
    g_cadence_request_generation.fetch_add(1, std::memory_order_release);
}

void QueueCorrection(bool enabled) {
    g_requested_correction.store(enabled, std::memory_order_relaxed);
    g_correction_request_generation.fetch_add(1, std::memory_order_release);
}

bool ReadTimingPair(std::uintptr_t object, TimingPair* pair) {
    return pair && SafeRead(object + 0x354, &pair->fps) &&
           SafeRead(object + 0x358, &pair->enable);
}

bool WriteTimingPair(std::uintptr_t object, const TimingPair& pair) {
    const unsigned char disabled = 0;
    const bool write_ok = SafeWrite(object + 0x358, disabled) &&
                          SafeWrite(object + 0x354, pair.fps) &&
                          SafeWrite(object + 0x358, pair.enable);
    TimingPair actual{};
    return write_ok && ReadTimingPair(object, &actual) &&
           SameFloatBits(actual.fps, pair.fps) &&
           actual.enable == pair.enable;
}

ControlResult ApplyCadence(const CadenceRequest& request) {
    if (request.mode == CadenceMode::unlimited) {
        LogEvent("unlimited_unsupported", ControlResult::unsupported);
        return ControlResult::unsupported;
    }
    CriticalSectionGuard timing_guard(&g_timing_lock);
    std::uintptr_t object = 0;
    if (!ReadPointer(g_image + kRvaCentralGlobal, &object)) {
        return ControlResult::failed;
    }
    TimingPair before{};
    if (!ReadTimingPair(object, &before)) return ControlResult::failed;
    if (!g_original_timing_valid) {
        g_timing_object = object;
        g_original_timing = before;
        g_original_timing_valid = true;
    } else if (object != g_timing_object) {
        FailClosed("timing_object_changed");
        return ControlResult::failed;
    }
    const TimingWritePlan plan = ResolveCadenceRequest(
        request, g_original_timing.fps, g_original_timing.enable);
    if (!plan.supported) return ControlResult::invalid;
    const TimingPair desired{plan.fps, plan.enable};
    if (!WriteTimingPair(object, desired)) {
        const bool rollback = WriteTimingPair(object, before);
        if (!rollback) FailClosed("cadence_rollback_failed");
        LogTransition("apply_failed", before, desired, ControlResult::failed);
        return ControlResult::failed;
    }
    TimingPair after{};
    ReadTimingPair(object, &after);
    g_timing_modified = plan.use_override;
    g_timing_restored.store(!plan.use_override, std::memory_order_release);
    g_cadence_mode.store(request.mode, std::memory_order_release);
    g_target_fps.store(plan.use_override ? request.fps : 0.0f,
                       std::memory_order_release);
    g_state.store(StateForCadence(request), std::memory_order_release);
    g_cadence_transitions.fetch_add(1, std::memory_order_relaxed);
    LogTransition(plan.use_override ? "numeric" : "native", before, after,
                  ControlResult::applied);
    LogEvent(plan.use_override ? "cadence_numeric" : "cadence_native",
             ControlResult::applied);
    return ControlResult::applied;
}

bool RestoreTimingExact() {
    CriticalSectionGuard timing_guard(&g_timing_lock);
    if (!g_original_timing_valid || g_timing_object == 0) {
        g_timing_restored.store(true, std::memory_order_release);
        return true;
    }
    TimingPair before{};
    ReadTimingPair(g_timing_object, &before);
    const bool success = WriteTimingPair(g_timing_object, g_original_timing);
    TimingPair after{};
    ReadTimingPair(g_timing_object, &after);
    LogTransition("restore", before, after,
                  success ? ControlResult::applied : ControlResult::failed);
    if (success) g_timing_modified = false;
    g_timing_restored.store(success, std::memory_order_release);
    return success;
}

void ProcessPendingControls() {
    const std::uint64_t cadence_generation =
        g_cadence_request_generation.load(std::memory_order_acquire);
    if (cadence_generation !=
        g_cadence_applied_generation.load(std::memory_order_relaxed)) {
        const CadenceRequest request{
            g_requested_mode.load(std::memory_order_relaxed),
            g_requested_fps.load(std::memory_order_relaxed)};
        const ControlResult result = ApplyCadence(request);
        g_last_control.store(result, std::memory_order_release);
        g_cadence_applied_generation.store(cadence_generation,
                                           std::memory_order_release);
    }
    const std::uint64_t correction_generation =
        g_correction_request_generation.load(std::memory_order_acquire);
    if (correction_generation !=
        g_correction_applied_generation.load(std::memory_order_relaxed)) {
        const bool enabled =
            g_requested_correction.load(std::memory_order_relaxed);
        if (g_watch_active.load(std::memory_order_acquire)) {
            FailClosed("correction_transition_while_armed");
        } else {
            g_correction_active.store(enabled, std::memory_order_release);
            g_distance_previous = {};
            g_distance_current = {};
            g_correction_transitions.fetch_add(1, std::memory_order_relaxed);
            LogEvent(enabled ? "correction_on" : "correction_off",
                     ControlResult::applied);
        }
        g_correction_applied_generation.store(correction_generation,
                                              std::memory_order_release);
    }
}

bool ApplyWatchCommand(WatchCommand command, DWORD tid,
                       const std::uintptr_t addresses[3]) {
    HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                                   THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION,
                               FALSE, tid);
    if (!thread) return false;
    bool suspended = false;
    bool success = false;
    if (SuspendThread(thread) != static_cast<DWORD>(-1)) {
        suspended = true;
        CONTEXT context{};
        context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(thread, &context)) {
            if (command == WatchCommand::arm) {
                EnterCriticalSection(&g_watch_lock);
                g_watch_original = {context.Dr0, context.Dr1, context.Dr2,
                                    context.Dr3, context.Dr6, context.Dr7};
                g_watch_original_valid = true;
                LeaveCriticalSection(&g_watch_lock);
                context.Dr0 = addresses[0];
                context.Dr1 = addresses[1];
                context.Dr2 = addresses[2];
                context.Dr3 = 0;
                context.Dr6 = 0;
                context.Dr7 = ConfigurePrototypeDr7(context.Dr7);
                success = SetThreadContext(thread, &context) != FALSE;
            } else if (command == WatchCommand::disarm) {
                EnterCriticalSection(&g_watch_lock);
                const DebugRegisterState original = g_watch_original;
                const bool valid = g_watch_original_valid;
                LeaveCriticalSection(&g_watch_lock);
                if (valid) {
                    context.Dr0 = original.dr0;
                    context.Dr1 = original.dr1;
                    context.Dr2 = original.dr2;
                    context.Dr3 = original.dr3;
                    context.Dr6 = original.dr6;
                    context.Dr7 = original.dr7;
                    success = SetThreadContext(thread, &context) != FALSE;
                    CONTEXT restored{};
                    restored.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    success = success && GetThreadContext(thread, &restored) &&
                              SameDebugState(context, restored);
                }
            }
        }
    }
    if (suspended) ResumeThread(thread);
    CloseHandle(thread);
    return success;
}

DWORD WINAPI WatchController(void*) {
    for (;;) {
        if (WaitForSingleObject(g_watch_request, INFINITE) != WAIT_OBJECT_0) {
            return 1;
        }
        WatchCommand command = WatchCommand::none;
        DWORD tid = 0;
        std::uintptr_t addresses[3]{};
        EnterCriticalSection(&g_watch_lock);
        command = g_watch_command;
        tid = g_watch_target_tid;
        std::memcpy(addresses, g_watch_addresses, sizeof(addresses));
        g_watch_command = WatchCommand::none;
        LeaveCriticalSection(&g_watch_lock);
        if (command == WatchCommand::stop) {
            g_watch_result.store(1, std::memory_order_release);
            SetEvent(g_watch_done);
            return 0;
        }
        const bool success = ApplyWatchCommand(command, tid, addresses);
        if (success && command == WatchCommand::arm) {
            g_watch_active_tid.store(tid, std::memory_order_release);
            g_watch_active.store(true, std::memory_order_release);
            g_debug_restored.store(false, std::memory_order_release);
        } else if (success && command == WatchCommand::disarm) {
            g_watch_active.store(false, std::memory_order_release);
            g_watch_active_tid.store(0, std::memory_order_release);
            EnterCriticalSection(&g_watch_lock);
            g_watch_original_valid = false;
            LeaveCriticalSection(&g_watch_lock);
            g_debug_restored.store(true, std::memory_order_release);
        }
        g_watch_result.store(success ? 1 : -1, std::memory_order_release);
        SetEvent(g_watch_done);
    }
}

bool IssueWatchCommand(WatchCommand command, DWORD tid,
                       const std::uintptr_t addresses[3]) {
    if (!g_watch_request || !g_watch_done || !g_watch_thread) return false;
    ResetEvent(g_watch_done);
    g_watch_result.store(0, std::memory_order_release);
    EnterCriticalSection(&g_watch_lock);
    g_watch_command = command;
    g_watch_target_tid = tid;
    if (addresses) {
        std::memcpy(g_watch_addresses, addresses, sizeof(g_watch_addresses));
    }
    LeaveCriticalSection(&g_watch_lock);
    SetEvent(g_watch_request);
    return WaitForSingleObject(g_watch_done, 1000) == WAIT_OBJECT_0 &&
           g_watch_result.load(std::memory_order_acquire) == 1;
}

bool ArmWatch(const PlayerSnapshot& snapshot, void* context) {
    if (!snapshot.position_valid || !g_distance_previous.valid ||
        !g_distance_current.valid ||
        g_watch_active.load(std::memory_order_acquire)) return false;
    const std::uintptr_t addresses[3] = {
        g_image + kRvaComparison,
        g_image + kRvaRecurrenceExit,
        snapshot.transform + 0x80,
    };
    EnterCriticalSection(&g_watch_lock);
    g_watch_shared_context = reinterpret_cast<std::uintptr_t>(context);
    g_watch_transform_base = snapshot.transform + 0xE0;
    g_watch_update_index = g_distance_current.index;
    g_watch_hits = 0;
    LeaveCriticalSection(&g_watch_lock);
    const bool armed = IssueWatchCommand(WatchCommand::arm,
                                         GetCurrentThreadId(), addresses);
    if (!armed) FailClosed("debug_arm_failed");
    return armed;
}

bool DisarmWatch() {
    if (!g_watch_active.load(std::memory_order_acquire)) {
        g_debug_restored.store(true, std::memory_order_release);
        return true;
    }
    const bool disarmed = IssueWatchCommand(
        WatchCommand::disarm,
        g_watch_active_tid.load(std::memory_order_acquire), nullptr);
    if (!disarmed) FailClosed("debug_restore_failed");
    return disarmed;
}

LONG CALLBACK WatchExceptionHandler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord ||
        pointers->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
        !g_watch_active.load(std::memory_order_acquire) ||
        GetCurrentThreadId() !=
            g_watch_active_tid.load(std::memory_order_acquire)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CONTEXT* context = pointers->ContextRecord;
    const DWORD64 hit_mask = context->Dr6 & 0xFull;
    if (hit_mask == 0) return EXCEPTION_CONTINUE_SEARCH;
    if (hit_mask & 3ull) {
        context->EFlags |= 0x10000u;
        if (context->Rdi + 0xE0 != g_watch_transform_base) {
            context->Dr6 = 0;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    if (++g_watch_hits > 8) {
        context->Dr7 = 0;
        context->Dr6 = 0;
        FailClosed("debug_hit_bound_exceeded");
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    const bool comparison = context->Rip == g_image + kRvaComparison;
    const bool recurrence_exit =
        context->Rip == g_image + kRvaRecurrenceExit;
    const bool commit = (hit_mask & 4ull) != 0;
    if (comparison) {
        std::uintptr_t player = 0;
        std::uintptr_t modules = 0;
        std::uintptr_t owner = 0;
        const bool chain_ok = context->Rbx != 0 &&
            SafeRead(context->Rbx + 8, &player) && player != 0 &&
            SafeRead(player + 0x1F90, &modules) && modules != 0 &&
            SafeRead(modules + 0x68, &owner) && owner != 0 &&
            owner + 0xE0 == g_watch_transform_base;
        ComparisonGuardInput input{};
        bool input_ok = chain_ok &&
            SafeCopy(context->Rip, input.opcode, sizeof(input.opcode)) &&
            SafeCopy(owner + 0x80, input.owner_position, 12) &&
            SafeCopy(context->Rsp + 0x30, input.getter_position, 12);
        if (input_ok) {
            std::memcpy(input.old_position, &context->Xmm6, 12);
            input.previous = g_distance_previous;
            input.current = g_distance_current;
            input.expected_index = g_watch_update_index;
            input.expected_transform = g_watch_transform_base - 0xE0;
            DWORD raw_bits = static_cast<DWORD>(context->Xmm0.Low);
            std::memcpy(&input.raw, &raw_bits, sizeof(input.raw));
            ComparisonDecision decision{};
            input_ok = EvaluateComparison(input, &decision);
            if (input_ok) {
                g_comparisons.fetch_add(1, std::memory_order_relaxed);
                DWORD corrected_bits = 0;
                std::memcpy(&corrected_bits, &decision.normalized,
                            sizeof(corrected_bits));
                context->Xmm0.Low =
                    (context->Xmm0.Low & 0xFFFFFFFF00000000ull) |
                    corrected_bits;
                g_corrections.fetch_add(1, std::memory_order_relaxed);
            }
        }
        if (!input_ok) FailClosed("comparison_guard_failed");
    } else if (!recurrence_exit && !commit) {
        FailClosed("unexpected_debug_hit");
    }
    context->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}

bool InitializeWatch() {
    g_watch_request = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    g_watch_done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_watch_request || !g_watch_done) return false;
    g_watch_veh = AddVectoredExceptionHandler(1, WatchExceptionHandler);
    if (!g_watch_veh) return false;
    g_watch_thread = CreateThread(nullptr, 0, WatchController,
                                  nullptr, 0, nullptr);
    return g_watch_thread != nullptr;
}

bool ShutdownWatch() {
    bool success = DisarmWatch();
    if (g_watch_thread) {
        success = IssueWatchCommand(WatchCommand::stop, 0, nullptr) && success;
        WaitForSingleObject(g_watch_thread, 1000);
        CloseHandle(g_watch_thread);
        g_watch_thread = nullptr;
    }
    if (g_watch_veh) {
        RemoveVectoredExceptionHandler(g_watch_veh);
        g_watch_veh = nullptr;
    }
    if (g_watch_request) CloseHandle(g_watch_request);
    if (g_watch_done) CloseHandle(g_watch_done);
    g_watch_request = nullptr;
    g_watch_done = nullptr;
    g_debug_restored.store(success, std::memory_order_release);
    return success;
}

void __fastcall HookSpatialBoundary(void* self, void* context) {
    const std::uintptr_t caller =
        reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const std::uintptr_t caller_rva = caller >= g_image ? caller - g_image : 0;
    if (g_correction_active.load(std::memory_order_acquire) &&
        caller_rva == kCallerDisarm &&
        g_watch_active.load(std::memory_order_acquire)) {
        std::uintptr_t watched_context = 0;
        EnterCriticalSection(&g_watch_lock);
        watched_context = g_watch_shared_context;
        LeaveCriticalSection(&g_watch_lock);
        if (watched_context == reinterpret_cast<std::uintptr_t>(context)) {
            DisarmWatch();
        }
    }
    g_original_spatial_boundary(self, context);
    if (g_correction_active.load(std::memory_order_acquire) &&
        caller_rva == kCallerArm) {
        ArmWatch(ReadPlayerSnapshot(), context);
    }
}

void __fastcall HookBroadUpdate(void* self, void* context) {
    ProcessPendingControls();
    ProcessPendingCaptureControl();
    const bool correcting =
        g_correction_active.load(std::memory_order_acquire);
    const bool capturing =
        g_capture_state.load(std::memory_order_acquire) ==
        CaptureState::recording;
    float delta = 0.0f;
    PlayerSnapshot before{};
    bool delta_valid = false;
    if (correcting || capturing) {
        delta_valid = context &&
            SafeRead(reinterpret_cast<std::uintptr_t>(context) + 0x08, &delta) &&
            std::isfinite(delta) && delta > 0.0f;
    }
    if (correcting) {
        std::uintptr_t central = 0;
        float central_delta = 0.0f;
        const bool central_valid =
            ReadPointer(g_image + kRvaCentralGlobal, &central) &&
            SafeRead(central + 0x268, &central_delta) &&
            std::isfinite(central_delta);
        if (!delta_valid || !central_valid ||
            std::fabs(delta - central_delta) > 1e-7f) {
            FailClosed("central_delta_mismatch");
        }
        before = ReadPlayerSnapshot();
    }
    g_original_broad_update(self, context);
    const long long now = Qpc();
    if (g_fps_window.Add(now, g_frequency.QuadPart)) {
        g_actual_fps.store(g_fps_window.last_fps, std::memory_order_release);
    }
    const std::uint64_t index =
        g_updates.fetch_add(1, std::memory_order_relaxed) + 1;
    if (capturing) AppendCaptureSample(index, delta, delta_valid);
    if (correcting && delta_valid &&
        g_correction_active.load(std::memory_order_acquire)) {
        g_distance_previous = g_distance_current;
        g_distance_current.index = index;
        g_distance_current.delta = delta;
        g_distance_current.transform = before.transform;
        std::memcpy(g_distance_current.position, before.position,
                    sizeof(g_distance_current.position));
        g_distance_current.valid = before.position_valid;
    }
}

bool HashExecutable(const wchar_t* path,
                    std::array<unsigned char, 32>* digest) {
    if (!path || !digest) return false;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ |
        FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD object_size = 0;
    DWORD result_size = 0;
    std::array<unsigned char, 4096> buffer{};
    unsigned char* object = nullptr;
    bool ok = BCryptOpenAlgorithmProvider(
        &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
        BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<unsigned char*>(&object_size),
                          sizeof(object_size), &result_size, 0) == 0;
    if (ok) {
        object = new (std::nothrow) unsigned char[object_size];
        ok = object != nullptr &&
             BCryptCreateHash(algorithm, &hash, object, object_size,
                              nullptr, 0, 0) == 0;
    }
    while (ok) {
        DWORD read = 0;
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()),
                      &read, nullptr)) {
            ok = false;
            break;
        }
        if (read == 0) break;
        ok = BCryptHashData(hash, buffer.data(), read, 0) == 0;
    }
    if (ok) {
        ok = BCryptFinishHash(hash, digest->data(),
                              static_cast<ULONG>(digest->size()), 0) == 0;
    }
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    delete[] object;
    CloseHandle(file);
    return ok;
}

bool ValidateRuntimeImage() {
    wchar_t process_path[MAX_PATH * 4]{};
    if (!GetModuleFileNameW(nullptr, process_path, _countof(process_path))) {
        return false;
    }
    const wchar_t* filename = std::wcsrchr(process_path, L'\\');
    filename = filename ? filename + 1 : process_path;
    if (_wcsicmp(filename, L"DarkSoulsIII.exe") != 0) return false;

    constexpr std::array<unsigned char, 32> expected_hash = {
        0xEF,0x5E,0x07,0xC5,0x52,0x22,0xF1,0x4F,
        0xFD,0xDE,0xCF,0x2C,0x72,0x4A,0x0C,0x2A,
        0x95,0xCE,0xF0,0xD4,0xDA,0x0E,0x07,0x5B,
        0x0D,0xE0,0xBB,0x10,0x8B,0x69,0x49,0x8C,
    };
    std::array<unsigned char, 32> actual_hash{};
    if (!HashExecutable(process_path, &actual_hash) ||
        actual_hash != expected_hash) {
        return false;
    }

    struct Signature {
        std::uintptr_t rva;
        unsigned char bytes[15];
    };
    constexpr Signature signatures[] = {
        {kRvaBroadUpdate,
         {0x48,0x8B,0xC4,0x57,0x48,0x83,0xEC,0x50,0x48,0xC7,0x40,0xC8,0xFE,0xFF,0xFF}},
        {kRvaSpatialBoundary,
         {0x40,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8B,0xEC}},
        {kRvaComparison,
         {0x0F,0x2F,0xF8,0x76,0x2F,0xF3,0x0F,0x10,0x83,0x5C,0x02,0x00,0x00,0x0F,0x2F}},
        {kRvaRecurrenceExit,
         {0x48,0x8B,0x4C,0x24,0x40,0x48,0x33,0xCC,0xE8,0xDE,0xA2,0xA0,0x01,0x4C,0x8D}},
    };
    unsigned char actual[15]{};
    for (const Signature& signature : signatures) {
        if (!SafeCopy(g_image + signature.rva, actual, sizeof(actual)) ||
            std::memcmp(actual, signature.bytes, sizeof(actual)) != 0) {
            return false;
        }
    }
    return true;
}

bool InstallHook(std::uintptr_t rva, void* detour, void** original) {
    void* target = reinterpret_cast<void*>(g_image + rva);
    const MH_STATUS created = MH_CreateHook(target, detour, original);
    const MH_STATUS enabled = created == MH_OK
        ? MH_EnableHook(target) : created;
    return created == MH_OK && enabled == MH_OK;
}

bool VerifyActiveTiming() {
    CriticalSectionGuard timing_guard(&g_timing_lock);
    if (!g_timing_modified) return true;
    std::uintptr_t object = 0;
    TimingPair actual{};
    return ReadPointer(g_image + kRvaCentralGlobal, &object) &&
           object == g_timing_object && ReadTimingPair(object, &actual) &&
           SameFloatBits(actual.fps,
               g_target_fps.load(std::memory_order_acquire)) &&
           actual.enable == 1;
}

DWORD WINAPI RuntimeThread(void*) {
    g_image = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    QueryPerformanceFrequency(&g_frequency);
    InitializeCriticalSection(&g_timing_lock);
    InitializeCriticalSection(&g_watch_lock);
    g_stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (g_stop_event && g_stop_requested.load(std::memory_order_acquire)) {
        SetEvent(g_stop_event);
    }

    bool ok = g_stop_event != nullptr && ValidateRuntimeImage();
    if (ok) {
        g_capture_storage_ready = InitializeCaptureStorage();
        ok = g_capture_storage_ready && InitializeWatch();
    }
    const MH_STATUS minhook = ok ? MH_Initialize() : MH_ERROR_NOT_INITIALIZED;
    ok = ok && minhook == MH_OK;
    if (ok) {
        ok = InstallHook(kRvaBroadUpdate,
                         reinterpret_cast<void*>(&HookBroadUpdate),
                         reinterpret_cast<void**>(&g_original_broad_update)) &&
             InstallHook(kRvaSpatialBoundary,
                         reinterpret_cast<void*>(&HookSpatialBoundary),
                         reinterpret_cast<void**>(&g_original_spatial_boundary));
    }

    if (ok) {
        g_state.store(RuntimeState::standby, std::memory_order_release);
        g_last_control.store(ControlResult::applied, std::memory_order_release);
    } else {
        g_state.store(RuntimeState::failed, std::memory_order_release);
        g_guard_failed.store(true, std::memory_order_release);
        g_guard_failures.fetch_add(1, std::memory_order_relaxed);
        g_last_control.store(ControlResult::failed, std::memory_order_release);
    }
    g_ready.store(true, std::memory_order_release);

    bool running = ok;
    while (running) {
        const DWORD wait = WaitForSingleObject(g_stop_event, 250);
        if (wait == WAIT_OBJECT_0) {
            running = false;
        } else if (wait == WAIT_TIMEOUT) {
            if (g_capture_state.load(std::memory_order_acquire) ==
                CaptureState::exporting) {
                ExportCapture(
                    !g_capture_overflow.load(std::memory_order_acquire));
            }
            if (!VerifyActiveTiming()) {
                FailClosed("timing_drift");
                running = false;
            }
        } else {
            FailClosed("control_wait_failed");
            running = false;
        }
    }

    g_state.store(RuntimeState::stopping, std::memory_order_release);
    g_correction_active.store(false, std::memory_order_release);
    const bool watch_ok = DisarmWatch();
    bool hooks_ok = true;
    if (minhook == MH_OK) {
        hooks_ok = MH_DisableHook(MH_ALL_HOOKS) == MH_OK;
        hooks_ok = MH_Uninitialize() == MH_OK && hooks_ok;
    }
    const CaptureState capture_state =
        g_capture_state.load(std::memory_order_acquire);
    if (capture_state == CaptureState::recording) {
        g_capture_state.store(CaptureState::exporting,
                              std::memory_order_release);
        ExportCapture(false);
    } else if (capture_state == CaptureState::exporting) {
        ExportCapture(!g_capture_overflow.load(std::memory_order_acquire));
    }
    const bool watch_shutdown_ok = ShutdownWatch();
    const bool timing_ok = RestoreTimingExact();
    const bool complete = watch_ok && hooks_ok && watch_shutdown_ok && timing_ok;
    g_state.store(complete ? RuntimeState::stopped : RuntimeState::failed,
                  std::memory_order_release);
    g_last_control.store(complete ? ControlResult::applied
                                  : ControlResult::failed,
                         std::memory_order_release);
    if (!complete) {
        g_guard_failed.store(true, std::memory_order_release);
        g_guard_failures.fetch_add(1, std::memory_order_relaxed);
    }
    DeleteCriticalSection(&g_watch_lock);
    DeleteCriticalSection(&g_timing_lock);
    return complete ? 0u : 1u;
}

}  // namespace

namespace i0012 {

bool Start(HMODULE module) {
    (void)module;
    bool expected = false;
    if (!g_started.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel)) {
        return false;
    }
    g_runtime_thread = CreateThread(
        nullptr, 0, RuntimeThread, nullptr, 0, nullptr);
    if (!g_runtime_thread) {
        g_state.store(RuntimeState::failed, std::memory_order_release);
        g_guard_failed.store(true, std::memory_order_release);
        g_guard_failures.fetch_add(1, std::memory_order_relaxed);
        g_last_control.store(ControlResult::failed, std::memory_order_release);
        g_ready.store(true, std::memory_order_release);
        return false;
    }
    return true;
}

void RequestNumericCadence(float fps) {
    if (!IsNumericFps(fps) || !g_ready.load(std::memory_order_acquire) ||
        g_state.load(std::memory_order_acquire) == RuntimeState::failed ||
        g_state.load(std::memory_order_acquire) == RuntimeState::stopped ||
        g_capture_state.load(std::memory_order_acquire) ==
            CaptureState::recording ||
        g_capture_state.load(std::memory_order_acquire) ==
            CaptureState::exporting) {
        g_last_control.store(ControlResult::invalid, std::memory_order_release);
        return;
    }
    QueueCadence(CadenceMode::numeric, ClampSliderFps(fps));
    g_last_control.store(ControlResult::queued, std::memory_order_release);
}

void RequestNativeCadence() {
    if (!g_ready.load(std::memory_order_acquire) ||
        g_capture_state.load(std::memory_order_acquire) ==
            CaptureState::recording ||
        g_capture_state.load(std::memory_order_acquire) ==
            CaptureState::exporting) {
        g_last_control.store(ControlResult::invalid, std::memory_order_release);
        return;
    }
    QueueCadence(CadenceMode::native, 0.0f);
    g_last_control.store(ControlResult::queued, std::memory_order_release);
}

void RequestCorrection(bool enabled) {
    if (!g_ready.load(std::memory_order_acquire) ||
        g_state.load(std::memory_order_acquire) == RuntimeState::failed ||
        g_state.load(std::memory_order_acquire) == RuntimeState::stopped ||
        g_capture_state.load(std::memory_order_acquire) ==
            CaptureState::recording ||
        g_capture_state.load(std::memory_order_acquire) ==
            CaptureState::exporting) {
        g_last_control.store(ControlResult::invalid, std::memory_order_release);
        return;
    }
    QueueCorrection(enabled);
    g_last_control.store(ControlResult::queued, std::memory_order_release);
}

void RequestCaptureStart() {
    if (!g_ready.load(std::memory_order_acquire) ||
        g_state.load(std::memory_order_acquire) == RuntimeState::failed ||
        g_state.load(std::memory_order_acquire) == RuntimeState::stopped) {
        g_last_control.store(ControlResult::invalid, std::memory_order_release);
        return;
    }
    QueueCapture(CaptureCommand::start);
    g_last_control.store(ControlResult::queued, std::memory_order_release);
}

void RequestCaptureStop() {
    if (g_capture_state.load(std::memory_order_acquire) !=
        CaptureState::recording) {
        g_last_control.store(ControlResult::invalid, std::memory_order_release);
        return;
    }
    QueueCapture(CaptureCommand::stop);
    g_last_control.store(ControlResult::queued, std::memory_order_release);
}

void RequestStop() {
    g_stop_requested.store(true, std::memory_order_release);
    if (g_stop_event) SetEvent(g_stop_event);
}

bool StopAndWait(unsigned long timeout_ms) {
    RequestStop();
    HANDLE thread = g_runtime_thread;
    if (!thread) return true;
    const DWORD wait = WaitForSingleObject(thread, timeout_ms);
    if (wait != WAIT_OBJECT_0) return false;
    CloseHandle(thread);
    g_runtime_thread = nullptr;
    if (g_stop_event) {
        CloseHandle(g_stop_event);
        g_stop_event = nullptr;
    }
    return true;
}

RuntimeStatus GetStatus() {
    RuntimeStatus status{};
    status.state = g_state.load(std::memory_order_acquire);
    status.cadence_mode = g_cadence_mode.load(std::memory_order_acquire);
    status.last_control_result =
        g_last_control.load(std::memory_order_acquire);
    status.ready = g_ready.load(std::memory_order_acquire);
    status.correction_active =
        g_correction_active.load(std::memory_order_acquire);
    status.guard_failed = g_guard_failed.load(std::memory_order_acquire);
    status.timing_restored = g_timing_restored.load(std::memory_order_acquire);
    status.debug_restored = g_debug_restored.load(std::memory_order_acquire);
    status.target_fps = g_target_fps.load(std::memory_order_acquire);
    status.actual_fps = g_actual_fps.load(std::memory_order_acquire);
    status.updates = g_updates.load(std::memory_order_acquire);
    status.comparisons = g_comparisons.load(std::memory_order_acquire);
    status.corrections = g_corrections.load(std::memory_order_acquire);
    status.guard_failures = g_guard_failures.load(std::memory_order_acquire);
    status.cadence_transitions =
        g_cadence_transitions.load(std::memory_order_acquire);
    status.correction_transitions =
        g_correction_transitions.load(std::memory_order_acquire);
    status.capture_state =
        g_capture_state.load(std::memory_order_acquire);
    status.matrix_cell = g_matrix_cell.load(std::memory_order_acquire);
    status.capture_samples =
        g_capture_sample_count.load(std::memory_order_acquire);
    status.capture_invalid =
        g_capture_invalid_count.load(std::memory_order_acquire);
    status.capture_attempt =
        g_capture_attempt.load(std::memory_order_acquire);
    status.capture_overflow =
        g_capture_overflow.load(std::memory_order_acquire);
    return status;
}

}  // namespace i0012
