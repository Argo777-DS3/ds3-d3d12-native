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
#include <cwchar>
#include <new>

namespace {

constexpr std::uintptr_t kRvaProducer = 0x005E6580;
constexpr std::uintptr_t kRvaCentralGlobal = 0x0489DD10;
constexpr std::uintptr_t kRvaWorldChrManGlobal = 0x0477FDB8;
constexpr float kReferenceDelta = 0.0166666005f;
constexpr float kNativeGain = 0.2f;
constexpr std::size_t kMaximumSamples = 4096;
constexpr double kCaptureSeconds = 3.0;
constexpr double kFailClosedSeconds = 5.0;

struct TimingPair { float fps = 0.0f; unsigned char enabled = 0; };
struct Identity {
    std::uintptr_t player = 0;
    std::uintptr_t modules = 0;
    std::uintptr_t movement = 0;
    std::uintptr_t producer = 0;
    bool valid = false;
};
struct Sample {
    long long qpc_entry = 0;
    long long qpc_exit = 0;
    std::uint64_t sequence = 0;
    float delta_argument = 0.0f;
    float central_delta = 0.0f;
    float state_before = 0.0f;
    float native_state = 0.0f;
    float final_state = 0.0f;
    float retention = 0.0f;
    float step_ratio = 0.0f;
    unsigned int mode = 0;
    unsigned int branch = 0;
    unsigned int correction_enabled = 0;
    unsigned int correction_applied = 0;
    unsigned int valid = 0;
};

using FnProducer = float(__fastcall*)(void*, float, void*, void*, unsigned char);

std::uintptr_t g_image = 0;
FnProducer g_original = nullptr;
LARGE_INTEGER g_qpc_frequency{};
HANDLE g_stop_event = nullptr;
HANDLE g_ready_event = nullptr;
HANDLE g_complete_event = nullptr;
HANDLE g_target60_event = nullptr;
HANDLE g_target240_event = nullptr;
HANDLE g_enable_event = nullptr;
HANDLE g_disable_event = nullptr;
HANDLE g_ack_event = nullptr;
std::atomic<bool> g_capture_arm{false};
std::atomic<bool> g_capture_active{false};
std::atomic<bool> g_export_requested{false};
std::atomic<bool> g_correction_enabled{false};
std::atomic<bool> g_failed{false};
std::atomic<std::uint64_t> g_sample_count{0};
std::atomic<std::uint64_t> g_invalid_count{0};
std::atomic<std::uint64_t> g_filtered_count{0};
std::atomic<std::uint64_t> g_applied_count{0};
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

long long Qpc() { LARGE_INTEGER value{}; QueryPerformanceCounter(&value); return value.QuadPart; }
double ElapsedSeconds(long long start, long long end) {
    return static_cast<double>(end - start) / static_cast<double>(g_qpc_frequency.QuadPart);
}
template <typename T> bool ReadValue(std::uintptr_t address, T* value) {
    SIZE_T bytes = 0;
    return value && address && ReadProcessMemory(GetCurrentProcess(),
        reinterpret_cast<const void*>(address), value, sizeof(T), &bytes) && bytes == sizeof(T);
}
template <typename T> bool ReadPointer(std::uintptr_t address, T* value) {
    return ReadValue(address, value) && *value != 0;
}
template <typename T> bool WriteValue(std::uintptr_t address, const T& value) {
    SIZE_T bytes = 0;
    return address && WriteProcessMemory(GetCurrentProcess(),
        reinterpret_cast<void*>(address), &value, sizeof(T), &bytes) && bytes == sizeof(T);
}

Identity ReadIdentity() {
    Identity identity{};
    std::uintptr_t world = 0;
    identity.valid = ReadPointer(g_image + kRvaWorldChrManGlobal, &world) &&
        ReadPointer(world + 0x80, &identity.player) &&
        ReadPointer(identity.player + 0x1F90, &identity.modules) &&
        ReadPointer(identity.modules + 0x68, &identity.movement) &&
        ReadPointer(identity.player + 0x50, &identity.producer);
    return identity;
}
bool SameIdentity(const Identity& a, const Identity& b) {
    return a.valid && b.valid && a.player == b.player && a.modules == b.modules &&
        a.movement == b.movement && a.producer == b.producer;
}
bool ReadTiming(std::uintptr_t object, TimingPair* pair) {
    return pair && ReadValue(object + 0x354, &pair->fps) &&
        ReadValue(object + 0x358, &pair->enabled) && std::isfinite(pair->fps);
}
bool ApplyTarget(int fps) {
    if (g_capture_active.load() || g_export_requested.load()) return false;
    std::uintptr_t object = 0;
    if (!ReadPointer(g_image + kRvaCentralGlobal, &object)) return false;
    if (!g_timing_saved) {
        if (!ReadTiming(object, &g_original_timing)) return false;
        g_timing_object = object;
        g_timing_saved = true;
    } else if (object != g_timing_object) return false;
    const float requested = static_cast<float>(fps);
    const unsigned char enabled = 1;
    if (!WriteValue(object + 0x354, requested) || !WriteValue(object + 0x358, enabled)) return false;
    TimingPair actual{};
    const bool ok = ReadTiming(object, &actual) &&
        std::bit_cast<std::uint32_t>(actual.fps) == std::bit_cast<std::uint32_t>(requested) &&
        actual.enabled == enabled;
    if (ok) g_target_fps.store(fps);
    return ok;
}
bool RestoreTiming() {
    if (!g_timing_saved) return true;
    std::uintptr_t object = 0;
    if (!ReadPointer(g_image + kRvaCentralGlobal, &object) || object != g_timing_object) return false;
    if (!WriteValue(object + 0x354, g_original_timing.fps) ||
        !WriteValue(object + 0x358, g_original_timing.enabled)) return false;
    TimingPair actual{};
    return ReadTiming(object, &actual) &&
        std::bit_cast<std::uint32_t>(actual.fps) == std::bit_cast<std::uint32_t>(g_original_timing.fps) &&
        actual.enabled == g_original_timing.enabled;
}

bool HashExecutable(const wchar_t* path, std::array<unsigned char, 32>* digest) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE |
        FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD object_size = 0, result_size = 0; unsigned char* object = nullptr;
    std::array<unsigned char, 4096> buffer{};
    bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
        BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<unsigned char*>(&object_size), sizeof(object_size), &result_size, 0) == 0;
    if (ok) { object = new (std::nothrow) unsigned char[object_size];
        ok = object && BCryptCreateHash(algorithm, &hash, object, object_size, nullptr, 0, 0) == 0; }
    while (ok) { DWORD read = 0; if (!ReadFile(file, buffer.data(),
        static_cast<DWORD>(buffer.size()), &read, nullptr)) { ok = false; break; }
        if (!read) break; ok = BCryptHashData(hash, buffer.data(), read, 0) == 0; }
    if (ok) ok = BCryptFinishHash(hash, digest->data(), static_cast<ULONG>(digest->size()), 0) == 0;
    if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    delete[] object; CloseHandle(file); return ok;
}
bool ValidateRuntimeImage() {
    wchar_t path[MAX_PATH * 4]{}; if (!GetModuleFileNameW(nullptr, path, _countof(path))) return false;
    constexpr std::array<unsigned char, 32> expected = {
        0xEF,0x5E,0x07,0xC5,0x52,0x22,0xF1,0x4F,0xFD,0xDE,0xCF,0x2C,0x72,0x4A,0x0C,0x2A,
        0x95,0xCE,0xF0,0xD4,0xDA,0x0E,0x07,0x5B,0x0D,0xE0,0xBB,0x10,0x8B,0x69,0x49,0x8C };
    std::array<unsigned char, 32> actual{}; if (!HashExecutable(path, &actual) || actual != expected) return false;
    constexpr std::array<unsigned char, 16> prolog = {
        0x40,0x55,0x53,0x56,0x48,0x8D,0x6C,0x24,0xC1,0x48,0x81,0xEC,0xC0,0x00,0x00,0x00 };
    std::array<unsigned char, prolog.size()> bytes{};
    SIZE_T read = 0; return ReadProcessMemory(GetCurrentProcess(),
        reinterpret_cast<void*>(g_image + kRvaProducer), bytes.data(), bytes.size(), &read) &&
        read == bytes.size() && bytes == prolog;
}
bool InitializeStorage() {
    wchar_t local[MAX_PATH * 4]{}; if (!GetEnvironmentVariableW(L"LOCALAPPDATA", local, _countof(local))) return false;
    wchar_t root[MAX_PATH * 4]{}; _snwprintf_s(root, _countof(root), _TRUNCATE,
        L"%s\\DS3I0013ProducerTimeFix", local);
    if (!CreateDirectoryW(root, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    SYSTEMTIME utc{}; GetSystemTime(&utc);
    _snwprintf_s(g_session_dir, _countof(g_session_dir), _TRUNCATE,
        L"%s\\%04u%02u%02uT%02u%02u%02uZ-pid%lu", root, utc.wYear, utc.wMonth,
        utc.wDay, utc.wHour, utc.wMinute, utc.wSecond, static_cast<unsigned long>(GetCurrentProcessId()));
    if (!CreateDirectoryW(g_session_dir, nullptr)) return false;
    wchar_t path[MAX_PATH * 4]{}; _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\CONTROL", g_session_dir);
    return _wfopen_s(&g_control, path, L"wb") == 0 && g_control;
}
void LogControl(const char* event, bool success) {
    if (!g_control) return;
    std::fprintf(g_control, "%lld,%s,target=%d,correction=%u,success=%u,samples=%llu,filtered=%llu,applied=%llu,invalid=%llu\n",
        Qpc(), event, g_target_fps.load(), g_correction_enabled.load() ? 1u : 0u, success ? 1u : 0u,
        static_cast<unsigned long long>(g_sample_count.load()),
        static_cast<unsigned long long>(g_filtered_count.load()),
        static_cast<unsigned long long>(g_applied_count.load()),
        static_cast<unsigned long long>(g_invalid_count.load()));
    std::fflush(g_control);
}
bool ExportCapture() {
    if (g_inflight.load() != 0) return false;
    const auto raw = g_sample_count.load(); const std::size_t count = static_cast<std::size_t>(
        raw > kMaximumSamples ? kMaximumSamples : raw);
    wchar_t path[MAX_PATH * 4]{}; _snwprintf_s(path, _countof(path), _TRUNCATE,
        L"%s\\target-%d-%s-trial-%u.csv", g_session_dir, g_target_fps.load(),
        g_correction_enabled.load() ? L"on" : L"off", g_trial.load());
    FILE* file = nullptr; if (_wfopen_s(&file, path, L"wb") != 0 || !file) return false;
    std::fprintf(file, "qpc_entry,qpc_exit,sequence,delta_argument,central_delta,state_before,native_state,final_state,retention,step_ratio,mode,branch,correction_enabled,correction_applied,valid\n");
    for (std::size_t i = 0; i < count; ++i) { const Sample& s = g_samples[i];
        std::fprintf(file, "%lld,%lld,%llu,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%u,%u,%u,%u,%u\n",
            s.qpc_entry, s.qpc_exit, static_cast<unsigned long long>(s.sequence),
            s.delta_argument, s.central_delta, s.state_before, s.native_state, s.final_state,
            s.retention, s.step_ratio, s.mode, s.branch, s.correction_enabled,
            s.correction_applied, s.valid); }
    const bool closed = std::fclose(file) == 0;
    const bool accepted = closed && count >= 2 && raw <= kMaximumSamples && g_invalid_count.load() == 0;
    LogControl(accepted ? "capture_saved" : "capture_rejected", accepted); return accepted;
}
void BeginCapture() {
    const Identity identity = ReadIdentity(); if (!identity.valid) { LogControl("capture_arm", false); return; }
    g_identity = identity; g_sample_count.store(0); g_invalid_count.store(0);
    g_filtered_count.store(0); g_applied_count.store(0); g_capture_start_qpc = Qpc();
    g_trial.fetch_add(1); g_capture_active.store(true); LogControl("capture_arm", true);
}

float __fastcall HookProducer(void* owner, float delta, void* context, void* vector, unsigned char flag) {
    const bool selected = reinterpret_cast<std::uintptr_t>(owner) == g_identity.producer && g_identity.valid;
    const bool capture = selected && g_capture_active.load();
    const bool correct = selected && g_correction_enabled.load();
    if (!capture && !correct) { if (g_capture_active.load()) g_filtered_count.fetch_add(1);
        return g_original(owner, delta, context, vector, flag); }
    g_inflight.fetch_add(1);
    Sample sample{}; sample.qpc_entry = Qpc(); sample.delta_argument = delta;
    sample.correction_enabled = correct ? 1u : 0u;
    unsigned char mode = 0; const bool before = ReadValue(reinterpret_cast<std::uintptr_t>(owner) + 0x2DC,
        &sample.state_before) && ReadValue(reinterpret_cast<std::uintptr_t>(owner) + 0x378, &sample.retention) &&
        ReadValue(reinterpret_cast<std::uintptr_t>(owner) + 0x374, &mode) &&
        ReadValue(g_timing_object + 0x268, &sample.central_delta);
    sample.mode = mode;
    const float native = g_original(owner, delta, context, vector, flag);
    sample.native_state = native; sample.final_state = native; sample.step_ratio = 1.0f;
    const bool finite = before && std::isfinite(delta) && std::isfinite(sample.central_delta) &&
        std::isfinite(sample.state_before) && std::isfinite(native) && std::isfinite(sample.retention);
    if (correct && finite && delta > 0.0f && delta < 0.1f) {
        const bool native_reference = std::bit_cast<std::uint32_t>(delta) ==
            std::bit_cast<std::uint32_t>(kReferenceDelta);
        const bool decay = std::bit_cast<std::uint32_t>(native) ==
            std::bit_cast<std::uint32_t>(sample.state_before * sample.retention);
        if (native_reference) sample.branch = 1;
        else if (native == 0.0f) sample.branch = 4;
        else if (std::fabs(native) == 0.5f) sample.branch = 5;
        else {
            const double ratio = static_cast<double>(delta) / static_cast<double>(kReferenceDelta);
            float corrected = native;
            if (decay && sample.retention > 0.0f && sample.retention <= 1.0f) {
                corrected = static_cast<float>(static_cast<double>(sample.state_before) *
                    std::pow(static_cast<double>(sample.retention), ratio));
                sample.branch = 3;
            } else {
                corrected = static_cast<float>(static_cast<double>(sample.state_before) +
                    static_cast<double>(native - sample.state_before) * ratio);
                sample.branch = 2;
            }
            if (corrected > 0.5f) corrected = 0.5f;
            if (corrected < -0.5f) corrected = -0.5f;
            sample.step_ratio = static_cast<float>(ratio); sample.final_state = corrected;
            if (std::isfinite(corrected) && WriteValue(reinterpret_cast<std::uintptr_t>(owner) + 0x2DC, corrected)) {
                sample.correction_applied = 1; g_applied_count.fetch_add(1);
            } else { sample.branch = 6; g_invalid_count.fetch_add(1); sample.final_state = native; }
        }
    } else if (correct) sample.branch = 6;
    else sample.branch = 0;
    sample.qpc_exit = Qpc(); const Identity after = ReadIdentity();
    sample.valid = finite && SameIdentity(after, g_identity) ? 1u : 0u;
    if (capture) { const auto index = g_sample_count.fetch_add(1); sample.sequence = index + 1;
        if (index < kMaximumSamples) g_samples[static_cast<std::size_t>(index)] = sample;
        else g_invalid_count.fetch_add(1); if (!sample.valid) g_invalid_count.fetch_add(1);
        if (index >= kMaximumSamples || ElapsedSeconds(g_capture_start_qpc, sample.qpc_exit) >= kCaptureSeconds) {
            g_capture_active.store(false); g_export_requested.store(true); } }
    g_inflight.fetch_sub(1); return sample.correction_applied ? sample.final_state : native;
}

void CreateEvents() {
    const DWORD pid = GetCurrentProcessId(); wchar_t name[128]{};
    auto create = [&](const wchar_t* suffix, BOOL manual) { _snwprintf_s(name, _countof(name), _TRUNCATE,
        L"Local\\DS3_I0013_TIMEFIX_%s_%lu", suffix, static_cast<unsigned long>(pid));
        return CreateEventW(nullptr, manual, FALSE, name); };
    g_stop_event=create(L"STOP",TRUE); g_ready_event=create(L"READY",TRUE);
    g_complete_event=create(L"COMPLETE",TRUE); g_target60_event=create(L"TARGET60",FALSE);
    g_target240_event=create(L"TARGET240",FALSE); g_enable_event=create(L"ENABLE",FALSE);
    g_disable_event=create(L"DISABLE",FALSE); g_ack_event=create(L"ACK",TRUE);
}
DWORD WINAPI RuntimeThread(void*) {
    g_image = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    QueryPerformanceFrequency(&g_qpc_frequency); CreateEvents();
    bool ok = g_stop_event && g_ready_event && g_complete_event && g_target60_event &&
        g_target240_event && g_enable_event && g_disable_event && g_ack_event &&
        InitializeStorage() && ValidateRuntimeImage();
    const MH_STATUS initialized = ok ? MH_Initialize() : MH_ERROR_NOT_INITIALIZED;
    ok = ok && initialized == MH_OK;
    if (ok) ok = MH_CreateHook(reinterpret_cast<void*>(g_image + kRvaProducer),
        reinterpret_cast<void*>(&HookProducer), reinterpret_cast<void**>(&g_original)) == MH_OK &&
        MH_EnableHook(reinterpret_cast<void*>(g_image + kRvaProducer)) == MH_OK;
    if (ok) ok = ApplyTarget(60);
    g_identity = ReadIdentity();
    LogControl("ready", ok && g_identity.valid); if (!ok || !g_identity.valid) g_failed.store(true);
    if (g_ready_event) SetEvent(g_ready_event);
    bool f4_previous = false;
    while (ok && WaitForSingleObject(g_stop_event, 10) == WAIT_TIMEOUT) {
        const bool f4 = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
        if (f4 && !f4_previous && !g_capture_active.load() && !g_export_requested.load()) g_capture_arm.store(true);
        f4_previous = f4; if (g_capture_arm.exchange(false)) BeginCapture();
        if (g_capture_active.load() && ElapsedSeconds(g_capture_start_qpc, Qpc()) >= kFailClosedSeconds) {
            g_capture_active.store(false); g_invalid_count.fetch_add(1); g_export_requested.store(true); }
        if (g_export_requested.load() && g_inflight.load() == 0) {
            g_export_requested.store(false); if (!ExportCapture()) g_failed.store(true); }
        const bool r60=WaitForSingleObject(g_target60_event,0)==WAIT_OBJECT_0;
        const bool r240=WaitForSingleObject(g_target240_event,0)==WAIT_OBJECT_0;
        const bool ren=WaitForSingleObject(g_enable_event,0)==WAIT_OBJECT_0;
        const bool rdis=WaitForSingleObject(g_disable_event,0)==WAIT_OBJECT_0;
        if (r60 || r240) { const bool changed=r60!=r240 && ApplyTarget(r60?60:240);
            LogControl("target_change",changed); if(g_ack_event)SetEvent(g_ack_event); if(!changed)g_failed.store(true); }
        if (ren || rdis) { bool changed=ren!=rdis && !g_capture_active.load();
            if (changed && ren) { g_identity=ReadIdentity(); changed=g_identity.valid; }
            if (changed) g_correction_enabled.store(ren);
            LogControl(ren?"correction_enable":"correction_disable",changed);
            if(g_ack_event)SetEvent(g_ack_event); if(!changed)g_failed.store(true); }
    }
    g_correction_enabled.store(false); if (g_capture_active.exchange(false)) {
        g_invalid_count.fetch_add(1); g_export_requested.store(true); }
    for (unsigned int i=0;i<1000 && g_inflight.load()!=0;i++) Sleep(1);
    if (g_export_requested.exchange(false) && !ExportCapture()) g_failed.store(true);
    bool hooks_ok=true; if(initialized==MH_OK){ hooks_ok=MH_DisableHook(MH_ALL_HOOKS)==MH_OK;
        hooks_ok=MH_Uninitialize()==MH_OK && hooks_ok; }
    const bool restored=RestoreTiming(); LogControl("restore",hooks_ok&&restored);
    if(g_control){std::fclose(g_control);g_control=nullptr;} ok=ok&&hooks_ok&&restored&&!g_failed.load();
    if(g_complete_event)SetEvent(g_complete_event); return ok?0u:1u;
}
}  // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(instance);
        HANDLE thread=CreateThread(nullptr,0,RuntimeThread,nullptr,0,nullptr);
        if(!thread)return FALSE;CloseHandle(thread);} return TRUE;
}
