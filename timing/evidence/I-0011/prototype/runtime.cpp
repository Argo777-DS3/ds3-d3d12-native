#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <intrin.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "MinHook.h"
#include "runtime_core.hpp"
#include "public_status.hpp"
#include "../../I-0010/prototype/prototype_core.hpp"
#include "../../I-0010/prototype/debug_config.hpp"

namespace {

constexpr std::uintptr_t kRvaBroadUpdate = 0x00D57900;
constexpr std::uintptr_t kRvaSpatialBoundary = 0x02344730;
constexpr std::uintptr_t kRvaCentralGlobal = 0x0489DD10;
constexpr std::uintptr_t kRvaWorldChrManGlobal = 0x0477FDB8;
constexpr std::uintptr_t kRvaComparison = 0x005C73D8;
constexpr std::uintptr_t kRvaRecurrenceExit = 0x005C7435;
constexpr std::uintptr_t kCallerArm = 0x00A1429C;
constexpr std::uintptr_t kCallerDisarm = 0x00DE50EC;
constexpr int kOverlayWidth = 440;
constexpr int kOverlayHeight = 356;

HMODULE g_self = nullptr;
std::uintptr_t g_image = 0;
LARGE_INTEGER g_frequency{};
std::atomic<RuntimeState> g_state{RuntimeState::standby};
std::atomic<CadenceMode> g_cadence_mode{CadenceMode::native};
std::atomic<float> g_target_fps{0.0f};
std::atomic<float> g_actual_fps{0.0f};
std::atomic<bool> g_correction_active{false};
std::atomic<bool> g_guard_failed{false};
std::atomic<bool> g_overlay_requested{false};
std::atomic<bool> g_overlay_visible{false};
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

HANDLE g_stop_event = nullptr;
HANDLE g_ready_event = nullptr;
HANDLE g_complete_event = nullptr;
HANDLE g_show_event = nullptr;
HANDLE g_hide_event = nullptr;
HANDLE g_fix_on_event = nullptr;
HANDLE g_fix_off_event = nullptr;
HANDLE g_native_event = nullptr;
HANDLE g_unlimited_event = nullptr;
HANDLE g_status_event = nullptr;
HANDLE g_ack_event = nullptr;
HANDLE g_status_mapping = nullptr;
RuntimePublicStatus* g_public_status = nullptr;
HANDLE g_overlay_thread = nullptr;
HWND g_game_window = nullptr;
SliderDragState g_slider_drag{};

CRITICAL_SECTION g_log_lock{};
CRITICAL_SECTION g_timing_lock{};
FILE* g_events = nullptr;
FILE* g_transitions = nullptr;
wchar_t g_session_dir[MAX_PATH * 4]{};

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

void EventName(wchar_t* output, size_t count, const wchar_t* action) {
    _snwprintf_s(output, count, _TRUNCATE, L"Local\\DS3_I0011_%s_%lu",
                 action, GetCurrentProcessId());
}

void WriteMarker(const wchar_t* name, const char* detail) {
    if (g_session_dir[0] == L'\0') return;
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\%s",
                 g_session_dir, name);
    FILE* file = nullptr;
    if (_wfopen_s(&file, path, L"wb") == 0 && file) {
        std::fprintf(file, "%s\n", detail);
        std::fclose(file);
    }
}

void PublishStatus(ControlResult result = ControlResult::none) {
    if (result != ControlResult::none) {
        g_last_control.store(result, std::memory_order_release);
    }
    if (!g_public_status) return;
    RuntimePublicStatus status{};
    status.state = static_cast<std::uint32_t>(
        g_state.load(std::memory_order_acquire));
    status.cadence_mode = static_cast<std::uint32_t>(
        g_cadence_mode.load(std::memory_order_acquire));
    status.correction_active =
        g_correction_active.load(std::memory_order_acquire) ? 1u : 0u;
    status.overlay_visible =
        g_overlay_visible.load(std::memory_order_acquire) ? 1u : 0u;
    status.guard_failed =
        g_guard_failed.load(std::memory_order_acquire) ? 1u : 0u;
    status.timing_restored =
        g_timing_restored.load(std::memory_order_acquire) ? 1u : 0u;
    status.debug_restored =
        g_debug_restored.load(std::memory_order_acquire) ? 1u : 0u;
    status.last_control_result = static_cast<std::uint32_t>(
        g_last_control.load(std::memory_order_acquire));
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
    *g_public_status = status;
    MemoryBarrier();
}

void LogEvent(const char* event, ControlResult result) {
    PublishStatus(result);
    if (!g_events) return;
    EnterCriticalSection(&g_log_lock);
    std::fprintf(g_events,
        "%lld,%s,%u,%s,%u,%.3f,%.3f,%u,%u,%llu,%llu,%llu,%llu\n",
        Qpc(), event, static_cast<unsigned int>(result),
        RuntimeStateName(g_state.load(std::memory_order_acquire)),
        static_cast<unsigned int>(
            g_cadence_mode.load(std::memory_order_acquire)),
        static_cast<double>(g_target_fps.load(std::memory_order_acquire)),
        static_cast<double>(g_actual_fps.load(std::memory_order_acquire)),
        g_correction_active.load(std::memory_order_acquire) ? 1u : 0u,
        g_overlay_visible.load(std::memory_order_acquire) ? 1u : 0u,
        static_cast<unsigned long long>(
            g_updates.load(std::memory_order_acquire)),
        static_cast<unsigned long long>(
            g_comparisons.load(std::memory_order_acquire)),
        static_cast<unsigned long long>(
            g_corrections.load(std::memory_order_acquire)),
        static_cast<unsigned long long>(
            g_guard_failures.load(std::memory_order_acquire)));
    std::fflush(g_events);
    LeaveCriticalSection(&g_log_lock);
}

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

void LogTransition(const char* action, const TimingPair& before,
                   const TimingPair& after, ControlResult result) {
    if (!g_transitions) return;
    EnterCriticalSection(&g_log_lock);
    std::fprintf(g_transitions,
        "%lld,%s,0x%016llX,%.9g,%u,%.9g,%u,%u\n", Qpc(), action,
        static_cast<unsigned long long>(g_timing_object),
        static_cast<double>(before.fps),
        static_cast<unsigned int>(before.enable),
        static_cast<double>(after.fps),
        static_cast<unsigned int>(after.enable),
        static_cast<unsigned int>(result));
    std::fflush(g_transitions);
    LeaveCriticalSection(&g_log_lock);
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
    const bool correcting =
        g_correction_active.load(std::memory_order_acquire);
    float delta = 0.0f;
    PlayerSnapshot before{};
    bool delta_valid = false;
    if (correcting) {
        delta_valid = context &&
            SafeRead(reinterpret_cast<std::uintptr_t>(context) + 0x08, &delta) &&
            std::isfinite(delta) && delta > 0.0f;
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

RECT ToRect(const IntRect& value) {
    return {value.left, value.top, value.right, value.bottom};
}

void Fill(HDC dc, const IntRect& rect, COLORREF color) {
    RECT native = ToRect(rect);
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &native, brush);
    DeleteObject(brush);
}

void DrawButton(HDC dc, const IntRect& rect, const char* text,
                bool enabled, bool selected) {
    const COLORREF background = !enabled ? RGB(45, 48, 54)
        : selected ? RGB(40, 126, 172) : RGB(65, 70, 78);
    Fill(dc, rect, background);
    RECT native = ToRect(rect);
    SetTextColor(dc, enabled ? RGB(245, 247, 250) : RGB(145, 150, 158));
    DrawTextA(dc, text, -1, &native,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawOverlay(HDC dc) {
    RECT client{};
    GetClipBox(dc, &client);
    HBRUSH background = CreateSolidBrush(RGB(24, 27, 32));
    FillRect(dc, &client, background);
    DeleteObject(background);
    Fill(dc, {0, 0, kOverlayWidth, 4}, RGB(45, 149, 202));
    Fill(dc, {12, 42, 428, 96}, RGB(31, 35, 41));
    SetBkMode(dc, TRANSPARENT);
    HFONT font = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH, L"Segoe UI");
    HFONT title_font = CreateFontW(-21, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE,
                                   FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH,
                                   L"Segoe UI");
    HGDIOBJ old_font = SelectObject(dc, title_font);
    SetTextColor(dc, RGB(247, 249, 252));
    TextOutA(dc, 20, 14, "DS3 Remaster Runtime", 20);
    SelectObject(dc, font);
    SetTextColor(dc, RGB(115, 184, 220));
    TextOutA(dc, 315, 18, "FPS / TIMING", 12);

    char line[160]{};
    const RuntimeState state = g_state.load(std::memory_order_acquire);
    const CadenceMode mode = g_cadence_mode.load(std::memory_order_acquire);
    const float target = g_target_fps.load(std::memory_order_acquire);
    const float displayed_target = g_slider_drag.active
        ? g_slider_drag.preview_fps : target;
    const float actual = g_actual_fps.load(std::memory_order_acquire);
    _snprintf_s(line, _countof(line), _TRUNCATE, "State: %s",
                RuntimeStateName(state));
    SetTextColor(dc, state == RuntimeState::failed
        ? RGB(255, 105, 105) : RGB(142, 214, 166));
    TextOutA(dc, 20, 49, line, static_cast<int>(std::strlen(line)));
    if (mode == CadenceMode::numeric) {
        _snprintf_s(line, _countof(line), _TRUNCATE,
                    "FPS Limit: %.0f%s    Actual: %.1f", displayed_target,
                    g_slider_drag.active ? " (release to apply)" : "", actual);
    } else {
        _snprintf_s(line, _countof(line), _TRUNCATE,
                    "FPS Limit: Native    Actual: %.1f", actual);
    }
    SetTextColor(dc, RGB(229, 233, 239));
    TextOutA(dc, 20, 74, line, static_cast<int>(std::strlen(line)));
    TextOutA(dc, 20, 101, "30", 2);
    TextOutA(dc, 388, 101, "360", 3);

    const IntRect slider = SliderRect();
    Fill(dc, slider, RGB(63, 68, 76));
    const float slider_value = g_slider_drag.active ? g_slider_drag.preview_fps
        : mode == CadenceMode::numeric ? target : 60.0f;
    const int thumb_x = SliderXFromFps(slider_value, slider);
    Fill(dc, {slider.left, slider.top, thumb_x, slider.bottom},
         RGB(40, 126, 172));
    Fill(dc, {thumb_x - 4, slider.top - 4, thumb_x + 5, slider.bottom + 4},
         RGB(240, 243, 247));

    for (std::size_t i = 0; i < kFpsPresets.size(); ++i) {
        char preset[16]{};
        _snprintf_s(preset, _countof(preset), _TRUNCATE, "%d",
                    kFpsPresets[i]);
        DrawButton(dc, PresetRect(i), preset, true,
                   mode == CadenceMode::numeric &&
                   static_cast<int>(target) == kFpsPresets[i]);
    }
    DrawButton(dc, UnlimitedRect(), "Unlimited unavailable", false, false);
    DrawButton(dc, NativeRect(), "Native", true,
               mode == CadenceMode::native);
    DrawButton(dc, FixRect(),
               g_correction_active.load(std::memory_order_acquire)
                   ? "Locomotion Timing Fix: ON"
                   : "Locomotion Timing Fix: OFF",
               true, g_correction_active.load(std::memory_order_acquire));
    DrawButton(dc, StopRect(), "Restore & Stop", true, false);

    SetTextColor(dc, RGB(155, 161, 170));
    const char* note =
        g_last_control.load(std::memory_order_acquire) ==
                ControlResult::unsupported
            ? "Unlimited has no proven limiter-off state; no change applied."
            : "HOME closes overlay | controls apply at an update boundary";
    TextOutA(dc, 20, 330, note, static_cast<int>(std::strlen(note)));
    SelectObject(dc, old_font);
    DeleteObject(font);
    DeleteObject(title_font);
}

void RestoreGameFocus() {
    if (g_game_window && IsWindow(g_game_window)) {
        SetForegroundWindow(g_game_window);
    }
}

LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message,
                                   WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            DrawOverlay(dc);
            EndPaint(window, &paint);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            const int x = GET_X_LPARAM(lparam);
            const int y = GET_Y_LPARAM(lparam);
            const OverlayAction action = HitTestOverlay(x, y);
            if (action == OverlayAction::slider) {
                BeginSliderDrag(&g_slider_drag, x, SliderRect());
                SetCapture(window);
            } else if (action >= OverlayAction::preset_30 &&
                       action <= OverlayAction::preset_240) {
                const std::size_t index = static_cast<std::size_t>(
                    static_cast<std::uint32_t>(action) -
                    static_cast<std::uint32_t>(OverlayAction::preset_30));
                QueueCadence(CadenceMode::numeric,
                             static_cast<float>(kFpsPresets[index]));
            } else if (action == OverlayAction::unlimited) {
                QueueCadence(CadenceMode::unlimited, 0.0f);
            } else if (action == OverlayAction::native) {
                QueueCadence(CadenceMode::native, 0.0f);
            } else if (action == OverlayAction::toggle_fix) {
                QueueCorrection(
                    !g_correction_active.load(std::memory_order_acquire));
            } else if (action == OverlayAction::stop && g_stop_event) {
                SetEvent(g_stop_event);
            }
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        }
        case WM_MOUSEMOVE:
            if (g_slider_drag.active && (wparam & MK_LBUTTON) &&
                GetCapture() == window) {
                UpdateSliderDrag(&g_slider_drag, GET_X_LPARAM(lparam),
                                 SliderRect());
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONUP: {
            float committed_fps = 0.0f;
            if (CommitSliderDrag(&g_slider_drag, GET_X_LPARAM(lparam),
                                 SliderRect(), &committed_fps)) {
                QueueCadence(CadenceMode::numeric, committed_fps);
                InvalidateRect(window, nullptr, FALSE);
            }
            if (GetCapture() == window) ReleaseCapture();
            return 0;
        }
        case WM_CAPTURECHANGED:
            CancelSliderDrag(&g_slider_drag);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        case WM_CLOSE:
            g_overlay_requested.store(false, std::memory_order_release);
            ShowWindow(window, SW_HIDE);
            RestoreGameFocus();
            return 0;
        default:
            return DefWindowProcW(window, message, wparam, lparam);
    }
}

struct WindowSearch {
    DWORD pid = 0;
    HWND result = nullptr;
};

BOOL CALLBACK FindGameWindowCallback(HWND window, LPARAM parameter) {
    auto* search = reinterpret_cast<WindowSearch*>(parameter);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid != search->pid || !IsWindowVisible(window) ||
        GetWindow(window, GW_OWNER) != nullptr) return TRUE;
    wchar_t class_name[64]{};
    GetClassNameW(window, class_name, _countof(class_name));
    if (std::wcscmp(class_name, L"I0011OverlayWindow") == 0) return TRUE;
    RECT rect{};
    if (!GetClientRect(window, &rect) || rect.right <= 0 || rect.bottom <= 0) {
        return TRUE;
    }
    search->result = window;
    return FALSE;
}

HWND FindGameWindow() {
    WindowSearch search{GetCurrentProcessId(), nullptr};
    EnumWindows(FindGameWindowCallback, reinterpret_cast<LPARAM>(&search));
    return search.result;
}

DWORD WINAPI OverlayThread(void*) {
    HWND game = FindGameWindow();
    if (!game) return 1;
    g_game_window = game;
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = OverlayWindowProc;
    window_class.hInstance = g_self;
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.lpszClassName = L"I0011OverlayWindow";
    if (!RegisterClassExW(&window_class) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 2;
    HWND overlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        window_class.lpszClassName, L"DS3 Remaster Runtime", WS_POPUP,
        0, 0, kOverlayWidth, kOverlayHeight, game, nullptr, g_self, nullptr);
    if (!overlay) return 3;
    SetLayeredWindowAttributes(overlay, 0, 242, LWA_ALPHA);
    bool home_was_down = false;
    bool actually_visible = false;
    int overlay_x = 0;
    int overlay_y = 0;
    ULONGLONG last_repaint = 0;
    for (;;) {
        if (WaitForSingleObject(g_stop_event, 0) == WAIT_OBJECT_0) break;
        const bool home_down = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
        if (home_down && !home_was_down) {
            const bool requested =
                !g_overlay_requested.load(std::memory_order_acquire);
            g_overlay_requested.store(requested, std::memory_order_release);
            if (!requested) RestoreGameFocus();
            LogEvent(requested ? "overlay_open" : "overlay_close",
                     ControlResult::applied);
        }
        home_was_down = home_down;
        const bool requested =
            g_overlay_requested.load(std::memory_order_acquire);
        const HWND foreground = GetForegroundWindow();
        const bool game_has_focus = foreground == game;
        const bool overlay_has_focus = foreground == overlay;
        const bool should_show = ShouldShowOverlay(
            requested, game_has_focus, overlay_has_focus,
            IsIconic(game) != FALSE);
        if (should_show) {
            POINT origin{0, 0};
            ClientToScreen(game, &origin);
            const int desired_x = origin.x + 18;
            const int desired_y = origin.y + 18;
            if (OverlayPositionChanged(overlay_x, overlay_y,
                                       desired_x, desired_y)) {
                SetWindowPos(overlay, HWND_TOPMOST, desired_x, desired_y,
                             kOverlayWidth, kOverlayHeight,
                             SWP_NOACTIVATE | SWP_NOSIZE);
                overlay_x = desired_x;
                overlay_y = desired_y;
            }
            if (!actually_visible) {
                ShowWindow(overlay, SW_SHOW);
                SetForegroundWindow(overlay);
                SetFocus(overlay);
                last_repaint = 0;
            }
            const ULONGLONG now = GetTickCount64();
            if (last_repaint == 0 || now - last_repaint >= 100) {
                InvalidateRect(overlay, nullptr, FALSE);
                last_repaint = now;
            }
        } else if (actually_visible) {
            ShowWindow(overlay, SW_HIDE);
        }
        actually_visible = should_show;
        g_overlay_visible.store(should_show, std::memory_order_release);
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        Sleep(16);
    }
    g_overlay_visible.store(false, std::memory_order_release);
    RestoreGameFocus();
    DestroyWindow(overlay);
    g_game_window = nullptr;
    UnregisterClassW(window_class.lpszClassName, g_self);
    return 0;
}

bool ValidateRuntimeImage() {
    wchar_t process_path[MAX_PATH * 4]{};
    if (!GetModuleFileNameW(nullptr, process_path, _countof(process_path))) {
        return false;
    }
    const wchar_t* filename = std::wcsrchr(process_path, L'\\');
    filename = filename ? filename + 1 : process_path;
    if (_wcsicmp(filename, L"DarkSoulsIII.exe") != 0) return false;
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

bool ValidateConfig() {
    wchar_t module_path[MAX_PATH * 4]{};
    if (!GetModuleFileNameW(g_self, module_path, _countof(module_path))) {
        return false;
    }
    wchar_t* slash = std::wcsrchr(module_path, L'\\');
    if (!slash) return false;
    *slash = L'\0';
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE,
                 L"%s\\..\\prototype-config.txt", module_path);
    FILE* file = nullptr;
    if (_wfopen_s(&file, path, L"rt") != 0 || !file) return false;
    bool inert = false;
    bool home = false;
    bool minimum = false;
    bool maximum = false;
    bool unlimited = false;
    wchar_t line[128]{};
    while (std::fgetws(line, _countof(line), file)) {
        if (std::wcsstr(line, L"start_inert=1") == line) inert = true;
        if (std::wcsstr(line, L"overlay_key=HOME") == line) home = true;
        if (std::wcsstr(line, L"numeric_min=30") == line) minimum = true;
        if (std::wcsstr(line, L"numeric_max=360") == line) maximum = true;
        if (std::wcsstr(line, L"unlimited=unsupported") == line) unlimited = true;
    }
    std::fclose(file);
    return inert && home && minimum && maximum && unlimited;
}

bool BuildSessionDirectory() {
    wchar_t module_path[MAX_PATH * 4]{};
    if (!GetModuleFileNameW(g_self, module_path, _countof(module_path))) {
        return false;
    }
    wchar_t* slash = std::wcsrchr(module_path, L'\\');
    if (!slash) return false;
    *slash = L'\0';
    wchar_t sessions[MAX_PATH * 4]{};
    _snwprintf_s(sessions, _countof(sessions), _TRUNCATE,
                 L"%s\\..\\..\\sessions", module_path);
    CreateDirectoryW(sessions, nullptr);
    SYSTEMTIME time{};
    GetSystemTime(&time);
    _snwprintf_s(g_session_dir, _countof(g_session_dir), _TRUNCATE,
        L"%s\\%04u%02u%02uT%02u%02u%02uZ-pid%lu", sessions,
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
        time.wSecond, GetCurrentProcessId());
    return CreateDirectoryW(g_session_dir, nullptr) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
}

bool OpenLogs() {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\events.csv",
                 g_session_dir);
    if (_wfopen_s(&g_events, path, L"wb") != 0 || !g_events) return false;
    std::fprintf(g_events,
        "qpc,event,result,state,mode,target_fps,actual_fps,fix,overlay,updates,comparisons,corrections,guard_failures\n");
    _snwprintf_s(path, _countof(path), _TRUNCATE,
                 L"%s\\transitions.csv", g_session_dir);
    if (_wfopen_s(&g_transitions, path, L"wb") != 0 || !g_transitions) {
        return false;
    }
    std::fprintf(g_transitions,
        "qpc,action,object,fps_before,enable_before,fps_after,enable_after,result\n");
    return true;
}

bool InstallHook(std::uintptr_t rva, void* detour, void** original) {
    void* target = reinterpret_cast<void*>(g_image + rva);
    const MH_STATUS created = MH_CreateHook(target, detour, original);
    const MH_STATUS enabled = created == MH_OK
        ? MH_EnableHook(target) : created;
    return created == MH_OK && enabled == MH_OK;
}

bool CreateControlObjects() {
    auto create_event = [](const wchar_t* action, BOOL manual_reset) {
        wchar_t name[128]{};
        EventName(name, _countof(name), action);
        return CreateEventW(nullptr, manual_reset, FALSE, name);
    };
    g_stop_event = create_event(L"STOP", TRUE);
    g_ready_event = create_event(L"READY", TRUE);
    g_complete_event = create_event(L"COMPLETE", TRUE);
    g_show_event = create_event(L"SHOW", FALSE);
    g_hide_event = create_event(L"HIDE", FALSE);
    g_fix_on_event = create_event(L"FIX_ON", FALSE);
    g_fix_off_event = create_event(L"FIX_OFF", FALSE);
    g_native_event = create_event(L"NATIVE", FALSE);
    g_unlimited_event = create_event(L"UNLIMITED", FALSE);
    g_status_event = create_event(L"STATUS", FALSE);
    g_ack_event = create_event(L"ACK", TRUE);
    wchar_t mapping_name[128]{};
    EventName(mapping_name, _countof(mapping_name), L"STATUS_MAP");
    g_status_mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        sizeof(RuntimePublicStatus), mapping_name);
    if (g_status_mapping) {
        g_public_status = static_cast<RuntimePublicStatus*>(MapViewOfFile(
            g_status_mapping, FILE_MAP_ALL_ACCESS, 0, 0,
            sizeof(RuntimePublicStatus)));
    }
    return g_stop_event && g_ready_event && g_complete_event &&
           g_show_event && g_hide_event && g_fix_on_event &&
           g_fix_off_event && g_native_event && g_unlimited_event &&
           g_status_event && g_ack_event && g_status_mapping &&
           g_public_status;
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
    InitializeCriticalSection(&g_log_lock);
    InitializeCriticalSection(&g_timing_lock);
    InitializeCriticalSection(&g_watch_lock);
    bool ok = CreateControlObjects() && ValidateRuntimeImage() &&
              ValidateConfig() && BuildSessionDirectory() && OpenLogs() &&
              InitializeWatch();
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
        g_overlay_thread = CreateThread(nullptr, 0, OverlayThread,
                                        nullptr, 0, nullptr);
        ok = g_overlay_thread != nullptr;
    }
    if (!ok) {
        g_state.store(RuntimeState::failed, std::memory_order_release);
        g_guard_failed.store(true, std::memory_order_release);
    }
    LogEvent(ok ? "ready" : "startup_failed",
             ok ? ControlResult::applied : ControlResult::failed);
    WriteMarker(ok ? L"READY" : L"FAILED",
                ok ? "STANDBY: hooks installed; timing, correction and overlay are off"
                   : "I-0011 startup failed closed");
    if (g_ready_event) SetEvent(g_ready_event);
    if (!ok && g_stop_event) SetEvent(g_stop_event);

    HANDLE controls[] = {
        g_stop_event, g_show_event, g_hide_event, g_fix_on_event,
        g_fix_off_event, g_native_event, g_unlimited_event, g_status_event,
    };
    bool running = ok;
    while (running) {
        const DWORD wait = WaitForMultipleObjects(
            static_cast<DWORD>(_countof(controls)), controls, FALSE, 250);
        if (wait == WAIT_OBJECT_0) {
            running = false;
        } else if (wait >= WAIT_OBJECT_0 + 1 &&
                   wait < WAIT_OBJECT_0 + _countof(controls)) {
            const DWORD index = wait - WAIT_OBJECT_0;
            switch (index) {
                case 1:
                    g_overlay_requested.store(true, std::memory_order_release);
                    break;
                case 2:
                    g_overlay_requested.store(false, std::memory_order_release);
                    break;
                case 3:
                    QueueCorrection(true);
                    break;
                case 4:
                    QueueCorrection(false);
                    break;
                case 5:
                    QueueCadence(CadenceMode::native, 0.0f);
                    break;
                case 6:
                    QueueCadence(CadenceMode::unlimited, 0.0f);
                    break;
                case 7:
                    PublishStatus();
                    break;
                default:
                    break;
            }
            PublishStatus(ControlResult::applied);
            if (g_ack_event) SetEvent(g_ack_event);
        } else if (wait == WAIT_TIMEOUT) {
            if (!VerifyActiveTiming()) {
                FailClosed("timing_drift");
                running = false;
            }
            PublishStatus();
        } else {
            FailClosed("control_wait_failed");
            running = false;
        }
    }

    g_state.store(RuntimeState::stopping, std::memory_order_release);
    g_overlay_requested.store(false, std::memory_order_release);
    g_correction_active.store(false, std::memory_order_release);
    if (g_overlay_thread) {
        WaitForSingleObject(g_overlay_thread, 2000);
        CloseHandle(g_overlay_thread);
        g_overlay_thread = nullptr;
    }
    const bool watch_ok = DisarmWatch();
    bool hooks_ok = true;
    if (minhook == MH_OK) {
        hooks_ok = MH_DisableHook(MH_ALL_HOOKS) == MH_OK;
        hooks_ok = MH_Uninitialize() == MH_OK && hooks_ok;
    }
    const bool watch_shutdown_ok = ShutdownWatch();
    const bool timing_ok = RestoreTimingExact();
    const bool complete = watch_ok && hooks_ok && watch_shutdown_ok && timing_ok;
    LogEvent(complete ? "complete" : "cleanup_failed",
             complete ? ControlResult::applied : ControlResult::failed);
    WriteMarker(complete ? L"COMPLETE" : L"CLEANUP-FAILED",
                complete
                    ? "timing, correction, hooks and debug registers restored"
                    : "one or more cleanup invariants failed");
    PublishStatus(complete ? ControlResult::applied : ControlResult::failed);
    if (g_events) std::fclose(g_events);
    if (g_transitions) std::fclose(g_transitions);
    g_events = nullptr;
    g_transitions = nullptr;
    if (g_complete_event) SetEvent(g_complete_event);
    return complete ? 0u : 1u;
}

}  // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, RuntimeThread,
                                     nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
