#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cwchar>

#include "runtime_core.hpp"
#include "public_status.hpp"

namespace {

void PrintLastError(const wchar_t* operation) {
    std::fwprintf(stderr, L"%s failed: Win32 error %lu\n", operation,
                  GetLastError());
}

void EventName(wchar_t* output, size_t count, const wchar_t* action, DWORD pid) {
    _snwprintf_s(output, count, _TRUNCATE, L"Local\\DS3_I0011_%s_%lu",
                 action, pid);
}

bool PrintStatus(DWORD pid) {
    wchar_t name[128]{};
    EventName(name, _countof(name), L"STATUS_MAP", pid);
    HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, name);
    if (!mapping) {
        PrintLastError(L"OpenFileMappingW(status)");
        return false;
    }
    const auto* status = static_cast<const RuntimePublicStatus*>(
        MapViewOfFile(mapping, FILE_MAP_READ, 0, 0,
                      sizeof(RuntimePublicStatus)));
    if (!status) {
        PrintLastError(L"MapViewOfFile(status)");
        CloseHandle(mapping);
        return false;
    }
    const bool valid = status->magic == kRuntimeStatusMagic &&
                       status->version == kRuntimeStatusVersion;
    if (valid) {
        const auto state = static_cast<RuntimeState>(status->state);
        const auto mode = static_cast<CadenceMode>(status->cadence_mode);
        const char* target = mode == CadenceMode::numeric ? "numeric" :
            mode == CadenceMode::unlimited ? "unlimited" : "native";
        std::printf(
            "state=%s mode=%s target=%.0f actual=%.2f fix=%s overlay=%s "
            "control=%u restored=%u debug_restored=%u failed=%u "
            "updates=%llu comparisons=%llu corrections=%llu guards=%llu "
            "cadence_transitions=%llu fix_transitions=%llu\n",
            RuntimeStateName(state), target,
            static_cast<double>(status->target_fps),
            static_cast<double>(status->actual_fps),
            status->correction_active ? "ON" : "OFF",
            status->overlay_visible ? "OPEN" : "CLOSED",
            status->last_control_result, status->timing_restored,
            status->debug_restored, status->guard_failed,
            static_cast<unsigned long long>(status->updates),
            static_cast<unsigned long long>(status->comparisons),
            static_cast<unsigned long long>(status->corrections),
            static_cast<unsigned long long>(status->guard_failures),
            static_cast<unsigned long long>(status->cadence_transitions),
            static_cast<unsigned long long>(status->correction_transitions));
    }
    UnmapViewOfFile(status);
    CloseHandle(mapping);
    return valid;
}

int Inject(DWORD pid, const wchar_t* dll_path) {
    wchar_t existing_name[128]{};
    EventName(existing_name, _countof(existing_name), L"STATUS_MAP", pid);
    HANDLE existing = OpenFileMappingW(FILE_MAP_READ, FALSE, existing_name);
    if (existing) {
        CloseHandle(existing);
        std::fprintf(stderr,
            "I-0011 runtime is already loaded in PID %lu; refusing reinjection.\n",
            pid);
        return 16;
    }
    wchar_t absolute[MAX_PATH * 4]{};
    if (!GetFullPathNameW(dll_path, _countof(absolute), absolute, nullptr)) {
        PrintLastError(L"GetFullPathNameW");
        return 2;
    }
    const DWORD attributes = GetFileAttributesW(absolute);
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        std::fwprintf(stderr, L"DLL not found: %s\n", absolute);
        return 2;
    }
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!process) {
        PrintLastError(L"OpenProcess");
        return 3;
    }
    const SIZE_T bytes = (std::wcslen(absolute) + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes,
                                  MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        PrintLastError(L"VirtualAllocEx");
        CloseHandle(process);
        return 4;
    }
    SIZE_T written = 0;
    if (!WriteProcessMemory(process, remote, absolute, bytes, &written) ||
        written != bytes) {
        PrintLastError(L"WriteProcessMemory");
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return 5;
    }
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    auto load_library = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(kernel32, "LoadLibraryW"));
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, load_library,
                                       remote, 0, nullptr);
    if (!thread) {
        PrintLastError(L"CreateRemoteThread");
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return 6;
    }
    const DWORD wait = WaitForSingleObject(thread, 15000);
    DWORD module_result = 0;
    GetExitCodeThread(thread, &module_result);
    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);
    if (wait != WAIT_OBJECT_0 || module_result == 0) {
        std::fwprintf(stderr,
                      L"LoadLibraryW did not complete (wait=%lu result=0x%08lX).\n",
                      wait, module_result);
        return 7;
    }
    wchar_t ready_name[128]{};
    EventName(ready_name, _countof(ready_name), L"READY", pid);
    HANDLE ready = nullptr;
    for (int attempt = 0; attempt < 100 && !ready; ++attempt) {
        ready = OpenEventW(SYNCHRONIZE, FALSE, ready_name);
        if (!ready) Sleep(100);
    }
    if (!ready || WaitForSingleObject(ready, 20000) != WAIT_OBJECT_0) {
        if (ready) CloseHandle(ready);
        PrintLastError(L"READY wait");
        return 8;
    }
    CloseHandle(ready);
    std::puts("I-0011 loaded in STANDBY; timing, correction and overlay remain off.");
    return PrintStatus(pid) ? 0 : 9;
}

int Control(DWORD pid, const wchar_t* action) {
    wchar_t request_name[128]{}, ack_name[128]{};
    EventName(request_name, _countof(request_name), action, pid);
    EventName(ack_name, _countof(ack_name), L"ACK", pid);
    HANDLE request = OpenEventW(EVENT_MODIFY_STATE, FALSE, request_name);
    HANDLE ack = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, ack_name);
    if (!request || !ack) {
        PrintLastError(L"OpenEventW(control)");
        if (request) CloseHandle(request);
        if (ack) CloseHandle(ack);
        return 10;
    }
    ResetEvent(ack);
    SetEvent(request);
    const DWORD result = WaitForSingleObject(ack, 20000);
    CloseHandle(request);
    CloseHandle(ack);
    if (result != WAIT_OBJECT_0) return 11;
    return PrintStatus(pid) ? 0 : 12;
}

int Stop(DWORD pid) {
    wchar_t stop_name[128]{}, complete_name[128]{};
    EventName(stop_name, _countof(stop_name), L"STOP", pid);
    EventName(complete_name, _countof(complete_name), L"COMPLETE", pid);
    HANDLE stop = OpenEventW(EVENT_MODIFY_STATE, FALSE, stop_name);
    HANDLE complete = OpenEventW(SYNCHRONIZE, FALSE, complete_name);
    if (!stop || !complete) {
        PrintLastError(L"OpenEventW(stop/complete)");
        if (stop) CloseHandle(stop);
        if (complete) CloseHandle(complete);
        return 13;
    }
    SetEvent(stop);
    const DWORD result = WaitForSingleObject(complete, 20000);
    CloseHandle(stop);
    CloseHandle(complete);
    if (result != WAIT_OBJECT_0) return 14;
    return PrintStatus(pid) ? 0 : 15;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        std::fwprintf(stderr,
            L"Usage: I0011RuntimeController.exe inject <pid> <dll> | "
            L"show|hide|fix-on|fix-off|native|unlimited|status|stop <pid>\n");
        return 1;
    }
    const DWORD pid = wcstoul(argv[2], nullptr, 10);
    if (pid == 0) return 1;
    if (_wcsicmp(argv[1], L"inject") == 0 && argc == 4) {
        return Inject(pid, argv[3]);
    }
    struct Command {
        const wchar_t* argument;
        const wchar_t* event;
    };
    constexpr Command commands[] = {
        {L"show", L"SHOW"}, {L"hide", L"HIDE"},
        {L"fix-on", L"FIX_ON"}, {L"fix-off", L"FIX_OFF"},
        {L"native", L"NATIVE"}, {L"unlimited", L"UNLIMITED"},
        {L"status", L"STATUS"},
    };
    for (const Command& command : commands) {
        if (_wcsicmp(argv[1], command.argument) == 0 && argc == 3) {
            return Control(pid, command.event);
        }
    }
    if (_wcsicmp(argv[1], L"stop") == 0 && argc == 3) {
        return Stop(pid);
    }
    std::fwprintf(stderr, L"Invalid arguments.\n");
    return 1;
}
