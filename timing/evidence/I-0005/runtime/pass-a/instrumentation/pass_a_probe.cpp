#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include <atomic>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <cwchar>

#include "MinHook.h"

namespace {

constexpr uintptr_t kRvaD57900  = 0x00D57900;
constexpr uintptr_t kRva023410  = 0x00023410;
constexpr uintptr_t kRva0499C0  = 0x000499C0;
constexpr uintptr_t kRvaD5E30   = 0x000D5E30;
constexpr uintptr_t kRva192CF00 = 0x0192CF00;
constexpr uintptr_t kRvaDD4B60  = 0x00DD4B60;
constexpr uintptr_t kRvaED8E50  = 0x00ED8E50;
constexpr uintptr_t kRvaEDAAE0  = 0x00EDAAE0;
constexpr uintptr_t kRvaCentralGlobal = 0x0489DD10;

HMODULE g_self = nullptr;
uintptr_t g_image = 0;
LARGE_INTEGER g_frequency{};
std::atomic<unsigned long long> g_sequence{0};
std::atomic<bool> g_stopped{false};
CRITICAL_SECTION g_log_lock{};

FILE* g_calls = nullptr;
FILE* g_callbacks = nullptr;
FILE* g_central = nullptr;
FILE* g_selector = nullptr;
FILE* g_pacing = nullptr;
FILE* g_present = nullptr;
FILE* g_hooks = nullptr;
wchar_t g_session_dir[MAX_PATH * 4]{};

HANDLE g_stop_event = nullptr;
HANDLE g_ready_event = nullptr;
HANDLE g_complete_event = nullptr;

using FnD57900 = void(__fastcall*)(void*, void*);
using FnFloat = void(__fastcall*)(void*, float);
using FnD5E30 = unsigned long long(__fastcall*)(void*, unsigned int, uintptr_t, uintptr_t);
using Fn192CF00 = long long(__fastcall*)(void*, unsigned int);
using FnDD4B60 = void(__fastcall*)(void*, void*);
using FnED8E50 = void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
using FnEDAAE0 = bool(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

FnD57900 g_orig_d57900 = nullptr;
FnFloat g_orig_023410 = nullptr;
FnFloat g_orig_0499c0 = nullptr;
FnD5E30 g_orig_d5e30 = nullptr;
Fn192CF00 g_orig_192cf00 = nullptr;
FnDD4B60 g_orig_dd4b60 = nullptr;
FnED8E50 g_orig_ed8e50 = nullptr;
FnEDAAE0 g_orig_edaae0 = nullptr;

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

void FlushAll() {
    FILE* files[] = {g_calls, g_callbacks, g_central, g_selector, g_pacing, g_present, g_hooks};
    for (FILE* file : files) {
        if (file) std::fflush(file);
    }
}

FILE* OpenCsv(const wchar_t* name, const char* header) {
    wchar_t path[MAX_PATH * 4]{};
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\%s", g_session_dir, name);
    FILE* file = nullptr;
    _wfopen_s(&file, path, L"wb");
    if (file) {
        static char buffers[7][1 << 20];
        static LONG buffer_index = -1;
        LONG index = InterlockedIncrement(&buffer_index);
        if (index >= 0 && index < 7) std::setvbuf(file, buffers[index], _IOFBF, sizeof(buffers[index]));
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
    std::fwprintf(file, L"schema=I-0005-pass-a-v1\n");
    std::fwprintf(file, L"pid=%lu\n", GetCurrentProcessId());
    std::fwprintf(file, L"exe=%s\n", exe);
    std::fwprintf(file, L"image_base=0x%016llX\n", static_cast<unsigned long long>(g_image));
    std::fwprintf(file, L"qpc_frequency=%lld\n", g_frequency.QuadPart);
    std::fwprintf(file, L"probe_mode=observation_only\n");
    std::fwprintf(file, L"timing_state_writes=none\n");
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

void __fastcall HookD57900(void* self, void* context) {
    float delta = 0.0f;
    bool valid = context && SafeRead(reinterpret_cast<uintptr_t>(context) + 0x08, &delta);
    if (g_calls) {
        EnterCriticalSection(&g_log_lock);
        std::fprintf(g_calls, "%llu,%lld,%lu,D57900,0x%016llX,0x%016llX,%.9g,%u\n",
                     NextSequence(), Qpc(), GetCurrentThreadId(),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(self)),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(context)),
                     delta, valid ? 1u : 0u);
        LeaveCriticalSection(&g_log_lock);
    }
    g_orig_d57900(self, context);
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
    if (g_callbacks) {
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
    unsigned long long result = g_orig_d5e30(state, requested, a3, a4);
    long long end = Qpc();
    long long deadline_after = 0;
    unsigned int counter_after = 0;
    if (state) {
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x18, &deadline_after);
        SafeRead(reinterpret_cast<uintptr_t>(state) + 0x20, &counter_after);
    }
    if (g_pacing) {
        EnterCriticalSection(&g_log_lock);
        std::fprintf(g_pacing,
                     "%llu,%lld,%lld,%lu,0x%016llX,%u,%u,%u,%lld,%u,%lld,%u,%llu\n",
                     NextSequence(), begin, end, GetCurrentThreadId(),
                     static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(state)),
                     requested, active, mode, deadline_before, counter_before, deadline_after,
                     counter_after, result);
        LeaveCriticalSection(&g_log_lock);
    }
    return result;
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
    g_pacing = OpenCsv(L"presentation-pacing.csv", "seq,qpc_before,qpc_after,tid,state,requested,active,mode,stamp_before,count_before,stamp_after,count_after,returned_sync_interval");
    g_present = OpenCsv(L"present.csv", "seq,qpc_before,qpc_after,tid,state,swap_chain,sync_interval,result");
    g_hooks = OpenCsv(L"hook-status.csv", "name,rva,address,create_status,create_text,enable_status,enable_text");
    return g_calls && g_callbacks && g_central && g_selector && g_pacing && g_present && g_hooks;
}

void CloseLogs() {
    FILE** files[] = {&g_calls, &g_callbacks, &g_central, &g_selector, &g_pacing, &g_present, &g_hooks};
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

DWORD WINAPI ProbeThread(void*) {
    g_image = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    QueryPerformanceFrequency(&g_frequency);
    InitializeCriticalSection(&g_log_lock);
    BuildSessionDirectory();
    WriteSessionInfo();
    bool ok = OpenLogs();
    SnapshotModules(L"modules-start.csv");

    wchar_t stop_name[128]{}, ready_name[128]{}, complete_name[128]{};
    _snwprintf_s(stop_name, _countof(stop_name), _TRUNCATE, L"Local\\DS3_I0005_PASSA_STOP_%lu", GetCurrentProcessId());
    _snwprintf_s(ready_name, _countof(ready_name), _TRUNCATE, L"Local\\DS3_I0005_PASSA_READY_%lu", GetCurrentProcessId());
    _snwprintf_s(complete_name, _countof(complete_name), _TRUNCATE, L"Local\\DS3_I0005_PASSA_COMPLETE_%lu", GetCurrentProcessId());
    g_stop_event = CreateEventW(nullptr, TRUE, FALSE, stop_name);
    g_ready_event = CreateEventW(nullptr, TRUE, FALSE, ready_name);
    g_complete_event = CreateEventW(nullptr, TRUE, FALSE, complete_name);

    MH_STATUS init = MH_Initialize();
    if (g_hooks) {
        std::fprintf(g_hooks, "MH_Initialize,0x0,0x0,%d,%s,%d,%s\n", static_cast<int>(init),
                     MH_StatusToString(init), static_cast<int>(init), MH_StatusToString(init));
    }
    ok = ok && init == MH_OK;
    if (init == MH_OK) {
        ok = InstallHook("D57900", kRvaD57900, reinterpret_cast<void*>(&HookD57900), &g_orig_d57900) && ok;
        ok = InstallHook("023410", kRva023410, reinterpret_cast<void*>(&Hook023410), &g_orig_023410) && ok;
        ok = InstallHook("0499C0", kRva0499C0, reinterpret_cast<void*>(&Hook0499C0), &g_orig_0499c0) && ok;
        ok = InstallHook("D5E30", kRvaD5E30, reinterpret_cast<void*>(&HookD5E30), &g_orig_d5e30) && ok;
        ok = InstallHook("192CF00", kRva192CF00, reinterpret_cast<void*>(&Hook192CF00), &g_orig_192cf00) && ok;
        ok = InstallHook("DD4B60", kRvaDD4B60, reinterpret_cast<void*>(&HookDD4B60), &g_orig_dd4b60) && ok;
        ok = InstallHook("ED8E50", kRvaED8E50, reinterpret_cast<void*>(&HookED8E50), &g_orig_ed8e50) && ok;
        ok = InstallHook("EDAAE0", kRvaEDAAE0, reinterpret_cast<void*>(&HookEDAAE0), &g_orig_edaae0) && ok;
    }
    FlushAll();
    WriteMarker(ok ? L"READY" : L"FAILED", ok ? "all hooks enabled" : "one or more hooks failed");
    if (g_ready_event) SetEvent(g_ready_event);

    while (g_stop_event && WaitForSingleObject(g_stop_event, 1000) == WAIT_TIMEOUT) {
        EnterCriticalSection(&g_log_lock);
        FlushAll();
        LeaveCriticalSection(&g_log_lock);
    }

    if (init == MH_OK) {
        MH_DisableHook(MH_ALL_HOOKS);
        Sleep(100);
    }
    g_stopped.store(true, std::memory_order_release);
    EnterCriticalSection(&g_log_lock);
    FlushAll();
    LeaveCriticalSection(&g_log_lock);
    SnapshotModules(L"modules-stop.csv");
    WriteMarker(L"COMPLETE", "hooks disabled and logs flushed");
    if (g_complete_event) SetEvent(g_complete_event);
    CloseLogs();
    if (init == MH_OK) MH_Uninitialize();
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
