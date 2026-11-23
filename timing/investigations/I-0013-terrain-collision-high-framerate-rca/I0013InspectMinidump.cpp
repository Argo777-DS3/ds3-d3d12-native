#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

struct ModuleRecord {
    uint64_t base = 0;
    uint64_t size = 0;
    std::wstring name;
};

struct DumpView {
    const unsigned char* base = nullptr;
    MINIDUMP_MEMORY64_LIST* memory64 = nullptr;
    MINIDUMP_MEMORY_LIST* memory = nullptr;

    const void* read(uint64_t address, size_t size) const {
        if (memory64) {
            uint64_t rva = memory64->BaseRva;
            for (ULONG64 index = 0; index < memory64->NumberOfMemoryRanges; ++index) {
                const auto& range = memory64->MemoryRanges[index];
                const uint64_t start = range.StartOfMemoryRange;
                const uint64_t bytes = range.DataSize;
                if (address >= start && address - start <= bytes &&
                    size <= bytes - (address - start)) {
                    return base + rva + (address - start);
                }
                rva += bytes;
            }
        }
        if (memory) {
            for (ULONG32 index = 0; index < memory->NumberOfMemoryRanges; ++index) {
                const auto& range = memory->MemoryRanges[index];
                const uint64_t start = range.StartOfMemoryRange;
                const uint64_t bytes = range.Memory.DataSize;
                if (address >= start && address - start <= bytes &&
                    size <= bytes - (address - start)) {
                    return base + range.Memory.Rva + (address - start);
                }
            }
        }
        return nullptr;
    }
};

template <typename T>
T* stream(void* dump, MINIDUMP_STREAM_TYPE type) {
    PMINIDUMP_DIRECTORY directory = nullptr;
    void* pointer = nullptr;
    ULONG size = 0;
    if (!MiniDumpReadDumpStream(dump, type, &directory, &pointer, &size)) return nullptr;
    return static_cast<T*>(pointer);
}

std::wstring dumpString(const unsigned char* base, RVA rva) {
    if (!rva) return L"";
    const auto* value = reinterpret_cast<const MINIDUMP_STRING*>(base + rva);
    return std::wstring(value->Buffer, value->Length / sizeof(wchar_t));
}

const ModuleRecord* moduleFor(const std::vector<ModuleRecord>& modules, uint64_t address) {
    for (const auto& module : modules) {
        if (address >= module.base && address - module.base < module.size) return &module;
    }
    return nullptr;
}

void printAddress(const std::vector<ModuleRecord>& modules, uint64_t gameBase,
                  uint64_t address);

void dumpQwords(const DumpView& view, const std::vector<ModuleRecord>& modules,
                uint64_t gameBase, const wchar_t* label, uint64_t address,
                unsigned count) {
    std::wprintf(L"memory_%ls address=0x%016llX:\n", label, address);
    for (unsigned index = 0; index < count; ++index) {
        const uint64_t slot = address + index * sizeof(uint64_t);
        const auto* value = static_cast<const uint64_t*>(view.read(slot, sizeof(uint64_t)));
        if (!value) {
            std::wprintf(L"  [0x%03X] unavailable\n", index * 8);
            break;
        }
        std::wprintf(L"  [0x%03X] 0x%016llX", index * 8, *value);
        if (moduleFor(modules, *value)) {
            std::wprintf(L" -> ");
            printAddress(modules, gameBase, *value);
        }
        std::wprintf(L"\n");
    }
}

void printAddress(const std::vector<ModuleRecord>& modules, uint64_t gameBase,
                  uint64_t address) {
    if (const auto* module = moduleFor(modules, address)) {
        std::wprintf(L"0x%016llX %ls+0x%llX", address, module->name.c_str(),
                     address - module->base);
    } else if (gameBase) {
        std::wprintf(L"0x%016llX unmapped game_delta=%s0x%llX", address,
                     address >= gameBase ? L"+" : L"-",
                     address >= gameBase ? address - gameBase : gameBase - address);
    } else {
        std::wprintf(L"0x%016llX unmapped", address);
    }
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) {
        std::fwprintf(stderr, L"usage: I0013InspectMinidump.exe DUMP\n");
        return 1;
    }
    HANDLE file = CreateFileW(argv[1], GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 2;
    HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping) { CloseHandle(file); return 3; }
    void* mapped = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!mapped) { CloseHandle(mapping); CloseHandle(file); return 4; }

    const auto* base = static_cast<const unsigned char*>(mapped);
    auto* exception = stream<MINIDUMP_EXCEPTION_STREAM>(mapped, ExceptionStream);
    auto* moduleList = stream<MINIDUMP_MODULE_LIST>(mapped, ModuleListStream);
    auto* threadList = stream<MINIDUMP_THREAD_LIST>(mapped, ThreadListStream);
    DumpView view{base, stream<MINIDUMP_MEMORY64_LIST>(mapped, Memory64ListStream),
                  stream<MINIDUMP_MEMORY_LIST>(mapped, MemoryListStream)};
    if (!exception || !moduleList || !threadList) {
        std::fwprintf(stderr, L"required minidump stream missing\n");
        UnmapViewOfFile(mapped); CloseHandle(mapping); CloseHandle(file); return 5;
    }

    std::vector<ModuleRecord> modules;
    uint64_t gameBase = 0;
    for (ULONG32 index = 0; index < moduleList->NumberOfModules; ++index) {
        const auto& input = moduleList->Modules[index];
        ModuleRecord record{input.BaseOfImage, input.SizeOfImage,
                            dumpString(base, input.ModuleNameRva)};
        if (record.name.find(L"DarkSoulsIII.exe") != std::wstring::npos) gameBase = record.base;
        modules.push_back(std::move(record));
    }

    std::wprintf(L"thread_id=%lu\n", exception->ThreadId);
    std::wprintf(L"exception_code=0x%08lX\n", exception->ExceptionRecord.ExceptionCode);
    std::wprintf(L"exception_flags=0x%08lX\n", exception->ExceptionRecord.ExceptionFlags);
    std::wprintf(L"exception_address=");
    printAddress(modules, gameBase, exception->ExceptionRecord.ExceptionAddress);
    std::wprintf(L"\nexception_parameters=%lu", exception->ExceptionRecord.NumberParameters);
    for (ULONG32 index = 0; index < exception->ExceptionRecord.NumberParameters; ++index) {
        std::wprintf(L" 0x%llX", exception->ExceptionRecord.ExceptionInformation[index]);
    }
    std::wprintf(L"\ngame_base=0x%016llX\n", gameBase);

    if (exception->ThreadContext.DataSize < sizeof(CONTEXT)) {
        std::fwprintf(stderr, L"x64 CONTEXT unavailable (%lu bytes)\n",
                      exception->ThreadContext.DataSize);
        UnmapViewOfFile(mapped); CloseHandle(mapping); CloseHandle(file); return 6;
    }
    const auto* context = reinterpret_cast<const CONTEXT*>(
        base + exception->ThreadContext.Rva);
    struct NamedRegister { const wchar_t* name; uint64_t value; };
    const NamedRegister registers[] = {
        {L"RIP", context->Rip}, {L"RSP", context->Rsp}, {L"RBP", context->Rbp},
        {L"RAX", context->Rax}, {L"RBX", context->Rbx}, {L"RCX", context->Rcx},
        {L"RDX", context->Rdx}, {L"RSI", context->Rsi}, {L"RDI", context->Rdi},
        {L"R8", context->R8}, {L"R9", context->R9}, {L"R10", context->R10},
        {L"R11", context->R11}, {L"R12", context->R12}, {L"R13", context->R13},
        {L"R14", context->R14}, {L"R15", context->R15},
        {L"DR0", context->Dr0}, {L"DR1", context->Dr1}, {L"DR2", context->Dr2},
        {L"DR3", context->Dr3}, {L"DR6", context->Dr6}, {L"DR7", context->Dr7},
    };
    std::wprintf(L"registers:\n");
    for (const auto& reg : registers) {
        std::wprintf(L"  %ls=", reg.name);
        printAddress(modules, gameBase, reg.value);
        std::wprintf(L"\n");
    }
    std::wprintf(L"  EFLAGS=0x%08lX MXCSR=0x%08lX\n", context->EFlags, context->MxCsr);
    dumpQwords(view, modules, gameBase, L"rbx", context->Rbx, 12);
    dumpQwords(view, modules, gameBase, L"rdi", context->Rdi, 12);
    dumpQwords(view, modules, gameBase, L"r13", context->R13, 12);
    uint64_t stackTarget = 0;
    if (const auto* value = static_cast<const uint64_t*>(
            view.read(context->Rsp, sizeof(uint64_t)))) stackTarget = *value;
    dumpQwords(view, modules, gameBase, L"stack_target", stackTarget, 12);
    dumpQwords(view, modules, gameBase, L"valid_ddb120", gameBase + 0x00DDB120, 8);
    dumpQwords(view, modules, gameBase, L"ddb120_data_ref", gameBase + 0x04A41580, 8);

    const MINIDUMP_THREAD* exceptionThread = nullptr;
    for (ULONG32 index = 0; index < threadList->NumberOfThreads; ++index) {
        if (threadList->Threads[index].ThreadId == exception->ThreadId) {
            exceptionThread = &threadList->Threads[index];
            break;
        }
    }
    if (exceptionThread) {
        std::wprintf(L"thread_stack_start=0x%016llX size=0x%lX\n",
                     exceptionThread->Stack.StartOfMemoryRange,
                     exceptionThread->Stack.Memory.DataSize);
    }

    std::wprintf(L"stack_qwords:\n");
    for (unsigned index = 0; index < 160; ++index) {
        const uint64_t address = context->Rsp + index * sizeof(uint64_t);
        const auto* value = static_cast<const uint64_t*>(view.read(address, sizeof(uint64_t)));
        if (!value) break;
        const auto* mappedModule = moduleFor(modules, *value);
        const bool pointerLike = mappedModule ||
            (*value >= 0x0000000100000000ull && *value <= 0x00007fffffffffffull);
        if (!pointerLike && index >= 32) continue;
        std::wprintf(L"  [rsp+0x%03X] 0x%016llX -> ", index * 8, *value);
        printAddress(modules, gameBase, *value);
        std::wprintf(L"\n");
    }

    UnmapViewOfFile(mapped);
    CloseHandle(mapping);
    CloseHandle(file);
    return 0;
}
