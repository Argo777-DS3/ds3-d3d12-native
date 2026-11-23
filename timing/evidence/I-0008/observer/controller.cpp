#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <cstdlib>
int wmain(int argc,wchar_t** argv){
    if(argc!=4||wcscmp(argv[1],L"observe")){std::puts("Use verified Invoke-Observer.ps1; requires an already running offline game.");return 1;}
    DWORD target=wcstoul(argv[2],nullptr,10);if(!target)return 2;
    HANDLE process=OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,target);
    if(!process)return 3;
    size_t bytes=(wcslen(argv[3])+1)*sizeof(wchar_t);
    void* remote=VirtualAllocEx(process,nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!remote||!WriteProcessMemory(process,remote,argv[3],bytes,nullptr))return 4;
    auto load=reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"));
    HANDLE loading=CreateRemoteThread(process,nullptr,0,load,remote,0,nullptr);
    if(!loading||WaitForSingleObject(loading,10000)!=WAIT_OBJECT_0)return 5;
    DWORD loaded=0;GetExitCodeThread(loading,&loaded);CloseHandle(loading);
    VirtualFreeEx(process,remote,0,MEM_RELEASE);CloseHandle(process);if(!loaded)return 6;
    wchar_t arm_name[80],complete_name[80];
    swprintf_s(arm_name,L"Local\\DS3_I0008_RING_ARM_%lu",target);
    swprintf_s(complete_name,L"Local\\DS3_I0008_RING_COMPLETE_%lu",target);
    HANDLE arm=nullptr,complete=nullptr;
    for(unsigned i=0;i<100;i++){
        if(!arm)arm=OpenEventW(EVENT_MODIFY_STATE,FALSE,arm_name);
        if(!complete)complete=OpenEventW(SYNCHRONIZE,FALSE,complete_name);
        if(arm&&complete)break;Sleep(50);
    }
    if(!arm||!complete)return 7;
    if(!SetEvent(arm))return 8;
    DWORD waited=WaitForSingleObject(complete,20000);CloseHandle(arm);CloseHandle(complete);
    if(waited!=WAIT_OBJECT_0){std::puts("NO COMPLETE. Do not reinject. Inspect process and restoration status.");return 9;}
    std::puts("Capture flushed. Inspect binary header and analyzer before accepting restoration or results.");return 0;
}
