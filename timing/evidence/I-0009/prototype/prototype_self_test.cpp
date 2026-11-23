#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <limits>

#include "debug_config.hpp"
#include "prototype_core.hpp"

namespace {

volatile float g_values[3] = {1.0f, 2.0f, 3.0f};
volatile float g_raw = 0.5f;
volatile float g_result = 0.0f;
std::atomic<DWORD> g_target_tid{0};
std::atomic<unsigned int> g_hits{0};
std::atomic<unsigned int> g_slots[4]{};
std::atomic<unsigned int> g_patch_count{0};
HANDLE g_ready = nullptr;
HANDLE g_go = nullptr;
HANDLE g_wrote = nullptr;
HANDLE g_exit = nullptr;

__declspec(noinline) void Commit() { g_values[0] = g_values[0] + 1.0f; }
__declspec(noinline) float Compare(float raw) {
    g_values[1] = g_values[1] + 1.0f;
    return raw < 1.0f ? 0.5f : 1.0f;
}
__declspec(noinline) void Exit() { g_values[2] = g_values[2] + 1.0f; }
__declspec(noinline) void Unused() { g_values[0] = g_values[0] + 2.0f; }

LONG CALLBACK Handler(EXCEPTION_POINTERS* pointers) {
    if (!pointers || !pointers->ExceptionRecord || !pointers->ContextRecord ||
        pointers->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
        GetCurrentThreadId() != g_target_tid.load(std::memory_order_acquire) ||
        (pointers->ContextRecord->Dr6 & 0xFull) == 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CONTEXT* context = pointers->ContextRecord;
    g_hits.fetch_add(1, std::memory_order_relaxed);
    for (unsigned int slot = 0; slot < 4; ++slot) {
        if (context->Dr6 & (1ull << slot)) g_slots[slot]++;
    }
    if (context->Dr6 & 1ull) {
        DWORD bits = static_cast<DWORD>(context->Xmm0.Low);
        float raw = 0.0f;
        float corrected = 0.0f;
        std::memcpy(&raw, &bits, sizeof(raw));
        if (NormalizeDistanceTest(raw, 1.0f / 165.0f, &corrected)) {
            std::memcpy(&bits, &corrected, sizeof(bits));
            context->Xmm0.Low =
                (context->Xmm0.Low & 0xFFFFFFFF00000000ull) | bits;
            g_patch_count++;
        }
    }
    if (context->Dr6 & 3ull) context->EFlags |= 0x10000u;
    context->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}

DWORD WINAPI TestThread(void*) {
    g_target_tid.store(GetCurrentThreadId(), std::memory_order_release);
    SetEvent(g_ready);
    if (WaitForSingleObject(g_go, 2000) != WAIT_OBJECT_0) return 1;
    Commit();
    g_result = Compare(g_raw);
    Exit();
    SetEvent(g_wrote);
    return WaitForSingleObject(g_exit, 2000) == WAIT_OBJECT_0 ? 0 : 2;
}

bool TestStateMachine() {
    PrototypeState state = PrototypeState::standby;
    PrototypeState next = state;
    if (NextPrototypeState(state, PrototypeAction::enable, &next) ||
        !NextPrototypeState(state, PrototypeAction::prepare, &next) ||
        next != PrototypeState::observe) return false;
    state = next;
    if (!NextPrototypeState(state, PrototypeAction::enable, &next) ||
        next != PrototypeState::correct) return false;
    state = next;
    if (NextPrototypeState(state, PrototypeAction::prepare, &next) ||
        !NextPrototypeState(state, PrototypeAction::disable, &next) ||
        next != PrototypeState::observe) return false;
    state = next;
    return NextPrototypeState(state, PrototypeAction::stop, &next) &&
           next == PrototypeState::stopping;
}

bool TestComparisonGuard() {
    ComparisonGuardInput input{};
    input.opcode[0] = 0x0F;
    input.opcode[1] = 0x2F;
    input.opcode[2] = 0xF8;
    input.previous = DistanceFrame{40, 1.0f / 165.0f, 123,
                                   {1.0f, 2.0f, 3.0f}, true};
    input.current = DistanceFrame{41, 1.0f / 165.0f, 123,
                                  {2.0f, 3.0f, 4.0f}, true};
    input.expected_index = 41;
    input.expected_transform = 123;
    std::memcpy(input.owner_position, input.current.position, 12);
    std::memcpy(input.getter_position, input.current.position, 12);
    std::memcpy(input.old_position, input.previous.position, 12);
    input.raw = 0.5f;
    ComparisonDecision decision{};
    if (!EvaluateComparison(input, &decision) ||
        std::fabs(decision.normalized - 1.375f) > 1e-6f ||
        !decision.native_reduces || decision.normalized_reduces) return false;
    input.opcode[0] = 0x90;
    if (EvaluateComparison(input, &decision)) return false;
    input.opcode[0] = 0x0F;
    input.expected_transform = 456;
    if (EvaluateComparison(input, &decision)) return false;
    input.expected_transform = 123;
    input.previous.delta = std::numeric_limits<float>::infinity();
    return !EvaluateComparison(input, &decision);
}

}  // namespace

int main() {
    if (!TestStateMachine()) {
        std::fprintf(stderr, "FAIL: control-state transitions.\n");
        return 1;
    }
    if (!TestComparisonGuard()) {
        std::fprintf(stderr, "FAIL: accepted comparison guard/formula.\n");
        return 2;
    }
    std::printf("PASS: standby/prepare/enable/disable/stop and guarded normalization.\n");

    g_ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_go = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_wrote = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_exit = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    PVOID veh = AddVectoredExceptionHandler(1, Handler);
    HANDLE thread = CreateThread(nullptr, 0, TestThread, nullptr, 0, nullptr);
    if (!g_ready || !g_go || !g_wrote || !g_exit || !veh || !thread ||
        WaitForSingleObject(g_ready, 2000) != WAIT_OBJECT_0) return 3;

    if (SuspendThread(thread) == static_cast<DWORD>(-1)) return 4;
    CONTEXT original{};
    original.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    CONTEXT armed{};
    armed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    bool ok = GetThreadContext(thread, &original) != FALSE;
    armed = original;
    armed.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    armed.Dr0 = reinterpret_cast<DWORD64>(&Compare);
    armed.Dr1 = reinterpret_cast<DWORD64>(&Exit);
    armed.Dr2 = reinterpret_cast<DWORD64>(&g_values[0]);
    armed.Dr3 = reinterpret_cast<DWORD64>(&Unused);
    armed.Dr6 = 0;
    armed.Dr7 = ConfigurePrototypeDr7(armed.Dr7);
    ok = ok && SetThreadContext(thread, &armed) != FALSE;
    ResumeThread(thread);
    if (!ok) return 5;

    SetEvent(g_go);
    if (WaitForSingleObject(g_wrote, 2000) != WAIT_OBJECT_0) return 6;
    if (SuspendThread(thread) == static_cast<DWORD>(-1)) return 7;
    CONTEXT current{};
    current.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ok = GetThreadContext(thread, &current) != FALSE;
    current.Dr0 = original.Dr0;
    current.Dr1 = original.Dr1;
    current.Dr2 = original.Dr2;
    current.Dr3 = original.Dr3;
    current.Dr6 = original.Dr6;
    current.Dr7 = original.Dr7;
    ok = ok && SetThreadContext(thread, &current) != FALSE;
    CONTEXT restored{};
    restored.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ok = ok && GetThreadContext(thread, &restored) != FALSE &&
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

    if (!ok || g_patch_count != 1 || g_result != 1.0f ||
        g_hits != 3 || g_slots[0] != 1 || g_slots[1] != 1 ||
        g_slots[2] != 1 || g_slots[3] != 0) {
        std::fprintf(stderr, "FAIL: hardware breakpoint patch/restoration.\n");
        return 8;
    }
    std::printf("PASS: live low-XMM0 correction, RF resume, commit watch and exact debug restoration.\n");
    return 0;
}
