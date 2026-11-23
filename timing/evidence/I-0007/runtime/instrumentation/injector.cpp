#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdlib>
#include <cstdio>
#include <cwchar>

namespace {

void PrintLastError(const wchar_t* operation) {
    std::fwprintf(stderr, L"%s failed: Win32 error %lu\n", operation, GetLastError());
}

int Inject(DWORD pid, const wchar_t* dll_path) {
    wchar_t absolute[MAX_PATH * 4]{};
    if (!GetFullPathNameW(dll_path, _countof(absolute), absolute, nullptr)) {
        PrintLastError(L"GetFullPathNameW");
        return 2;
    }
    DWORD attributes = GetFileAttributesW(absolute);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        std::fwprintf(stderr, L"DLL not found: %s\n", absolute);
        return 2;
    }

    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                                 FALSE, pid);
    if (!process) {
        PrintLastError(L"OpenProcess");
        return 3;
    }
    const SIZE_T bytes = (std::wcslen(absolute) + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        PrintLastError(L"VirtualAllocEx");
        CloseHandle(process);
        return 4;
    }
    SIZE_T written = 0;
    if (!WriteProcessMemory(process, remote, absolute, bytes, &written) || written != bytes) {
        PrintLastError(L"WriteProcessMemory");
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return 5;
    }
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    auto load_library = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel32, "LoadLibraryW"));
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, load_library, remote, 0, nullptr);
    if (!thread) {
        PrintLastError(L"CreateRemoteThread");
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return 6;
    }
    DWORD wait = WaitForSingleObject(thread, 15000);
    DWORD module_result = 0;
    GetExitCodeThread(thread, &module_result);
    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);
    if (wait != WAIT_OBJECT_0 || module_result == 0) {
        std::fwprintf(stderr, L"LoadLibraryW did not complete successfully (wait=%lu, result=0x%08lX)\n",
                      wait, module_result);
        return 7;
    }

    wchar_t ready_name[128]{};
    _snwprintf_s(ready_name, _countof(ready_name), _TRUNCATE,
                 L"Local\\DS3_I0007_CANDIDATE_WATCH_READY_%lu", pid);
    HANDLE ready = nullptr;
    for (int i = 0; i < 100 && !ready; ++i) {
        ready = OpenEventW(SYNCHRONIZE, FALSE, ready_name);
        if (!ready) Sleep(100);
    }
    if (!ready) {
        PrintLastError(L"OpenEventW(ready)");
        return 8;
    }
    wait = WaitForSingleObject(ready, 20000);
    CloseHandle(ready);
    if (wait != WAIT_OBJECT_0) {
        std::fwprintf(stderr, L"Probe ready wait failed: %lu\n", wait);
        return 9;
    }
    std::wprintf(L"Injected probe into PID %lu and received ready signal.\n", pid);
    return 0;
}

int Stop(DWORD pid) {
    wchar_t stop_name[128]{}, complete_name[128]{};
    _snwprintf_s(stop_name, _countof(stop_name), _TRUNCATE,
                 L"Local\\DS3_I0007_CANDIDATE_WATCH_STOP_%lu", pid);
    _snwprintf_s(complete_name, _countof(complete_name), _TRUNCATE,
                 L"Local\\DS3_I0007_CANDIDATE_WATCH_COMPLETE_%lu", pid);
    HANDLE stop = OpenEventW(EVENT_MODIFY_STATE, FALSE, stop_name);
    HANDLE complete = OpenEventW(SYNCHRONIZE, FALSE, complete_name);
    if (!stop || !complete) {
        PrintLastError(L"OpenEventW(stop/complete)");
        if (stop) CloseHandle(stop);
        if (complete) CloseHandle(complete);
        return 10;
    }
    SetEvent(stop);
    DWORD wait = WaitForSingleObject(complete, 20000);
    CloseHandle(stop);
    CloseHandle(complete);
    if (wait != WAIT_OBJECT_0) {
        std::fwprintf(stderr, L"Probe completion wait failed: %lu\n", wait);
        return 11;
    }
    std::wprintf(L"Probe stopped and flushed for PID %lu.\n", pid);
    return 0;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        std::fwprintf(stderr, L"Usage: I0007CandidateWatchInjector.exe inject <pid> <dll> | stop <pid>\n");
        return 1;
    }
    DWORD pid = wcstoul(argv[2], nullptr, 10);
    if (pid == 0) {
        std::fwprintf(stderr, L"Invalid PID.\n");
        return 1;
    }
    if (_wcsicmp(argv[1], L"inject") == 0 && argc == 4) return Inject(pid, argv[3]);
    if (_wcsicmp(argv[1], L"stop") == 0 && argc == 3) return Stop(pid);
    std::fwprintf(stderr, L"Invalid arguments.\n");
    return 1;
}
