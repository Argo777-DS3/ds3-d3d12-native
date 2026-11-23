#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "MinHook.h"
#include "debug_config.hpp"
#include "prototype_core.hpp"
#include "public_status.hpp"

namespace {

constexpr std::uintptr_t kRvaBroadUpdate = 0x00D57900;
constexpr std::uintptr_t kRvaSpatialBoundary = 0x02344730;
constexpr std::uintptr_t kRvaCentralGlobal = 0x0489DD10;
constexpr std::uintptr_t kRvaWorldChrManGlobal = 0x0477FDB8;
constexpr std::uintptr_t kRvaComparison = 0x005C73D8;
constexpr std::uintptr_t kRvaRecurrenceExit = 0x005C7435;
constexpr std::uintptr_t kCallerArm = 0x00A1429C;
constexpr std::uintptr_t kCallerDisarm = 0x00DE50EC;
constexpr float kTargetFps = 240.0f;

HMODULE g_self = nullptr;
std::uintptr_t g_image = 0;
LARGE_INTEGER g_frequency{};
std::atomic<PrototypeState> g_state{PrototypeState::standby};
std::atomic<bool> g_guard_failed{false};

HANDLE g_stop_event = nullptr;
HANDLE g_ready_event = nullptr;
HANDLE g_complete_event = nullptr;
HANDLE g_prepare_event = nullptr;
HANDLE g_enable_event = nullptr;
HANDLE g_disable_event = nullptr;
HANDLE g_status_event = nullptr;
HANDLE g_ack_event = nullptr;
HANDLE g_status_mapping = nullptr;
PrototypePublicStatus* g_public_status = nullptr;

CRITICAL_SECTION g_log_lock{};
FILE* g_events = nullptr;
FILE* g_override_log = nullptr;
wchar_t g_session_dir[MAX_PATH * 4]{};

std::atomic<std::uint64_t> g_updates{0};
std::atomic<std::uint64_t> g_delta_ns_sum{0};
std::atomic<std::uint64_t> g_arms{0};
std::atomic<std::uint64_t> g_disarms{0};
std::atomic<std::uint64_t> g_comparisons{0};
std::atomic<std::uint64_t> g_recurrence_exits{0};
std::atomic<std::uint64_t> g_commits{0};
std::atomic<std::uint64_t> g_corrections{0};
std::atomic<std::uint64_t> g_native_reduce_corrected{0};
std::atomic<std::uint64_t> g_normalized_reduce_retained{0};
std::atomic<std::uint64_t> g_guard_failures{0};
std::atomic<std::uint64_t> g_central_delta_checks{0};
std::atomic<std::uint64_t> g_central_delta_mismatches{0};
std::atomic<std::uint64_t> g_animation_samples{0};
std::atomic<std::uint64_t> g_animation_changes{0};
std::atomic<bool> g_debug_restored{false};
std::atomic<bool> g_timing_restored{false};

bool g_timing_applied = false;
std::uintptr_t g_timing_object = 0;
float g_original_fps = 0.0f;
unsigned char g_original_enable = 0;

thread_local DistanceFrame g_distance_previous{};
thread_local DistanceFrame g_distance_current{};
thread_local bool g_animation_clock_valid = false;
thread_local float g_animation_clock = 0.0f;

using FnBroadUpdate = void(__fastcall*)(void*, void*);
using FnSpatialBoundary = void(__fastcall*)(void*, void*);
FnBroadUpdate g_original_broad_update = nullptr;
FnSpatialBoundary g_original_spatial_boundary = nullptr;

struct PlayerSnapshot {
    std::uintptr_t player = 0;
    std::uintptr_t transform = 0;
    float position[3]{};
    float animation_clock = 0.0f;
    bool position_valid = false;
    bool animation_valid = false;
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

bool IsActiveState(PrototypeState state) {
    return state == PrototypeState::observe ||
           state == PrototypeState::correct;
}

void EventName(wchar_t* output, size_t count, const wchar_t* action) {
    _snwprintf_s(output, count, _TRUNCATE, L"Local\\DS3_I0010_%s_%lu",
                 action, GetCurrentProcessId());
}

void WriteMarker(const wchar_t* name, const char* detail) {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\%s",
                 g_session_dir, name);
    FILE* file = nullptr;
    if (_wfopen_s(&file, path, L"wb") == 0 && file) {
        std::fprintf(file, "%s\n", detail);
        std::fclose(file);
    }
}

float MeasuredFps() {
    const std::uint64_t updates = g_updates.load(std::memory_order_acquire);
    const std::uint64_t nanoseconds =
        g_delta_ns_sum.load(std::memory_order_acquire);
    if (updates == 0 || nanoseconds == 0) return 0.0f;
    return static_cast<float>(
        (static_cast<double>(updates) * 1000000000.0) /
        static_cast<double>(nanoseconds));
}

void PublishStatus(bool control_success) {
    if (!g_public_status) return;
    PrototypePublicStatus status{};
    const PrototypeState state = g_state.load(std::memory_order_acquire);
    status.state = static_cast<std::uint32_t>(state);
    status.timing_override_active = g_timing_applied ? 1u : 0u;
    status.correction_active = state == PrototypeState::correct ? 1u : 0u;
    status.guard_failed = g_guard_failed.load(std::memory_order_acquire) ? 1u : 0u;
    status.debug_restored = g_debug_restored.load(std::memory_order_acquire) ? 1u : 0u;
    status.timing_restored = g_timing_restored.load(std::memory_order_acquire) ? 1u : 0u;
    status.last_control_success = control_success ? 1u : 0u;
    status.measured_fps = MeasuredFps();
    status.updates = g_updates.load(std::memory_order_acquire);
    status.arms = g_arms.load(std::memory_order_acquire);
    status.disarms = g_disarms.load(std::memory_order_acquire);
    status.comparisons = g_comparisons.load(std::memory_order_acquire);
    status.recurrence_exits = g_recurrence_exits.load(std::memory_order_acquire);
    status.commits = g_commits.load(std::memory_order_acquire);
    status.corrections = g_corrections.load(std::memory_order_acquire);
    status.native_reduce_corrected =
        g_native_reduce_corrected.load(std::memory_order_acquire);
    status.normalized_reduce_retained =
        g_normalized_reduce_retained.load(std::memory_order_acquire);
    status.guard_failures = g_guard_failures.load(std::memory_order_acquire);
    status.central_delta_checks =
        g_central_delta_checks.load(std::memory_order_acquire);
    status.central_delta_mismatches =
        g_central_delta_mismatches.load(std::memory_order_acquire);
    status.animation_samples =
        g_animation_samples.load(std::memory_order_acquire);
    status.animation_changes =
        g_animation_changes.load(std::memory_order_acquire);
    *g_public_status = status;
    MemoryBarrier();
}

void LogEvent(const char* event, bool success) {
    PublishStatus(success);
    if (!g_events || !g_public_status) return;
    EnterCriticalSection(&g_log_lock);
    const PrototypePublicStatus status = *g_public_status;
    std::fprintf(
        g_events,
        "%lld,%s,%u,%s,%u,%u,%.6f,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
        Qpc(), event, success ? 1u : 0u,
        PrototypeStateName(static_cast<PrototypeState>(status.state)),
        status.timing_override_active, status.correction_active,
        static_cast<double>(status.measured_fps),
        static_cast<unsigned long long>(status.updates),
        static_cast<unsigned long long>(status.arms),
        static_cast<unsigned long long>(status.disarms),
        static_cast<unsigned long long>(status.comparisons),
        static_cast<unsigned long long>(status.recurrence_exits),
        static_cast<unsigned long long>(status.commits),
        static_cast<unsigned long long>(status.corrections),
        static_cast<unsigned long long>(status.native_reduce_corrected),
        static_cast<unsigned long long>(status.normalized_reduce_retained),
        static_cast<unsigned long long>(status.guard_failures),
        static_cast<unsigned long long>(status.central_delta_checks),
        static_cast<unsigned long long>(status.central_delta_mismatches),
        static_cast<unsigned long long>(status.animation_samples),
        static_cast<unsigned long long>(status.animation_changes));
    std::fflush(g_events);
    LeaveCriticalSection(&g_log_lock);
}

void FailClosed(const char* reason) {
    const bool first = !g_guard_failed.exchange(true, std::memory_order_acq_rel);
    g_guard_failures.fetch_add(1, std::memory_order_relaxed);
    g_state.store(PrototypeState::failed, std::memory_order_release);
    if (first) LogEvent(reason, false);
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

    std::uintptr_t modules = 0;
    std::uintptr_t animation = 0;
    if (snapshot.player && ReadPointer(snapshot.player + 0x1F90, &modules) &&
        ReadPointer(modules + 0x10, &animation) &&
        SafeRead(animation + 0x24, &snapshot.animation_clock)) {
        snapshot.animation_valid = std::isfinite(snapshot.animation_clock);
    }
    return snapshot;
}

void ResetTelemetry() {
    g_updates = 0;
    g_delta_ns_sum = 0;
    g_arms = 0;
    g_disarms = 0;
    g_comparisons = 0;
    g_recurrence_exits = 0;
    g_commits = 0;
    g_corrections = 0;
    g_native_reduce_corrected = 0;
    g_normalized_reduce_retained = 0;
    g_guard_failures = 0;
    g_central_delta_checks = 0;
    g_central_delta_mismatches = 0;
    g_animation_samples = 0;
    g_animation_changes = 0;
    g_distance_previous = {};
    g_distance_current = {};
    g_animation_clock_valid = false;
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
                              restored.Dr0 == original.dr0 &&
                              restored.Dr1 == original.dr1 &&
                              restored.Dr2 == original.dr2 &&
                              restored.Dr3 == original.dr3 &&
                              restored.Dr6 == original.dr6 &&
                              restored.Dr7 == original.dr7;
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
            g_arms.fetch_add(1, std::memory_order_relaxed);
        } else if (success && command == WatchCommand::disarm) {
            g_watch_active.store(false, std::memory_order_release);
            g_watch_active_tid.store(0, std::memory_order_release);
            EnterCriticalSection(&g_watch_lock);
            g_watch_original_valid = false;
            LeaveCriticalSection(&g_watch_lock);
            g_disarms.fetch_add(1, std::memory_order_relaxed);
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
    if (addresses) std::memcpy(g_watch_addresses, addresses,
                               sizeof(g_watch_addresses));
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
    if (!g_watch_active.load(std::memory_order_acquire)) return true;
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
                if (decision.normalized_reduces) {
                    g_normalized_reduce_retained.fetch_add(
                        1, std::memory_order_relaxed);
                }
                if (g_state.load(std::memory_order_acquire) ==
                    PrototypeState::correct) {
                    DWORD corrected_bits = 0;
                    std::memcpy(&corrected_bits, &decision.normalized,
                                sizeof(corrected_bits));
                    context->Xmm0.Low =
                        (context->Xmm0.Low & 0xFFFFFFFF00000000ull) |
                        corrected_bits;
                    g_corrections.fetch_add(1, std::memory_order_relaxed);
                    if (decision.native_reduces &&
                        !decision.normalized_reduces) {
                        g_native_reduce_corrected.fetch_add(
                            1, std::memory_order_relaxed);
                    }
                }
            }
        }
        if (!input_ok) FailClosed("comparison_guard_failed");
    } else if (recurrence_exit) {
        g_recurrence_exits.fetch_add(1, std::memory_order_relaxed);
    } else if (commit) {
        g_commits.fetch_add(1, std::memory_order_relaxed);
    } else {
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
    if (!DisarmWatch()) return false;
    if (g_watch_thread) {
        if (!IssueWatchCommand(WatchCommand::stop, 0, nullptr)) return false;
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
    return true;
}

void __fastcall HookSpatialBoundary(void* self, void* context) {
    const std::uintptr_t caller =
        reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const std::uintptr_t caller_rva =
        caller >= g_image ? caller - g_image : 0;
    const PrototypeState state = g_state.load(std::memory_order_acquire);
    if (IsActiveState(state) && caller_rva == kCallerDisarm &&
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
    const PrototypeState after = g_state.load(std::memory_order_acquire);
    if (IsActiveState(after) && caller_rva == kCallerArm) {
        ArmWatch(ReadPlayerSnapshot(), context);
    }
}

void __fastcall HookBroadUpdate(void* self, void* context) {
    const PrototypeState state = g_state.load(std::memory_order_acquire);
    const bool active = IsActiveState(state);
    float delta = 0.0f;
    PlayerSnapshot before{};
    bool delta_valid = false;
    if (active) {
        delta_valid = context &&
            SafeRead(reinterpret_cast<std::uintptr_t>(context) + 0x08, &delta) &&
            std::isfinite(delta) && delta > 0.0f;
        std::uintptr_t central = 0;
        float central_delta = 0.0f;
        const bool central_valid =
            ReadPointer(g_image + kRvaCentralGlobal, &central) &&
            SafeRead(central + 0x268, &central_delta) &&
            std::isfinite(central_delta);
        g_central_delta_checks.fetch_add(1, std::memory_order_relaxed);
        if (!delta_valid || !central_valid ||
            std::fabs(delta - central_delta) > 1e-7f) {
            g_central_delta_mismatches.fetch_add(1, std::memory_order_relaxed);
            FailClosed("i0008_central_delta_mismatch");
        }
        before = ReadPlayerSnapshot();
        if (before.animation_valid) {
            g_animation_samples.fetch_add(1, std::memory_order_relaxed);
            if (g_animation_clock_valid &&
                before.animation_clock != g_animation_clock) {
                g_animation_changes.fetch_add(1, std::memory_order_relaxed);
            }
            g_animation_clock = before.animation_clock;
            g_animation_clock_valid = true;
        }
    }

    g_original_broad_update(self, context);

    if (active && delta_valid &&
        IsActiveState(g_state.load(std::memory_order_acquire))) {
        const std::uint64_t index =
            g_updates.fetch_add(1, std::memory_order_relaxed) + 1;
        g_delta_ns_sum.fetch_add(
            static_cast<std::uint64_t>(
                std::llround(static_cast<double>(delta) * 1000000000.0)),
            std::memory_order_relaxed);
        g_distance_previous = g_distance_current;
        g_distance_current.index = index;
        g_distance_current.delta = delta;
        g_distance_current.transform = before.transform;
        std::memcpy(g_distance_current.position, before.position,
                    sizeof(g_distance_current.position));
        g_distance_current.valid = before.position_valid;
    }
}

bool ApplyTimingOverride() {
    if (g_timing_applied) return false;
    std::uintptr_t object = 0;
    if (!ReadPointer(g_image + kRvaCentralGlobal, &object)) return false;
    float before_fps = 0.0f;
    unsigned char before_enable = 0;
    if (!SafeRead(object + 0x354, &before_fps) ||
        !SafeRead(object + 0x358, &before_enable)) return false;
    g_timing_object = object;
    g_original_fps = before_fps;
    g_original_enable = before_enable;
    const unsigned char enabled = 1;
    const bool write_ok = SafeWrite(object + 0x354, kTargetFps) &&
                          SafeWrite(object + 0x358, enabled);
    float after_fps = 0.0f;
    unsigned char after_enable = 0;
    const bool read_ok = SafeRead(object + 0x354, &after_fps) &&
                         SafeRead(object + 0x358, &after_enable);
    const bool success = write_ok && read_ok &&
                         std::fabs(after_fps - kTargetFps) < 0.001f &&
                         after_enable == 1;
    if (success) {
        g_timing_applied = true;
        g_timing_restored.store(false, std::memory_order_release);
    } else {
        const unsigned char disabled = 0;
        const bool rollback_write =
            SafeWrite(object + 0x358, disabled) &&
            SafeWrite(object + 0x354, before_fps) &&
            SafeWrite(object + 0x358, before_enable);
        float rollback_fps = 0.0f;
        unsigned char rollback_enable = 0;
        const bool rollback_read =
            SafeRead(object + 0x354, &rollback_fps) &&
            SafeRead(object + 0x358, &rollback_enable);
        const bool rollback = rollback_write && rollback_read &&
            std::fabs(rollback_fps - before_fps) < 0.001f &&
            rollback_enable == before_enable;
        g_timing_applied = !rollback;
        g_timing_restored.store(rollback, std::memory_order_release);
        if (!rollback) FailClosed("timing_apply_rollback_failed");
    }
    if (g_override_log) {
        std::fprintf(g_override_log,
            "%lld,apply,0x%016llX,%.9g,%u,%.9g,%u,%u\n", Qpc(),
            static_cast<unsigned long long>(object),
            static_cast<double>(before_fps),
            static_cast<unsigned int>(before_enable),
            static_cast<double>(after_fps),
            static_cast<unsigned int>(after_enable), success ? 1u : 0u);
        std::fflush(g_override_log);
    }
    return success;
}

bool VerifyTimingOverride() {
    if (!g_timing_applied || g_timing_object == 0) return false;
    std::uintptr_t current = 0;
    float fps = 0.0f;
    unsigned char enabled = 0;
    return ReadPointer(g_image + kRvaCentralGlobal, &current) &&
           current == g_timing_object &&
           SafeRead(current + 0x354, &fps) &&
           SafeRead(current + 0x358, &enabled) &&
           std::fabs(fps - kTargetFps) < 0.001f && enabled == 1;
}

bool RestoreTimingOverride() {
    if (!g_timing_applied || g_timing_object == 0) {
        g_timing_restored.store(true, std::memory_order_release);
        return true;
    }
    float before_fps = 0.0f;
    unsigned char before_enable = 0;
    SafeRead(g_timing_object + 0x354, &before_fps);
    SafeRead(g_timing_object + 0x358, &before_enable);
    const unsigned char disabled = 0;
    const bool write_ok = SafeWrite(g_timing_object + 0x358, disabled) &&
                          SafeWrite(g_timing_object + 0x354, g_original_fps) &&
                          SafeWrite(g_timing_object + 0x358, g_original_enable);
    float after_fps = 0.0f;
    unsigned char after_enable = 0;
    const bool read_ok = SafeRead(g_timing_object + 0x354, &after_fps) &&
                         SafeRead(g_timing_object + 0x358, &after_enable);
    const bool success = write_ok && read_ok &&
                         std::fabs(after_fps - g_original_fps) < 0.001f &&
                         after_enable == g_original_enable;
    if (g_override_log) {
        std::fprintf(g_override_log,
            "%lld,restore,0x%016llX,%.9g,%u,%.9g,%u,%u\n", Qpc(),
            static_cast<unsigned long long>(g_timing_object),
            static_cast<double>(before_fps),
            static_cast<unsigned int>(before_enable),
            static_cast<double>(after_fps),
            static_cast<unsigned int>(after_enable), success ? 1u : 0u);
        std::fflush(g_override_log);
    }
    if (success) g_timing_applied = false;
    g_timing_restored.store(success, std::memory_order_release);
    return success;
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

bool InstallHook(const char* name, std::uintptr_t rva, void* detour,
                 void** original) {
    void* target = reinterpret_cast<void*>(g_image + rva);
    const MH_STATUS created = MH_CreateHook(target, detour, original);
    const MH_STATUS enabled =
        created == MH_OK ? MH_EnableHook(target) : created;
    if (g_events) {
        std::fprintf(g_events,
                     "%lld,hook_%s,%u,STANDBY,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n",
                     Qpc(), name,
                     created == MH_OK && enabled == MH_OK ? 1u : 0u);
        std::fflush(g_events);
    }
    return created == MH_OK && enabled == MH_OK;
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
    bool target_ok = false;
    bool disabled_ok = false;
    wchar_t line[128]{};
    while (std::fgetws(line, _countof(line), file)) {
        if (std::wcsstr(line, L"target_fps=240") == line) target_ok = true;
        if (std::wcsstr(line, L"start_enabled=0") == line) disabled_ok = true;
    }
    std::fclose(file);
    return target_ok && disabled_ok;
}

bool BuildSessionDirectory() {
    wchar_t module_path[MAX_PATH * 4]{};
    if (!GetModuleFileNameW(g_self, module_path, _countof(module_path))) {
        return false;
    }
    wchar_t* slash = std::wcsrchr(module_path, L'\\');
    if (!slash) return false;
    *slash = L'\0';
    wchar_t relative[MAX_PATH * 4]{};
    _snwprintf_s(relative, _countof(relative), _TRUNCATE,
                 L"%s\\..\\..\\sessions", module_path);
    wchar_t sessions[MAX_PATH * 4]{};
    if (!GetFullPathNameW(relative, _countof(sessions), sessions, nullptr)) {
        return false;
    }
    CreateDirectoryW(sessions, nullptr);
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    _snwprintf_s(g_session_dir, _countof(g_session_dir), _TRUNCATE,
        L"%s\\%04u%02u%02uT%02u%02u%02uZ-pid%lu", sessions,
        utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond,
        GetCurrentProcessId());
    return CreateDirectoryW(g_session_dir, nullptr) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
}

bool OpenLogs() {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE,
                 L"%s\\events.csv", g_session_dir);
    if (_wfopen_s(&g_events, path, L"wb") != 0 || !g_events) return false;
    std::fprintf(g_events,
        "qpc,event,success,state,timing,correction,measured_fps,updates,arms,disarms,comparisons,recurrence_exits,commits,corrections,native_reduce_corrected,normalized_reduce_retained,guard_failures,central_checks,central_mismatches,animation_samples,animation_changes\n");
    _snwprintf_s(path, _countof(path), _TRUNCATE,
                 L"%s\\override.csv", g_session_dir);
    if (_wfopen_s(&g_override_log, path, L"wb") != 0 || !g_override_log) {
        return false;
    }
    std::fprintf(g_override_log,
        "qpc,action,object,fps_before,enabled_before,fps_after,enabled_after,success\n");
    _snwprintf_s(path, _countof(path), _TRUNCATE,
                 L"%s\\session-info.txt", g_session_dir);
    FILE* info = nullptr;
    if (_wfopen_s(&info, path, L"wb") != 0 || !info) return false;
    std::fprintf(info,
        "schema=I-0010-locomotion-240-validation-v1\n"
        "game_build=Steam AppID 374320 / BuildID 10167187 / 1.15.2.0\n"
        "executable_sha256=EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C\n"
        "target_fps=240\nstart_state=STANDBY\nstart_enabled=0\n"
        "comparison_rva=005C73D8\nrecurrence_exit_rva=005C7435\n"
        "formula=raw*((1/60)/previous_broad_delta)\n"
        "i0008_control=central_plus_268_and_player_animation_plus_24\n");
    std::fclose(info);
    return true;
}

bool CreateControlObjects() {
    wchar_t name[128]{};
    EventName(name, _countof(name), L"STOP");
    g_stop_event = CreateEventW(nullptr, TRUE, FALSE, name);
    EventName(name, _countof(name), L"READY");
    g_ready_event = CreateEventW(nullptr, TRUE, FALSE, name);
    EventName(name, _countof(name), L"COMPLETE");
    g_complete_event = CreateEventW(nullptr, TRUE, FALSE, name);
    EventName(name, _countof(name), L"PREPARE");
    g_prepare_event = CreateEventW(nullptr, FALSE, FALSE, name);
    EventName(name, _countof(name), L"ENABLE");
    g_enable_event = CreateEventW(nullptr, FALSE, FALSE, name);
    EventName(name, _countof(name), L"DISABLE");
    g_disable_event = CreateEventW(nullptr, FALSE, FALSE, name);
    EventName(name, _countof(name), L"STATUS");
    g_status_event = CreateEventW(nullptr, FALSE, FALSE, name);
    EventName(name, _countof(name), L"ACK");
    g_ack_event = CreateEventW(nullptr, TRUE, FALSE, name);
    EventName(name, _countof(name), L"STATUS_MAP");
    g_status_mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        sizeof(PrototypePublicStatus), name);
    if (g_status_mapping) {
        g_public_status = static_cast<PrototypePublicStatus*>(
            MapViewOfFile(g_status_mapping, FILE_MAP_ALL_ACCESS, 0, 0,
                          sizeof(PrototypePublicStatus)));
    }
    return g_stop_event && g_ready_event && g_complete_event &&
           g_prepare_event && g_enable_event && g_disable_event &&
           g_status_event && g_ack_event && g_public_status;
}

void HandleControl(PrototypeAction action, const char* name) {
    PrototypeState current = g_state.load(std::memory_order_acquire);
    PrototypeState next = current;
    bool success = NextPrototypeState(current, action, &next);
    if (success && action == PrototypeAction::prepare) {
        ResetTelemetry();
        success = ApplyTimingOverride();
    }
    if (success) g_state.store(next, std::memory_order_release);
    LogEvent(name, success);
    if (g_ack_event) SetEvent(g_ack_event);
}

DWORD WINAPI PrototypeThread(void*) {
    g_image = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    QueryPerformanceFrequency(&g_frequency);
    InitializeCriticalSection(&g_log_lock);
    InitializeCriticalSection(&g_watch_lock);

    const bool controls_ok = CreateControlObjects();
    bool ok = controls_ok && ValidateRuntimeImage() && ValidateConfig() && BuildSessionDirectory() &&
              OpenLogs() && InitializeWatch();
    const MH_STATUS minhook = ok ? MH_Initialize() : MH_ERROR_NOT_INITIALIZED;
    ok = ok && minhook == MH_OK;
    if (ok) {
        ok = InstallHook("broad_update", kRvaBroadUpdate,
                         reinterpret_cast<void*>(&HookBroadUpdate),
                         reinterpret_cast<void**>(&g_original_broad_update)) && ok;
        ok = InstallHook("spatial_boundary", kRvaSpatialBoundary,
                         reinterpret_cast<void*>(&HookSpatialBoundary),
                         reinterpret_cast<void**>(&g_original_spatial_boundary)) && ok;
    }

    if (!ok) {
        g_state.store(PrototypeState::failed, std::memory_order_release);
        g_guard_failed.store(true, std::memory_order_release);
    }
    LogEvent(ok ? "ready" : "startup_failed", ok);
    WriteMarker(ok ? L"READY" : L"FAILED",
                ok ? "STANDBY: hooks installed; timing and correction are off"
                   : "prototype startup failed");
    if (g_ready_event) SetEvent(g_ready_event);
    if (!ok && g_stop_event) SetEvent(g_stop_event);

    HANDLE controls[5] = {g_stop_event, g_prepare_event, g_enable_event,
                          g_disable_event, g_status_event};
    bool running = ok;
    while (running) {
        const DWORD wait = WaitForMultipleObjects(5, controls, FALSE, 100);
        switch (wait) {
            case WAIT_OBJECT_0:
                running = false;
                break;
            case WAIT_OBJECT_0 + 1:
                HandleControl(PrototypeAction::prepare, "prepare");
                break;
            case WAIT_OBJECT_0 + 2:
                HandleControl(PrototypeAction::enable, "enable");
                break;
            case WAIT_OBJECT_0 + 3:
                HandleControl(PrototypeAction::disable, "disable");
                break;
            case WAIT_OBJECT_0 + 4:
                LogEvent("status", true);
                if (g_ack_event) SetEvent(g_ack_event);
                break;
            case WAIT_TIMEOUT:
                if (g_timing_applied && !VerifyTimingOverride()) {
                    FailClosed("timing_override_drift");
                }
                break;
            default:
                FailClosed("control_wait_failed");
                running = false;
                break;
        }
    }

    const bool failed = g_guard_failed.load(std::memory_order_acquire) || !ok;
    if (!failed) {
        g_state.store(PrototypeState::stopping, std::memory_order_release);
    }
    const bool debug_ok = ShutdownWatch();
    g_debug_restored.store(debug_ok, std::memory_order_release);
    const bool timing_ok = RestoreTimingOverride();
    if (minhook == MH_OK) {
        MH_DisableHook(MH_ALL_HOOKS);
        Sleep(100);
        MH_Uninitialize();
    }
    const bool complete = ok && !failed && debug_ok && timing_ok;
    LogEvent(complete ? "complete" : "failed", complete);
    WriteMarker(complete ? L"COMPLETE" : L"FAILED",
        complete
            ? "correction off; hooks disabled; debug registers and timing restored"
            : "cleanup or guard failure; inspect status and logs");
    if (g_complete_event) SetEvent(g_complete_event);
    if (g_events) std::fclose(g_events);
    if (g_override_log) std::fclose(g_override_log);
    g_events = nullptr;
    g_override_log = nullptr;
    return complete ? 0 : 1;
}

}  // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, PrototypeThread,
                                     nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
