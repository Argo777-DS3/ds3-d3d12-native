#include "game_bootstrap_shared.h"
#include "crash_dump.h"
#include "original_image.h"
#include <psapi.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
struct Handle { HANDLE h{}; ~Handle(){ if(h) CloseHandle(h); } };
using ds3native::bootstrap::CrashKind;using ds3native::bootstrap::CrashState;
const char* KindName(CrashKind kind) {
    switch(kind){case CrashKind::Exception:return "exception";case CrashKind::NativeFailure:return "native_failure";case CrashKind::Refusal:return "refusal";
    case CrashKind::DeviceRemoved:return "device_removed";case CrashKind::GpuHang:return "gpu_hang";default:return "none";}
}
const char* RemovedName(uint32_t reason) {
    switch(reason){case 0x887A0005:return "DXGI_ERROR_DEVICE_REMOVED";case 0x887A0006:return "DXGI_ERROR_DEVICE_HUNG";case 0x887A0007:return "DXGI_ERROR_DEVICE_RESET";
    case 0x887A0020:return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";case 0x887A002B:return "DXGI_ERROR_ACCESS_DENIED";case 0:return "S_OK";default:return "";}
}
std::string Utf8(const std::wstring& text) {
    std::string out(WideCharToMultiByte(CP_UTF8,0,text.c_str(),-1,nullptr,0,nullptr,nullptr),'\0');
    if(!out.empty()){WideCharToMultiByte(CP_UTF8,0,text.c_str(),-1,out.data(),static_cast<int>(out.size()),nullptr,nullptr);out.pop_back();}
    return out;
}
std::wstring Wide(const char* text) {
    std::wstring out(MultiByteToWideChar(CP_UTF8,0,text,-1,nullptr,0),L'\0');
    if(!out.empty()){MultiByteToWideChar(CP_UTF8,0,text,-1,out.data(),static_cast<int>(out.size()));out.pop_back();}
    return out;
}
std::string Json(std::string text) {for(auto& c:text)if(c=='\\')c='/';else if(c=='"' || static_cast<unsigned char>(c)<0x20)c='_';return text;}
// I-0082 (DESIGN 3): the controller is the one process that survives the game, so it writes the dump the module asks for, from
// outside (a corrupt heap or an exhausted stack in the game does not stop it), then the report with the faulting thread's
// stack named from the module's PDB as it is now (a later build's PDB would no longer match the dump).
void ServeCrash(ds3native::bootstrap::Control& control,HANDLE process,DWORD pid,const std::filesystem::path& journal,const std::wstring& module_folder) {
    auto& crash=control.crash;
    constexpr auto requested=static_cast<LONG>(CrashState::Requested);
    if(InterlockedCompareExchange(&crash.state,static_cast<LONG>(CrashState::Taken),requested)!=requested)return;
    const auto folder=journal.parent_path()/L"crashes";std::error_code ignored;std::filesystem::create_directories(folder,ignored);
    static int attempts{};   // a failed attempt keeps its number: the next one never meets its files
    const auto stem=journal.stem().wstring()+L"-crash"+std::to_wstring(++attempts);
    const auto dump=(folder/(stem+L".dmp")).wstring();
    const bool written=ds3native::crash::WriteDump(process,pid,crash.thread,crash.exception,dump.c_str(),L"game_bootstrap.dll",control.bounded_seconds>0);   // a play session's dump without the process data (M2)
    std::string report="DS3 DX12 native crash report (I-0082)\nkind: "+std::string(KindName(crash.kind))+"\n";
    char line[600]{};
    sprintf_s(line,"code: 0x%08X\nreason: %.250s\nthread: %u\ndevice removed reason: 0x%08X %s\n",crash.code,crash.reason,crash.thread,
        static_cast<uint32_t>(crash.removed_reason),RemovedName(static_cast<uint32_t>(crash.removed_reason)));report+=line;
    uint32_t code{};uint64_t address{};
    if(crash.exception && ds3native::crash::ExceptionHead(process,crash.exception,code,address)){
        sprintf_s(line,"exception: 0x%08X at 0x%llX\n",code,static_cast<unsigned long long>(address));report+=line;}
    report+="dump: "+(written?Utf8(dump):std::string("not written"))+"\njournal: "+Utf8(journal.wstring())+
        " (the device-removal rows: native_device_removed, native_dred_list, native_dred_page_fault)\nstack of the faulting thread:\n";
    report+=ds3native::crash::StackReport(process,crash.thread,crash.exception,module_folder.c_str());
    std::ofstream(folder/(stem+L".txt"),std::ios::binary)<<report;
    if(written){wcsncpy_s(crash.dump,dump.c_str(),_TRUNCATE);InterlockedIncrement(&crash.dumps);}
    InterlockedExchange(&crash.state,static_cast<LONG>(written?CrashState::Written:CrashState::Failed));
}
// The player's message in play mode, after the game closed (French: the player's language).
void TellPlayer(const ds3native::bootstrap::Crash& crash,DWORD exit_code,const std::filesystem::path& journal) {
    // Security audit L4 (2026-11-23): the strings read back from the shared section are bounded (the user's other processes can write it).
    wchar_t what[700]{};const auto reason=Wide(std::string(crash.reason,strnlen(crash.reason,sizeof(crash.reason))).c_str());
    const std::wstring dump(crash.dump,wcsnlen(crash.dump,std::size(crash.dump)));
    switch(crash.kind){
    case CrashKind::GpuHang:swprintf_s(what,L"le GPU a cessé de répondre (aucune image terminée en 5 secondes).");break;
    case CrashKind::DeviceRemoved:swprintf_s(what,L"le pilote graphique a perdu la carte (périphérique D3D12 retiré, code 0x%08X %hs).",
        static_cast<uint32_t>(crash.removed_reason),RemovedName(static_cast<uint32_t>(crash.removed_reason)));break;
    case CrashKind::NativeFailure:swprintf_s(what,L"une erreur du rendu natif : %.300s.",reason.c_str());break;
    case CrashKind::Refusal:swprintf_s(what,L"un appel du jeu que le rendu natif ne prend pas en charge : %.300s.",reason.c_str());break;
    case CrashKind::Exception:swprintf_s(what,L"une erreur fatale (exception 0x%08X, %.200s).",crash.code,reason.c_str());break;
    default:swprintf_s(what,L"une sortie inattendue (code 0x%08lX).",exit_code);break;}
    std::wstring text=L"Dark Souls III (rendu D3D12 natif) s'est fermé : "+std::wstring(what);
    // Security audit M2: the dump holds part of the game's memory; the player is told before sharing it.
    if(!dump.empty())text+=L"\n\nUn minidump et un rapport ont été écrits dans :\n"+std::filesystem::path(dump).parent_path().wstring()+
        L"\n(le minidump contient une partie de la mémoire du jeu : ne le partage qu'avec des personnes de confiance)";
    text+=L"\n\nLe journal de la partie :\n"+journal.wstring();
    MessageBoxW(nullptr,text.c_str(),L"Dark Souls III DX12",MB_OK|MB_ICONERROR|MB_SETFOREGROUND|MB_TOPMOST);
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=6) { std::puts("game_bootstrap_ctl <pid> <dll> <new journal> <reference|native> <seconds:1..7200, 0: play until the game exits (native, or reference: the original renderer)>"); return 2; }
    const DWORD pid=wcstoul(argv[1],nullptr,10),seconds=wcstoul(argv[5],nullptr,10);
    // D-0182 (the launcher): seconds 0 is a native play session, no bound (the controller waits for the game to exit).
    // I-0085: a bounded run up to two hours (the night endurance test; 900 s before).
    // I-0100 tranche 2 (Thomas, 2026-10-26): seconds 0 with reference is a play in the original renderer (its own pop-in rows).
    const bool play=!seconds && (!wcscmp(argv[4],L"native") || !wcscmp(argv[4],L"reference"));
    if(!pid || (!seconds && !play) || seconds>7200 || (wcscmp(argv[4],L"reference") && wcscmp(argv[4],L"native"))) return 2;
    const bool native=!wcscmp(argv[4],L"native");
    const auto dll=std::filesystem::absolute(argv[2]).wstring();
    const auto log=std::filesystem::absolute(argv[3]).wstring();
    const std::filesystem::path journal(log);
    const auto module_folder=std::filesystem::path(dll).parent_path().wstring();   // I-0082: the PDB of the module beside it
    if(log.size()>=1024 || std::filesystem::exists(log)) return 3;
    Handle process{OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|
        PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_TERMINATE|SYNCHRONIZE,FALSE,pid)};
    if(!process.h) return 4;
    wchar_t actual[32768]{}; DWORD chars=32768;
    if(!QueryFullProcessImageNameW(process.h,0,actual,&chars)) return 5;
    // I-0137 (I-0120 A1): the target is the original executable wherever it is installed (original_image.h), never a path.
    if(const auto identity=ds3native::bootstrap::CheckOriginalImage(actual);identity!=ds3native::bootstrap::OriginalImage::Ok){
        std::printf("{\"refused\":\"target_image\",\"reason\":\"%s\"}\n",ds3native::bootstrap::OriginalImageName(identity));std::fflush(stdout);return 5;}
    wchar_t name[96]{}; ds3native::bootstrap::Name(name,pid);
    Handle mapping{CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(ds3native::bootstrap::Control),name)};
    if(!mapping.h || GetLastError()==ERROR_ALREADY_EXISTS) return 6;
    auto* control=static_cast<ds3native::bootstrap::Control*>(MapViewOfFile(mapping.h,FILE_MAP_ALL_ACCESS,0,0,sizeof(ds3native::bootstrap::Control)));
    if(!control) return 7;
    control->version=ds3native::bootstrap::control_version; control->pid=pid; control->mode=native?ds3native::bootstrap::Mode::NativeBootstrap:ds3native::bootstrap::Mode::Reference;
    wcscpy_s(control->journal,log.c_str()); control->bounded_seconds=seconds; control->magic=ds3native::bootstrap::signature;
    const size_t bytes=(dll.size()+1)*sizeof(wchar_t);
    void* remote=VirtualAllocEx(process.h,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    SIZE_T n{};
    if(!remote || !WriteProcessMemory(process.h,remote,dll.c_str(),bytes,&n) || n!=bytes) return 8;
    const auto load=reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"));
    Handle worker{CreateRemoteThread(process.h,nullptr,0,load,remote,0,nullptr)};
    if(!worker.h || WaitForSingleObject(worker.h,10000)!=WAIT_OBJECT_0) return 9;
    if(!VirtualFreeEx(process.h,remote,0,MEM_RELEASE)) return 10;
    const ULONGLONG start=GetTickCount64();
    while(!control->ready && WaitForSingleObject(process.h,0)==WAIT_TIMEOUT && GetTickCount64()-start<10000) {ServeCrash(*control,process.h,pid,journal,module_folder);Sleep(10);}
    std::printf("{\"pid\":%lu,\"ready\":%ld,\"native\":%s}\n",pid,control->ready,native?"true":"false"); std::fflush(stdout);
    if(control->ready!=1) {
        // This controller owns only the explicitly named test process. Never
        // leave a failed, partly instrumented native test running.
        TerminateProcess(process.h,0xD3126611); WaitForSingleObject(process.h,10000); return 11;
    }
    while(WaitForSingleObject(process.h,50)==WAIT_TIMEOUT && (play || GetTickCount64()-start<uint64_t(seconds)*1000)) ServeCrash(*control,process.h,pid,journal,module_folder);
    if(WaitForSingleObject(process.h,0)==WAIT_TIMEOUT) {
        InterlockedExchange(&control->stop,1);
        if(native) {
            const ULONGLONG drain=GetTickCount64();
            while(!control->closed && WaitForSingleObject(process.h,20)==WAIT_TIMEOUT && GetTickCount64()-drain<3000) ServeCrash(*control,process.h,pid,journal,module_folder);
        }
        TerminateProcess(process.h,0xD3126612); WaitForSingleObject(process.h,10000);
    }
    DWORD code{}; GetExitCodeProcess(process.h,&code);
    const auto& crash=control->crash;
    const std::wstring dump(crash.dump,wcsnlen(crash.dump,std::size(crash.dump)));   // security audit L4: bounded
    const std::string crash_dir=!dump.empty()?Json(Utf8(std::filesystem::path(dump).parent_path().wstring())):"";
    std::printf("{\"exit\":%lu,\"phase_calls\":%lld,\"native_frames\":%lld,\"closed\":%ld,\"legacy_refused\":%ld,\"faults\":%lld,\"crash_kind\":\"%s\",\"crash_code\":\"0x%08X\",\"crash_dumps\":%ld,\"crash_dir\":\"%s\"}\n",
        code,control->phase_calls,control->native_frames,control->closed,control->legacy_refused,control->faults,KindName(crash.kind),crash.code,crash.dumps,crash_dir.c_str());
    std::fflush(stdout);
    // I-0082: in play, the player is told why the game closed: a native fault exit (or a fault the module recorded) and an
    // abnormal exit code. A bounded test run reports in the row above instead (no window).
    const bool fault_exit=code==0xD3126606 || code==0xD3126607 || code==0xD3126608;
    // A recorded exception counts only when the game died of it (its exit code); one handled further on (and the exit heap
    // fault at a quit) does not.
    const bool died_of_exception=crash.kind==CrashKind::Exception && code==crash.code;
    if(play && native && (fault_exit || died_of_exception || (crash.kind!=CrashKind::None && crash.kind!=CrashKind::Exception)))TellPlayer(crash,code,journal);
    UnmapViewOfFile(control);
    return 0; // Native qualification is decided from the receipt, not this status.
}
