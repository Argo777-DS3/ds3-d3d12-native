#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

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

constexpr std::uintptr_t kRvaBroadUpdate = 0x00D57900;
constexpr std::uintptr_t kRvaCentralGlobal = 0x0489DD10;
constexpr std::uintptr_t kRvaWorldChrManGlobal = 0x0477FDB8;
constexpr std::size_t kMaximumSamples = 2048;
constexpr float kCaptureSeconds = 3.0f;

struct TimingPair {
    float fps = 0.0f;
    unsigned char enabled = 0;
};

struct StageSnapshot {
    std::uintptr_t player = 0;
    std::uintptr_t modules = 0;
    std::uintptr_t movement_state = 0;
    std::uintptr_t cache = 0;
    std::uintptr_t root_transform = 0;
    std::array<float, 4> source{};
    float source_extra = 0.0f;
    int selector_a = 0;
    int selector_b = 0;
    std::array<float, 4> vertical_filter{};
    unsigned char vertical_filter_flag = 0;
    std::array<float, 12> cache_transform{};
    std::array<float, 3> root{};
    bool valid = false;
};

struct Sample {
    long long qpc = 0;
    std::uint64_t update_index = 0;
    float delta = 0.0f;
    StageSnapshot stage{};
    unsigned int identity_valid = 0;
};

using FnBroadUpdate = void(__fastcall*)(void*, void*);

std::uintptr_t g_image = 0;
FnBroadUpdate g_original_broad_update = nullptr;
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
std::atomic<std::uint64_t> g_updates{0};
std::atomic<std::uint64_t> g_sample_count{0};
std::atomic<std::uint64_t> g_invalid_count{0};
std::atomic<unsigned int> g_trial{0};
std::atomic<int> g_target_fps{60};
std::array<Sample, kMaximumSamples> g_samples{};
StageSnapshot g_identity{};
float g_capture_elapsed = 0.0f;

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

StageSnapshot ReadStages() {
    StageSnapshot snapshot{};
    std::uintptr_t world = 0;
    std::uintptr_t list = 0;
    if (!ReadPointer(g_image + kRvaWorldChrManGlobal, &world) ||
        !ReadPointer(world + 0x80, &snapshot.player) ||
        !ReadPointer(snapshot.player + 0x1F90, &snapshot.modules) ||
        !ReadPointer(snapshot.modules + 0x68, &snapshot.movement_state) ||
        !ReadPointer(snapshot.player + 0x50, &snapshot.cache) ||
        !ReadPointer(world + 0x40, &list) ||
        !ReadPointer(list + 0x28, &snapshot.root_transform)) {
        return snapshot;
    }
    const bool reads =
        ReadArray(snapshot.movement_state + 0x80,
                  snapshot.source.data(), snapshot.source.size()) &&
        ReadValue(snapshot.movement_state + 0x3CC,
                  &snapshot.source_extra) &&
        ReadValue(snapshot.movement_state + 0x1F0, &snapshot.selector_a) &&
        ReadValue(snapshot.movement_state + 0x3C8, &snapshot.selector_b) &&
        ReadArray(snapshot.cache + 0x2E0, snapshot.vertical_filter.data(),
                  snapshot.vertical_filter.size()) &&
        ReadValue(snapshot.cache + 0x2F0,
                  &snapshot.vertical_filter_flag) &&
        ReadArray(snapshot.cache + 0x120, snapshot.cache_transform.data(),
                  snapshot.cache_transform.size()) &&
        ReadArray(snapshot.root_transform + 0x80, snapshot.root.data(),
                  snapshot.root.size());
    snapshot.valid = reads &&
        Finite(snapshot.source.data(), snapshot.source.size()) &&
        std::isfinite(snapshot.source_extra) &&
        Finite(snapshot.vertical_filter.data(),
               snapshot.vertical_filter.size()) &&
        Finite(snapshot.cache_transform.data(),
               snapshot.cache_transform.size()) &&
        Finite(snapshot.root.data(), snapshot.root.size());
    return snapshot;
}

bool SameIdentity(const StageSnapshot& left, const StageSnapshot& right) {
    return left.player == right.player &&
        left.modules == right.modules &&
        left.movement_state == right.movement_state &&
        left.cache == right.cache &&
        left.root_transform == right.root_transform;
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
    constexpr std::array<unsigned char, 15> expected_bytes = {
        0x48,0x8B,0xC4,0x57,0x48,0x83,0xEC,0x50,
        0x48,0xC7,0x40,0xC8,0xFE,0xFF,0xFF,
    };
    std::array<unsigned char, 15> actual_bytes{};
    return ReadArray(g_image + kRvaBroadUpdate, actual_bytes.data(),
                     actual_bytes.size()) && actual_bytes == expected_bytes;
}

bool InitializeStorage() {
    wchar_t local[MAX_PATH * 4]{};
    const DWORD length = GetEnvironmentVariableW(
        L"LOCALAPPDATA", local, _countof(local));
    if (!length || length >= _countof(local)) return false;
    wchar_t root[MAX_PATH * 4]{};
    _snwprintf_s(root, _countof(root), _TRUNCATE,
                 L"%s\\DS3I0013StageObserver", local);
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
    std::fprintf(g_control, "%lld,%s,target=%d,success=%u\n", Qpc(), event,
                 g_target_fps.load(std::memory_order_acquire),
                 success ? 1u : 0u);
    std::fflush(g_control);
}

void WriteBits(FILE* file, const float* values, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        std::fprintf(file, "%08X", std::bit_cast<std::uint32_t>(values[index]));
    }
}

bool ExportCapture() {
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
        "qpc,update_index,delta,player,modules,movement_state,cache,root_transform,"
        "source_x,source_z,source_y,source_w,source_extra,selector_a,selector_b,"
        "filter_2e0,filter_2e4,filter_2e8,filter_2ec,filter_2f0,"
        "cache_x,cache_z,cache_y,root_x,root_z,root_y,cache48_bits,valid\n");
    for (std::size_t index = 0; index < count; ++index) {
        const Sample& sample = g_samples[index];
        const StageSnapshot& stage = sample.stage;
        std::fprintf(file,
            "%lld,%llu,%.9g,0x%llX,0x%llX,0x%llX,0x%llX,0x%llX,"
            "%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%.9g,%.9g,%.9g,"
            "%.9g,%u,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,",
            sample.qpc, static_cast<unsigned long long>(sample.update_index),
            sample.delta, static_cast<unsigned long long>(stage.player),
            static_cast<unsigned long long>(stage.modules),
            static_cast<unsigned long long>(stage.movement_state),
            static_cast<unsigned long long>(stage.cache),
            static_cast<unsigned long long>(stage.root_transform),
            stage.source[0], stage.source[1], stage.source[2], stage.source[3],
            stage.source_extra, stage.selector_a, stage.selector_b,
            stage.vertical_filter[0], stage.vertical_filter[1],
            stage.vertical_filter[2], stage.vertical_filter[3],
            static_cast<unsigned int>(stage.vertical_filter_flag),
            stage.cache_transform[3], stage.cache_transform[7],
            stage.cache_transform[11], stage.root[0], stage.root[1],
            stage.root[2]);
        WriteBits(file, stage.cache_transform.data(),
                  stage.cache_transform.size());
        std::fprintf(file, ",%u\n", sample.identity_valid);
    }
    const bool closed = std::fclose(file) == 0;
    const bool accepted_shape = closed && count >= 2 &&
        g_invalid_count.load(std::memory_order_acquire) == 0;
    LogControl(accepted_shape ? "capture_saved" : "capture_rejected",
               accepted_shape);
    return accepted_shape;
}

void BeginCapture() {
    const StageSnapshot stage = ReadStages();
    if (!stage.valid) {
        LogControl("capture_arm", false);
        return;
    }
    g_identity = stage;
    g_capture_elapsed = 0.0f;
    g_sample_count.store(0, std::memory_order_relaxed);
    g_invalid_count.store(0, std::memory_order_relaxed);
    g_trial.fetch_add(1, std::memory_order_relaxed);
    g_capture_active.store(true, std::memory_order_release);
    LogControl("capture_arm", true);
}

void AppendSample(float delta, bool delta_valid, std::uint64_t update_index) {
    if (!g_capture_active.load(std::memory_order_acquire)) return;
    const std::uint64_t raw_index =
        g_sample_count.fetch_add(1, std::memory_order_relaxed);
    if (raw_index >= kMaximumSamples) {
        g_capture_active.store(false, std::memory_order_release);
        g_export_requested.store(true, std::memory_order_release);
        g_invalid_count.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    Sample sample{};
    sample.qpc = Qpc();
    sample.update_index = update_index;
    sample.delta = delta;
    sample.stage = ReadStages();
    const bool valid = delta_valid && sample.stage.valid &&
        SameIdentity(sample.stage, g_identity);
    sample.identity_valid = valid ? 1u : 0u;
    g_samples[static_cast<std::size_t>(raw_index)] = sample;
    if (!valid) g_invalid_count.fetch_add(1, std::memory_order_relaxed);
    if (delta_valid) g_capture_elapsed += delta;
    if (g_capture_elapsed >= kCaptureSeconds) {
        g_capture_active.store(false, std::memory_order_release);
        g_export_requested.store(true, std::memory_order_release);
    }
}

void __fastcall HookBroadUpdate(void* self, void* context) {
    float delta = 0.0f;
    const bool delta_valid = context &&
        ReadValue(reinterpret_cast<std::uintptr_t>(context) + 0x08, &delta) &&
        std::isfinite(delta) && delta > 0.0f;
    g_original_broad_update(self, context);
    const std::uint64_t update_index =
        g_updates.fetch_add(1, std::memory_order_relaxed) + 1;
    if (g_capture_arm.exchange(false, std::memory_order_acq_rel) &&
        !g_capture_active.load(std::memory_order_acquire) &&
        !g_export_requested.load(std::memory_order_acquire)) {
        BeginCapture();
    }
    AppendSample(delta, delta_valid, update_index);
}

void CreateNamedEvents() {
    const DWORD pid = GetCurrentProcessId();
    wchar_t name[128]{};
    auto create = [&](const wchar_t* suffix, BOOL manual) {
        _snwprintf_s(name, _countof(name), _TRUNCATE,
            L"Local\\DS3_I0013_STAGE_%s_%lu", suffix,
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
        ok = MH_CreateHook(reinterpret_cast<void*>(g_image + kRvaBroadUpdate),
                           reinterpret_cast<void*>(&HookBroadUpdate),
                           reinterpret_cast<void**>(&g_original_broad_update)) == MH_OK &&
             MH_EnableHook(reinterpret_cast<void*>(g_image + kRvaBroadUpdate)) == MH_OK;
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
        if (g_export_requested.exchange(false, std::memory_order_acq_rel)) {
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
        ExportCapture();
    } else if (g_export_requested.exchange(false, std::memory_order_acq_rel)) {
        ExportCapture();
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
        HANDLE thread = CreateThread(nullptr, 0, RuntimeThread, nullptr, 0, nullptr);
        if (!thread) return FALSE;
        CloseHandle(thread);
    }
    return TRUE;
}
