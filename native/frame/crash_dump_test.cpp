// I-0082 controls: the minidump writer and the stack report on threads of this process (a parked thread, then a thread inside
// its exception filter), and the device-removal report on a WARP device removed on purpose.
#include "crash_dump.h"
#include "../renderer/backend.h"
#include "../renderer/device_removal.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
using namespace ds3native;
void Require(bool condition,const char* what){if(!condition)throw std::runtime_error(what);}
HANDLE parked_ready{},parked_release{};
__declspec(noinline) DWORD WINAPI ParkedThread(void*) {SetEvent(parked_ready);WaitForSingleObject(parked_release,INFINITE);return 0;}
// The faulting thread holds its exception filter (its EXCEPTION_POINTERS stay valid) while the main thread dumps it.
EXCEPTION_POINTERS* volatile caught{};
LONG HoldInFilter(EXCEPTION_POINTERS* pointers) {caught=pointers;SetEvent(parked_ready);WaitForSingleObject(parked_release,INFINITE);return EXCEPTION_EXECUTE_HANDLER;}
__declspec(noinline) void RaiseInFaultingThread() {__try{RaiseException(0xE0000082,0,0,nullptr);}__except(HoldInFilter(GetExceptionInformation())){}}
__declspec(noinline) DWORD WINAPI FaultingThread(void*) {RaiseInFaultingThread();return 0;}
bool IsMinidump(const std::filesystem::path& path) {
    char magic[4]{};std::ifstream(path,std::ios::binary).read(magic,4);return std::string(magic,4)=="MDMP" && std::filesystem::file_size(path)>4096;
}
void DumpThread(LPTHREAD_START_ROUTINE routine,const char* expected,bool exception,const std::filesystem::path& folder) {
    parked_ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);parked_release=CreateEventW(nullptr,TRUE,FALSE,nullptr);caught=nullptr;
    DWORD id{};const HANDLE thread=CreateThread(nullptr,0,routine,nullptr,0,&id);Require(thread && WaitForSingleObject(parked_ready,5000)==WAIT_OBJECT_0,"test thread start");
    const auto path=folder/(std::string(expected)+".dmp");
    const uint64_t pointers=exception?reinterpret_cast<uint64_t>(caught):0;
    Require(crash::WriteDump(GetCurrentProcess(),GetCurrentProcessId(),id,pointers,path.c_str(),L"crash_dump_test.exe"),"dump written");
    Require(IsMinidump(path),"dump signature and size");
    Require(!crash::WriteDump(GetCurrentProcess(),GetCurrentProcessId(),id,pointers,path.c_str(),L"crash_dump_test.exe"),"an existing dump is never overwritten");
    if(exception){uint32_t code{};uint64_t address{};Require(crash::ExceptionHead(GetCurrentProcess(),pointers,code,address) && code==0xE0000082 && address,"exception head");}
    wchar_t self[MAX_PATH]{};GetModuleFileNameW(nullptr,self,MAX_PATH);
    const auto stack=crash::StackReport(GetCurrentProcess(),id,pointers,std::filesystem::path(self).parent_path().c_str());
    std::printf("%s stack:\n%s",expected,stack.c_str());
    Require(stack.find(expected)!=std::string::npos,"the stack names the thread's function from the PDB");
    SetEvent(parked_release);WaitForSingleObject(thread,5000);CloseHandle(thread);CloseHandle(parked_ready);CloseHandle(parked_release);
}
// A hung queue (waiting on a fence nobody signals) ahead of a list with a copy, then the device removed on purpose: the report
// gives the removal's reason and its DRED row. DRED records breadcrumbs for GPU faults only (TDR, page fault), not for an
// application's RemoveDevice: none is listed here (device state 0, unknown), on WARP and on the hardware alike (I-0082 RESULT).
void RemovalReport(bool warp) {
    DeviceQueues queues;device_removal_data=true;queues.Open(warp,false);
    Require(queues.RemovalDataActive(),"DRED enabled before the device");
    Require(!queues.DebugLayerActive(),"no debug layer unless asked");
    // I-0109: the device's highest level; 12_2 guarantees SM 6.5, DXR 1.1, binding tier 3 and mesh shaders, so the reads must say so.
    {const auto& c=queues.Capabilities();
        Require(c.feature_level>=D3D_FEATURE_LEVEL_11_0 && c.feature_level<=D3D_FEATURE_LEVEL_12_2,"a feature level from 11_0 to 12_2");
        Require(c.feature_level!=D3D_FEATURE_LEVEL_12_2 || (c.shader_model>=D3D_SHADER_MODEL_6_5 && c.raytracing>=D3D12_RAYTRACING_TIER_1_1 &&
            c.binding==D3D12_RESOURCE_BINDING_TIER_3 && c.mesh>=D3D12_MESH_SHADER_TIER_1 && queues.device5),"12_2's guarantees read");
        std::printf("device caps (%s): level %x, shader model %x, raytracing %d, binding %d, mesh %d, enhanced barriers %d\n",warp?"WARP":"hardware",
            unsigned(c.feature_level),unsigned(c.shader_model),int(c.raytracing),int(c.binding),int(c.mesh),int(c.enhanced_barriers));}
    Require(SUCCEEDED(DescribeRemoval(queues.device.Get()).reason),"a live device reports S_OK");
    using Microsoft::WRL::ComPtr;
    ComPtr<ID3D12Resource> source,target;D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC desc{D3D12_RESOURCE_DIMENSION_BUFFER,0,4096,1,1,1,DXGI_FORMAT_UNKNOWN,{1,0},D3D12_TEXTURE_LAYOUT_ROW_MAJOR};
    Require(SUCCEEDED(queues.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&source))) &&
        SUCCEEDED(queues.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&target))),"buffers");
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    Require(SUCCEEDED(queues.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))) &&
        SUCCEEDED(queues.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))),"list");
    list->SetName(L"control list");list->CopyBufferRegion(target.Get(),0,source.Get(),0,4096);Require(SUCCEEDED(list->Close()),"list close");
    ComPtr<ID3D12Fence> never;Require(SUCCEEDED(queues.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&never))),"fence");
    queues.queue->Wait(never.Get(),1);queues.Execute(list.Get());
    ComPtr<ID3D12Device5> removable;Require(SUCCEEDED(queues.device.As(&removable)),"ID3D12Device5");removable->RemoveDevice();
    const auto report=DescribeRemoval(queues.device.Get());
    std::printf("%s removed: 0x%08lX\n",warp?"warp":"hardware",static_cast<unsigned long>(report.reason));for(const auto& row:report.rows)std::printf("%s\n",row.c_str());
    Require(FAILED(report.reason),"a removed device reports its reason");
    Require(!report.rows.empty() && report.rows.back().find("native_dred")!=std::string::npos,"the DRED rows");
    Require(std::string(BreadcrumbOpName(D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED))=="DrawIndexedInstanced","operation names");
    queues.QuarantineToProcessExit();   // a removed device: nothing is waited for
}
}
int main() {
    try {
        const auto folder=std::filesystem::temp_directory_path()/("ds3-crash-dump-test-"+std::to_string(GetCurrentProcessId()));
        std::filesystem::create_directories(folder);
        DumpThread(&ParkedThread,"ParkedThread",false,folder);
        DumpThread(&FaultingThread,"RaiseInFaultingThread",true,folder);
        std::filesystem::remove_all(folder);
        RemovalReport(true);RemovalReport(false);
        std::puts("PASS crash dump and device removal controls");return 0;
    } catch(const std::exception& e) {std::printf("FAIL %s\n",e.what());return 1;}
}
