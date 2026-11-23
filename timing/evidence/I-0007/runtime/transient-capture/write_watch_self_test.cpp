#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdio>
#include "patch_debug_config.hpp"
#include "../causal-patch/distance_normalization.hpp"
#include "../causal-patch/shared_log.hpp"
#include "../causal-patch/capture_lifetime.hpp"
#include <limits>

namespace {

bool TestSharedLog() {
    wchar_t directory[MAX_PATH]{}, filename[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, directory) || !GetTempFileNameW(directory, L"d3p", 0, filename)) return false;
    FILE* legacy = nullptr;
    if (_wfopen_s(&legacy, filename, L"wb") != 0 || !legacy) return false;
    HANDLE denied = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    const DWORD failure = GetLastError();
    const bool reproduced = denied == INVALID_HANDLE_VALUE && failure == ERROR_SHARING_VIOLATION;
    if (denied != INVALID_HANDLE_VALUE) CloseHandle(denied);
    std::fclose(legacy);
    FILE* writer = OpenSharedCaptureLog(filename);
    if (!writer) { DeleteFileW(filename); return false; }
    std::fputs("first\n", writer); std::fflush(writer);
    HANDLE reader = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    char first[6]{}, second[7]{}; DWORD bytes1=0, bytes2=0;
    bool ok = reader != INVALID_HANDLE_VALUE && ReadFile(reader, first, 6, &bytes1, nullptr);
    std::fputs("second\n", writer); std::fflush(writer);
    ok = ok && ReadFile(reader, second, 7, &bytes2, nullptr) && bytes1 == 6 && bytes2 == 7 &&
        std::memcmp(first, "first\n", 6) == 0 && std::memcmp(second, "second\n", 7) == 0;
    HANDLE other_writer = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    ok = ok && other_writer == INVALID_HANDLE_VALUE;
    if (other_writer != INVALID_HANDLE_VALUE) CloseHandle(other_writer);
    if (reader != INVALID_HANDLE_VALUE) CloseHandle(reader);
    std::fclose(writer);
    // Exact temporary file returned above, not a computed recursive target.
    ok = DeleteFileW(filename) != FALSE && ok;
    if (reproduced && ok) std::printf("PASS: legacy read lock reproduced; shared read, successive flushes, exclusive writer and temporary-file cleanup verified.\n");
    return reproduced && ok;
}

volatile float g_values[3] = {1.0f, 2.0f, 3.0f};
std::atomic<DWORD> g_target_tid{0};
std::atomic<unsigned int> g_hits{0};
std::atomic<unsigned int> g_slots[4]{};
__declspec(noinline) void Third() { g_values[0] = g_values[0] + 1.0f; }
volatile float g_raw = 0.5f;
volatile float g_comparison_result = 0.0f;
std::atomic<unsigned int> g_patch_count{0};
__declspec(noinline) float First(float raw) { g_values[1] = g_values[1] + 1.0f; return raw < 1.0f ? 0.5f : 1.0f; }
__declspec(noinline) void Second() { g_values[2] = g_values[2] + 1.0f; }
__declspec(noinline) void Fourth() { g_values[2] = g_values[2] + 2.0f; }
HANDLE g_ready = nullptr;
HANDLE g_go = nullptr;
HANDLE g_wrote = nullptr;
HANDLE g_exit = nullptr;

LONG CALLBACK Handler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord ||
        pointers->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
        GetCurrentThreadId() != g_target_tid.load(std::memory_order_acquire) ||
        (pointers->ContextRecord->Dr6 & 0xFull) == 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    g_hits.fetch_add(1, std::memory_order_relaxed);
    if ((pointers->ContextRecord->Dr6 & 1ull) != 0) {
        DWORD bits = static_cast<DWORD>(pointers->ContextRecord->Xmm0.Low);
        float raw = 0.0f, fixed = 0.0f;
        std::memcpy(&raw, &bits, 4);
        if (NormalizeDistanceTest(raw, 1.0f / 165.0f, &fixed)) {
            std::memcpy(&bits, &fixed, 4);
            pointers->ContextRecord->Xmm0.Low =
                (pointers->ContextRecord->Xmm0.Low & 0xFFFFFFFF00000000ull) | bits;
            g_patch_count++;
        }
    }
    for (unsigned int i=0; i<4; ++i) if (pointers->ContextRecord->Dr6 & (1ull << i)) g_slots[i]++;
    if (pointers->ContextRecord->Dr6 & 0xBull) pointers->ContextRecord->EFlags |= 0x10000u;
    pointers->ContextRecord->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}

DWORD WINAPI WriterThread(void*) {
    g_target_tid.store(GetCurrentThreadId(), std::memory_order_release);
    SetEvent(g_ready);
    if (WaitForSingleObject(g_go, 2000) != WAIT_OBJECT_0) return 1;
    Fourth();
    Third();
    g_comparison_result = First(g_raw);
    Second();

    SetEvent(g_wrote);
    return WaitForSingleObject(g_exit, 2000) == WAIT_OBJECT_0 ? 0 : 2;
}

bool SameDebugState(const CONTEXT& left, const CONTEXT& right) {
    return left.Dr0 == right.Dr0 && left.Dr1 == right.Dr1 &&
           left.Dr2 == right.Dr2 && left.Dr3 == right.Dr3 &&
           left.Dr6 == right.Dr6 && left.Dr7 == right.Dr7;
}

}  // namespace

int main() {
    if (!TestSharedLog()) { std::fprintf(stderr, "FAIL: shared live log test.\n"); return 20; }
    PostMeasureCapture tail;
    tail.Begin();
    if (!tail.Advance(true, 3.0f) || !tail.active || tail.Advance(false, 0.01f)) return 21;
    tail.Begin();
    if (!tail.Advance(true, 9.0f) || tail.Advance(true, 1.0f)) return 22;
    std::printf("PASS: correction capture persists after measurement until input release; bounded timeout verified.\n");
    float normalized = 0.0f;
    if (!NormalizeDistanceTest(0.5f, 1.0f / 60.0f, &normalized) || normalized != 0.5f ||
        !NormalizeDistanceTest(0.5f, 1.0f / 165.0f, &normalized) || std::fabs(normalized - 1.375f) > 1e-6f ||
        NormalizeDistanceTest(0.5f, 0.0f, &normalized) || NormalizeDistanceTest(-1.0f, 1.0f, &normalized) ||
        NormalizeDistanceTest(1.0f, std::numeric_limits<float>::infinity(), &normalized)) {
        std::fprintf(stderr, "FAIL: time normalization.\n"); return 8;
    }
    DistanceFrame previous{1, 1.0f / 60.0f, 123, {1, 2, 3}, true};
    DistanceFrame frame_now{2, 1.0f / 165.0f, 123, {2, 3, 4}, true};
    if (!MatchDistanceFrames(previous, frame_now, 2, 123, frame_now.position, previous.position) ||
        MatchDistanceFrames(previous, frame_now, 3, 123, frame_now.position, previous.position) ||
        MatchDistanceFrames(previous, frame_now, 2, 456, frame_now.position, previous.position) ||
        MatchDistanceFrames(previous, frame_now, 2, 123, previous.position, frame_now.position)) {
        std::fprintf(stderr, "FAIL: interval identity.\n"); return 9;
    }
    if ((reinterpret_cast<uintptr_t>(&g_values[0]) & 3u) != 0 ||
        (reinterpret_cast<uintptr_t>(&g_values[1]) & 3u) != 0 ||
        (reinterpret_cast<uintptr_t>(&g_values[2]) & 3u) != 0) {
        std::fprintf(stderr, "FAIL: test values are not four-byte aligned.\n");
        return 1;
    }

    g_ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_go = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_wrote = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_exit = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    PVOID veh = AddVectoredExceptionHandler(1, Handler);
    HANDLE thread = CreateThread(nullptr, 0, WriterThread, nullptr, 0, nullptr);
    if (!g_ready || !g_go || !g_wrote || !g_exit || !veh || !thread ||
        WaitForSingleObject(g_ready, 2000) != WAIT_OBJECT_0) {
        std::fprintf(stderr, "FAIL: setup.\n");
        return 2;
    }

    if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
        std::fprintf(stderr, "FAIL: suspend for arm.\n");
        return 3;
    }
    CONTEXT original{};
    original.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    CONTEXT armed{};
    armed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    bool arm_ok = GetThreadContext(thread, &original) != FALSE;
    armed = original;
    armed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    armed.Dr0 = reinterpret_cast<DWORD64>(&First);
    armed.Dr1 = reinterpret_cast<DWORD64>(&Second);
    armed.Dr2 = reinterpret_cast<DWORD64>(&g_values[0]);
    armed.Dr3 = reinterpret_cast<DWORD64>(&Fourth);
    armed.Dr6 = 0;
    armed.Dr7 = ConfigurePatchDr7(armed.Dr7);
    arm_ok = arm_ok && SetThreadContext(thread, &armed) != FALSE;
    ResumeThread(thread);
    if (!arm_ok) {
        std::fprintf(stderr, "FAIL: arm.\n");
        return 4;
    }

    SetEvent(g_go);
    if (WaitForSingleObject(g_wrote, 2000) != WAIT_OBJECT_0) {
        std::fprintf(stderr, "FAIL: writer timeout.\n");
        return 5;
    }

    if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
        std::fprintf(stderr, "FAIL: suspend for restore.\n");
        return 6;
    }
    CONTEXT current{};
    current.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    bool restore_ok = GetThreadContext(thread, &current) != FALSE;
    current.Dr0 = original.Dr0;
    current.Dr1 = original.Dr1;
    current.Dr2 = original.Dr2;
    current.Dr3 = original.Dr3;
    current.Dr6 = original.Dr6;
    current.Dr7 = original.Dr7;
    restore_ok = restore_ok && SetThreadContext(thread, &current) != FALSE;
    CONTEXT restored{};
    restored.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    restore_ok = restore_ok && GetThreadContext(thread, &restored) != FALSE &&
                 SameDebugState(original, restored);
    ResumeThread(thread);
    SetEvent(g_exit);
    WaitForSingleObject(thread, 2000);

    RemoveVectoredExceptionHandler(veh);
    CloseHandle(thread);
    CloseHandle(g_ready);
    CloseHandle(g_go);
    CloseHandle(g_wrote);
    CloseHandle(g_exit);

    const unsigned int hits = g_hits.load(std::memory_order_acquire);
    if (!restore_ok || g_patch_count != 1 || g_comparison_result != 1.0f || hits != 4 || g_slots[3]!=1 || g_slots[0]!=1 || g_slots[1]!=1 || g_slots[2]!=1 || g_values[0] != 2.0f ||
        g_values[1] != 3.0f || g_values[2] != 6.0f) {
        std::fprintf(stderr,
                     "FAIL: hits=%u restored=%u values=%.1f/%.1f/%.1f.\n",
                     hits, restore_ok ? 1u : 0u,
                     static_cast<double>(g_values[0]),
                     static_cast<double>(g_values[1]),
                     static_cast<double>(g_values[2]));
        return 7;
    }
    std::printf("PASS: time normalization, interval identity, live XMM0 comparison correction, three execute traps plus commit-write trap, RF resume and exact debug-state restoration.\n");
    return 0;
}
