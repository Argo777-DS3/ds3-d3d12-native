#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <intrin.h>

#include <MinHook.h>

#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <new>

namespace {

constexpr std::uintptr_t kRvaAggregator = 0x005C8100;
constexpr std::array<std::uintptr_t, 3> kAllowedCallerReturns = {
    0x005C6977, 0x005C6F47, 0x005CAA80,
};
constexpr std::uintptr_t kRvaCentralGlobal = 0x0489DD10;
constexpr std::uintptr_t kRvaWorldChrManGlobal = 0x0477FDB8;
constexpr std::size_t kMaximumSamples = 4096;
constexpr double kCaptureSeconds = 3.0;
constexpr double kFailClosedSeconds = 5.0;

struct TimingPair {
    float fps = 0.0f;
    unsigned char enabled = 0;
};

struct Identity {
    std::uintptr_t player = 0;
    std::uintptr_t modules = 0;
    std::uintptr_t movement = 0;
    std::uintptr_t cache_owner = 0;
    bool valid = false;
};

struct Sample {
    long long qpc_entry = 0;
    long long qpc_exit = 0;
    std::uint64_t sequence = 0;
    std::uintptr_t caller_return = 0;
    std::uintptr_t owner = 0;
    std::uintptr_t movement = 0;
    std::uintptr_t output = 0;
    std::uintptr_t owner_vtable = 0;
    std::uintptr_t owner_e0_target = 0;
    std::uintptr_t actor = 0;
    std::uintptr_t actor_vtable = 0;
    std::uintptr_t actor_178_target = 0;
    std::array<float, 4> source_before{};
    std::array<float, 4> source_after{};
    float source_extra = 0.0f;
    int selector_a = 0;
    int selector_b = 0;
    std::array<float, 12> cache_before{};
    std::array<float, 12> final_output{};
    float carried = 0.0f;
    float producer_state = 0.0f;
    float retention = 0.0f;
    float observed_delta = 0.0f;
    unsigned char producer_mode = 0;
    int mode_a = 0;
    int mode_b = 0;
    float vertical_filter = 0.0f;
    float override_gate = 0.0f;
    unsigned int local_predicate = 0;
    unsigned int valid = 0;
};

using FnAggregator = void*(__fastcall*)(void*, float*);

std::uintptr_t g_image = 0;
FnAggregator g_original_aggregator = nullptr;
LARGE_INTEGER g_qpc_frequency{};
HANDLE g_stop_event = nullptr;
HANDLE g_ready_event = nullptr;
HANDLE g_complete_event = nullptr;
HANDLE g_target60_event = nullptr;
HANDLE g_target240_event = nullptr;
HANDLE g_ack_event = nullptr;

std::atomic<bool> g_capture_arm{false};
std::atomic<bool> g_capture_active{false};
std::atomic<bool> g_export_requested{false};
std::atomic<bool> g_failed{false};
std::atomic<std::uint64_t> g_sample_count{0};
std::atomic<std::uint64_t> g_invalid_count{0};
std::atomic<std::uint64_t> g_filtered_call_count{0};
std::atomic<unsigned int> g_inflight{0};
std::atomic<unsigned int> g_trial{0};
std::atomic<int> g_target_fps{60};
std::array<Sample, kMaximumSamples> g_samples{};
Identity g_identity{};
long long g_capture_start_qpc = 0;

std::uintptr_t g_timing_object = 0;
TimingPair g_original_timing{};
bool g_timing_saved = false;
wchar_t g_session_dir[MAX_PATH * 4]{};
FILE* g_control = nullptr;

long long Qpc() {
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}

double ElapsedSeconds(long long start, long long end) {
    return g_qpc_frequency.QuadPart > 0
        ? static_cast<double>(end - start) /
            static_cast<double>(g_qpc_frequency.QuadPart)
        : 0.0;
}

template <typename T>
bool ReadValue(std::uintptr_t address, T* value) {
    SIZE_T bytes = 0;
    return value && address &&
        ReadProcessMemory(GetCurrentProcess(),
                          reinterpret_cast<const void*>(address), value,
                          sizeof(T), &bytes) && bytes == sizeof(T);
}

template <typename T>
bool ReadPointer(std::uintptr_t address, T* value) {
    return ReadValue(address, value) && *value != 0;
}

template <typename T>
bool ReadArray(std::uintptr_t address, T* values, std::size_t count) {
    SIZE_T bytes = 0;
    const SIZE_T wanted = sizeof(T) * count;
    return values && address && count &&
        ReadProcessMemory(GetCurrentProcess(),
                          reinterpret_cast<const void*>(address), values,
                          wanted, &bytes) && bytes == wanted;
}

template <typename T>
bool WriteValue(std::uintptr_t address, const T& value) {
    SIZE_T bytes = 0;
    return address && WriteProcessMemory(
        GetCurrentProcess(), reinterpret_cast<void*>(address), &value,
        sizeof(T), &bytes) && bytes == sizeof(T);
}

bool Finite(const float* values, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(values[index])) return false;
    }
    return true;
}

Identity ReadIdentity() {
    Identity identity{};
    std::uintptr_t world = 0;
    identity.valid =
        ReadPointer(g_image + kRvaWorldChrManGlobal, &world) &&
        ReadPointer(world + 0x80, &identity.player) &&
        ReadPointer(identity.player + 0x1F90, &identity.modules) &&
        ReadPointer(identity.modules + 0x68, &identity.movement) &&
        ReadPointer(identity.player + 0x50, &identity.cache_owner);
    return identity;
}

bool SameIdentity(const Identity& left, const Identity& right) {
    return left.valid && right.valid &&
        left.player == right.player && left.modules == right.modules &&
        left.movement == right.movement &&
        left.cache_owner == right.cache_owner;
}

bool ReadTiming(std::uintptr_t object, TimingPair* pair) {
    return pair && ReadValue(object + 0x354, &pair->fps) &&
        ReadValue(object + 0x358, &pair->enabled) &&
        std::isfinite(pair->fps);
}

bool ApplyTarget(int fps) {
    if (g_capture_active.load(std::memory_order_acquire) ||
        g_export_requested.load(std::memory_order_acquire)) return false;
    std::uintptr_t object = 0;
    if (!ReadPointer(g_image + kRvaCentralGlobal, &object)) return false;
    if (!g_timing_saved) {
        if (!ReadTiming(object, &g_original_timing)) return false;
        g_timing_object = object;
        g_timing_saved = true;
    } else if (object != g_timing_object) {
        return false;
    }
    const float requested = static_cast<float>(fps);
    const unsigned char enabled = 1;
    if (!WriteValue(object + 0x354, requested) ||
        !WriteValue(object + 0x358, enabled)) return false;
    TimingPair actual{};
    const bool ok = ReadTiming(object, &actual) &&
        std::bit_cast<std::uint32_t>(actual.fps) ==
            std::bit_cast<std::uint32_t>(requested) &&
        actual.enabled == enabled;
    if (ok) g_target_fps.store(fps, std::memory_order_release);
    return ok;
}

bool RestoreTiming() {
    if (!g_timing_saved) return true;
    std::uintptr_t object = 0;
    if (!ReadPointer(g_image + kRvaCentralGlobal, &object) ||
        object != g_timing_object) return false;
    if (!WriteValue(object + 0x354, g_original_timing.fps) ||
        !WriteValue(object + 0x358, g_original_timing.enabled)) return false;
    TimingPair actual{};
    return ReadTiming(object, &actual) &&
        std::bit_cast<std::uint32_t>(actual.fps) ==
            std::bit_cast<std::uint32_t>(g_original_timing.fps) &&
        actual.enabled == g_original_timing.enabled;
}

bool HashExecutable(const wchar_t* path,
                    std::array<unsigned char, 32>* digest) {
    if (!path || !digest) return false;
    HANDLE file = CreateFileW(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr);
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
        ok = object && BCryptCreateHash(algorithm, &hash, object, object_size,
                                        nullptr, 0, 0) == 0;
    }
    while (ok) {
        DWORD read = 0;
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()),
                      &read, nullptr)) {
            ok = false;
            break;
        }
        if (!read) break;
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
    wchar_t path[MAX_PATH * 4]{};
    if (!GetModuleFileNameW(nullptr, path, _countof(path))) return false;
    const wchar_t* name = std::wcsrchr(path, L'\\');
    name = name ? name + 1 : path;
    if (_wcsicmp(name, L"DarkSoulsIII.exe") != 0) return false;
    constexpr std::array<unsigned char, 32> expected_hash = {
        0xEF,0x5E,0x07,0xC5,0x52,0x22,0xF1,0x4F,
        0xFD,0xDE,0xCF,0x2C,0x72,0x4A,0x0C,0x2A,
        0x95,0xCE,0xF0,0xD4,0xDA,0x0E,0x07,0x5B,
        0x0D,0xE0,0xBB,0x10,0x8B,0x69,0x49,0x8C,
    };
    std::array<unsigned char, 32> actual_hash{};
    if (!HashExecutable(path, &actual_hash) || actual_hash != expected_hash) {
        return false;
    }
    constexpr std::array<unsigned char, 18> expected_hook = {
        0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,
        0x10,0x57,0x48,0x81,0xEC,0x90,0x00,0x00,0x00,
    };
    std::array<unsigned char, expected_hook.size()> actual_hook{};
    return ReadArray(g_image + kRvaAggregator, actual_hook.data(),
                     actual_hook.size()) && actual_hook == expected_hook;
}

bool InitializeStorage() {
    wchar_t local[MAX_PATH * 4]{};
    const DWORD length = GetEnvironmentVariableW(
        L"LOCALAPPDATA", local, _countof(local));
    if (!length || length >= _countof(local)) return false;
    wchar_t root[MAX_PATH * 4]{};
    _snwprintf_s(root, _countof(root), _TRUNCATE,
                 L"%s\\DS3I0013ProducerStateObserver", local);
    if (!CreateDirectoryW(root, nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS) return false;
    SYSTEMTIME utc{};
    GetSystemTime(&utc);
    _snwprintf_s(g_session_dir, _countof(g_session_dir), _TRUNCATE,
        L"%s\\%04u%02u%02uT%02u%02u%02uZ-pid%lu", root,
        utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute,
        utc.wSecond, static_cast<unsigned long>(GetCurrentProcessId()));
    if (!CreateDirectoryW(g_session_dir, nullptr)) return false;
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE,
                 L"%s\\CONTROL", g_session_dir);
    return _wfopen_s(&g_control, path, L"wb") == 0 && g_control;
}

void LogControl(const char* event, bool success) {
    if (!g_control) return;
    std::fprintf(g_control,
        "%lld,%s,target=%d,success=%u,samples=%llu,filtered=%llu,invalid=%llu\n",
        Qpc(), event, g_target_fps.load(std::memory_order_acquire),
        success ? 1u : 0u,
        static_cast<unsigned long long>(
            g_sample_count.load(std::memory_order_acquire)),
        static_cast<unsigned long long>(
            g_filtered_call_count.load(std::memory_order_acquire)),
        static_cast<unsigned long long>(
            g_invalid_count.load(std::memory_order_acquire)));
    std::fflush(g_control);
}

void WriteBits(FILE* file, const float* values, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        std::fprintf(file, "%08X", std::bit_cast<std::uint32_t>(values[index]));
    }
}

bool ExportCapture() {
    if (g_inflight.load(std::memory_order_acquire) != 0) return false;
    const std::uint64_t raw_count =
        g_sample_count.load(std::memory_order_acquire);
    const std::size_t count = static_cast<std::size_t>(
        raw_count > kMaximumSamples ? kMaximumSamples : raw_count);
    const unsigned int trial = g_trial.load(std::memory_order_acquire);
    const int target = g_target_fps.load(std::memory_order_acquire);
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE,
        L"%s\\target-%d-trial-%u.csv", g_session_dir, target, trial);
    FILE* file = nullptr;
    if (_wfopen_s(&file, path, L"wb") != 0 || !file) return false;
    std::fprintf(file,
        "qpc_entry,qpc_exit,sequence,caller_return,owner,movement,output,"
        "owner_vtable,owner_e0_target,actor,actor_vtable,actor_178_target,"
        "source_before_x,source_before_z,source_before_y,source_before_w,"
        "source_after_x,source_after_z,source_after_y,source_after_w,"
        "source_extra,selector_a,selector_b,carried,producer_state,"
        "retention,observed_delta,producer_mode,mode_a,mode_b,"
        "vertical_filter,override_gate,local_predicate,"
        "cache_before_x,cache_before_z,cache_before_y,"
        "final_x,final_z,final_y,cache_before48_bits,final_output48_bits,valid\n");
    for (std::size_t index = 0; index < count; ++index) {
        const Sample& sample = g_samples[index];
        std::fprintf(file,
            "%lld,%lld,%llu,0x%llX,0x%llX,0x%llX,0x%llX,"
            "0x%llX,0x%llX,0x%llX,0x%llX,0x%llX,"
            "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,"
            "%.9g,%.9g,%.9g,%.9g,%u,%d,%d,%.9g,%.9g,%u,"
            "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,",
            sample.qpc_entry, sample.qpc_exit,
            static_cast<unsigned long long>(sample.sequence),
            static_cast<unsigned long long>(sample.caller_return),
            static_cast<unsigned long long>(sample.owner),
            static_cast<unsigned long long>(sample.movement),
            static_cast<unsigned long long>(sample.output),
            static_cast<unsigned long long>(sample.owner_vtable),
            static_cast<unsigned long long>(sample.owner_e0_target),
            static_cast<unsigned long long>(sample.actor),
            static_cast<unsigned long long>(sample.actor_vtable),
            static_cast<unsigned long long>(sample.actor_178_target),
            sample.source_before[0], sample.source_before[1],
            sample.source_before[2], sample.source_before[3],
            sample.source_after[0], sample.source_after[1],
            sample.source_after[2], sample.source_after[3],
            sample.source_extra, sample.selector_a, sample.selector_b,
            sample.carried, sample.producer_state, sample.retention,
            sample.observed_delta, static_cast<unsigned int>(sample.producer_mode),
            sample.mode_a, sample.mode_b,
            sample.vertical_filter, sample.override_gate,
            sample.local_predicate,
            sample.cache_before[3], sample.cache_before[7],
            sample.cache_before[11], sample.final_output[3],
            sample.final_output[7], sample.final_output[11]);
        WriteBits(file, sample.cache_before.data(), sample.cache_before.size());
        std::fputc(',', file);
        WriteBits(file, sample.final_output.data(), sample.final_output.size());
        std::fprintf(file, ",%u\n", sample.valid);
    }
    const bool closed = std::fclose(file) == 0;
    const bool accepted_shape = closed && count >= 2 &&
        g_invalid_count.load(std::memory_order_acquire) == 0 &&
        raw_count <= kMaximumSamples;
    LogControl(accepted_shape ? "capture_saved" : "capture_rejected",
               accepted_shape);
    return accepted_shape;
}

void BeginCapture() {
    const Identity identity = ReadIdentity();
    if (!identity.valid) {
        LogControl("capture_arm", false);
        return;
    }
    g_identity = identity;
    g_sample_count.store(0, std::memory_order_relaxed);
    g_invalid_count.store(0, std::memory_order_relaxed);
    g_filtered_call_count.store(0, std::memory_order_relaxed);
    g_capture_start_qpc = Qpc();
    g_trial.fetch_add(1, std::memory_order_relaxed);
    g_capture_active.store(true, std::memory_order_release);
    LogControl("capture_arm", true);
}

void* __fastcall HookAggregator(void* owner, float* output) {
    const std::uintptr_t caller =
        reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const std::uintptr_t caller_rva = caller - g_image;
    bool allowed_caller = false;
    for (const std::uintptr_t allowed : kAllowedCallerReturns) {
        if (caller_rva == allowed) allowed_caller = true;
    }
    const bool selected =
        g_capture_active.load(std::memory_order_acquire) &&
        reinterpret_cast<std::uintptr_t>(owner) == g_identity.cache_owner &&
        allowed_caller;
    if (!selected) {
        if (g_capture_active.load(std::memory_order_relaxed)) {
            g_filtered_call_count.fetch_add(1, std::memory_order_relaxed);
        }
        return g_original_aggregator(owner, output);
    }

    g_inflight.fetch_add(1, std::memory_order_acq_rel);
    if (!g_capture_active.load(std::memory_order_acquire)) {
        g_inflight.fetch_sub(1, std::memory_order_release);
        return g_original_aggregator(owner, output);
    }

    Sample sample{};
    sample.qpc_entry = Qpc();
    sample.caller_return = caller_rva;
    sample.owner = reinterpret_cast<std::uintptr_t>(owner);
    sample.movement = g_identity.movement;
    sample.output = reinterpret_cast<std::uintptr_t>(output);
    bool provenance_ok =
        ReadPointer(sample.owner, &sample.owner_vtable) &&
        ReadPointer(sample.owner_vtable + 0xE0, &sample.owner_e0_target) &&
        ReadPointer(sample.owner + 0x8, &sample.actor) &&
        ReadPointer(sample.actor, &sample.actor_vtable) &&
        ReadPointer(sample.actor_vtable + 0x178, &sample.actor_178_target) &&
        sample.owner_vtable >= g_image && sample.actor_vtable >= g_image &&
        sample.owner_e0_target >= g_image && sample.actor_178_target >= g_image;
    if (provenance_ok) {
        sample.owner_vtable -= g_image;
        sample.actor_vtable -= g_image;
        sample.owner_e0_target -= g_image;
        sample.actor_178_target -= g_image;
    }
    const bool before_ok = provenance_ok &&
        ReadArray(sample.movement + 0x80, sample.source_before.data(),
                  sample.source_before.size()) &&
        ReadValue(sample.movement + 0x3CC, &sample.source_extra) &&
        ReadValue(sample.movement + 0x1F0, &sample.selector_a) &&
        ReadValue(sample.movement + 0x3C8, &sample.selector_b) &&
        ReadValue(sample.owner + 0x250, &sample.carried) &&
        ReadValue(sample.owner + 0x2DC, &sample.producer_state) &&
        ReadValue(sample.owner + 0x378, &sample.retention) &&
        ReadValue(g_timing_object + 0x268, &sample.observed_delta) &&
        ReadValue(sample.owner + 0x374, &sample.producer_mode) &&
        ReadValue(sample.owner + 0x254, &sample.mode_a) &&
        ReadValue(sample.owner + 0x258, &sample.mode_b) &&
        ReadValue(sample.owner + 0x2E4, &sample.vertical_filter) &&
        ReadValue(sample.owner + 0x2EC, &sample.override_gate) &&
        ReadArray(sample.owner + 0x120,
                  sample.cache_before.data(), sample.cache_before.size());
    sample.local_predicate =
        sample.carried != 0.0f && sample.mode_a != 2 &&
        (sample.mode_a != 0 || sample.mode_b != 3) ? 1u : 0u;

    void* const result = g_original_aggregator(owner, output);

    sample.qpc_exit = Qpc();
    const Identity after_identity = ReadIdentity();
    const bool after_ok =
        result == output &&
        ReadArray(sample.movement + 0x80, sample.source_after.data(),
                  sample.source_after.size()) &&
        ReadArray(sample.output, sample.final_output.data(),
                  sample.final_output.size());
    const bool valid = before_ok && after_ok &&
        SameIdentity(after_identity, g_identity) &&
        Finite(sample.source_before.data(), sample.source_before.size()) &&
        Finite(sample.source_after.data(), sample.source_after.size()) &&
        std::isfinite(sample.source_extra) && std::isfinite(sample.carried) &&
        std::isfinite(sample.producer_state) && std::isfinite(sample.retention) &&
        std::isfinite(sample.observed_delta) &&
        std::isfinite(sample.vertical_filter) &&
        std::isfinite(sample.override_gate) &&
        Finite(sample.cache_before.data(), sample.cache_before.size()) &&
        Finite(sample.final_output.data(), sample.final_output.size());
    sample.valid = valid ? 1u : 0u;

    const std::uint64_t raw_index =
        g_sample_count.fetch_add(1, std::memory_order_relaxed);
    sample.sequence = raw_index + 1;
    if (raw_index < kMaximumSamples) {
        g_samples[static_cast<std::size_t>(raw_index)] = sample;
    } else {
        g_invalid_count.fetch_add(1, std::memory_order_relaxed);
    }
    if (!valid) g_invalid_count.fetch_add(1, std::memory_order_relaxed);

    if (raw_index >= kMaximumSamples ||
        ElapsedSeconds(g_capture_start_qpc, sample.qpc_exit) >=
            kCaptureSeconds) {
        g_capture_active.store(false, std::memory_order_release);
        g_export_requested.store(true, std::memory_order_release);
    }
    g_inflight.fetch_sub(1, std::memory_order_release);
    return result;
}

void CreateNamedEvents() {
    const DWORD pid = GetCurrentProcessId();
    wchar_t name[128]{};
    auto create = [&](const wchar_t* suffix, BOOL manual) {
        _snwprintf_s(name, _countof(name), _TRUNCATE,
            L"Local\\DS3_I0013_STATE_%s_%lu", suffix,
            static_cast<unsigned long>(pid));
        return CreateEventW(nullptr, manual, FALSE, name);
    };
    g_stop_event = create(L"STOP", TRUE);
    g_ready_event = create(L"READY", TRUE);
    g_complete_event = create(L"COMPLETE", TRUE);
    g_target60_event = create(L"TARGET60", FALSE);
    g_target240_event = create(L"TARGET240", FALSE);
    g_ack_event = create(L"ACK", TRUE);
}

DWORD WINAPI RuntimeThread(void*) {
    g_image = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    QueryPerformanceFrequency(&g_qpc_frequency);
    CreateNamedEvents();
    bool ok = g_stop_event && g_ready_event && g_complete_event &&
        g_target60_event && g_target240_event && g_ack_event &&
        InitializeStorage() && ValidateRuntimeImage();
    const MH_STATUS initialized = ok ? MH_Initialize() : MH_ERROR_NOT_INITIALIZED;
    ok = ok && initialized == MH_OK;
    if (ok) {
        ok = MH_CreateHook(reinterpret_cast<void*>(g_image + kRvaAggregator),
                           reinterpret_cast<void*>(&HookAggregator),
                           reinterpret_cast<void**>(
                               &g_original_aggregator)) == MH_OK &&
             MH_EnableHook(reinterpret_cast<void*>(
                 g_image + kRvaAggregator)) == MH_OK;
    }
    if (ok) ok = ApplyTarget(60);
    LogControl("ready", ok);
    if (!ok) g_failed.store(true, std::memory_order_release);
    if (g_ready_event) SetEvent(g_ready_event);

    bool f4_previous = false;
    while (ok && WaitForSingleObject(g_stop_event, 10) == WAIT_TIMEOUT) {
        const bool f4 = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
        if (f4 && !f4_previous &&
            !g_capture_active.load(std::memory_order_acquire) &&
            !g_export_requested.load(std::memory_order_acquire)) {
            g_capture_arm.store(true, std::memory_order_release);
        }
        f4_previous = f4;
        if (g_capture_arm.exchange(false, std::memory_order_acq_rel)) {
            BeginCapture();
        }
        if (g_capture_active.load(std::memory_order_acquire) &&
            ElapsedSeconds(g_capture_start_qpc, Qpc()) >=
                kFailClosedSeconds) {
            g_capture_active.store(false, std::memory_order_release);
            g_invalid_count.fetch_add(1, std::memory_order_relaxed);
            g_export_requested.store(true, std::memory_order_release);
        }
        if (g_export_requested.load(std::memory_order_acquire) &&
            g_inflight.load(std::memory_order_acquire) == 0) {
            g_export_requested.store(false, std::memory_order_release);
            if (!ExportCapture()) g_failed.store(true, std::memory_order_release);
        }
        const bool request60 =
            WaitForSingleObject(g_target60_event, 0) == WAIT_OBJECT_0;
        const bool request240 =
            WaitForSingleObject(g_target240_event, 0) == WAIT_OBJECT_0;
        if (request60 || request240) {
            const bool changed = request60 != request240 &&
                ApplyTarget(request60 ? 60 : 240);
            LogControl("target_change", changed);
            if (g_ack_event) SetEvent(g_ack_event);
            if (!changed) g_failed.store(true, std::memory_order_release);
        }
    }

    if (g_capture_active.exchange(false, std::memory_order_acq_rel)) {
        g_invalid_count.fetch_add(1, std::memory_order_relaxed);
        g_export_requested.store(true, std::memory_order_release);
    }
    for (unsigned int spin = 0;
         spin < 1000 && g_inflight.load(std::memory_order_acquire) != 0;
         ++spin) {
        Sleep(1);
    }
    if (g_export_requested.exchange(false, std::memory_order_acq_rel)) {
        if (!ExportCapture()) g_failed.store(true, std::memory_order_release);
    }
    bool hooks_ok = true;
    if (initialized == MH_OK) {
        hooks_ok = MH_DisableHook(MH_ALL_HOOKS) == MH_OK;
        hooks_ok = MH_Uninitialize() == MH_OK && hooks_ok;
    }
    const bool restored = RestoreTiming();
    LogControl("restore", hooks_ok && restored);
    if (g_control) {
        std::fclose(g_control);
        g_control = nullptr;
    }
    ok = ok && hooks_ok && restored &&
        !g_failed.load(std::memory_order_acquire);
    if (g_complete_event) SetEvent(g_complete_event);
    return ok ? 0u : 1u;
}

}  // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, RuntimeThread, nullptr, 0,
                                     nullptr);
        if (!thread) return FALSE;
        CloseHandle(thread);
    }
    return TRUE;
}
