// I-0082: the minidump and the stack report of a native fault (crash_dump.h).
#include "crash_dump.h"
#include <dbghelp.h>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace ds3native::crash {
namespace {
struct Handle {HANDLE h{};~Handle(){if(h)CloseHandle(h);}};
// Every module's data segments are left out except the one named (DarkSoulsIII.exe's alone are tens of megabytes).
BOOL CALLBACK KeepDataOf(PVOID param,const PMINIDUMP_CALLBACK_INPUT input,PMINIDUMP_CALLBACK_OUTPUT output) {
    if(!input || !output)return TRUE;
    if(input->CallbackType==ModuleCallback && param){
        const wchar_t* path=input->Module.FullPath;const wchar_t* name=path?std::wcsrchr(path,L'\\'):nullptr;name=name?name+1:path;
        if(!name || _wcsicmp(name,static_cast<const wchar_t*>(param)))output->ModuleWriteFlags&=~ModuleWriteDataSeg;}
    return TRUE;
}
bool Read(HANDLE process,uint64_t address,void* out,size_t bytes) noexcept {
    SIZE_T read{};return address && ReadProcessMemory(process,reinterpret_cast<const void*>(address),out,bytes,&read) && read==bytes;
}
std::string Hex(uint64_t v){char text[24]{};sprintf_s(text,"%llX",static_cast<unsigned long long>(v));return text;}
}

bool WriteDump(HANDLE process,DWORD pid,DWORD thread,uint64_t exception,const wchar_t* path,const wchar_t* keep_module,bool with_process_data) noexcept {
    Handle file{CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr)};
    if(file.h==INVALID_HANDLE_VALUE){file.h=nullptr;return false;}
    MINIDUMP_EXCEPTION_INFORMATION fault{};fault.ThreadId=thread;
    fault.ExceptionPointers=reinterpret_cast<PEXCEPTION_POINTERS>(exception);fault.ClientPointers=TRUE;   // the record lives in `process`
    MINIDUMP_CALLBACK_INFORMATION callback{&KeepDataOf,const_cast<wchar_t*>(keep_module)};
    const auto type=static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs|MiniDumpWithIndirectlyReferencedMemory|MiniDumpWithThreadInfo|
        MiniDumpWithUnloadedModules|MiniDumpWithFullMemoryInfo|(with_process_data?MiniDumpWithProcessThreadData:0));
    const bool written=MiniDumpWriteDump(process,pid,file.h,type,exception?&fault:nullptr,nullptr,&callback)!=FALSE;
    CloseHandle(file.h);file.h=nullptr;
    if(!written)DeleteFileW(path);
    return written;
}

bool ExceptionHead(HANDLE process,uint64_t exception,uint32_t& code,uint64_t& address) noexcept {
    EXCEPTION_POINTERS pointers{};EXCEPTION_RECORD record{};
    if(!Read(process,exception,&pointers,sizeof pointers) || !Read(process,reinterpret_cast<uint64_t>(pointers.ExceptionRecord),&record,sizeof record))return false;
    code=record.ExceptionCode;address=reinterpret_cast<uint64_t>(record.ExceptionAddress);return true;
}

std::string StackReport(HANDLE process,DWORD thread,uint64_t exception,const wchar_t* symbol_path,unsigned max_frames) {
    Handle target{OpenThread(THREAD_GET_CONTEXT|THREAD_SUSPEND_RESUME|THREAD_QUERY_INFORMATION,FALSE,thread)};
    if(!target.h)return "  (thread not opened)\n";
    alignas(16) CONTEXT context{};bool suspended=false;
    if(exception){EXCEPTION_POINTERS pointers{};
        if(!Read(process,exception,&pointers,sizeof pointers) || !Read(process,reinterpret_cast<uint64_t>(pointers.ContextRecord),&context,sizeof context))
            return "  (exception context unreadable)\n";}
    else{if(SuspendThread(target.h)==static_cast<DWORD>(-1))return "  (thread not suspended)\n";suspended=true;
        context.ContextFlags=CONTEXT_FULL;
        if(!GetThreadContext(target.h,&context)){ResumeThread(target.h);return "  (thread context unreadable)\n";}}
    SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_UNDNAME|SYMOPT_LOAD_LINES|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_NO_PROMPTS);
    std::string text;
    if(!SymInitializeW(process,symbol_path?symbol_path:L".",TRUE)){if(suspended)ResumeThread(target.h);return "  (symbols unavailable)\n";}
    STACKFRAME64 frame{};frame.AddrPC.Offset=context.Rip;frame.AddrPC.Mode=AddrModeFlat;frame.AddrStack.Offset=context.Rsp;
    frame.AddrStack.Mode=AddrModeFlat;frame.AddrFrame.Offset=context.Rbp;frame.AddrFrame.Mode=AddrModeFlat;
    for(unsigned n=0;n<max_frames;++n){
        if(!StackWalk64(IMAGE_FILE_MACHINE_AMD64,process,target.h,&frame,&context,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr) ||
           !frame.AddrPC.Offset)break;
        const uint64_t pc=frame.AddrPC.Offset;
        // A return address points after its call: name the call (pc-1) for every frame but the innermost.
        const uint64_t at=n?pc-1:pc;
        char line[1024]{};int used=sprintf_s(line,"  #%02u ",n);
        IMAGEHLP_MODULEW64 module{};module.SizeOfStruct=sizeof module;
        char name[MAX_PATH]{};uint64_t base{};
        if(SymGetModuleInfoW64(process,at,&module)){base=module.BaseOfImage;
            const wchar_t* file=std::wcsrchr(module.ImageName,L'\\');file=file?file+1:module.ImageName;
            WideCharToMultiByte(CP_UTF8,0,file,-1,name,sizeof name-1,nullptr,nullptr);
            used+=sprintf_s(line+used,sizeof line-used,"%s+0x%llX",name,static_cast<unsigned long long>(pc-base));}
        else used+=sprintf_s(line+used,sizeof line-used,"0x%llX",static_cast<unsigned long long>(pc));
        if(base && !_wcsicmp(module.ModuleName,L"DarkSoulsIII")){
            // No symbols: the function start from .pdata, as Ghidra names it (FUN_ + its address at the 0x140000000 base).
            if(const auto* entry=static_cast<const IMAGE_RUNTIME_FUNCTION_ENTRY*>(SymFunctionTableAccess64(process,at)))
                used+=sprintf_s(line+used,sizeof line-used,"  FUN_%s+0x%llX",Hex(0x140000000ull+entry->BeginAddress).c_str(),
                    static_cast<unsigned long long>(at-base-entry->BeginAddress));}
        else if(base){
            alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO)+512]{};auto* symbol=reinterpret_cast<SYMBOL_INFO*>(buffer);
            symbol->SizeOfStruct=sizeof(SYMBOL_INFO);symbol->MaxNameLen=511;DWORD64 displacement{};
            if(SymFromAddr(process,at,&displacement,symbol))used+=sprintf_s(line+used,sizeof line-used,"  %.400s+0x%llX",symbol->Name,static_cast<unsigned long long>(displacement));
            IMAGEHLP_LINE64 source{};source.SizeOfStruct=sizeof source;DWORD line_displacement{};
            if(SymGetLineFromAddr64(process,at,&line_displacement,&source) && source.FileName){
                const char* file=std::strrchr(source.FileName,'\\');file=file?file+1:source.FileName;
                used+=sprintf_s(line+used,sizeof line-used,"  (%.200s:%lu)",file,source.LineNumber);}}
        text+=line;text+="\n";
    }
    SymCleanup(process);
    if(suspended)ResumeThread(target.h);
    return text.empty()?"  (no frame)\n":text;
}
}
