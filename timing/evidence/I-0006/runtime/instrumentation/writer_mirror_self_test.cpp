#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdio>
#include <cstring>

extern "C" void WriterMirrorRoutine(void* object, const float* candidate);
extern "C" void WriterMirrorBreakpoint();

namespace {

alignas(16) unsigned char g_object[0x200]{};
alignas(16) float g_candidate[4] = {5.0f, 6.0f, 7.0f, 0.0f};
std::atomic<DWORD> g_target_tid{0};
std::atomic<unsigned int> g_hits{0};
DWORD g_current_memory[4]{};
DWORD g_current_xmm1[4]{};
DWORD g_raw_candidate[4]{};
DWORD g_scalar_xmm0[4]{};
DWORD g_final_xmm6[4]{};
DWORD g_scalar_memory = 0;
DWORD g_mxcsr = 0;
uintptr_t g_rbx = 0;
HANDLE g_ready = nullptr;
HANDLE g_go = nullptr;
HANDLE g_done = nullptr;
HANDLE g_exit = nullptr;

template <typename T>
T& At(size_t offset) {
    return *reinterpret_cast<T*>(g_object + offset);
}

LONG CALLBACK Handler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord ||
        pointers->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
        GetCurrentThreadId() != g_target_tid.load(std::memory_order_acquire)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CONTEXT* context = pointers->ContextRecord;
    if ((context->Dr6 & 1ull) == 0 ||
        context->Rip != reinterpret_cast<DWORD64>(&WriterMirrorBreakpoint)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    g_hits.fetch_add(1, std::memory_order_relaxed);
    g_rbx = static_cast<uintptr_t>(context->Rbx);
    g_mxcsr = context->MxCsr;
    std::memcpy(g_current_memory, g_object + 0x80, sizeof(g_current_memory));
    std::memcpy(g_current_xmm1, &context->Xmm1, sizeof(g_current_xmm1));
    std::memcpy(g_raw_candidate,
                reinterpret_cast<const void*>(context->Rsp + 0x20),
                sizeof(g_raw_candidate));
    std::memcpy(g_scalar_xmm0, &context->Xmm0, sizeof(g_scalar_xmm0));
    std::memcpy(g_final_xmm6, &context->Xmm6, sizeof(g_final_xmm6));
    std::memcpy(&g_scalar_memory, g_object + 0x1C0, sizeof(g_scalar_memory));
    context->Dr6 &= ~1ull;
    context->EFlags |= 1u << 16;
    return EXCEPTION_CONTINUE_EXECUTION;
}

DWORD WINAPI TargetThread(void*) {
    g_target_tid.store(GetCurrentThreadId(), std::memory_order_release);
    SetEvent(g_ready);
    if (WaitForSingleObject(g_go, 2000) != WAIT_OBJECT_0) return 1;
    WriterMirrorRoutine(g_object, g_candidate);
    SetEvent(g_done);
    return WaitForSingleObject(g_exit, 2000) == WAIT_OBJECT_0 ? 0 : 2;
}

bool Equal4(const DWORD left[4], const DWORD right[4]) {
    return std::memcmp(left, right, sizeof(DWORD) * 4) == 0;
}

bool SameDebugState(const CONTEXT& left, const CONTEXT& right) {
    return left.Dr0 == right.Dr0 && left.Dr1 == right.Dr1 &&
           left.Dr2 == right.Dr2 && left.Dr3 == right.Dr3 &&
           left.Dr6 == right.Dr6 && left.Dr7 == right.Dr7;
}

}  // namespace

int main() {
    const DWORD current_bits[4] = {0x3F800000, 0x40000000, 0x40400000, 0x3F800000};
    const DWORD candidate_bits[4] = {0x40A00000, 0x40C00000, 0x40E00000, 0x00000000};
    const DWORD scalar_bits[4] = {0x3F000000, 0x3F000000, 0x3F000000, 0x3F000000};
    const DWORD final_bits[4] = {0x40400000, 0x40800000, 0x40A00000, 0x3F800000};
    std::memcpy(g_object + 0x80, current_bits, sizeof(current_bits));
    At<float>(0x1C0) = 0.5f;

    g_ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_go = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_exit = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    PVOID veh = AddVectoredExceptionHandler(1, Handler);
    HANDLE thread = CreateThread(nullptr, 0, TargetThread, nullptr, 0, nullptr);
    if (!g_ready || !g_go || !g_done || !g_exit || !veh || !thread ||
        WaitForSingleObject(g_ready, 2000) != WAIT_OBJECT_0) {
        std::fprintf(stderr, "FAIL: setup.\n");
        return 1;
    }

    if (SuspendThread(thread) == static_cast<DWORD>(-1)) return 2;
    CONTEXT original{};
    original.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    bool ok = GetThreadContext(thread, &original) != FALSE;
    CONTEXT armed = original;
    armed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    armed.Dr0 = reinterpret_cast<DWORD64>(&WriterMirrorBreakpoint);
    armed.Dr6 = 0;
    armed.Dr7 &= ~3ull;
    armed.Dr7 &= ~(0xFull << 16);
    armed.Dr7 |= 1ull;
    ok = ok && SetThreadContext(thread, &armed) != FALSE;
    ResumeThread(thread);
    if (!ok) return 3;

    SetEvent(g_go);
    if (WaitForSingleObject(g_done, 2000) != WAIT_OBJECT_0) return 4;
    if (SuspendThread(thread) == static_cast<DWORD>(-1)) return 5;
    CONTEXT restored = original;
    restored.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ok = SetThreadContext(thread, &restored) != FALSE;
    CONTEXT observed{};
    observed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ok = ok && GetThreadContext(thread, &observed) != FALSE &&
         SameDebugState(original, observed);
    ResumeThread(thread);
    SetEvent(g_exit);
    WaitForSingleObject(thread, 2000);

    DWORD committed[4]{};
    std::memcpy(committed, g_object + 0x80, sizeof(committed));
    const bool captured = g_hits.load(std::memory_order_acquire) == 1 &&
                          g_rbx == reinterpret_cast<uintptr_t>(g_object) &&
                          g_mxcsr != 0 && Equal4(g_current_memory, current_bits) &&
                          Equal4(g_current_xmm1, current_bits) &&
                          Equal4(g_raw_candidate, candidate_bits) &&
                          Equal4(g_scalar_xmm0, scalar_bits) &&
                          Equal4(g_final_xmm6, final_bits) &&
                          g_scalar_memory == scalar_bits[0] &&
                          Equal4(committed, final_bits);

    RemoveVectoredExceptionHandler(veh);
    CloseHandle(thread);
    CloseHandle(g_ready);
    CloseHandle(g_go);
    CloseHandle(g_done);
    CloseHandle(g_exit);

    if (!ok || !captured) {
        std::fprintf(stderr, "FAIL: hits=%u restored=%u captured=%u.\n",
                     g_hits.load(std::memory_order_acquire), ok ? 1u : 0u,
                     captured ? 1u : 0u);
        return 6;
    }
    std::printf("PASS: execute breakpoint captured pre-commit inputs/output and restored exact debug state.\n");
    return 0;
}
