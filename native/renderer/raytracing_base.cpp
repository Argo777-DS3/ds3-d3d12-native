#include "raytracing_base.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include "rt_selftest.dxil.h"   // dxc output (Build-GameBootstrap.ps1): const unsigned char rt_selftest_dxil[]

namespace ds3native {
using Microsoft::WRL::ComPtr;

HRESULT BindlessHeap::Open(ID3D12Device* device,uint32_t capacity) {
    if(heap_)return E_ILLEGAL_METHOD_CALL;
    const D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,capacity,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    if(const HRESULT made=CountedHeap(device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&heap_));FAILED(made))return made;
    heap_->SetName(L"native bindless heap");
    cpu_=heap_->GetCPUDescriptorHandleForHeapStart();
    stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    indices_=BindlessIndices(capacity);
    return S_OK;
}

namespace {
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,uint64_t bytes,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags,HRESULT& hr) {
    const D3D12_HEAP_PROPERTIES heap{type};
    const D3D12_RESOURCE_DESC desc{D3D12_RESOURCE_DIMENSION_BUFFER,0,bytes,1,1,1,DXGI_FORMAT_UNKNOWN,{1,0},D3D12_TEXTURE_LAYOUT_ROW_MAJOR,flags};
    ComPtr<ID3D12Resource> buffer;hr=CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&buffer));
    return buffer;
}
// Everything the self-test's list reads: released once the list completed, retired after its ticket when the wait failed, kept
// to process exit when the submission itself failed (no ticket proves its completion; backend.cpp's rule for a list in flight).
struct SelfTestObjects {
    ComPtr<ID3D12Resource> geometry,blas,tlas,scratch,instances,result,readback;
    ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList4> list;
};
}

RaytracingSelfTest RunRaytracingSelfTest(DeviceQueues& gpu,BindlessHeap& heap) noexcept {
    RaytracingSelfTest r;
    const auto& caps=gpu.Capabilities();
    if(!gpu.device5 || caps.raytracing<D3D12_RAYTRACING_TIER_1_1 || caps.binding<D3D12_RESOURCE_BINDING_TIER_3 || caps.shader_model<D3D_SHADER_MODEL_6_6 ||
       !caps.native_16bit){   // I-0126 R7b: the rt_reflect programmes read SHaRC's 16-bit types (the final review's L7)
        r.stage="unsupported";return r;}
    r.ran=true;const auto start=std::chrono::steady_clock::now();
    uint32_t scene_slot=BindlessIndices::none,result_slot=BindlessIndices::none;
    HRESULT hr=S_OK;
    std::shared_ptr<SelfTestObjects> kept;
    enum class Submission {none,attempted,ticketed,completed} submission=Submission::none;uint64_t ticket{};
    std::unique_ptr<std::shared_ptr<SelfTestObjects>> keep_to_exit;
    const auto fail=[&](const char* stage){r.stage=stage;r.failure=FAILED(hr)?hr:E_FAIL;return false;};
    const bool done=[&]()->bool {
        try {
            auto* device=gpu.device5.Get();
            if(!heap.IsOpen() && FAILED(hr=heap.Open(device)))return fail("bindless_heap");
            kept=std::make_shared<SelfTestObjects>();
            // The triangle and its instance, written once in upload memory (read by the builds in place).
            const float vertices[9]{0,0,0, 1,0,0, 0,1,0};
            auto& geometry=kept->geometry=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,sizeof vertices,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,hr);
            if(!geometry)return fail("vertex_buffer");
            void* mapped{};if(FAILED(hr=geometry->Map(0,nullptr,&mapped)))return fail("vertex_map");
            std::memcpy(mapped,vertices,sizeof vertices);geometry->Unmap(0,nullptr);
            D3D12_RAYTRACING_GEOMETRY_DESC triangle{D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE};
            triangle.Triangles.VertexFormat=DXGI_FORMAT_R32G32B32_FLOAT;triangle.Triangles.VertexCount=3;
            triangle.Triangles.VertexBuffer={geometry->GetGPUVirtualAddress(),3*sizeof(float)};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottom{D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,1,D3D12_ELEMENTS_LAYOUT_ARRAY};
            bottom.pGeometryDescs=&triangle;
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top{D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,1,D3D12_ELEMENTS_LAYOUT_ARRAY};
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottom_size{},top_size{};
            device->GetRaytracingAccelerationStructurePrebuildInfo(&bottom,&bottom_size);
            device->GetRaytracingAccelerationStructurePrebuildInfo(&top,&top_size);
            if(!bottom_size.ResultDataMaxSizeInBytes || !top_size.ResultDataMaxSizeInBytes)return fail("prebuild_info");
            const auto as_state=D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;const auto uav=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            auto& blas=kept->blas=Buffer(device,D3D12_HEAP_TYPE_DEFAULT,bottom_size.ResultDataMaxSizeInBytes,as_state,uav,hr);if(!blas)return fail("blas_buffer");
            auto& tlas=kept->tlas=Buffer(device,D3D12_HEAP_TYPE_DEFAULT,top_size.ResultDataMaxSizeInBytes,as_state,uav,hr);if(!tlas)return fail("tlas_buffer");
            auto& scratch=kept->scratch=Buffer(device,D3D12_HEAP_TYPE_DEFAULT,(std::max)(bottom_size.ScratchDataSizeInBytes,top_size.ScratchDataSizeInBytes),
                D3D12_RESOURCE_STATE_COMMON,uav,hr);if(!scratch)return fail("scratch_buffer");
            D3D12_RAYTRACING_INSTANCE_DESC instance{};instance.Transform[0][0]=instance.Transform[1][1]=instance.Transform[2][2]=1;
            instance.InstanceMask=0xFF;instance.AccelerationStructure=blas->GetGPUVirtualAddress();
            auto& instances=kept->instances=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,sizeof instance,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,hr);
            if(!instances)return fail("instance_buffer");
            if(FAILED(hr=instances->Map(0,nullptr,&mapped)))return fail("instance_map");
            std::memcpy(mapped,&instance,sizeof instance);instances->Unmap(0,nullptr);
            top.InstanceDescs=instances->GetGPUVirtualAddress();
            // Buffers start in COMMON (the scratch and the result promoted to UAV by their first use).
            auto& result=kept->result=Buffer(device,D3D12_HEAP_TYPE_DEFAULT,256,D3D12_RESOURCE_STATE_COMMON,uav,hr);if(!result)return fail("result_buffer");
            auto& readback=kept->readback=Buffer(device,D3D12_HEAP_TYPE_READBACK,256,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,hr);if(!readback)return fail("readback_buffer");
            // The two views, in the bindless heap.
            const uint64_t completed=gpu.CompletedValue();
            scene_slot=heap.Allocate(completed);result_slot=heap.Allocate(completed);
            if(scene_slot==BindlessIndices::none || result_slot==BindlessIndices::none)return fail("bindless_slots");
            D3D12_SHADER_RESOURCE_VIEW_DESC scene_view{DXGI_FORMAT_UNKNOWN,D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
            scene_view.RaytracingAccelerationStructure.Location=tlas->GetGPUVirtualAddress();
            device->CreateShaderResourceView(nullptr,&scene_view,heap.Cpu(scene_slot));
            D3D12_UNORDERED_ACCESS_VIEW_DESC result_view{DXGI_FORMAT_R32_TYPELESS,D3D12_UAV_DIMENSION_BUFFER};
            result_view.Buffer.NumElements=64;result_view.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
            device->CreateUnorderedAccessView(result.Get(),nullptr,&result_view,heap.Cpu(result_slot));
            // The DXIL programme carries its root signature (rt_selftest.hlsl).
            auto& root=kept->root;
            if(FAILED(hr=device->CreateRootSignature(0,rt_selftest_dxil,sizeof rt_selftest_dxil,IID_PPV_ARGS(&root))))return fail("root_signature");
            D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc{};pipeline_desc.pRootSignature=root.Get();pipeline_desc.CS={rt_selftest_dxil,sizeof rt_selftest_dxil};
            auto& pipeline=kept->pipeline;if(FAILED(hr=device->CreateComputePipelineState(&pipeline_desc,IID_PPV_ARGS(&pipeline))))return fail("pipeline");
            auto& allocator=kept->allocator;auto& list=kept->list;
            if(FAILED(hr=device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))))return fail("allocator");
            if(FAILED(hr=device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))))return fail("command_list4");
            list->SetName(L"native raytracing self-test");
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};
            build.Inputs=bottom;build.DestAccelerationStructureData=blas->GetGPUVirtualAddress();build.ScratchAccelerationStructureData=scratch->GetGPUVirtualAddress();
            list->BuildRaytracingAccelerationStructure(&build,0,nullptr);
            // The scratch is reused and the BLAS read by the TLAS build: one UAV barrier on each.
            const D3D12_RESOURCE_BARRIER after_bottom[2]{{D3D12_RESOURCE_BARRIER_TYPE_UAV},{D3D12_RESOURCE_BARRIER_TYPE_UAV}};
            D3D12_RESOURCE_BARRIER barriers[2]{after_bottom[0],after_bottom[1]};barriers[0].UAV.pResource=blas.Get();barriers[1].UAV.pResource=scratch.Get();
            list->ResourceBarrier(2,barriers);
            build.Inputs=top;build.DestAccelerationStructureData=tlas->GetGPUVirtualAddress();
            list->BuildRaytracingAccelerationStructure(&build,0,nullptr);
            barriers[0].UAV.pResource=tlas.Get();list->ResourceBarrier(1,barriers);
            ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);
            list->SetComputeRootSignature(root.Get());list->SetPipelineState(pipeline.Get());
            const uint32_t indices[2]{scene_slot,result_slot};list->SetComputeRoot32BitConstants(0,2,indices,0);
            list->Dispatch(1,1,1);
            D3D12_RESOURCE_BARRIER copy{D3D12_RESOURCE_BARRIER_TYPE_TRANSITION};
            copy.Transition={result.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&copy);
            list->CopyBufferRegion(readback.Get(),0,result.Get(),0,8);
            if(FAILED(hr=list->Close()))return fail("close");
            // Final review L1: the holder that keeps the objects to process exit is made before the submission, so keeping them
            // never allocates once the list may run.
            keep_to_exit=std::make_unique<std::shared_ptr<SelfTestObjects>>(kept);
            submission=Submission::attempted;ticket=gpu.Execute(list.Get());submission=Submission::ticketed;
            gpu.Complete(ticket);submission=Submission::completed;
            heap.Release(scene_slot,ticket);heap.Release(result_slot,ticket);scene_slot=result_slot=BindlessIndices::none;
            const D3D12_RANGE read{0,8};if(FAILED(hr=readback->Map(0,&read,&mapped)))return fail("readback_map");
            uint32_t answer[2]{};std::memcpy(answer,mapped,sizeof answer);const D3D12_RANGE none{0,0};readback->Unmap(0,&none);
            r.status=answer[0];std::memcpy(&r.t,&answer[1],sizeof r.t);
            // COMMITTED_TRIANGLE_HIT is 1; the hit at t=1 within float rounding.
            if(r.status!=1 || r.t<0.999f || r.t>1.001f)return fail("wrong_answer");
            r.stage="done";return true;
        } catch(...) {if(SUCCEEDED(hr))hr=E_UNEXPECTED;
            return fail(submission==Submission::attempted?"submit":submission==Submission::ticketed?"wait":"exception");}
    }();
    // Step review M2: a list that may still run keeps what it reads. A failed wait retires the objects and the slots after its
    // ticket; a failed submission (no ticket) keeps both to process exit. A slot of a step that failed before any submission was
    // never read: it is released against the last submission.
    if(submission==Submission::attempted){(void)keep_to_exit.release();scene_slot=result_slot=BindlessIndices::none;}
    else if(submission==Submission::ticketed){
        try{gpu.RetireAfter(ticket,kept);}catch(...){(void)keep_to_exit.release();}}
    try {
        const uint64_t after=submission==Submission::ticketed?ticket:gpu.LastSubmitted();
        if(scene_slot!=BindlessIndices::none)heap.Release(scene_slot,after);
        if(result_slot!=BindlessIndices::none)heap.Release(result_slot,after);
    } catch(...) {}
    r.passed=done;
    r.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    return r;
}

}
