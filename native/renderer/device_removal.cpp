// I-0082: the device-removal report (device_removal.h).
#include "device_removal.h"
#include <wrl/client.h>
#include <cstdio>

namespace ds3native {
namespace {
using Microsoft::WRL::ComPtr;
// A debug name as JSON text: UTF-8, quotes and backslashes and control characters replaced.
std::string Name(const wchar_t* wide,const char* narrow) {
    std::string text;
    if(wide && *wide){char buffer[256]{};WideCharToMultiByte(CP_UTF8,0,wide,-1,buffer,sizeof buffer-1,nullptr,nullptr);text=buffer;}
    else if(narrow)text=narrow;
    if(text.size()>120)text.resize(120);
    for(auto& c:text)if(c=='"' || c=='\\' || static_cast<unsigned char>(c)<0x20)c='_';
    return text;
}
}

const char* BreadcrumbOpName(D3D12_AUTO_BREADCRUMB_OP op) noexcept {
    switch(op){
    case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:return "SetMarker";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:return "BeginEvent";
    case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:return "EndEvent";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:return "DrawInstanced";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:return "DrawIndexedInstanced";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:return "ExecuteIndirect";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:return "Dispatch";
    case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:return "CopyBufferRegion";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:return "CopyTextureRegion";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:return "CopyResource";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:return "CopyTiles";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:return "ResolveSubresource";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:return "ClearRenderTargetView";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:return "ClearUnorderedAccessView";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:return "ClearDepthStencilView";
    case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:return "ResourceBarrier";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:return "ExecuteBundle";
    case D3D12_AUTO_BREADCRUMB_OP_PRESENT:return "Present";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:return "ResolveQueryData";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:return "BeginSubmission";
    case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:return "EndSubmission";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:return "AtomicCopyBufferUint";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:return "AtomicCopyBufferUint64";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:return "ResolveSubresourceRegion";
    case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:return "WriteBufferImmediate";
    case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:return "SetProtectedResourceSession";
    case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:return "SetPipelineState1";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:return "DispatchMesh";
    default:return nullptr;   // video, ray tracing and later operations: the number is written instead
    }
}

DeviceRemoval DescribeRemoval(ID3D12Device* device,size_t max_nodes) {
    DeviceRemoval result;
    if(!device)return result;
    result.reason=device->GetDeviceRemovedReason();
    if(SUCCEEDED(result.reason))return result;
    ComPtr<ID3D12DeviceRemovedExtendedData> dred;
    if(FAILED(device->QueryInterface(IID_PPV_ARGS(&dred)))){result.rows.push_back("{\"event\":\"native_dred\",\"available\":false}");return result;}
    // DRED 1.2: the state the runtime recorded at the removal (0 unknown, 3 hung, 6 fault, 7 page fault).
    int state=-1;{ComPtr<ID3D12DeviceRemovedExtendedData2> dred2;if(SUCCEEDED(dred.As(&dred2)))state=static_cast<int>(dred2->GetDeviceState());}
    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs{};
    if(FAILED(dred->GetAutoBreadcrumbsOutput(&breadcrumbs)))result.rows.push_back("{\"event\":\"native_dred\",\"breadcrumbs\":false,\"device_state\":"+std::to_string(state)+"}");
    else{size_t nodes{},finished{};
        for(const D3D12_AUTO_BREADCRUMB_NODE* node=breadcrumbs.pHeadAutoBreadcrumbNode;node;node=node->pNext){
            const UINT completed=node->pLastBreadcrumbValue?*node->pLastBreadcrumbValue:0;
            // A list the GPU finished says nothing about the fault: counted, not listed.
            if(completed>=node->BreadcrumbCount){++finished;continue;}
            if(nodes++>=max_nodes)continue;
            std::string row="{\"event\":\"native_dred_list\",\"list\":\""+Name(node->pCommandListDebugNameW,node->pCommandListDebugNameA)+
                "\",\"queue\":\""+Name(node->pCommandQueueDebugNameW,node->pCommandQueueDebugNameA)+"\",\"completed\":"+std::to_string(completed)+
                ",\"recorded\":"+std::to_string(node->BreadcrumbCount)+",\"ops\":[";
            // The operations around the first one not completed: four before it, the operation itself, three after.
            const UINT first=completed>4?completed-4:0,last=completed+4<node->BreadcrumbCount?completed+4:node->BreadcrumbCount;
            for(UINT k=first;k<last && node->pCommandHistory;++k){
                const char* op=BreadcrumbOpName(node->pCommandHistory[k]);
                row+=(k==first?"":",")+std::string("\"")+(k==completed?"> ":"")+(op?op:("op"+std::to_string(static_cast<int>(node->pCommandHistory[k]))).c_str())+"\"";}
            result.rows.push_back(row+"]}");}
        result.rows.push_back("{\"event\":\"native_dred\",\"breadcrumbs\":true,\"unfinished_lists\":"+std::to_string(nodes)+",\"finished_lists\":"+std::to_string(finished)+",\"device_state\":"+std::to_string(state)+"}");}
    D3D12_DRED_PAGE_FAULT_OUTPUT fault{};
    if(SUCCEEDED(dred->GetPageFaultAllocationOutput(&fault)) && fault.PageFaultVA){
        char head[96]{};sprintf_s(head,"{\"event\":\"native_dred_page_fault\",\"address\":\"0x%llX\"",static_cast<unsigned long long>(fault.PageFaultVA));
        std::string row=head;int k=0;
        for(const D3D12_DRED_ALLOCATION_NODE* a=fault.pHeadExistingAllocationNode;a && k<8;a=a->pNext,++k)
            row+=",\"existing_"+std::to_string(k)+"\":{\"type\":"+std::to_string(static_cast<int>(a->AllocationType))+",\"name\":\""+Name(a->ObjectNameW,a->ObjectNameA)+"\"}";
        k=0;
        for(const D3D12_DRED_ALLOCATION_NODE* a=fault.pHeadRecentFreedAllocationNode;a && k<8;a=a->pNext,++k)
            row+=",\"freed_"+std::to_string(k)+"\":{\"type\":"+std::to_string(static_cast<int>(a->AllocationType))+",\"name\":\""+Name(a->ObjectNameW,a->ObjectNameA)+"\"}";
        result.rows.push_back(row+"}");}
    return result;
}
}
