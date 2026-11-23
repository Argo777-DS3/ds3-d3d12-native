#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdio>
#include "increment_debug_config.hpp"

namespace {

volatile float g_values[3] = {1.0f, 2.0f, 3.0f};
std::atomic<DWORD> g_target_tid{0};
std::atomic<unsigned int> g_hits{0};
std::atomic<unsigned int> g_slots[3]{};
__declspec(noinline) void First() { g_values[1] = g_values[1] + 1.0f; }
__declspec(noinline) void Second() { g_values[2] = g_values[2] + 1.0f; }
HANDLE g_ready = nullptr;
HANDLE g_go = nullptr;
HANDLE g_wrote = nullptr;
HANDLE g_exit = nullptr;

LONG CALLBACK Handler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord ||
        pointers->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
        GetCurrentThreadId() != g_target_tid.load(std::memory_order_acquire) ||
        (pointers->ContextRecord->Dr6 & 0x7ull) == 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    g_hits.fetch_add(1, std::memory_order_relaxed);
    for (unsigned int i=0; i<3; ++i) if (pointers->ContextRecord->Dr6 & (1ull << i)) g_slots[i]++;
    if (pointers->ContextRecord->Dr6 & 3ull) pointers->ContextRecord->EFlags |= 0x10000u;
    pointers->ContextRecord->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}

DWORD WINAPI WriterThread(void*) {
    g_target_tid.store(GetCurrentThreadId(), std::memory_order_release);
    SetEvent(g_ready);
    if (WaitForSingleObject(g_go, 2000) != WAIT_OBJECT_0) return 1;
    g_values[0] = g_values[0] + 1.0f;
    First();
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
    armed.Dr6 = 0;
    armed.Dr7 = ConfigureIncrementDr7(armed.Dr7);
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
    if (!restore_ok || hits != 3 || g_slots[0]!=1 || g_slots[1]!=1 || g_slots[2]!=1 || g_values[0] != 2.0f ||
        g_values[1] != 3.0f || g_values[2] != 4.0f) {
        std::fprintf(stderr,
                     "FAIL: hits=%u restored=%u values=%.1f/%.1f/%.1f.\n",
                     hits, restore_ok ? 1u : 0u,
                     static_cast<double>(g_values[0]),
                     static_cast<double>(g_values[1]),
                     static_cast<double>(g_values[2]));
        return 7;
    }
    std::printf("PASS: two execute traps, one write trap, RF resume and exact debug-state restoration.\n");
    return 0;
}
