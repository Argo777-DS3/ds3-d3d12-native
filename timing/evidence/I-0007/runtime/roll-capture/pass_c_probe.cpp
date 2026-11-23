#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <xinput.h>
#include <intrin.h>

#include <atomic>
#include <cmath>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "MinHook.h"
#include "../causal-patch/patch_debug_config.hpp"
#include "../causal-patch/distance_normalization.hpp"
#include "../causal-patch/shared_log.hpp"
#include "../causal-patch/capture_lifetime.hpp"

namespace {

struct ProbeConfig {
    float target_fps = 60.0f;
    unsigned int auto_stop_seconds = 30;
    bool enable_timing_override = true;
    bool force_present_zero = false;
    bool causal_distance_patch = false;
    bool callback_detail = false;
    bool marker_hotkeys = true;
    bool locomotion_trials = false;
    bool locomotion_phase_rca = false;
    bool locomotion_write_watch = false;
    bool locomotion_writer_mirror = false;
    unsigned int semantic_sample_stride = 1;
    float locomotion_warmup_delta = 1.0f;
    float locomotion_measure_delta = 3.0f;
    wchar_t condition[96] = L"semantic-60-discovery";
};

ProbeConfig g_config{};

constexpr uintptr_t kRvaD57900  = 0x00D57900;
constexpr uintptr_t kRva023410  = 0x00023410;
constexpr uintptr_t kRva0499C0  = 0x000499C0;
constexpr uintptr_t kRvaD5E30   = 0x000D5E30;
constexpr uintptr_t kRva192CF00 = 0x0192CF00;
constexpr uintptr_t kRvaDD4B60  = 0x00DD4B60;
constexpr uintptr_t kRvaED8E50  = 0x00ED8E50;
constexpr uintptr_t kRvaEDAAE0  = 0x00EDAAE0;
constexpr uintptr_t kRvaEFDA0   = 0x000EFDA0;
constexpr uintptr_t kRvaF0D40   = 0x000F0D40;
constexpr uintptr_t kRva21CC20  = 0x0021CC20;
constexpr uintptr_t kRvaCentralGlobal = 0x0489DD10;
constexpr uintptr_t kRvaWorldChrManGlobal = 0x0477FDB8;
constexpr uintptr_t kRvaD56780 = 0x00D56780;
constexpr uintptr_t kRvaD56930 = 0x00D56930;
constexpr uintptr_t kRvaD56A90 = 0x00D56A90;
constexpr uintptr_t kRvaD58E20 = 0x00D58E20;
constexpr uintptr_t kRvaD5B140 = 0x00D5B140;
constexpr uintptr_t kRvaE6E750 = 0x00E6E750;
constexpr uintptr_t kRva2344730 = 0x02344730;
constexpr uintptr_t kCallerRvaA1429C = 0x00A1429C;
constexpr uintptr_t kCallerRvaDE50EC = 0x00DE50EC;
constexpr uintptr_t kRva9DC559 = 0x009DC559;

HMODULE g_self = nullptr;
uintptr_t g_image = 0;
LARGE_INTEGER g_frequency{};
std::atomic<unsigned long long> g_sequence{0};
std::atomic<bool> g_stopped{false};
std::atomic<bool> g_capture_active{false};
CRITICAL_SECTION g_log_lock{};

FILE* g_calls = nullptr;
FILE* g_callbacks = nullptr;
FILE* g_central = nullptr;
FILE* g_selector = nullptr;
FILE* g_pacing = nullptr;
FILE* g_present = nullptr;
FILE* g_hooks = nullptr;
FILE* g_override = nullptr;
FILE* g_consumers = nullptr;
FILE* g_semantic = nullptr;
FILE* g_markers = nullptr;
FILE* g_locomotion_updates = nullptr;
FILE* g_locomotion_events = nullptr;
FILE* g_locomotion_phases = nullptr;
FILE* g_locomotion_write_watch = nullptr;
FILE* g_locomotion_writer_mirror = nullptr;
wchar_t g_session_dir[MAX_PATH * 4]{};

std::atomic<unsigned long long> g_broad_update_index{0};
std::atomic<int> g_active_marker{0};
std::atomic<double> g_accumulated_delta{0.0};
std::atomic<unsigned long long> g_semantic_probe_samples{0};
std::atomic<unsigned long long> g_semantic_probe_sum_ticks{0};
std::atomic<unsigned long long> g_semantic_probe_max_ticks{0};
std::atomic<bool> g_locomotion_arm_requested{false};
std::atomic<bool> g_locomotion_busy{false};
std::atomic<unsigned int> g_locomotion_next_trial{1};

unsigned int g_locomotion_trial = 0;
double g_locomotion_stable_delta = 0.0;
double g_locomotion_measured_delta = 0.0;
bool g_locomotion_warmup_complete = false;
bool g_locomotion_measuring = false;
PostMeasureCapture g_post_measure;

using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
XInputGetStateFn g_xinput_get_state = nullptr;

struct PlayerSnapshot {
    uintptr_t world = 0;
    uintptr_t player = 0;
    uintptr_t transform = 0;
    bool position_valid = false;
    bool animation_valid = false;
    float x = 0.0f;
    float z = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    int animation_id = 0;
    float animation_time = 0.0f;
    float animation_max = 0.0f;
    float play_speed = 0.0f;
    int hp = 0;
    int stamina = 0;
};

struct InputSnapshot {
    unsigned int keyboard_mask = 0;
    unsigned int controller_connected = 0;
    unsigned int controller_buttons = 0;
    int left_x = 0;
    int left_y = 0;
    int right_x = 0;
    int right_y = 0;
    unsigned int left_trigger = 0;
    unsigned int right_trigger = 0;
};

uintptr_t g_override_object = 0;
float g_original_fps = 0.0f;
unsigned char g_original_enable = 0;
bool g_override_applied = false;

struct ConsumerAggregate {
    uintptr_t object = 0;
    unsigned long long calls = 0;
    unsigned long long active_calls = 0;
    unsigned long long increment_matches = 0;
    unsigned long long resets = 0;
    long long first_qpc = 0;
    long long last_qpc = 0;
    double sum_delta = 0.0;
    double sum_elapsed_increment = 0.0;
    float min_delta = 0.0f;
    float max_delta = 0.0f;
    float first_elapsed = 0.0f;
    float last_elapsed = 0.0f;
    float first_output = 0.0f;
    float last_output = 0.0f;
    float first_xyz[3]{};
    float last_xyz[3]{};
};

ConsumerAggregate g_efda0_aggregates[512]{};
ConsumerAggregate g_f0d40_aggregates[64]{};
ConsumerAggregate g_21cc20_aggregates[512]{};
CRITICAL_SECTION g_consumer_lock{};

HANDLE g_stop_event = nullptr;
HANDLE g_ready_event = nullptr;
HANDLE g_complete_event = nullptr;
HANDLE g_target60_event = nullptr, g_target165_event = nullptr, g_flush_event = nullptr, g_control_ack = nullptr;
unsigned int g_patch_rows_written = 0;

using FnD57900 = void(__fastcall*)(void*, void*);
using FnFloat = void(__fastcall*)(void*, float);
using FnD5E30 = unsigned long long(__fastcall*)(void*, unsigned int, uintptr_t, uintptr_t);
using Fn192CF00 = long long(__fastcall*)(void*, unsigned int);
using FnDD4B60 = void(__fastcall*)(void*, void*);
using FnED8E50 = void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
using FnEDAAE0 = bool(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
using FnConsumer = void(__fastcall*)(void*, float);
using Fn21CC20 = void(__fastcall*)(void*, float, void*);
using FnPhase1 = void(__fastcall*)(void*);
using FnPhase2 = void(__fastcall*)(void*, void*);

FnD57900 g_orig_d57900 = nullptr;
FnFloat g_orig_023410 = nullptr;
FnFloat g_orig_0499c0 = nullptr;
FnD5E30 g_orig_d5e30 = nullptr;
Fn192CF00 g_orig_192cf00 = nullptr;
FnDD4B60 g_orig_dd4b60 = nullptr;
FnED8E50 g_orig_ed8e50 = nullptr;
FnEDAAE0 g_orig_edaae0 = nullptr;
FnConsumer g_orig_efda0 = nullptr;
FnConsumer g_orig_f0d40 = nullptr;
Fn21CC20 g_orig_21cc20 = nullptr;
FnPhase1 g_orig_d56780 = nullptr;
FnPhase1 g_orig_d56930 = nullptr;
FnPhase1 g_orig_d56a90 = nullptr;
FnPhase2 g_orig_d58e20 = nullptr;
FnPhase2 g_orig_d5b140 = nullptr;
FnPhase2 g_orig_e6e750 = nullptr;
FnPhase2 g_orig_2344730 = nullptr;

thread_local unsigned int g_d5b140_depth = 0;
thread_local bool g_temporal_update_active = false;
thread_local unsigned long long g_temporal_update_index = 0;
thread_local float g_temporal_delta = 0.0f;
thread_local DistanceFrame g_distance_current{}, g_distance_previous{};
thread_local double g_temporal_accumulated_before = 0.0;
thread_local double g_temporal_accumulated_after = 0.0;

enum class WatchCommand : LONG {
    none = 0,
    arm = 1,
    disarm = 2,
    stop = 3,
};

struct DebugRegisterState {
    DWORD64 dr0 = 0;
    DWORD64 dr1 = 0;
    DWORD64 dr2 = 0;
    DWORD64 dr3 = 0;
    DWORD64 dr6 = 0;
    DWORD64 dr7 = 0;
};

struct WriteWatchHit {
    unsigned long long sequence = 0;
    long long qpc = 0;
    DWORD tid = 0;
    unsigned int trial = 0;
    unsigned long long update_index = 0;
    uintptr_t shared_context = 0;
    uintptr_t transform = 0;
    DWORD64 dr6 = 0;
    DWORD64 rip_after = 0;
    float arm_x = 0.0f;
    float arm_z = 0.0f;
    float arm_y = 0.0f;
    float hit_x = 0.0f;
    float hit_z = 0.0f;
    float hit_y = 0.0f;
    DWORD64 gpr[16]{};
    DWORD64 xmm[16][2]{};
    unsigned char code[64]{};
    unsigned int code_valid = 0;
    unsigned int input_valid = 0, output_valid = 0, nested_valid = 0;
    unsigned char input_bytes[0x60]{}, output_bytes[0x30]{}, nested_bytes[0x20]{};
    unsigned char owner_bytes[0x270]{}, additive_bytes[0x10]{};
    uintptr_t builder = 0, producer_vtable = 0, producer_target = 0;
    unsigned int producer_code_valid = 0, modifier_code_valid = 0;
    unsigned char producer_code[0x200]{}, modifier_code[0x80]{};
    unsigned int owner_valid = 0, additive_valid = 0;
    unsigned int patch_applied = 0, patch_failed = 0, measured = 0;
    float patch_raw = 0.0f, patch_corrected = 0.0f, patch_delta = 0.0f;
    unsigned long long patch_previous_index = 0, patch_current_index = 0;
};

struct WriterMirrorHit {
    unsigned long long sequence = 0;
    long long qpc = 0;
    DWORD tid = 0;
    unsigned int trial = 0;
    unsigned int marker = 0;
    unsigned long long update_index = 0;
    unsigned int invocation_index = 0;
    uintptr_t shared_context = 0;
    uintptr_t transform = 0;
    uintptr_t rbx = 0;
    uintptr_t rsp = 0;
    float temporal_delta = 0.0f;
    double accumulated_before = 0.0;
    double accumulated_after = 0.0;
    DWORD mxcsr = 0;
    uintptr_t outer_a8 = 0;
    uintptr_t outer_b0 = 0;
    uintptr_t selected = 0;
    uintptr_t nested = 0;
    unsigned char inhibit = 0;
    unsigned int current_memory_valid = 0;
    unsigned int raw_candidate_valid = 0;
    unsigned int outer_a8_read_valid = 0;
    unsigned int outer_b0_read_valid = 0;
    unsigned int nested_read_valid = 0;
    unsigned int effective_candidate_valid = 0;
    unsigned int scalar_memory_valid = 0;
    unsigned int inhibit_valid = 0;
    DWORD current_memory[4]{};
    DWORD current_xmm1[4]{};
    DWORD raw_candidate[4]{};
    DWORD effective_candidate[4]{};
    DWORD scalar_memory = 0;
    DWORD scalar_xmm0[4]{};
    DWORD final_xmm6[4]{};
};

constexpr unsigned int kMaxWriteWatchHits = 8192;
WriteWatchHit g_write_watch_hits[kMaxWriteWatchHits]{};
std::atomic<unsigned int> g_write_watch_hit_count{0};
std::atomic<unsigned int> g_write_watch_overflow{0};
std::atomic<unsigned int> g_write_watch_arm_hit_start{0};
std::atomic<DWORD> g_write_watch_active_tid{0};
std::atomic<bool> g_write_watch_active{false};
std::atomic<bool> g_write_watch_failed{false};
std::atomic<LONG> g_write_watch_result{0};

CRITICAL_SECTION g_write_watch_lock{};
WatchCommand g_write_watch_command = WatchCommand::none;
DWORD g_write_watch_target_tid = 0;
uintptr_t g_write_watch_addresses[4]{};
uintptr_t g_write_watch_shared_context = 0;
uintptr_t g_write_watch_transform = 0;
unsigned int g_write_watch_trial = 0;
unsigned long long g_write_watch_update_index = 0;
float g_write_watch_arm_position[3]{};
DebugRegisterState g_write_watch_original{};
bool g_write_watch_original_valid = false;

constexpr unsigned int kMaxWriterMirrorHits = 8192;
WriterMirrorHit g_writer_mirror_hits[kMaxWriterMirrorHits]{};
std::atomic<unsigned int> g_writer_mirror_hit_count{0};
std::atomic<unsigned int> g_writer_mirror_overflow{0};
std::atomic<unsigned int> g_writer_mirror_arm_hit_start{0};
std::atomic<unsigned int> g_writer_mirror_invocation_index{0};

HANDLE g_write_watch_request_event = nullptr;
HANDLE g_write_watch_done_event = nullptr;
HANDLE g_write_watch_controller_thread = nullptr;
PVOID g_write_watch_veh = nullptr;

unsigned long long NextSequence() {
    return g_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
}

long long Qpc() {
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}

template <typename T>
bool SafeRead(uintptr_t address, T* value) {
    __try {
        *value = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::memset(value, 0, sizeof(T));
        return false;
    }
}

template <typename T>
bool SafeWrite(uintptr_t address, const T& value) {
    __try {
        *reinterpret_cast<T*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadPointer(uintptr_t address, uintptr_t* value) {
    return SafeRead(address, value) && *value != 0;
}

bool SafeCopy(uintptr_t address, void* destination, size_t size) {
    __try {
        std::memcpy(destination, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::memset(destination, 0, size);
        return false;
    }
}

PlayerSnapshot ReadPlayerSnapshot() {
    PlayerSnapshot snapshot{};
    if (!ReadPointer(g_image + kRvaWorldChrManGlobal, &snapshot.world)) return snapshot;
    ReadPointer(snapshot.world + 0x80, &snapshot.player);

    uintptr_t character_list = 0;
    if (ReadPointer(snapshot.world + 0x40, &character_list) &&
        ReadPointer(character_list + 0x28, &snapshot.transform)) {
        snapshot.position_valid =
            SafeRead(snapshot.transform + 0x80, &snapshot.x) &&
            SafeRead(snapshot.transform + 0x84, &snapshot.z) &&
            SafeRead(snapshot.transform + 0x88, &snapshot.y) &&
            SafeRead(snapshot.transform + 0x74, &snapshot.angle);
    }

    uintptr_t modules = 0;
    if (snapshot.player && ReadPointer(snapshot.player + 0x1F90, &modules)) {
        uintptr_t animation_identity = 0;
        uintptr_t animation_clock = 0;
        uintptr_t animation_motion = 0;
        uintptr_t attributes = 0;
        bool id_ok = ReadPointer(modules + 0x80, &animation_identity) &&
                     SafeRead(animation_identity + 0xC8, &snapshot.animation_id);
        bool clock_ok = ReadPointer(modules + 0x10, &animation_clock) &&
                        SafeRead(animation_clock + 0x24, &snapshot.animation_time) &&
                        SafeRead(animation_clock + 0x2C, &snapshot.animation_max);
        bool speed_ok = ReadPointer(modules + 0x28, &animation_motion) &&
                        SafeRead(animation_motion + 0xA58, &snapshot.play_speed);
        snapshot.animation_valid = id_ok && clock_ok && speed_ok;
        if (ReadPointer(modules + 0x18, &attributes)) {
            SafeRead(attributes + 0xD8, &snapshot.hp);
            SafeRead(attributes + 0xF0, &snapshot.stamina);
        }
    }
    return snapshot;
}

DWORD64 ConfigureWriterMirrorDr7(DWORD64 original) {
    DWORD64 value = original;
    value &= ~3ull;
    value &= ~(0xFull << 16);
    value |= 1ull;  // local execute breakpoint in slot 0
    return value;
}

bool DebugCaptureEnabled() {
    return g_config.locomotion_write_watch || g_config.locomotion_writer_mirror;
}

bool ApplyWriteWatchCommand(WatchCommand command, DWORD tid,
                            const uintptr_t addresses[4]) {
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
                EnterCriticalSection(&g_write_watch_lock);
                g_write_watch_original.dr0 = context.Dr0;
                g_write_watch_original.dr1 = context.Dr1;
                g_write_watch_original.dr2 = context.Dr2;
                g_write_watch_original.dr3 = context.Dr3;
                g_write_watch_original.dr6 = context.Dr6;
                g_write_watch_original.dr7 = context.Dr7;
                g_write_watch_original_valid = true;
                LeaveCriticalSection(&g_write_watch_lock);
                context.Dr0 = addresses[0];
                if (!g_config.locomotion_writer_mirror) {
                    context.Dr1 = addresses[1];
                    context.Dr2 = addresses[2];
                    context.Dr3 = addresses[3];
                }
                context.Dr6 = 0;
                context.Dr7 = g_config.locomotion_writer_mirror
                    ? ConfigureWriterMirrorDr7(context.Dr7)
                    : ConfigurePatchDr7(context.Dr7);
                success = SetThreadContext(thread, &context) != FALSE;
            } else if (command == WatchCommand::disarm) {
                EnterCriticalSection(&g_write_watch_lock);
                const DebugRegisterState original = g_write_watch_original;
                const bool original_valid = g_write_watch_original_valid;
                LeaveCriticalSection(&g_write_watch_lock);
                if (original_valid) {
                    context.Dr0 = original.dr0;
                    context.Dr1 = original.dr1;
                    context.Dr2 = original.dr2;
                    context.Dr3 = original.dr3;
                    context.Dr6 = original.dr6;
                    context.Dr7 = original.dr7;
                    success = SetThreadContext(thread, &context) != FALSE;
                    CONTEXT restored{};
                    restored.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    success = success && GetThreadContext(thread, &restored) != FALSE &&
                        restored.Dr0 == original.dr0 && restored.Dr1 == original.dr1 &&
                        restored.Dr2 == original.dr2 && restored.Dr3 == original.dr3 &&
                        restored.Dr6 == original.dr6 && restored.Dr7 == original.dr7;
                }
            }
        }
    }
    if (suspended) ResumeThread(thread);
    CloseHandle(thread);
    return success;
}

DWORD WINAPI WriteWatchController(void*) {
    for (;;) {
        if (WaitForSingleObject(g_write_watch_request_event, INFINITE) != WAIT_OBJECT_0) {
            return 1;
        }

        WatchCommand command = WatchCommand::none;
        DWORD tid = 0;
        uintptr_t addresses[4]{};
        EnterCriticalSection(&g_write_watch_lock);
        command = g_write_watch_command;
        tid = g_write_watch_target_tid;
        for (unsigned int i = 0; i < 4; ++i) addresses[i] = g_write_watch_addresses[i];
        g_write_watch_command = WatchCommand::none;
        LeaveCriticalSection(&g_write_watch_lock);

        if (command == WatchCommand::stop) {
            g_write_watch_result.store(1, std::memory_order_release);
            SetEvent(g_write_watch_done_event);
            return 0;
        }

        const bool success = ApplyWriteWatchCommand(command, tid, addresses);
        if (success && command == WatchCommand::arm) {
            g_write_watch_active_tid.store(tid, std::memory_order_release);
            g_write_watch_active.store(true, std::memory_order_release);
        } else if (success && command == WatchCommand::disarm) {
            g_write_watch_active.store(false, std::memory_order_release);
            g_write_watch_active_tid.store(0, std::memory_order_release);
            EnterCriticalSection(&g_write_watch_lock);
            g_write_watch_original_valid = false;
            LeaveCriticalSection(&g_write_watch_lock);
        }
        g_write_watch_result.store(success ? 1 : -1, std::memory_order_release);
        SetEvent(g_write_watch_done_event);
    }
}

bool IssueWriteWatchCommand(WatchCommand command, DWORD tid,
                            const uintptr_t addresses[4]) {
    if (!g_write_watch_request_event || !g_write_watch_done_event ||
        !g_write_watch_controller_thread) {
        return false;
    }
    ResetEvent(g_write_watch_done_event);
    g_write_watch_result.store(0, std::memory_order_release);
    EnterCriticalSection(&g_write_watch_lock);
    g_write_watch_command = command;
    g_write_watch_target_tid = tid;
    if (addresses) {
        for (unsigned int i = 0; i < 4; ++i) g_write_watch_addresses[i] = addresses[i];
    }
    LeaveCriticalSection(&g_write_watch_lock);
    SetEvent(g_write_watch_request_event);
    return WaitForSingleObject(g_write_watch_done_event, 1000) == WAIT_OBJECT_0 &&
           g_write_watch_result.load(std::memory_order_acquire) == 1;
}

bool ArmWriteWatch(const PlayerSnapshot& snapshot, void* context) {
    if (!DebugCaptureEnabled() || !snapshot.position_valid ||
        snapshot.transform == 0 || g_write_watch_active.load(std::memory_order_acquire)) {
        return false;
    }
    const uintptr_t watched_base = snapshot.transform + 0xE0;
    const uintptr_t addresses[4] = {g_image + 0x005C73D8, g_image + 0x005C7435,
                                    snapshot.transform + 0x80, 0};
    float watched_values[3]{};
    if (!g_config.locomotion_writer_mirror &&
        (!SafeRead(watched_base, &watched_values[0]) ||
         !SafeRead(watched_base + 0x4, &watched_values[1]) ||
         !SafeRead(watched_base + 0x8, &watched_values[2]))) {
        return false;
    }
    EnterCriticalSection(&g_write_watch_lock);
    g_write_watch_shared_context = reinterpret_cast<uintptr_t>(context);
    g_write_watch_transform = watched_base;
    g_write_watch_trial = g_locomotion_trial;
    g_write_watch_update_index =
        g_broad_update_index.load(std::memory_order_relaxed);
    g_write_watch_arm_position[0] = g_config.locomotion_writer_mirror
        ? snapshot.x : watched_values[0];
    g_write_watch_arm_position[1] = g_config.locomotion_writer_mirror
        ? snapshot.z : watched_values[1];
    g_write_watch_arm_position[2] = g_config.locomotion_writer_mirror
        ? snapshot.y : watched_values[2];
    LeaveCriticalSection(&g_write_watch_lock);
    const bool armed = IssueWriteWatchCommand(WatchCommand::arm,
                                               GetCurrentThreadId(), addresses);
    if (armed) {
        g_write_watch_arm_hit_start.store(
            g_write_watch_hit_count.load(std::memory_order_acquire),
            std::memory_order_release);
        g_writer_mirror_arm_hit_start.store(
            g_writer_mirror_hit_count.load(std::memory_order_acquire),
            std::memory_order_release);
        g_writer_mirror_invocation_index.store(0, std::memory_order_release);
    } else {
        g_write_watch_failed.store(true, std::memory_order_release);
        if (g_stop_event) SetEvent(g_stop_event);
    }
    return armed;
}

bool DisarmWriteWatch() {
    if (!g_write_watch_active.load(std::memory_order_acquire)) return true;
    const DWORD tid = g_write_watch_active_tid.load(std::memory_order_acquire);
    const bool disarmed = IssueWriteWatchCommand(WatchCommand::disarm, tid, nullptr);
    if (!disarmed) {
        g_write_watch_failed.store(true, std::memory_order_release);
        if (g_stop_event) SetEvent(g_stop_event);
    }
    return disarmed;
}

LONG CaptureWriterMirrorHit(CONTEXT* context) {
    if ((context->Dr6 & 1ull) == 0 || context->Rip != g_image + kRva9DC559) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const unsigned int index =
        g_writer_mirror_hit_count.fetch_add(1, std::memory_order_relaxed);
    const unsigned int arm_start =
        g_writer_mirror_arm_hit_start.load(std::memory_order_acquire);
    if (index - arm_start >= 64 || index >= kMaxWriterMirrorHits) {
        g_writer_mirror_overflow.store(1, std::memory_order_release);
        g_write_watch_failed.store(true, std::memory_order_release);
        context->Dr7 &= ~3ull;
        context->Dr7 &= ~(0xFull << 16);
        context->Dr6 &= ~1ull;
        if (g_stop_event) SetEvent(g_stop_event);
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    WriterMirrorHit& hit = g_writer_mirror_hits[index];
    hit.sequence = NextSequence();
    hit.qpc = Qpc();
    hit.tid = GetCurrentThreadId();
    hit.trial = g_locomotion_trial;
    hit.marker = static_cast<unsigned int>(
        g_active_marker.load(std::memory_order_relaxed));
    hit.update_index = g_temporal_update_active
        ? g_temporal_update_index
        : g_broad_update_index.load(std::memory_order_relaxed);
    hit.invocation_index =
        g_writer_mirror_invocation_index.fetch_add(1, std::memory_order_relaxed) + 1;
    hit.rbx = static_cast<uintptr_t>(context->Rbx);
    hit.rsp = static_cast<uintptr_t>(context->Rsp);
    hit.temporal_delta = g_temporal_update_active ? g_temporal_delta : 0.0f;
    hit.accumulated_before = g_temporal_update_active
        ? g_temporal_accumulated_before
        : g_accumulated_delta.load(std::memory_order_relaxed);
    hit.accumulated_after = g_temporal_update_active
        ? g_temporal_accumulated_after
        : hit.accumulated_before;
    hit.mxcsr = context->MxCsr;
    EnterCriticalSection(&g_write_watch_lock);
    hit.shared_context = g_write_watch_shared_context;
    hit.transform = g_write_watch_transform;
    LeaveCriticalSection(&g_write_watch_lock);

    hit.current_memory_valid = SafeCopy(hit.rbx + 0x80, hit.current_memory,
                                        sizeof(hit.current_memory)) ? 1u : 0u;
    hit.raw_candidate_valid = SafeCopy(hit.rsp + 0x20, hit.raw_candidate,
                                       sizeof(hit.raw_candidate)) ? 1u : 0u;
    std::memcpy(hit.current_xmm1, &context->Xmm1, sizeof(hit.current_xmm1));
    std::memcpy(hit.scalar_xmm0, &context->Xmm0, sizeof(hit.scalar_xmm0));
    std::memcpy(hit.final_xmm6, &context->Xmm6, sizeof(hit.final_xmm6));
    hit.scalar_memory_valid = SafeRead(hit.rbx + 0x1C0, &hit.scalar_memory) ? 1u : 0u;
    hit.inhibit_valid = SafeRead(hit.rbx + 0x1E1, &hit.inhibit) ? 1u : 0u;
    hit.outer_a8_read_valid = SafeRead(hit.rbx + 0xA8, &hit.outer_a8) ? 1u : 0u;
    hit.outer_b0_read_valid = SafeRead(hit.rbx + 0xB0, &hit.outer_b0) ? 1u : 0u;
    if (hit.outer_b0_read_valid && hit.outer_b0 != 0) {
        hit.selected = hit.outer_b0;
    } else if (hit.outer_b0_read_valid && hit.outer_a8_read_valid) {
        hit.selected = hit.outer_a8;
    }
    if (hit.selected != 0) {
        hit.nested_read_valid = SafeRead(hit.selected + 0x40, &hit.nested) ? 1u : 0u;
    }
    if (hit.nested_read_valid && hit.nested != 0) {
        hit.effective_candidate_valid =
            SafeCopy(hit.nested + 0x70, hit.effective_candidate,
                     sizeof(hit.effective_candidate)) ? 1u : 0u;
    }

    context->Dr6 &= ~1ull;
    context->EFlags |= 1u << 16;  // resume past the current execute breakpoint
    return EXCEPTION_CONTINUE_EXECUTION;
}

LONG CALLBACK WriteWatchExceptionHandler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord ||
        pointers->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
        !g_write_watch_active.load(std::memory_order_acquire) ||
        GetCurrentThreadId() != g_write_watch_active_tid.load(std::memory_order_acquire)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    CONTEXT* context = pointers->ContextRecord;
    if (g_config.locomotion_writer_mirror) {
        return CaptureWriterMirrorHit(context);
    }
    const DWORD64 hit_mask = context->Dr6 & 0xFull;
    if (hit_mask == 0) return EXCEPTION_CONTINUE_SEARCH;
    // Execute traps stop before the instruction. Resume it once via RF.
    if ((hit_mask & 3ull) != 0) {
        context->EFlags |= 0x10000u;
        if (context->Rdi + 0xE0 != g_write_watch_transform) {
            context->Dr6 = 0;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    const unsigned int index =
        g_write_watch_hit_count.fetch_add(1, std::memory_order_relaxed);
    const unsigned int arm_start =
        g_write_watch_arm_hit_start.load(std::memory_order_acquire);
    if (index - arm_start >= 8) {
        g_write_watch_overflow.store(1, std::memory_order_release);
        g_write_watch_failed.store(true, std::memory_order_release);
        for (unsigned int slot = 0; slot < 4; ++slot) {
            context->Dr7 &= ~(3ull << (slot * 2));
            context->Dr7 &= ~(0xFull << (16 + slot * 4));
        }
        context->Dr6 = 0;
        if (g_stop_event) SetEvent(g_stop_event);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (index < kMaxWriteWatchHits) {
        WriteWatchHit& hit = g_write_watch_hits[index];
        hit.sequence = NextSequence();
        hit.qpc = Qpc();
        hit.tid = GetCurrentThreadId();
        hit.dr6 = context->Dr6;
        hit.rip_after = context->Rip;
        EnterCriticalSection(&g_write_watch_lock);
        hit.trial = g_write_watch_trial;
        hit.update_index = g_write_watch_update_index;
        hit.shared_context = g_write_watch_shared_context;
        hit.transform = g_write_watch_transform;
        hit.arm_x = g_write_watch_arm_position[0];
        hit.arm_z = g_write_watch_arm_position[1];
        hit.arm_y = g_write_watch_arm_position[2];
        LeaveCriticalSection(&g_write_watch_lock);
        hit.nested_valid = SafeCopy(hit.transform, hit.nested_bytes, sizeof(hit.nested_bytes)) ? 1u : 0u;
        hit.measured = g_locomotion_measuring ? 1u : 0u;
        const bool commit = (hit_mask & 4ull) != 0;
        hit.builder = commit ? 0 : context->Rbx;
        uintptr_t player = 0, modules = 0, owner = 0, timing = 0;
        const bool compare = context->Rip == g_image + 0x005C73D8;
        const bool final_value = context->Rip == g_image + 0x005C7435;
        const bool chain_ok = hit.builder && SafeRead(hit.builder + 8, &player) &&
            SafeRead(player + 0x1F90, &modules) && SafeRead(modules + 0x68, &owner) &&
            SafeRead(modules + 0x90, &timing) && owner + 0xE0 == hit.transform;
        if (!commit && (compare || final_value) && chain_ok) {
            hit.owner_valid = SafeCopy(hit.builder, hit.owner_bytes, sizeof(hit.owner_bytes)) ? 1u : 0u;
            hit.additive_valid = SafeCopy(timing + 0x18, hit.additive_bytes, sizeof(hit.additive_bytes)) ? 1u : 0u;
            const bool input_ok = SafeCopy(owner + 0x3C0, hit.input_bytes, 0x10) &&
                SafeCopy(owner + 0x1F0, hit.input_bytes + 0x10, 0x10) &&
                SafeCopy(owner + 0xA0, hit.input_bytes + 0x20, 0x10);
            hit.input_valid = input_ok ? 1u : 0u;
            // Before COMISS: getter output at RSP+30, old position in XMM6.
            // End: the first two vectors are current owner positions, not a new getter result.
            bool output_ok = SafeCopy(owner + 0x80, hit.output_bytes, 0x20);
            if (compare) output_ok = output_ok && SafeCopy(context->Rsp + 0x30, hit.output_bytes + 0x20, 0x10);
            hit.output_valid = output_ok ? 1u : 0u;
            hit.producer_vtable = 0;
            hit.producer_target = g_image + 0x005C72B0;
            hit.producer_code_valid = SafeCopy(hit.producer_target, hit.producer_code, sizeof(hit.producer_code)) ? 1u : 0u;
            hit.modifier_code_valid = SafeCopy(g_image + 0x005CA6ED, hit.modifier_code, sizeof(hit.modifier_code)) ? 1u : 0u;
        }
        if (commit) {
            hit.output_valid = SafeCopy(hit.transform - 0x60, hit.output_bytes, 0x20) ? 1u : 0u;
        }
        if (!hit.nested_valid || !hit.output_valid || (!commit &&
            (!hit.owner_valid || !hit.additive_valid || !hit.input_valid ||
             !hit.producer_code_valid || !hit.modifier_code_valid))) {
            g_write_watch_failed.store(true, std::memory_order_release);
            if (g_stop_event) SetEvent(g_stop_event);
        }
        SafeRead(hit.transform, &hit.hit_x);
        SafeRead(hit.transform + 0x4, &hit.hit_z);
        SafeRead(hit.transform + 0x8, &hit.hit_y);
        const DWORD64 gpr[16] = {
            context->Rax, context->Rbx, context->Rcx, context->Rdx,
            context->Rsi, context->Rdi, context->Rbp, context->Rsp,
            context->R8, context->R9, context->R10, context->R11,
            context->R12, context->R13, context->R14, context->R15,
        };
        std::memcpy(hit.gpr, gpr, sizeof(gpr));
        const M128A* xmm = &context->Xmm0;
        for (unsigned int xmm_index = 0; xmm_index < 16; ++xmm_index) {
            hit.xmm[xmm_index][0] = xmm[xmm_index].Low;
            hit.xmm[xmm_index][1] = static_cast<DWORD64>(xmm[xmm_index].High);
        }
        if (context->Rip >= 32 && SafeCopy(static_cast<uintptr_t>(context->Rip - 32),
                                           hit.code, sizeof(hit.code))) {
            hit.code_valid = 1;
        }
        if (compare && g_config.causal_distance_patch) {
            hit.patch_previous_index = g_distance_previous.index;
            hit.patch_current_index = g_distance_current.index;
            hit.patch_delta = g_distance_previous.delta;
            DWORD raw_bits = static_cast<DWORD>(context->Xmm0.Low);
            std::memcpy(&hit.patch_raw, &raw_bits, sizeof(raw_bits));
            const unsigned char expected_compare[3] = {0x0F, 0x2F, 0xF8};
            const bool code_ok = hit.code_valid &&
                std::memcmp(hit.code + 32, expected_compare, sizeof(expected_compare)) == 0;
            float positions[6]{};
            std::memcpy(positions, hit.output_bytes, 12);
            std::memcpy(positions + 3, hit.output_bytes + 16, 12);
            const bool pair_ok = code_ok && !g_write_watch_failed.load(std::memory_order_acquire) &&
                std::memcmp(hit.output_bytes, hit.output_bytes + 32, 12) == 0 &&
                std::memcmp(hit.output_bytes + 16, &context->Xmm6, 12) == 0 &&
                MatchDistanceFrames(g_distance_previous, g_distance_current, hit.update_index,
                    hit.transform - 0xE0, positions, positions + 3);
            if (pair_ok && NormalizeDistanceTest(hit.patch_raw, hit.patch_delta, &hit.patch_corrected)) {
                DWORD corrected_bits = 0;
                std::memcpy(&corrected_bits, &hit.patch_corrected, sizeof(corrected_bits));
                context->Xmm0.Low = (context->Xmm0.Low & 0xFFFFFFFF00000000ull) | corrected_bits;
                hit.patch_applied = 1;
            } else {
                hit.patch_failed = 1;
                g_write_watch_failed.store(true, std::memory_order_release);
                if (g_stop_event) SetEvent(g_stop_event);
            }
        }
    } else {
        g_write_watch_overflow.store(1, std::memory_order_release);
        g_write_watch_failed.store(true, std::memory_order_release);
        if (g_stop_event) SetEvent(g_stop_event);
    }
    context->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}

bool ValidWriteWatchConfiguration() {
    // This dedicated binary must never fall back to the old broad hook mode.
    if (!g_config.locomotion_write_watch || !g_config.causal_distance_patch) return false;
    if (g_config.locomotion_writer_mirror) return false;
    if (g_config.locomotion_write_watch && g_config.locomotion_writer_mirror) {
        return false;
    }
    const bool base_valid = g_config.locomotion_trials &&
                            g_config.locomotion_phase_rca &&
                            g_config.enable_timing_override;
    if (g_config.locomotion_write_watch) {
        return base_valid &&
               (std::fabs(g_config.target_fps - 60.0f) < 0.001f ||
                std::fabs(g_config.target_fps - 165.0f) < 0.001f);
    }
    return base_valid &&
           (std::fabs(g_config.target_fps - 60.0f) < 0.001f ||
            std::fabs(g_config.target_fps - 165.0f) < 0.001f);
}

bool InitializeWriteWatch() {
    if (!DebugCaptureEnabled()) return true;
    g_write_watch_request_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    g_write_watch_done_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_write_watch_request_event || !g_write_watch_done_event) return false;
    g_write_watch_veh = AddVectoredExceptionHandler(1, WriteWatchExceptionHandler);
    if (!g_write_watch_veh) return false;
    g_write_watch_controller_thread =
        CreateThread(nullptr, 0, WriteWatchController, nullptr, 0, nullptr);
    return g_write_watch_controller_thread != nullptr;
}

bool ShutdownWriteWatch() {
    if (!DebugCaptureEnabled()) return true;
    if (!DisarmWriteWatch()) return false;
    if (g_write_watch_controller_thread) {
        if (!IssueWriteWatchCommand(WatchCommand::stop, 0, nullptr)) return false;
        WaitForSingleObject(g_write_watch_controller_thread, 1000);
        CloseHandle(g_write_watch_controller_thread);
        g_write_watch_controller_thread = nullptr;
    }
    if (g_write_watch_veh) {
        RemoveVectoredExceptionHandler(g_write_watch_veh);
        g_write_watch_veh = nullptr;
    }
    if (g_write_watch_request_event) {
        CloseHandle(g_write_watch_request_event);
        g_write_watch_request_event = nullptr;
    }
    if (g_write_watch_done_event) {
        CloseHandle(g_write_watch_done_event);
        g_write_watch_done_event = nullptr;
    }
    return true;
}

InputSnapshot ReadInputSnapshot() {
    InputSnapshot input{};
    const int keys[] = {'Z', 'Q', 'S', 'D', VK_SHIFT, VK_SPACE, VK_LBUTTON, VK_RBUTTON};
    for (unsigned int i = 0; i < _countof(keys); ++i) {
        if ((GetAsyncKeyState(keys[i]) & 0x8000) != 0) input.keyboard_mask |= 1u << i;
    }
    if (g_xinput_get_state) {
        XINPUT_STATE state{};
        if (g_xinput_get_state(0, &state) == ERROR_SUCCESS) {
            input.controller_connected = 1;
            input.controller_buttons = state.Gamepad.wButtons;
            input.left_x = state.Gamepad.sThumbLX;
            input.left_y = state.Gamepad.sThumbLY;
            input.right_x = state.Gamepad.sThumbRX;
            input.right_y = state.Gamepad.sThumbRY;
            input.left_trigger = state.Gamepad.bLeftTrigger;
            input.right_trigger = state.Gamepad.bRightTrigger;
        }
    }
    return input;
}

bool DeterministicKeyboardSprintHeld(const InputSnapshot& input) {
    constexpr unsigned int kRequired = (1u << 0) | (1u << 5);  // Z + Space on AZERTY
    constexpr unsigned int kDisallowed = (1u << 1) | (1u << 2) | (1u << 3) |
                                         (1u << 4) | (1u << 6) | (1u << 7);
    const bool keyboard_ok = (input.keyboard_mask & kRequired) == kRequired &&
                             (input.keyboard_mask & kDisallowed) == 0;
    const bool controller_neutral = input.controller_buttons == 0 &&
                                    std::abs(input.left_x) <= 4096 &&
                                    std::abs(input.left_y) <= 4096 &&
                                    std::abs(input.right_x) <= 4096 &&
                                    std::abs(input.right_y) <= 4096 &&
                                    input.left_trigger == 0 && input.right_trigger == 0;
    return keyboard_ok && controller_neutral;
}

void LogLocomotionEvent(const char* event, unsigned int trial,
                        unsigned long long update_index, double accumulated_delta,
                        const PlayerSnapshot& player, const InputSnapshot& input,
                        long long qpc) {
    if (!g_locomotion_events) return;
    EnterCriticalSection(&g_log_lock);
    std::fprintf(
        g_locomotion_events,
        "%llu,%lld,%s,%u,%llu,%.12g,%u,%.9g,%.9g,%.9g,%.9g,%u,%d,%.9g,%.9g,%.9g,"
        "0x%X,%u,0x%X,%d,%d,%d,%d,%u,%u\n",
        NextSequence(), qpc, event, trial, update_index, accumulated_delta,
        player.position_valid ? 1u : 0u, player.x, player.z, player.y, player.angle,
        player.animation_valid ? 1u : 0u, player.animation_id, player.animation_time,
        player.animation_max, player.play_speed, input.keyboard_mask,
        input.controller_connected, input.controller_buttons, input.left_x, input.left_y,
        input.right_x, input.right_y, input.left_trigger, input.right_trigger);
    std::fflush(g_locomotion_events);
    LeaveCriticalSection(&g_log_lock);
}

void LogLocomotionUpdate(const char* phase, unsigned int trial,
                         unsigned long long update_index, float delta,
                         double accumulated_before, double accumulated_after,
                         long long qpc_before, long long qpc_after,
                         const PlayerSnapshot& before, const PlayerSnapshot& after,
                         const InputSnapshot& input_before, const InputSnapshot& input_after) {
    if (!g_locomotion_updates) return;
    EnterCriticalSection(&g_log_lock);
    std::fprintf(
        g_locomotion_updates,
        "%llu,%lld,%lld,%lu,%u,%s,%llu,%.9g,%.12g,%.12g,"
        "%u,%.9g,%.9g,%.9g,%.9g,%u,%.9g,%.9g,%.9g,%.9g,"
        "%u,%d,%.9g,%.9g,%.9g,%u,%d,%.9g,%.9g,%.9g,"
        "0x%X,%u,0x%X,%d,%d,%d,%d,%u,%u,"
        "0x%X,%u,0x%X,%d,%d,%d,%d,%u,%u\n",
        NextSequence(), qpc_before, qpc_after, GetCurrentThreadId(), trial, phase,
        update_index, delta, accumulated_before, accumulated_after,
        before.position_valid ? 1u : 0u, before.x, before.z, before.y, before.angle,
        after.position_valid ? 1u : 0u, after.x, after.z, after.y, after.angle,
        before.animation_valid ? 1u : 0u, before.animation_id, before.animation_time,
        before.animation_max, before.play_speed,
        after.animation_valid ? 1u : 0u, after.animation_id, after.animation_time,
        after.animation_max, after.play_speed,
        input_before.keyboard_mask, input_before.controller_connected,
        input_before.controller_buttons, input_before.left_x, input_before.left_y,
        input_before.right_x, input_before.right_y, input_before.left_trigger,
        input_before.right_trigger, input_after.keyboard_mask,
        input_after.controller_connected, input_after.controller_buttons,
        input_after.left_x, input_after.left_y, input_after.right_x, input_after.right_y,
        input_after.left_trigger, input_after.right_trigger);
    LeaveCriticalSection(&g_log_lock);
}

void LogLocomotionPhase(const char* phase, const char* event,
                        unsigned int call_index, void* self, void* context, uintptr_t caller,
                        const PlayerSnapshot& player) {
    if (!g_locomotion_phases) return;
    EnterCriticalSection(&g_log_lock);
    std::fprintf(
        g_locomotion_phases,
        "%llu,%lld,%lu,%u,%u,%s,%s,%u,0x%016llX,0x%016llX,"
        "0x%llX,%u,%.9g,%.9g,%.9g,%.9g,%u,%d,%.9g,%.9g,%.9g\n",
        NextSequence(), Qpc(), GetCurrentThreadId(), g_locomotion_trial,
        static_cast<unsigned int>(g_broad_update_index.load(std::memory_order_relaxed)),
        phase, event, call_index,
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(self)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(context)),
        static_cast<unsigned long long>(caller >= g_image ? caller - g_image : 0),
        player.position_valid ? 1u : 0u, player.x, player.z, player.y, player.angle,
        player.animation_valid ? 1u : 0u, player.animation_id, player.animation_time,
        player.animation_max, player.play_speed);
    LeaveCriticalSection(&g_log_lock);
}

bool CaptureLocomotionPhase() {
    return g_config.locomotion_phase_rca &&
           g_capture_active.load(std::memory_order_acquire) &&
           g_locomotion_trial != 0 && (g_locomotion_measuring || g_locomotion_stable_delta > 0.0);
}

void __fastcall HookD56780(void* self) {
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const bool capture = CaptureLocomotionPhase();
    if (capture) {
        LogLocomotionPhase("D56780", "before", 0, self, nullptr, caller,
                           ReadPlayerSnapshot());
    }
    g_orig_d56780(self);
    if (capture && CaptureLocomotionPhase()) {
        LogLocomotionPhase("D56780", "after", 0, self, nullptr, caller,
                           ReadPlayerSnapshot());
    }
}

void __fastcall HookD56930(void* self) {
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const bool capture = CaptureLocomotionPhase();
    if (capture) {
        LogLocomotionPhase("D56930", "before", 0, self, nullptr, caller,
                           ReadPlayerSnapshot());
    }
    g_orig_d56930(self);
    if (capture && CaptureLocomotionPhase()) {
        LogLocomotionPhase("D56930", "after", 0, self, nullptr, caller,
                           ReadPlayerSnapshot());
    }
}

void __fastcall HookD56A90(void* self) {
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const bool capture = CaptureLocomotionPhase();
    if (capture) {
        LogLocomotionPhase("D56A90", "before", 0, self, nullptr, caller,
                           ReadPlayerSnapshot());
    }
    g_orig_d56a90(self);
    if (capture && CaptureLocomotionPhase()) {
        LogLocomotionPhase("D56A90", "after", 0, self, nullptr, caller,
                           ReadPlayerSnapshot());
    }
}

void __fastcall HookD58E20(void* self, void* context) {
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const bool capture = CaptureLocomotionPhase();
    if (capture) {
        LogLocomotionPhase("D58E20", "before", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
    g_orig_d58e20(self, context);
    if (capture && CaptureLocomotionPhase()) {
        LogLocomotionPhase("D58E20", "after", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
}

void __fastcall Hook2344730(void* self, void* context) {
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const uintptr_t caller_rva = caller >= g_image ? caller - g_image : 0;
    const bool capture = CaptureLocomotionPhase();
    if (capture && DebugCaptureEnabled() &&
        caller_rva == kCallerRvaDE50EC &&
        g_write_watch_active.load(std::memory_order_acquire)) {
        uintptr_t shared_context = 0;
        EnterCriticalSection(&g_write_watch_lock);
        shared_context = g_write_watch_shared_context;
        LeaveCriticalSection(&g_write_watch_lock);
        if (shared_context == reinterpret_cast<uintptr_t>(context)) {
            DisarmWriteWatch();
        }
    }
    if (capture) {
        LogLocomotionPhase("2344730", "before", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
    g_orig_2344730(self, context);
    if (capture && CaptureLocomotionPhase()) {
        LogLocomotionPhase("2344730", "after", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
    if (capture && CaptureLocomotionPhase() && DebugCaptureEnabled() &&
        caller_rva == kCallerRvaA1429C) {
        const PlayerSnapshot snapshot = ReadPlayerSnapshot();
        ArmWriteWatch(snapshot, context);
    }
}

void __fastcall HookD5B140(void* self, void* context) {
    const bool outermost = g_d5b140_depth == 0;
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const bool capture_before = outermost && CaptureLocomotionPhase();
    if (capture_before) {
        LogLocomotionPhase("D5B140", "before", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
    ++g_d5b140_depth;
    g_orig_d5b140(self, context);
    --g_d5b140_depth;
    if (outermost && CaptureLocomotionPhase()) {
        LogLocomotionPhase("D5B140", "after", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
}

void __fastcall HookE6E750(void* self, void* context) {
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const bool capture = g_d5b140_depth != 0 && CaptureLocomotionPhase();
    if (capture) {
        LogLocomotionPhase("E6E750", "before", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
    g_orig_e6e750(self, context);
    if (capture && CaptureLocomotionPhase()) {
        LogLocomotionPhase("E6E750", "after", 0, self, context, caller,
                           ReadPlayerSnapshot());
    }
}

void UpdateLocomotionTrial(float delta, unsigned long long update_index,
                           double accumulated_before, double accumulated_after,
                           long long qpc_before, long long qpc_after,
                           const PlayerSnapshot& before, const PlayerSnapshot& after,
                           const InputSnapshot& input_before, const InputSnapshot& input_after) {
    if (!g_config.locomotion_trials || !g_capture_active.load(std::memory_order_acquire)) return;
    if (g_locomotion_arm_requested.exchange(false, std::memory_order_acq_rel)) {
        g_locomotion_trial = g_locomotion_next_trial.fetch_add(1, std::memory_order_relaxed);
        g_locomotion_stable_delta = 1.0; // capture gate only, never a movement operand
        g_locomotion_measured_delta = 0.0;
        g_locomotion_warmup_complete = false;
        g_locomotion_measuring = true;
        LogLocomotionEvent("armed", g_locomotion_trial, update_index, accumulated_before,
                           before, input_before, qpc_before);
        LogLocomotionEvent("measure_start", g_locomotion_trial, update_index, accumulated_before,
                           before, input_before, qpc_before);
    }
    if (g_locomotion_trial == 0) return;
    // A roll is an impulse/release action. Keep the identical correction active
    // for the whole bounded capture, independently of held sprint keys.
    LogLocomotionUpdate("measure", g_locomotion_trial, update_index, delta,
                        accumulated_before, accumulated_after, qpc_before, qpc_after,
                        before, after, input_before, input_after);
    g_locomotion_measured_delta += static_cast<double>(delta);
    if (g_locomotion_measured_delta >= g_config.locomotion_measure_delta) {
        LogLocomotionEvent("measure_end", g_locomotion_trial, update_index,
                           accumulated_after, after, input_after, qpc_after);
        LogLocomotionEvent("trial_end", g_locomotion_trial, update_index,
                           accumulated_after, after, input_after, qpc_after);
        g_locomotion_trial = 0;
        g_locomotion_measuring = false;
        g_locomotion_stable_delta = 0.0;
        g_locomotion_busy.store(false, std::memory_order_release);
    }
}

const char* MarkerName(int marker) {
    switch (marker) {
        case 1: return "idle";
        case 2: return "sprint_continuous";
        case 3: return "roll";
        case 4: return "representative_animation";
        case 5: return "collision_obstacle";
        case 6: return "camera_rotation";
        case 7: return "physics_movement";
        default: return "none";
    }
}

template <size_t N>
ConsumerAggregate* FindAggregate(ConsumerAggregate (&table)[N], uintptr_t object) {
    size_t start = (object >> 4) % N;
    for (size_t probe = 0; probe < N; ++probe) {
        ConsumerAggregate& entry = table[(start + probe) % N];
        if (entry.object == object || entry.object == 0) {
            if (entry.object == 0) entry.object = object;
            return &entry;
        }
    }
    return nullptr;
}

template <size_t N>
void RecordConsumer(ConsumerAggregate (&table)[N], uintptr_t object, float delta,
                    bool active, float elapsed_before, float elapsed_after,
                    float output_before, float output_after,
                    const float xyz_before[3], const float xyz_after[3]) {
    if (!object || !g_capture_active.load(std::memory_order_acquire)) return;
    long long now = Qpc();
    EnterCriticalSection(&g_consumer_lock);
    ConsumerAggregate* entry = FindAggregate(table, object);
    if (entry) {
        bool first = entry->calls == 0;
        if (first) {
            entry->first_qpc = now;
            entry->min_delta = delta;
            entry->max_delta = delta;
            entry->first_elapsed = elapsed_before;
            entry->first_output = output_before;
            for (size_t i = 0; i < 3; ++i) entry->first_xyz[i] = xyz_before[i];
        }
        entry->last_qpc = now;
        entry->calls += 1;
        if (active) entry->active_calls += 1;
        entry->sum_delta += delta;
        if (delta < entry->min_delta) entry->min_delta = delta;
        if (delta > entry->max_delta) entry->max_delta = delta;
        float increment = elapsed_after - elapsed_before;
        if (increment < -1e-6f) {
            entry->resets += 1;
        } else {
            entry->sum_elapsed_increment += increment;
            if (std::fabs(increment - delta) <= 1e-5f) entry->increment_matches += 1;
        }
        entry->last_elapsed = elapsed_after;
        entry->last_output = output_after;
        for (size_t i = 0; i < 3; ++i) entry->last_xyz[i] = xyz_after[i];
    }
    LeaveCriticalSection(&g_consumer_lock);
}

void TrimLine(wchar_t* text) {
    if (!text) return;
    size_t length = std::wcslen(text);
    while (length && (text[length - 1] == L'\r' || text[length - 1] == L'\n' ||
                      text[length - 1] == L' ' || text[length - 1] == L'\t')) {
        text[--length] = L'\0';
    }
}

bool ParseBool(const wchar_t* value) {
    return value && (_wcsicmp(value, L"1") == 0 || _wcsicmp(value, L"true") == 0 ||
                     _wcsicmp(value, L"yes") == 0);
}

void LoadConfig() {
    wchar_t module_path[MAX_PATH * 4]{};
    GetModuleFileNameW(g_self, module_path, _countof(module_path));
    wchar_t* slash = std::wcsrchr(module_path, L'\\');
    if (slash) *slash = L'\0';

    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\..\\pass_c_config.txt", module_path);
    wchar_t absolute[MAX_PATH * 4]{};
    GetFullPathNameW(path, _countof(absolute), absolute, nullptr);

    FILE* file = nullptr;
    _wfopen_s(&file, absolute, L"rt, ccs=UTF-8");
    if (!file) return;

    wchar_t line[512]{};
    while (std::fgetws(line, _countof(line), file)) {
        TrimLine(line);
        wchar_t* equals = std::wcschr(line, L'=');
        if (!equals) continue;
        *equals = L'\0';
        const wchar_t* key = line;
        const wchar_t* value = equals + 1;
        if (_wcsicmp(key, L"target_fps") == 0) {
            wchar_t* end = nullptr;
            float parsed = std::wcstof(value, &end);
            if (end != value && std::isfinite(parsed) && parsed >= 30.0f && parsed <= 300.0f) {
                g_config.target_fps = parsed;
            }
        } else if (_wcsicmp(key, L"auto_stop_seconds") == 0) {
            unsigned long parsed = std::wcstoul(value, nullptr, 10);
            if (parsed >= 10 && parsed <= 900) g_config.auto_stop_seconds = static_cast<unsigned int>(parsed);
        } else if (_wcsicmp(key, L"enable_timing_override") == 0) {
            g_config.enable_timing_override = ParseBool(value);
        } else if (_wcsicmp(key, L"force_present_zero") == 0) {
            g_config.force_present_zero = ParseBool(value);
        } else if (_wcsicmp(key, L"callback_detail") == 0) {
            g_config.callback_detail = ParseBool(value);
        } else if (_wcsicmp(key, L"marker_hotkeys") == 0) {
            g_config.marker_hotkeys = ParseBool(value);
        } else if (_wcsicmp(key, L"locomotion_trials") == 0) {
            g_config.locomotion_trials = ParseBool(value);
        } else if (_wcsicmp(key, L"locomotion_phase_rca") == 0) {
            g_config.locomotion_phase_rca = ParseBool(value);
        } else if (_wcsicmp(key, L"locomotion_write_watch") == 0) {
            g_config.locomotion_write_watch = ParseBool(value);
        } else if (_wcsicmp(key, L"locomotion_writer_mirror") == 0) {
            g_config.locomotion_writer_mirror = ParseBool(value);
        } else if (_wcsicmp(key, L"semantic_sample_stride") == 0) {
            unsigned long parsed = std::wcstoul(value, nullptr, 10);
            if (parsed >= 1 && parsed <= 120) {
                g_config.semantic_sample_stride = static_cast<unsigned int>(parsed);
            }
        } else if (_wcsicmp(key, L"locomotion_warmup_delta") == 0) {
            wchar_t* end = nullptr;
            float parsed = std::wcstof(value, &end);
            if (end != value && std::isfinite(parsed) && parsed >= 0.25f && parsed <= 5.0f) {
                g_config.locomotion_warmup_delta = parsed;
            }
        } else if (_wcsicmp(key, L"locomotion_measure_delta") == 0) {
            wchar_t* end = nullptr;
            float parsed = std::wcstof(value, &end);
            if (end != value && std::isfinite(parsed) && parsed >= 1.0f && parsed <= 10.0f) {
                g_config.locomotion_measure_delta = parsed;
            }
        } else if (_wcsicmp(key, L"causal_distance_patch") == 0) {
            g_config.causal_distance_patch = _wtoi(value) != 0;
        } else if (_wcsicmp(key, L"condition") == 0) {
            wcsncpy_s(g_config.condition, value, _TRUNCATE);
        }
    }
    std::fclose(file);
}

void FlushAll() {
    FILE* files[] = {g_calls, g_callbacks, g_central, g_selector, g_pacing, g_present,
                     g_hooks, g_override, g_consumers, g_semantic, g_markers,
                     g_locomotion_updates, g_locomotion_events, g_locomotion_phases,
                     g_locomotion_write_watch, g_locomotion_writer_mirror};
    for (FILE* file : files) {
        if (file) std::fflush(file);
    }
}

void LogOverride(const char* action, uintptr_t object, float fps_before, unsigned char enable_before,
                 float fps_after, unsigned char enable_after, bool success) {
    if (!g_override) return;
    EnterCriticalSection(&g_log_lock);
    std::fprintf(g_override, "%llu,%lld,%s,0x%016llX,%.9g,%u,%.9g,%u,%u\n",
                 NextSequence(), Qpc(), action, static_cast<unsigned long long>(object),
                 fps_before, static_cast<unsigned>(enable_before), fps_after,
                 static_cast<unsigned>(enable_after), success ? 1u : 0u);
    std::fflush(g_override);
    LeaveCriticalSection(&g_log_lock);
}

void ApplyOverrideIfPossible(const char* reapply_action = "reapply_after_drift") {
    if (!g_config.enable_timing_override) return;
    uintptr_t object = 0;
    if (!SafeRead(g_image + kRvaCentralGlobal, &object) || object == 0) return;

    if (g_override_applied && object != g_override_object) {
        g_capture_active.store(false, std::memory_order_release);
        LogOverride("object_changed", g_override_object, g_config.target_fps, 1, 0.0f, 0, false);
        g_override_applied = false;
        g_override_object = 0;
    }

    float current_fps = 0.0f;
    unsigned char current_enable = 0;
    if (!SafeRead(object + 0x354, &current_fps) || !SafeRead(object + 0x358, &current_enable)) return;

    if (!g_override_applied) {
        g_override_object = object;
        g_original_fps = current_fps;
        g_original_enable = current_enable;
        bool ok_fps = SafeWrite(object + 0x354, g_config.target_fps);
        const unsigned char enabled = 1;
        bool ok_enable = SafeWrite(object + 0x358, enabled);
        float after_fps = 0.0f;
        unsigned char after_enable = 0;
        bool ok_read = SafeRead(object + 0x354, &after_fps) && SafeRead(object + 0x358, &after_enable);
        g_override_applied = ok_fps && ok_enable && ok_read &&
                             std::fabs(after_fps - g_config.target_fps) < 0.001f && after_enable == 1;
        LogOverride("apply", object, current_fps, current_enable, after_fps, after_enable,
                    g_override_applied);
        g_capture_active.store(g_override_applied, std::memory_order_release);
        return;
    }

    if (std::fabs(current_fps - g_config.target_fps) >= 0.001f || current_enable != 1) {
        bool ok_fps = SafeWrite(object + 0x354, g_config.target_fps);
        const unsigned char enabled = 1;
        bool ok_enable = SafeWrite(object + 0x358, enabled);
        float after_fps = 0.0f;
        unsigned char after_enable = 0;
        bool ok_read = SafeRead(object + 0x354, &after_fps) && SafeRead(object + 0x358, &after_enable);
        LogOverride(reapply_action, object, current_fps, current_enable, after_fps,
                    after_enable, ok_fps && ok_enable && ok_read);
    }
}

void RestoreOverride() {
    if (!g_override_applied || g_override_object == 0) return;
    g_capture_active.store(false, std::memory_order_release);
    float before_fps = 0.0f;
    unsigned char before_enable = 0;
    SafeRead(g_override_object + 0x354, &before_fps);
    SafeRead(g_override_object + 0x358, &before_enable);
    const unsigned char disabled = 0;
    bool ok_disable = SafeWrite(g_override_object + 0x358, disabled);
    bool ok_fps = SafeWrite(g_override_object + 0x354, g_original_fps);
    bool ok_enable = SafeWrite(g_override_object + 0x358, g_original_enable);
    float after_fps = 0.0f;
    unsigned char after_enable = 0;
    bool ok_read = SafeRead(g_override_object + 0x354, &after_fps) &&
                   SafeRead(g_override_object + 0x358, &after_enable);
    bool success = ok_disable && ok_fps && ok_enable && ok_read &&
                   std::fabs(after_fps - g_original_fps) < 0.001f &&
                   after_enable == g_original_enable;
    LogOverride("restore", g_override_object, before_fps, before_enable, after_fps,
                after_enable, success);
    g_override_applied = false;
}

FILE* OpenCsv(const wchar_t* name, const char* header) {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\%s", g_session_dir, name);
    FILE* file = OpenSharedCaptureLog(path);
    if (file) {
        static char buffers[16][1 << 20];
        static LONG buffer_index = -1;
        LONG index = InterlockedIncrement(&buffer_index);
        if (index >= 0 && index < static_cast<LONG>(_countof(buffers))) {
            std::setvbuf(file, buffers[index], _IOFBF, sizeof(buffers[index]));
        }
        std::fprintf(file, "%s\n", header);
    }
    return file;
}

void BuildSessionDirectory() {
    wchar_t module_path[MAX_PATH * 4]{};
    GetModuleFileNameW(g_self, module_path, _countof(module_path));
    wchar_t* slash = std::wcsrchr(module_path, L'\\');
    if (slash) *slash = L'\0';

    wchar_t sessions_relative[MAX_PATH * 4]{};
    _snwprintf_s(sessions_relative, _countof(sessions_relative), _TRUNCATE,
                 L"%s\\..\\..\\sessions", module_path);
    wchar_t sessions[MAX_PATH * 4]{};
    GetFullPathNameW(sessions_relative, _countof(sessions), sessions, nullptr);
    CreateDirectoryW(sessions, nullptr);

    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    _snwprintf_s(g_session_dir, _countof(g_session_dir), _TRUNCATE,
                 L"%s\\%04u%02u%02uT%02u%02u%02uZ-pid%lu",
                 sessions, utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute,
                 utc.wSecond, GetCurrentProcessId());
    CreateDirectoryW(g_session_dir, nullptr);
}

void WriteSessionInfo() {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\session-info.txt", g_session_dir);
    FILE* file = nullptr;
    _wfopen_s(&file, path, L"wt, ccs=UTF-8");
    if (!file) return;
    wchar_t exe[MAX_PATH * 4]{};
    GetModuleFileNameW(nullptr, exe, _countof(exe));
    std::fwprintf(file, L"schema=I-0007-causal-distance-patch-v1\n");
    std::fwprintf(file, L"pid=%lu\n", GetCurrentProcessId());
    std::fwprintf(file, L"exe=%s\n", exe);
    std::fwprintf(file, L"image_base=0x%016llX\n", static_cast<unsigned long long>(g_image));
    std::fwprintf(file, L"qpc_frequency=%lld\n", g_frequency.QuadPart);
    std::fwprintf(file, L"condition=%s\n", g_config.condition);
    std::fwprintf(file, L"target_fps=%.9g\n", g_config.target_fps);
    std::fwprintf(file, L"enable_timing_override=%u\n", g_config.enable_timing_override ? 1u : 0u);
    std::fwprintf(file, L"force_present_zero=%u\n", g_config.force_present_zero ? 1u : 0u);
    std::fwprintf(file, L"callback_detail=%u\n", g_config.callback_detail ? 1u : 0u);
    std::fwprintf(file, L"marker_hotkeys=%u\n", g_config.marker_hotkeys ? 1u : 0u);
    std::fwprintf(file, L"locomotion_trials=%u\n", g_config.locomotion_trials ? 1u : 0u);
    std::fwprintf(file, L"locomotion_phase_rca=%u\n",
                  g_config.locomotion_phase_rca ? 1u : 0u);
    std::fwprintf(file, L"locomotion_write_watch=%u\n",
                  g_config.locomotion_write_watch ? 1u : 0u);
    std::fwprintf(file, L"locomotion_writer_mirror=%u\n",
                  g_config.locomotion_writer_mirror ? 1u : 0u);
    std::fwprintf(file, L"causal_distance_patch=%u\n", g_config.causal_distance_patch ? 1u : 0u);
    std::fwprintf(file, L"patch_site=005C73D8_XMM0_low_only\n");
    std::fwprintf(file, L"patch_lifetime=bounded_action_window_until_trial_end\n");
    std::fwprintf(file, L"patch_formula=raw_test*((1/60)/verified_previous_delta)\n");
    std::fwprintf(file, L"locomotion_action=single_forward_roll\n");
    std::fwprintf(file, L"locomotion_input=F4_then_single_forward_roll_release_all\n");
    std::fwprintf(file, L"semantic_sample_stride=%u\n", g_config.semantic_sample_stride);
    std::fwprintf(file, L"locomotion_warmup_delta=%.9g\n", g_config.locomotion_warmup_delta);
    std::fwprintf(file, L"locomotion_measure_delta=%.9g\n", g_config.locomotion_measure_delta);
    std::fwprintf(file, L"auto_stop_seconds=%u\n", g_config.auto_stop_seconds);
    std::fwprintf(file, L"world_chr_man_global_rva=0x%08llX\n",
                  static_cast<unsigned long long>(kRvaWorldChrManGlobal));
    std::fwprintf(file, L"probe_mode=reversible_semantic_invariant_comparison\n");
    std::fwprintf(file, L"game_directory_writes=none\n");
    std::fclose(file);
}

void SnapshotModules(const wchar_t* name) {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\%s", g_session_dir, name);
    FILE* file = nullptr;
    _wfopen_s(&file, path, L"wt, ccs=UTF-8");
    if (!file) return;
    std::fwprintf(file, L"module,base,size,path\n");
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                                           GetCurrentProcessId());
    if (snap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Module32FirstW(snap, &entry)) {
            do {
                std::fwprintf(file, L"\"%s\",0x%016llX,%lu,\"%s\"\n", entry.szModule,
                              static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(entry.modBaseAddr)),
                              entry.modBaseSize, entry.szExePath);
            } while (Module32NextW(snap, &entry));
        }
        CloseHandle(snap);
    }
    std::fclose(file);
}

void SnapshotCentral(const char* site, const char* phase) {
    uintptr_t object = 0;
    if (!SafeRead(g_image + kRvaCentralGlobal, &object) || object == 0 || !g_central) return;

    long long stamps[8]{};
    const unsigned stamp_offsets[8] = {0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58};
    for (size_t i = 0; i < 8; ++i) SafeRead(object + stamp_offsets[i], &stamps[i]);

    float scalar[57]{};
    const unsigned scalar_offsets[57] = {
        0x18, 0x260, 0x264, 0x268, 0x26C, 0x270, 0x274,
        0x27C, 0x280, 0x284, 0x288, 0x28C, 0x290, 0x294, 0x298,
        0x29C, 0x2A0, 0x2A4, 0x2A8, 0x2AC, 0x2B0, 0x2B4, 0x2B8,
        0x2BC, 0x2C0,
        0x2C4, 0x2C8, 0x2CC, 0x2D0, 0x2D4, 0x2D8, 0x2DC, 0x2E0,
        0x2E4, 0x2E8, 0x2EC, 0x2F0, 0x2F4, 0x2F8, 0x2FC, 0x300,
        0x304, 0x308, 0x30C, 0x310, 0x314, 0x318, 0x31C, 0x320,
        0x324, 0x328, 0x32C, 0x330, 0x334, 0x338, 0x33C, 0x340
    };
    for (size_t i = 0; i < 57; ++i) SafeRead(object + scalar_offsets[i], &scalar[i]);

    unsigned char flags[5]{};
    SafeRead(object + 0x278, &flags[0]);
    SafeRead(object + 0x279, &flags[1]);
    SafeRead(object + 0x27A, &flags[2]);
    SafeRead(object + 0x27B, &flags[3]);
    SafeRead(object + 0x358, &flags[4]);
    float fps = 0.0f;
    SafeRead(object + 0x354, &fps);
    long long v344 = 0;
    unsigned int v34c = 0, v350 = 0, v35c = 0;
    unsigned char v360 = 0;
    SafeRead(object + 0x344, &v344);
    SafeRead(object + 0x34C, &v34c);
    SafeRead(object + 0x350, &v350);
    SafeRead(object + 0x35C, &v35c);
    SafeRead(object + 0x360, &v360);

    EnterCriticalSection(&g_log_lock);
    std::fprintf(g_central, "%llu,%lld,%lu,%s,%s,0x%016llX",
                 NextSequence(), Qpc(), GetCurrentThreadId(), site, phase,
                 static_cast<unsigned long long>(object));
    for (long long value : stamps) std::fprintf(g_central, ",%lld", value);
    for (float value : scalar) std::fprintf(g_central, ",%.9g", value);
    std::fprintf(g_central, ",%u,%u,%u,%u,%lld,%u,%u,%.9g,%u,%u,%u\n",
                 flags[0], flags[1], flags[2], flags[3], v344, flags[4], v360,
                 fps, v34c, v350, v35c);
    LeaveCriticalSection(&g_log_lock);
}

void LogSemanticSample(float delta, unsigned long long update_index, double accumulated_delta) {
    if (!g_semantic || !g_capture_active.load(std::memory_order_acquire)) return;
    if ((update_index - 1) % g_config.semantic_sample_stride != 0) return;
    const long long probe_start = Qpc();
    const PlayerSnapshot player = ReadPlayerSnapshot();
    const InputSnapshot input = ReadInputSnapshot();
    const int marker = g_active_marker.load(std::memory_order_acquire);
    EnterCriticalSection(&g_log_lock);
    std::fprintf(g_semantic,
                 "%llu,%lld,%lu,%llu,%d,%s,%.9g,%.12g,0x%016llX,0x%016llX,0x%016llX,%u,"
                 "%.9g,%.9g,%.9g,%.9g,%u,%d,%.9g,%.9g,%.9g,%d,%d,0x%X,%u,0x%X,%d,%d,%d,%d,%u,%u\n",
                 NextSequence(), Qpc(), GetCurrentThreadId(), update_index, marker,
                 MarkerName(marker), delta, accumulated_delta,
                 static_cast<unsigned long long>(player.world),
                 static_cast<unsigned long long>(player.player),
                 static_cast<unsigned long long>(player.transform),
                 player.position_valid ? 1u : 0u, player.x, player.z, player.y, player.angle,
                 player.animation_valid ? 1u : 0u, player.animation_id, player.animation_time,
                 player.animation_max, player.play_speed, player.hp, player.stamina,
                 input.keyboard_mask, input.controller_connected, input.controller_buttons,
                 input.left_x, input.left_y, input.right_x, input.right_y,
                 input.left_trigger, input.right_trigger);
    LeaveCriticalSection(&g_log_lock);
    const unsigned long long probe_ticks =
        static_cast<unsigned long long>(Qpc() - probe_start);
    g_semantic_probe_samples.fetch_add(1, std::memory_order_relaxed);
    g_semantic_probe_sum_ticks.fetch_add(probe_ticks, std::memory_order_relaxed);
    unsigned long long observed_max = g_semantic_probe_max_ticks.load(std::memory_order_relaxed);
    while (probe_ticks > observed_max &&
           !g_semantic_probe_max_ticks.compare_exchange_weak(
               observed_max, probe_ticks, std::memory_order_relaxed)) {
    }
}

void WriteProbeOverhead() {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE,
                 L"%s\\probe-overhead.csv", g_session_dir);
    FILE* file = nullptr;
    _wfopen_s(&file, path, L"wb");
    if (!file) return;
    const unsigned long long samples =
        g_semantic_probe_samples.load(std::memory_order_relaxed);
    const unsigned long long sum_ticks =
        g_semantic_probe_sum_ticks.load(std::memory_order_relaxed);
    const unsigned long long max_ticks =
        g_semantic_probe_max_ticks.load(std::memory_order_relaxed);
    const double mean_us = samples && g_frequency.QuadPart
        ? static_cast<double>(sum_ticks) * 1000000.0 /
              static_cast<double>(g_frequency.QuadPart) / static_cast<double>(samples)
        : 0.0;
    const double max_us = g_frequency.QuadPart
        ? static_cast<double>(max_ticks) * 1000000.0 /
              static_cast<double>(g_frequency.QuadPart)
        : 0.0;
    std::fprintf(file, "samples,qpc_frequency,sum_ticks,max_ticks,mean_us,max_us\n");
    std::fprintf(file, "%llu,%lld,%llu,%llu,%.9g,%.9g\n",
                 samples, g_frequency.QuadPart, sum_ticks, max_ticks, mean_us, max_us);
    std::fclose(file);
}

void LogMarkerEvent(const char* event, int marker) {
    if (!g_markers) return;
    const PlayerSnapshot player = ReadPlayerSnapshot();
    EnterCriticalSection(&g_log_lock);
    std::fprintf(g_markers,
                 "%llu,%lld,%s,%d,%s,%llu,%.12g,%u,%.9g,%.9g,%.9g,%.9g,%u,%d,%.9g,%.9g,%.9g\n",
                 NextSequence(), Qpc(), event, marker, MarkerName(marker),
                 g_broad_update_index.load(std::memory_order_acquire),
                 g_accumulated_delta.load(std::memory_order_acquire),
                 player.position_valid ? 1u : 0u, player.x, player.z, player.y, player.angle,
                 player.animation_valid ? 1u : 0u, player.animation_id,
                 player.animation_time, player.animation_max, player.play_speed);
    std::fflush(g_markers);
    LeaveCriticalSection(&g_log_lock);
}

void PollMarkerHotkeys() {
    if (!g_config.marker_hotkeys) return;
    static bool f4_was_down = false;
    const bool f4_down = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
    if (f4_down && !f4_was_down && g_config.locomotion_trials) {
        bool expected = false;
        if (g_locomotion_busy.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel)) {
            g_locomotion_arm_requested.store(true, std::memory_order_release);
        }
    }
    f4_was_down = f4_down;

    static bool was_down[8]{};
    const int keys[8] = {VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12};
    for (int index = 0; index < 8; ++index) {
        const bool down = (GetAsyncKeyState(keys[index]) & 0x8000) != 0;
        if (down && !was_down[index]) {
            int active = g_active_marker.load(std::memory_order_acquire);
            if (index == 7) {
                if (active != 0) {
                    LogMarkerEvent("end", active);
                    g_active_marker.store(0, std::memory_order_release);
                }
            } else {
                const int selected = index + 1;
                if (active != 0) LogMarkerEvent("end", active);
                if (active == selected) {
                    g_active_marker.store(0, std::memory_order_release);
                } else {
                    g_active_marker.store(selected, std::memory_order_release);
                    LogMarkerEvent("start", selected);
                }
            }
        }
        was_down[index] = down;
    }
}

void __fastcall HookD57900(void* self, void* context) {
    float delta = 0.0f;
    bool valid = context && SafeRead(reinterpret_cast<uintptr_t>(context) + 0x08, &delta);
    const bool capture_locomotion = valid && g_config.locomotion_trials &&
                                    g_capture_active.load(std::memory_order_acquire);
    long long locomotion_qpc_before = 0;
    PlayerSnapshot locomotion_before{};
    InputSnapshot locomotion_input_before{};
    if (capture_locomotion) {
        locomotion_qpc_before = Qpc();
        locomotion_before = ReadPlayerSnapshot();
        locomotion_input_before = ReadInputSnapshot();
    }
    if (g_calls) {
        EnterCriticalSection(&g_log_lock);
        std::fprintf(g_calls, "%llu,%lld,%lu,D57900,0x%016llX,0x%016llX,%.9g,%u\n",
                     NextSequence(), Qpc(), GetCurrentThreadId(),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(self)),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(context)),
                     delta, valid ? 1u : 0u);
        LeaveCriticalSection(&g_log_lock);
    }
    if (capture_locomotion && g_locomotion_trial != 0 &&
        !g_locomotion_measuring && g_locomotion_warmup_complete &&
        DeterministicKeyboardSprintHeld(locomotion_input_before)) {
        g_locomotion_measuring = true;
        g_locomotion_measured_delta = 0.0;
        LogLocomotionEvent("measure_start", g_locomotion_trial,
                           g_broad_update_index.load(std::memory_order_relaxed) + 1,
                           g_accumulated_delta.load(std::memory_order_relaxed),
                           locomotion_before, locomotion_input_before,
                           locomotion_qpc_before);
    }
    if (valid && g_capture_active.load(std::memory_order_acquire)) {
        g_temporal_update_active = true;
        g_temporal_update_index =
            g_broad_update_index.load(std::memory_order_relaxed) + 1;
        g_temporal_delta = delta;
        g_temporal_accumulated_before =
            g_accumulated_delta.load(std::memory_order_relaxed);
        g_temporal_accumulated_after =
            g_temporal_accumulated_before + static_cast<double>(delta);
    }
    g_orig_d57900(self, context);
    g_temporal_update_active = false;
    if (valid && g_capture_active.load(std::memory_order_acquire)) {
        long long locomotion_qpc_after = 0;
        PlayerSnapshot locomotion_after{};
        InputSnapshot locomotion_input_after{};
        if (capture_locomotion) {
            locomotion_after = ReadPlayerSnapshot();
            locomotion_input_after = ReadInputSnapshot();
            locomotion_qpc_after = Qpc();
        }
        const unsigned long long update_index =
            g_broad_update_index.fetch_add(1, std::memory_order_relaxed) + 1;
        const double accumulated_before =
            g_accumulated_delta.fetch_add(static_cast<double>(delta), std::memory_order_relaxed);
        const double accumulated = accumulated_before + static_cast<double>(delta);
        if (capture_locomotion) {
            g_distance_previous = g_distance_current;
            g_distance_current.index = update_index;
            g_distance_current.delta = delta;
            g_distance_current.transform = locomotion_before.transform;
            g_distance_current.position[0] = locomotion_before.x;
            g_distance_current.position[1] = locomotion_before.z;
            g_distance_current.position[2] = locomotion_before.y;
            g_distance_current.valid = locomotion_before.position_valid;
            UpdateLocomotionTrial(delta, update_index, accumulated_before, accumulated,
                                  locomotion_qpc_before, locomotion_qpc_after,
                                  locomotion_before, locomotion_after,
                                  locomotion_input_before, locomotion_input_after);
        }
        LogSemanticSample(delta, update_index, accumulated);
    }
}

void LogFloatCall(const char* site, void* self, float delta) {
    if (!g_calls) return;
    EnterCriticalSection(&g_log_lock);
    std::fprintf(g_calls, "%llu,%lld,%lu,%s,0x%016llX,0x0000000000000000,%.9g,1\n",
                 NextSequence(), Qpc(), GetCurrentThreadId(), site,
                 static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(self)), delta);
    LeaveCriticalSection(&g_log_lock);
}

void __fastcall Hook023410(void* self, float delta) {
    LogFloatCall("023410", self, delta);
    g_orig_023410(self, delta);
}

void __fastcall Hook0499C0(void* self, float delta) {
    LogFloatCall("0499C0", self, delta);
    if (g_callbacks && g_config.callback_detail) {
        uintptr_t owner = 0, begin = 0, end = 0;
        unsigned count = 0;
        bool valid = self && SafeRead(reinterpret_cast<uintptr_t>(self) + 0x10, &owner) && owner &&
                     SafeRead(owner + 0x10, &begin) && SafeRead(owner + 0x18, &end) &&
                     end >= begin && ((end - begin) % sizeof(uintptr_t) == 0) &&
                     (end - begin) / sizeof(uintptr_t) <= 256;
        if (valid) count = static_cast<unsigned>((end - begin) / sizeof(uintptr_t));
        EnterCriticalSection(&g_log_lock);
        std::fprintf(g_callbacks, "%llu,%lld,%lu,%.9g,0x%016llX,%u,%u,\"",
                     NextSequence(), Qpc(), GetCurrentThreadId(), delta,
                     static_cast<unsigned long long>(owner), count, valid ? 1u : 0u);
        for (unsigned i = 0; valid && i < count; ++i) {
            uintptr_t object = 0, vtable = 0, target = 0;
            bool item_valid = SafeRead(begin + i * sizeof(uintptr_t), &object) && object &&
                              SafeRead(object, &vtable) && vtable && SafeRead(vtable + 0x10, &target);
            if (i) std::fputc(';', g_callbacks);
            if (item_valid) {
                unsigned long long rva = target >= g_image ? target - g_image : 0;
                std::fprintf(g_callbacks, "%u:0x%016llX:0x%016llX:0x%llX", i,
                             static_cast<unsigned long long>(object),
                             static_cast<unsigned long long>(target), rva);
            } else {
                std::fprintf(g_callbacks, "%u:INVALID", i);
            }
        }
        std::fprintf(g_callbacks, "\"\n");
        LeaveCriticalSection(&g_log_lock);
    }
    g_orig_0499c0(self, delta);
}

unsigned long long __fastcall HookD5E30(void* state, unsigned int requested, uintptr_t a3, uintptr_t a4) {
    unsigned char active = 0;
    unsigned int mode = 0, counter_before = 0;
    long long deadline_before = 0;
    if (state) {
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x08, &active);
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x0C, &mode);
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x18, &deadline_before);
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x20, &counter_before);
    }
    long long begin = Qpc();
    unsigned long long original_result = g_orig_d5e30(state, requested, a3, a4);
    long long end = Qpc();
    unsigned long long final_result = g_config.force_present_zero ? 0 : original_result;
    long long deadline_after = 0;
    unsigned int counter_after = 0;
    if (state) {
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x18, &deadline_after);
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x20, &counter_after);
    }
    if (g_pacing) {
        EnterCriticalSection(&g_log_lock);
        std::fprintf(g_pacing,
                     "%llu,%lld,%lld,%lu,0x%016llX,%u,%u,%u,%lld,%u,%lld,%u,%llu,%llu,%u\n",
                     NextSequence(), begin, end, GetCurrentThreadId(),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(state)),
                     requested, active, mode, deadline_before, counter_before, deadline_after,
                     counter_after, original_result, final_result,
                     g_config.force_present_zero ? 1u : 0u);
        LeaveCriticalSection(&g_log_lock);
    }
    return final_result;
}

long long __fastcall Hook192CF00(void* state, unsigned int sync_interval) {
    long long begin = Qpc();
    long long result = g_orig_192cf00(state, sync_interval);
    long long end = Qpc();
    uintptr_t swap_chain = 0;
    if (state) SafeRead(reinterpret_cast<uintptr_t>(state), &swap_chain);
    if (g_present) {
        EnterCriticalSection(&g_log_lock);
        std::fprintf(g_present, "%llu,%lld,%lld,%lu,0x%016llX,0x%016llX,%u,%lld\n",
                     NextSequence(), begin, end, GetCurrentThreadId(),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(state)),
                     static_cast<unsigned long long>(swap_chain), sync_interval, result);
        LeaveCriticalSection(&g_log_lock);
    }
    return result;
}

void __fastcall HookDD4B60(void* object, void* wrapped_input) {
    unsigned int selector = 0;
    float input = 0.0f, before = 0.0f, after = 0.0f;
    bool valid = object && wrapped_input &&
                 SafeRead(reinterpret_cast<uintptr_t>(object) + 0x40, &selector) &&
                 SafeRead(reinterpret_cast<uintptr_t>(object) + 0x38, &before) &&
                 SafeRead(reinterpret_cast<uintptr_t>(wrapped_input) + 0x08, &input);
    long long begin = Qpc();
    g_orig_dd4b60(object, wrapped_input);
    long long end = Qpc();
    if (object) SafeRead(reinterpret_cast<uintptr_t>(object) + 0x38, &after);
    if (g_selector) {
        EnterCriticalSection(&g_log_lock);
        std::fprintf(g_selector,
                     "%llu,%lld,%lld,%lu,0x%016llX,%u,%.9g,%.9g,%.9g,%u\n",
                     NextSequence(), begin, end, GetCurrentThreadId(),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(object)),
                     selector, input, before, after, valid ? 1u : 0u);
        LeaveCriticalSection(&g_log_lock);
    }
}

void __fastcall HookED8E50(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4) {
    SnapshotCentral("ED8E50", "before");
    g_orig_ed8e50(a1, a2, a3, a4);
    SnapshotCentral("ED8E50", "after");
}

bool __fastcall HookEDAAE0(uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4) {
    SnapshotCentral("EDAAE0", "before");
    bool result = g_orig_edaae0(a1, a2, a3, a4);
    SnapshotCentral("EDAAE0", "after");
    return result;
}

void ReadXyz(uintptr_t object, float values[3]) {
    SafeRead(object + 0x10, &values[0]);
    SafeRead(object + 0x14, &values[1]);
    SafeRead(object + 0x18, &values[2]);
}

void __fastcall HookEFDA0(void* object_pointer, float delta) {
    uintptr_t object = reinterpret_cast<uintptr_t>(object_pointer);
    float elapsed_before = 0.0f, output_before = 0.0f, duration_a = 0.0f, duration_b = 0.0f;
    float xyz_before[3]{}, xyz_after[3]{};
    if (object) {
        SafeRead(object + 0x198, &elapsed_before);
        SafeRead(object + 0x19C, &output_before);
        SafeRead(object + 0x184, &duration_a);
        SafeRead(object + 0x188, &duration_b);
        ReadXyz(object, xyz_before);
    }
    g_orig_efda0(object_pointer, delta);
    float elapsed_after = 0.0f, output_after = 0.0f;
    if (object) {
        SafeRead(object + 0x198, &elapsed_after);
        SafeRead(object + 0x19C, &output_after);
        ReadXyz(object, xyz_after);
    }
    RecordConsumer(g_efda0_aggregates, object, delta, duration_a != 0.0f || duration_b != 0.0f,
                   elapsed_before, elapsed_after, output_before, output_after,
                   xyz_before, xyz_after);
}

void __fastcall HookF0D40(void* object_pointer, float delta) {
    uintptr_t object = reinterpret_cast<uintptr_t>(object_pointer);
    float elapsed_before = 0.0f, output_before = 0.0f, duration_a = 0.0f, duration_b = 0.0f;
    float xyz_before[3]{}, xyz_after[3]{};
    if (object) {
        SafeRead(object + 0x214, &elapsed_before);
        SafeRead(object + 0x218, &output_before);
        SafeRead(object + 0x200, &duration_a);
        SafeRead(object + 0x204, &duration_b);
        ReadXyz(object, xyz_before);
    }
    g_orig_f0d40(object_pointer, delta);
    float elapsed_after = 0.0f, output_after = 0.0f;
    if (object) {
        SafeRead(object + 0x214, &elapsed_after);
        SafeRead(object + 0x218, &output_after);
        ReadXyz(object, xyz_after);
    }
    RecordConsumer(g_f0d40_aggregates, object, delta, duration_a != 0.0f || duration_b != 0.0f,
                   elapsed_before, elapsed_after, output_before, output_after,
                   xyz_before, xyz_after);
}

void __fastcall Hook21CC20(void* object_pointer, float delta, void* third) {
    uintptr_t object = reinterpret_cast<uintptr_t>(object_pointer);
    unsigned char active_before = 0;
    float elapsed_before = 0.0f, output_before = 0.0f;
    float xyz_zero[3]{};
    if (object) {
        SafeRead(object + 0x4EC, &active_before);
        SafeRead(object + 0x4F0, &elapsed_before);
        SafeRead(object + 0x260, &output_before);
    }
    g_orig_21cc20(object_pointer, delta, third);
    float elapsed_after = 0.0f, output_after = 0.0f;
    if (object) {
        SafeRead(object + 0x4F0, &elapsed_after);
        SafeRead(object + 0x260, &output_after);
    }
    RecordConsumer(g_21cc20_aggregates, object, delta, active_before != 0,
                   elapsed_before, elapsed_after, output_before, output_after,
                   xyz_zero, xyz_zero);
}

template <typename T>
bool InstallHook(const char* name, uintptr_t rva, void* detour, T* original) {
    void* target = reinterpret_cast<void*>(g_image + rva);
    MH_STATUS create = MH_CreateHook(target, detour, reinterpret_cast<void**>(original));
    MH_STATUS enable = create == MH_OK ? MH_EnableHook(target) : create;
    if (g_hooks) {
        std::fprintf(g_hooks, "%s,0x%llX,0x%016llX,%d,%s,%d,%s\n", name,
                     static_cast<unsigned long long>(rva),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(target)),
                     static_cast<int>(create), MH_StatusToString(create),
                     static_cast<int>(enable), MH_StatusToString(enable));
        std::fflush(g_hooks);
    }
    return create == MH_OK && enable == MH_OK;
}

bool OpenLogs() {
    g_calls = OpenCsv(L"call-sequence.csv", "seq,qpc,tid,site,self,context,delta,valid");
    g_callbacks = OpenCsv(L"callbacks.csv", "seq,qpc,tid,delta,owner,count,valid,entries_index_object_target_rva");

    const char* central_header =
        "seq,qpc,tid,site,phase,object,qpc20,qpc28,qpc30,qpc38,qpc40,qpc48,qpc50,qpc58,"
        "f018,f260,f264,f268,f26c,f270,f274,f27c,f280,f284,f288,f28c,f290,f294,f298,"
        "f29c,f2a0,f2a4,f2a8,f2ac,f2b0,f2b4,f2b8,f2bc,f2c0,f2c4,f2c8,f2cc,f2d0,f2d4,"
        "f2d8,f2dc,f2e0,f2e4,f2e8,f2ec,f2f0,f2f4,f2f8,f2fc,f300,f304,f308,f30c,f310,"
        "f314,f318,f31c,f320,f324,f328,f32c,f330,f334,f338,f33c,f340,flag278,flag279,flag27a,"
        "flag27b,i344,flag358,flag360,fps354,u34c,u350,u35c";
    g_central = OpenCsv(L"central-state.csv", central_header);
    g_selector = OpenCsv(L"selector.csv", "seq,qpc_before,qpc_after,tid,object,selector,input,before,after,valid");
    g_pacing = OpenCsv(L"presentation-pacing.csv", "seq,qpc_before,qpc_after,tid,state,requested,active,mode,stamp_before,count_before,stamp_after,count_after,original_sync_interval,returned_sync_interval,forced_zero");
    g_present = OpenCsv(L"present.csv", "seq,qpc_before,qpc_after,tid,state,swap_chain,sync_interval,result");
    g_hooks = OpenCsv(L"hook-status.csv", "name,rva,address,create_status,create_text,enable_status,enable_text");
    g_override = OpenCsv(L"override-transitions.csv", "seq,qpc,action,object,fps_before,enable_before,fps_after,enable_after,success");
    g_consumers = OpenCsv(L"consumer-summary.csv", "site,object,calls,active_calls,first_qpc,last_qpc,span_s,sum_delta,min_delta,max_delta,increment_matches,resets,sum_elapsed_increment,first_elapsed,last_elapsed,first_output,last_output,first_x,first_y,first_z,last_x,last_y,last_z,xyz_distance");
    g_semantic = OpenCsv(
        L"semantic-state.csv",
        "seq,qpc,tid,update_index,marker_id,marker_name,delta,accumulated_delta,world,player,transform,position_valid,x,z,y,angle,animation_valid,animation_id,animation_time,animation_max,play_speed,hp,stamina,keyboard_mask,controller_connected,controller_buttons,left_x,left_y,right_x,right_y,left_trigger,right_trigger");
    g_markers = OpenCsv(
        L"markers.csv",
        "seq,qpc,event,marker_id,marker_name,update_index,accumulated_delta,position_valid,x,z,y,angle,animation_valid,animation_id,animation_time,animation_max,play_speed");
    g_locomotion_updates = OpenCsv(
        L"locomotion-updates.csv",
        "seq,qpc_before,qpc_after,tid,trial,phase,update_index,delta,accumulated_delta_before,accumulated_delta_after,"
        "before_position_valid,before_x,before_z,before_y,before_angle,after_position_valid,after_x,after_z,after_y,after_angle,"
        "before_animation_valid,before_animation_id,before_animation_time,before_animation_max,before_play_speed,"
        "after_animation_valid,after_animation_id,after_animation_time,after_animation_max,after_play_speed,"
        "before_keyboard_mask,before_controller_connected,before_controller_buttons,before_left_x,before_left_y,before_right_x,before_right_y,before_left_trigger,before_right_trigger,"
        "after_keyboard_mask,after_controller_connected,after_controller_buttons,after_left_x,after_left_y,after_right_x,after_right_y,after_left_trigger,after_right_trigger");
    g_locomotion_events = OpenCsv(
        L"locomotion-events.csv",
        "seq,qpc,event,trial,update_index,accumulated_delta,position_valid,x,z,y,angle,animation_valid,animation_id,animation_time,animation_max,play_speed,keyboard_mask,controller_connected,controller_buttons,left_x,left_y,right_x,right_y,left_trigger,right_trigger");
    g_locomotion_phases = OpenCsv(
        L"locomotion-phases.csv",
        "seq,qpc,tid,trial,broad_update_index,phase,event,call_index,self,context,caller_rva,"
        "position_valid,x,z,y,angle,animation_valid,animation_id,animation_time,animation_max,play_speed");
    g_locomotion_write_watch = OpenCsv(
        L"causal-distance-patch.csv",
        "seq,qpc,tid,trial,broad_update_index,shared_context,owner_e0_address,dr6,rip_at_trap,function_begin,function_end,"
        "arm_c0,arm_c1,arm_c2,hit_c0,hit_c1,hit_c2,code_valid,overflow,"
        "rax,rbx,rcx,rdx,rsi,rdi,rbp,rsp,r8,r9,r10,r11,r12,r13,r14,r15,"
        "xmm0_lo,xmm0_hi,xmm1_lo,xmm1_hi,xmm2_lo,xmm2_hi,xmm3_lo,xmm3_hi,"
        "xmm4_lo,xmm4_hi,xmm5_lo,xmm5_hi,xmm6_lo,xmm6_hi,xmm7_lo,xmm7_hi,"
        "xmm8_lo,xmm8_hi,xmm9_lo,xmm9_hi,xmm10_lo,xmm10_hi,xmm11_lo,xmm11_hi,"
        "xmm12_lo,xmm12_hi,xmm13_lo,xmm13_hi,xmm14_lo,xmm14_hi,xmm15_lo,xmm15_hi,code_bytes,input_valid,output_valid,nested_valid,input_bytes,output_bytes,nested_bytes,owner_valid,additive_valid,builder_bytes,timing_18_bytes,"
        "builder_address,producer_vtable,producer_target,producer_code_valid,modifier_code_valid,feedback_code,application_code,measured,patch_applied,patch_failed,patch_raw,patch_corrected,patch_delta,patch_previous_index,patch_current_index");
    g_locomotion_writer_mirror = OpenCsv(
        L"locomotion-writer-mirror.csv",
        "seq,qpc,tid,trial,marker_id,broad_update_index,writer_invocation_index,observation_rva,"
        "shared_context,transform,rbx,rsp,delta,accumulated_delta_before,accumulated_delta_after,mxcsr,"
        "source,outer_a8,outer_b0,selected,nested,inhibit,will_commit,overflow,"
        "current_memory_valid,raw_candidate_valid,outer_a8_read_valid,outer_b0_read_valid,"
        "nested_read_valid,effective_candidate_valid,scalar_memory_valid,inhibit_valid,rbx_matches_transform,"
        "current_mem_b0,current_mem_b1,current_mem_b2,current_mem_b3,"
        "current_mem_f0,current_mem_f1,current_mem_f2,current_mem_f3,"
        "current_xmm1_b0,current_xmm1_b1,current_xmm1_b2,current_xmm1_b3,"
        "raw_candidate_b0,raw_candidate_b1,raw_candidate_b2,raw_candidate_b3,"
        "raw_candidate_f0,raw_candidate_f1,raw_candidate_f2,raw_candidate_f3,"
        "effective_candidate_b0,effective_candidate_b1,effective_candidate_b2,effective_candidate_b3,"
        "effective_candidate_f0,effective_candidate_f1,effective_candidate_f2,effective_candidate_f3,"
        "scalar_memory_bits,scalar_memory_f,"
        "scalar_xmm0_b0,scalar_xmm0_b1,scalar_xmm0_b2,scalar_xmm0_b3,"
        "scalar_xmm0_f0,scalar_xmm0_f1,scalar_xmm0_f2,scalar_xmm0_f3,"
        "final_xmm6_b0,final_xmm6_b1,final_xmm6_b2,final_xmm6_b3,"
        "final_xmm6_f0,final_xmm6_f1,final_xmm6_f2,final_xmm6_f3");
    return g_calls && g_callbacks && g_central && g_selector && g_pacing && g_present &&
           g_hooks && g_override && g_consumers && g_semantic && g_markers &&
           g_locomotion_updates && g_locomotion_events && g_locomotion_phases &&
           g_locomotion_write_watch && g_locomotion_writer_mirror;
}

template <size_t N>
void WriteConsumerTable(const char* site, ConsumerAggregate (&table)[N]) {
    if (!g_consumers) return;
    for (const ConsumerAggregate& entry : table) {
        if (entry.object == 0 || entry.calls == 0) continue;
        double span = static_cast<double>(entry.last_qpc - entry.first_qpc) /
                      static_cast<double>(g_frequency.QuadPart);
        double dx = static_cast<double>(entry.last_xyz[0] - entry.first_xyz[0]);
        double dy = static_cast<double>(entry.last_xyz[1] - entry.first_xyz[1]);
        double dz = static_cast<double>(entry.last_xyz[2] - entry.first_xyz[2]);
        double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        std::fprintf(g_consumers,
                     "%s,0x%016llX,%llu,%llu,%lld,%lld,%.9g,%.12g,%.9g,%.9g,%llu,%llu,%.12g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
                     site, static_cast<unsigned long long>(entry.object), entry.calls,
                     entry.active_calls, entry.first_qpc, entry.last_qpc, span,
                     entry.sum_delta, entry.min_delta, entry.max_delta,
                     entry.increment_matches, entry.resets, entry.sum_elapsed_increment,
                     entry.first_elapsed, entry.last_elapsed, entry.first_output,
                     entry.last_output, entry.first_xyz[0], entry.first_xyz[1],
                     entry.first_xyz[2], entry.last_xyz[0], entry.last_xyz[1],
                     entry.last_xyz[2], distance);
    }
}

void WriteConsumerSummary() {
    EnterCriticalSection(&g_consumer_lock);
    WriteConsumerTable("EFDA0", g_efda0_aggregates);
    WriteConsumerTable("F0D40", g_f0d40_aggregates);
    WriteConsumerTable("21CC20", g_21cc20_aggregates);
    std::fflush(g_consumers);
    LeaveCriticalSection(&g_consumer_lock);
}

void WriteWriteWatchRows() {
    if (!g_locomotion_write_watch) return;
    const unsigned int observed =
        g_write_watch_hit_count.load(std::memory_order_acquire);
    const unsigned int count = observed < kMaxWriteWatchHits ? observed : kMaxWriteWatchHits;
    for (unsigned int index = g_patch_rows_written; index < count; ++index) {
        const WriteWatchHit& hit = g_write_watch_hits[index];
        DWORD64 image_base = 0;
        const PRUNTIME_FUNCTION runtime_function =
            RtlLookupFunctionEntry(hit.rip_after, &image_base, nullptr);
        const DWORD64 function_begin = runtime_function
            ? image_base + runtime_function->BeginAddress
            : 0;
        const DWORD64 function_end = runtime_function
            ? image_base + runtime_function->EndAddress
            : 0;
        std::fprintf(
            g_locomotion_write_watch,
            "%llu,%lld,%lu,%u,%llu,0x%016llX,0x%016llX,0x%llX,"
            "0x%016llX,0x%016llX,0x%016llX,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%u,%u",
            hit.sequence, hit.qpc, hit.tid, hit.trial, hit.update_index,
            static_cast<unsigned long long>(hit.shared_context),
            static_cast<unsigned long long>(hit.transform),
            static_cast<unsigned long long>(hit.dr6),
            static_cast<unsigned long long>(hit.rip_after),
            static_cast<unsigned long long>(function_begin),
            static_cast<unsigned long long>(function_end),
            hit.arm_x, hit.arm_z, hit.arm_y, hit.hit_x, hit.hit_z, hit.hit_y,
            hit.code_valid, g_write_watch_overflow.load(std::memory_order_acquire));
        for (DWORD64 value : hit.gpr) {
            std::fprintf(g_locomotion_write_watch, ",0x%016llX",
                         static_cast<unsigned long long>(value));
        }
        for (const auto& value : hit.xmm) {
            std::fprintf(g_locomotion_write_watch, ",0x%016llX,0x%016llX",
                         static_cast<unsigned long long>(value[0]),
                         static_cast<unsigned long long>(value[1]));
        }
        std::fputc(',', g_locomotion_write_watch);
        for (unsigned char byte : hit.code) {
            std::fprintf(g_locomotion_write_watch, "%02X", byte);
        }
        std::fprintf(g_locomotion_write_watch, ",%u,%u,%u", hit.input_valid, hit.output_valid, hit.nested_valid);
        std::fputc(',', g_locomotion_write_watch);
        for (unsigned char byte : hit.input_bytes) std::fprintf(g_locomotion_write_watch, "%02X", byte);
        std::fputc(',', g_locomotion_write_watch);
        for (unsigned char byte : hit.output_bytes) std::fprintf(g_locomotion_write_watch, "%02X", byte);
        std::fputc(',', g_locomotion_write_watch);
        for (unsigned char byte : hit.nested_bytes) std::fprintf(g_locomotion_write_watch, "%02X", byte);
        std::fprintf(g_locomotion_write_watch, ",%u,%u,", hit.owner_valid, hit.additive_valid);
        for (unsigned char byte : hit.owner_bytes) std::fprintf(g_locomotion_write_watch, "%02X", byte);
        std::fputc(',', g_locomotion_write_watch);
        for (unsigned char byte : hit.additive_bytes) std::fprintf(g_locomotion_write_watch, "%02X", byte);
        std::fprintf(g_locomotion_write_watch, ",0x%016llX,0x%016llX,0x%016llX,%u,%u,",
            static_cast<unsigned long long>(hit.builder), static_cast<unsigned long long>(hit.producer_vtable),
            static_cast<unsigned long long>(hit.producer_target), hit.producer_code_valid, hit.modifier_code_valid);
        for (unsigned char byte : hit.producer_code) std::fprintf(g_locomotion_write_watch, "%02X", byte);
        std::fputc(',', g_locomotion_write_watch);
        for (unsigned char byte : hit.modifier_code) std::fprintf(g_locomotion_write_watch, "%02X", byte);
        std::fprintf(g_locomotion_write_watch, ",%u,%u,%u,%.9g,%.9g,%.9g,%llu,%llu",
            hit.measured, hit.patch_applied, hit.patch_failed,
            static_cast<double>(hit.patch_raw), static_cast<double>(hit.patch_corrected),
            static_cast<double>(hit.patch_delta), hit.patch_previous_index, hit.patch_current_index);
        std::fputc('\n', g_locomotion_write_watch);
    }
    g_patch_rows_written = count;
    std::fflush(g_locomotion_write_watch);
}

float FloatFromBits(DWORD bits) {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void WriteBitVector(FILE* file, const DWORD bits[4], bool include_float) {
    for (unsigned int lane = 0; lane < 4; ++lane) {
        std::fprintf(file, ",0x%08lX", static_cast<unsigned long>(bits[lane]));
    }
    if (include_float) {
        for (unsigned int lane = 0; lane < 4; ++lane) {
            std::fprintf(file, ",%.9g", static_cast<double>(FloatFromBits(bits[lane])));
        }
    }
}

void WriteWriterMirrorRows() {
    if (!g_locomotion_writer_mirror) return;
    const unsigned int observed =
        g_writer_mirror_hit_count.load(std::memory_order_acquire);
    const unsigned int count = observed < kMaxWriterMirrorHits
        ? observed
        : kMaxWriterMirrorHits;
    for (unsigned int index = 0; index < count; ++index) {
        const WriterMirrorHit& hit = g_writer_mirror_hits[index];
        const char* source = hit.outer_b0_read_valid && hit.outer_b0 != 0
            ? "B0"
            : (hit.outer_b0_read_valid && hit.outer_a8_read_valid &&
               hit.outer_a8 != 0 ? "A8" : "NONE");
        const unsigned int will_commit = hit.inhibit_valid && hit.inhibit == 0 ? 1u : 0u;
        std::fprintf(
            g_locomotion_writer_mirror,
            "%llu,%lld,%lu,%u,%u,%llu,%u,0x%08llX,"
            "0x%016llX,0x%016llX,0x%016llX,0x%016llX,%.9g,%.12g,%.12g,0x%08lX,"
            "%s,0x%016llX,0x%016llX,0x%016llX,0x%016llX,%u,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,%u",
            hit.sequence, hit.qpc, hit.tid, hit.trial, hit.marker,
            hit.update_index, hit.invocation_index,
            static_cast<unsigned long long>(kRva9DC559),
            static_cast<unsigned long long>(hit.shared_context),
            static_cast<unsigned long long>(hit.transform),
            static_cast<unsigned long long>(hit.rbx),
            static_cast<unsigned long long>(hit.rsp),
            static_cast<double>(hit.temporal_delta), hit.accumulated_before,
            hit.accumulated_after, static_cast<unsigned long>(hit.mxcsr), source,
            static_cast<unsigned long long>(hit.outer_a8),
            static_cast<unsigned long long>(hit.outer_b0),
            static_cast<unsigned long long>(hit.selected),
            static_cast<unsigned long long>(hit.nested),
            static_cast<unsigned int>(hit.inhibit), will_commit,
            g_writer_mirror_overflow.load(std::memory_order_acquire),
            hit.current_memory_valid, hit.raw_candidate_valid,
            hit.outer_a8_read_valid, hit.outer_b0_read_valid,
            hit.nested_read_valid, hit.effective_candidate_valid,
            hit.scalar_memory_valid, hit.inhibit_valid,
            hit.rbx == hit.transform ? 1u : 0u);
        WriteBitVector(g_locomotion_writer_mirror, hit.current_memory, true);
        WriteBitVector(g_locomotion_writer_mirror, hit.current_xmm1, false);
        WriteBitVector(g_locomotion_writer_mirror, hit.raw_candidate, true);
        WriteBitVector(g_locomotion_writer_mirror, hit.effective_candidate, true);
        std::fprintf(g_locomotion_writer_mirror, ",0x%08lX,%.9g",
                     static_cast<unsigned long>(hit.scalar_memory),
                     static_cast<double>(FloatFromBits(hit.scalar_memory)));
        WriteBitVector(g_locomotion_writer_mirror, hit.scalar_xmm0, true);
        WriteBitVector(g_locomotion_writer_mirror, hit.final_xmm6, true);
        std::fputc('\n', g_locomotion_writer_mirror);
    }
    std::fflush(g_locomotion_writer_mirror);
}

void CloseLogs() {
    FILE** files[] = {&g_calls, &g_callbacks, &g_central, &g_selector, &g_pacing, &g_present,
                      &g_hooks, &g_override, &g_consumers, &g_semantic, &g_markers,
                      &g_locomotion_updates, &g_locomotion_events, &g_locomotion_phases,
                      &g_locomotion_write_watch, &g_locomotion_writer_mirror};
    for (FILE** file : files) {
        if (*file) {
            std::fflush(*file);
            std::fclose(*file);
            *file = nullptr;
        }
    }
}

void WriteMarker(const wchar_t* name, const char* text) {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\%s", g_session_dir, name);
    FILE* file = nullptr;
    _wfopen_s(&file, path, L"wb");
    if (file) {
        std::fprintf(file, "%s\n", text);
        std::fclose(file);
    }
}

void PollLiveControl() {
    const bool target60 = g_target60_event && WaitForSingleObject(g_target60_event, 0) == WAIT_OBJECT_0;
    const bool target165 = g_target165_event && WaitForSingleObject(g_target165_event, 0) == WAIT_OBJECT_0;
    const bool flush = g_flush_event && WaitForSingleObject(g_flush_event, 0) == WAIT_OBJECT_0;
    if (!target60 && !target165 && !flush) return;
    if (g_locomotion_busy.load(std::memory_order_acquire) ||
        g_locomotion_arm_requested.load(std::memory_order_acquire) ||
        g_write_watch_active.load(std::memory_order_acquire) ||
        (target60 && target165) || !g_override_applied) {
        WriteMarker(L"CONTROL", "rejected: active trial/watch, conflicting request or no timing override");
    } else {
        WriteWriteWatchRows();
        if (target60 || target165) {
            g_config.target_fps = target60 ? 60.0f : 165.0f;
            ApplyOverrideIfPossible("target_change");
            float actual = 0.0f;
            unsigned char enabled = 0;
            const bool success = SafeRead(g_override_object + 0x354, &actual) &&
                SafeRead(g_override_object + 0x358, &enabled) &&
                actual == g_config.target_fps && enabled == 1;
            WriteMarker(L"CONTROL", success ? (target60 ? "target=60 success=1" : "target=165 success=1")
                                             : "target change failed");
            if (!success && g_stop_event) SetEvent(g_stop_event);
        } else {
            WriteMarker(L"CONTROL", "flushed success=1; capture still active, restoration pending");
        }
        EnterCriticalSection(&g_log_lock);
        FlushAll();
        LeaveCriticalSection(&g_log_lock);
    }
    if (g_control_ack) SetEvent(g_control_ack);
}

DWORD WINAPI ProbeThread(void*) {
    g_image = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    QueryPerformanceFrequency(&g_frequency);
    HMODULE xinput = LoadLibraryW(L"xinput1_4.dll");
    if (!xinput) xinput = LoadLibraryW(L"xinput1_3.dll");
    if (xinput) {
        g_xinput_get_state =
            reinterpret_cast<XInputGetStateFn>(GetProcAddress(xinput, "XInputGetState"));
    }
    InitializeCriticalSection(&g_log_lock);
    InitializeCriticalSection(&g_consumer_lock);
    InitializeCriticalSection(&g_write_watch_lock);
    LoadConfig();
    BuildSessionDirectory();
    WriteSessionInfo();
    const bool config_ok = ValidWriteWatchConfiguration();
    bool ok = OpenLogs() && config_ok;
    SnapshotModules(L"modules-start.csv");

    wchar_t stop_name[128]{}, ready_name[128]{}, complete_name[128]{};
    _snwprintf_s(stop_name, _countof(stop_name), _TRUNCATE, L"Local\\DS3_I0007_ROLL_PATCH_STOP_%lu", GetCurrentProcessId());
    _snwprintf_s(ready_name, _countof(ready_name), _TRUNCATE, L"Local\\DS3_I0007_ROLL_PATCH_READY_%lu", GetCurrentProcessId());
    _snwprintf_s(complete_name, _countof(complete_name), _TRUNCATE, L"Local\\DS3_I0007_ROLL_PATCH_COMPLETE_%lu", GetCurrentProcessId());
    g_stop_event = CreateEventW(nullptr, TRUE, FALSE, stop_name);
    g_ready_event = CreateEventW(nullptr, TRUE, FALSE, ready_name);
    g_complete_event = CreateEventW(nullptr, TRUE, FALSE, complete_name);
    wchar_t control_name[128]{};
    _snwprintf_s(control_name, _countof(control_name), _TRUNCATE, L"Local\\DS3_I0007_ROLL_PATCH_TARGET60_%lu", GetCurrentProcessId());
    g_target60_event = CreateEventW(nullptr, FALSE, FALSE, control_name);
    _snwprintf_s(control_name, _countof(control_name), _TRUNCATE, L"Local\\DS3_I0007_ROLL_PATCH_TARGET165_%lu", GetCurrentProcessId());
    g_target165_event = CreateEventW(nullptr, FALSE, FALSE, control_name);
    _snwprintf_s(control_name, _countof(control_name), _TRUNCATE, L"Local\\DS3_I0007_ROLL_PATCH_FLUSH_%lu", GetCurrentProcessId());
    g_flush_event = CreateEventW(nullptr, FALSE, FALSE, control_name);
    _snwprintf_s(control_name, _countof(control_name), _TRUNCATE, L"Local\\DS3_I0007_ROLL_PATCH_ACK_%lu", GetCurrentProcessId());
    g_control_ack = CreateEventW(nullptr, TRUE, FALSE, control_name);
    ok = ok && g_target60_event && g_target165_event && g_flush_event && g_control_ack;
    const bool write_watch_initialized = config_ok && InitializeWriteWatch();
    ok = ok && write_watch_initialized;

    MH_STATUS init = MH_Initialize();
    if (g_hooks) {
        std::fprintf(g_hooks, "MH_Initialize,0x0,0x0,%d,%s,%d,%s\n", static_cast<int>(init),
                     MH_StatusToString(init), static_cast<int>(init), MH_StatusToString(init));
    }
    ok = ok && init == MH_OK;
    if (init == MH_OK && config_ok && write_watch_initialized) {
        ok = InstallHook("D57900", kRvaD57900, reinterpret_cast<void*>(&HookD57900), &g_orig_d57900) && ok;
        ok = InstallHook("192CF00", kRva192CF00, reinterpret_cast<void*>(&Hook192CF00), &g_orig_192cf00) && ok;
        if (!g_config.locomotion_writer_mirror && !g_config.locomotion_write_watch) {
            ok = InstallHook("023410", kRva023410, reinterpret_cast<void*>(&Hook023410), &g_orig_023410) && ok;
            ok = InstallHook("0499C0", kRva0499C0, reinterpret_cast<void*>(&Hook0499C0), &g_orig_0499c0) && ok;
            ok = InstallHook("D5E30", kRvaD5E30, reinterpret_cast<void*>(&HookD5E30), &g_orig_d5e30) && ok;
            ok = InstallHook("DD4B60", kRvaDD4B60, reinterpret_cast<void*>(&HookDD4B60), &g_orig_dd4b60) && ok;
            ok = InstallHook("ED8E50", kRvaED8E50, reinterpret_cast<void*>(&HookED8E50), &g_orig_ed8e50) && ok;
            ok = InstallHook("EDAAE0", kRvaEDAAE0, reinterpret_cast<void*>(&HookEDAAE0), &g_orig_edaae0) && ok;
            ok = InstallHook("EFDA0", kRvaEFDA0, reinterpret_cast<void*>(&HookEFDA0), &g_orig_efda0) && ok;
            ok = InstallHook("F0D40", kRvaF0D40, reinterpret_cast<void*>(&HookF0D40), &g_orig_f0d40) && ok;
            ok = InstallHook("21CC20", kRva21CC20, reinterpret_cast<void*>(&Hook21CC20), &g_orig_21cc20) && ok;
        }
        if (g_config.locomotion_phase_rca) {
            if (!g_config.locomotion_writer_mirror && !g_config.locomotion_write_watch) {
                ok = InstallHook("D56780", kRvaD56780, reinterpret_cast<void*>(&HookD56780),
                                 &g_orig_d56780) && ok;
                ok = InstallHook("D56930", kRvaD56930, reinterpret_cast<void*>(&HookD56930),
                                 &g_orig_d56930) && ok;
                ok = InstallHook("D56A90", kRvaD56A90, reinterpret_cast<void*>(&HookD56A90),
                                 &g_orig_d56a90) && ok;
                ok = InstallHook("D58E20", kRvaD58E20, reinterpret_cast<void*>(&HookD58E20),
                                 &g_orig_d58e20) && ok;
                ok = InstallHook("D5B140", kRvaD5B140, reinterpret_cast<void*>(&HookD5B140),
                                 &g_orig_d5b140) && ok;
                ok = InstallHook("E6E750", kRvaE6E750, reinterpret_cast<void*>(&HookE6E750),
                                 &g_orig_e6e750) && ok;
            }
            ok = InstallHook("2344730", kRva2344730, reinterpret_cast<void*>(&Hook2344730),
                             &g_orig_2344730) && ok;
        }
    }
    FlushAll();
    WriteMarker(ok ? L"READY" : L"FAILED", ok ? "all hooks enabled" : "one or more hooks failed");
    if (g_ready_event) SetEvent(g_ready_event);
    if (!ok && g_stop_event) SetEvent(g_stop_event);

    const long long run_start = Qpc();
    long long last_flush = run_start;
    while (g_stop_event && WaitForSingleObject(g_stop_event, 10) == WAIT_TIMEOUT) {
        PollLiveControl();
        ApplyOverrideIfPossible();
        PollMarkerHotkeys();
        long long now = Qpc();
        if (now - last_flush >= g_frequency.QuadPart) {
            EnterCriticalSection(&g_log_lock);
            FlushAll();
            LeaveCriticalSection(&g_log_lock);
            last_flush = now;
        }
        if (g_config.auto_stop_seconds != 0 &&
            now - run_start >= static_cast<long long>(g_config.auto_stop_seconds) * g_frequency.QuadPart) {
            SetEvent(g_stop_event);
        }
    }

    const int active_marker = g_active_marker.exchange(0, std::memory_order_acq_rel);
    if (active_marker != 0) LogMarkerEvent("capture_stop", active_marker);
    g_capture_active.store(false, std::memory_order_release);
    const bool write_watch_restored = ShutdownWriteWatch();
    RestoreOverride();
    if (init == MH_OK) {
        MH_DisableHook(MH_ALL_HOOKS);
        Sleep(100);
    }
    g_stopped.store(true, std::memory_order_release);
    WriteConsumerSummary();
    WriteWriteWatchRows();
    WriteWriterMirrorRows();
    WriteProbeOverhead();
    EnterCriticalSection(&g_log_lock);
    FlushAll();
    LeaveCriticalSection(&g_log_lock);
    SnapshotModules(L"modules-stop.csv");
    WriteMarker(write_watch_restored ? L"COMPLETE" : L"FAILED",
                write_watch_restored
                    ? "hooks disabled, debug registers restored, and logs flushed"
                    : "debug-register restoration failed; handler retained");
    if (g_complete_event) SetEvent(g_complete_event);
    CloseLogs();
    if (init == MH_OK) MH_Uninitialize();
    if (xinput) FreeLibrary(xinput);
    return 0;
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, ProbeThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
