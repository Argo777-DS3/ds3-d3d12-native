#include "backend.h"
#include "gbuffer_encoder.h"
#include <d3dcompiler.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
using namespace ds3native;
namespace {
void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("GPU control operation failed");}
ComPtr<ID3DBlob> Shader(const char* text,const char* profile){
    ComPtr<ID3DBlob> blob,errors;Check(D3DCompile(text,std::strlen(text),nullptr,nullptr,nullptr,"main",profile,
        D3DCOMPILE_ENABLE_STRICTNESS,0,&blob,&errors));return blob;
}
}
int main(int argc,char** argv){
    try {
        if(argc!=2 || (std::strcmp(argv[1],"warp") && std::strcmp(argv[1],"hardware")))throw std::runtime_error("warp or hardware required");
        DeviceQueues gpu;gpu.Open(!std::strcmp(argv[1],"warp"),true);auto* d=gpu.device.Get();
        auto resource=[&](D3D12_RESOURCE_DESC desc,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear=nullptr){
            D3D12_HEAP_PROPERTIES properties{};properties.Type=heap;ComPtr<ID3D12Resource> value;
            Check(d->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,state,clear,IID_PPV_ARGS(&value)));return value;
        };
        auto buffer=[&](UINT64 bytes,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state){D3D12_RESOURCE_DESC desc{};
            desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;desc.Height=1;desc.DepthOrArraySize=1;
            desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return resource(desc,heap,state);};
        auto heap=[&](D3D12_DESCRIPTOR_HEAP_TYPE type,UINT count,D3D12_DESCRIPTOR_HEAP_FLAGS flags){
            D3D12_DESCRIPTOR_HEAP_DESC desc{type,count,flags};ComPtr<ID3D12DescriptorHeap> value;
            Check(d->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&value)));return value;};
        const auto rtvs=heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV,6,D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        const auto dsv=heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        const auto bindings=heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,1,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        D3D12_SHADER_RESOURCE_VIEW_DESC null_view{};null_view.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        null_view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;null_view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        null_view.Texture2D.MipLevels=1;d->CreateShaderResourceView(nullptr,&null_view,bindings->GetCPUDescriptorHandleForHeapStart());
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER parameter{};
        parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable={1,&range};
        D3D12_ROOT_SIGNATURE_DESC root_desc{1,&parameter,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
        ComPtr<ID3DBlob> root_blob,root_errors;Check(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&root_blob,&root_errors));
        ComPtr<ID3D12RootSignature> root;Check(d->CreateRootSignature(0,root_blob->GetBufferPointer(),root_blob->GetBufferSize(),IID_PPV_ARGS(&root)));
        const auto vs=Shader("struct O{float4 p:SV_Position;float v:TEXCOORD0;};O main(float2 p:POSITION,float v:TEXCOORD1){O o;o.p=float4(p,.25,1);o.v=v;return o;}","vs_5_0");
        const auto ps=Shader("struct O{float4 a:SV_Target0;float4 b:SV_Target1;float4 c:SV_Target2;float4 d:SV_Target3;float4 e:SV_Target4;float4 f:SV_Target5;};O main(float4 p:SV_Position,float v:TEXCOORD0){O o;o.a=float4(.125,v,1,1);o.b=float4(.25,v,1,1);o.c=float4(.375,v,1,1);o.d=float4(.5,v,1,1);o.e=float4(.625,v,1,1);o.f=float4(.75,v,1,1);return o;}","ps_5_0");
        const D3D12_INPUT_ELEMENT_DESC elements[]{{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"TEXCOORD",1,DXGI_FORMAT_R32_FLOAT,1,0,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1}};
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root.Get();
        pipeline.VS={vs->GetBufferPointer(),vs->GetBufferSize()};pipeline.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
        pipeline.InputLayout={elements,2};pipeline.SampleMask=UINT_MAX;pipeline.SampleDesc.Count=1;
        pipeline.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pipeline.NumRenderTargets=6;
        for(UINT slot=0;slot<6;++slot)pipeline.RTVFormats[slot]=DXGI_FORMAT_R8G8B8A8_UNORM;
        for(auto& blend:pipeline.BlendState.RenderTarget){blend.SrcBlend=D3D12_BLEND_ONE;blend.DestBlend=D3D12_BLEND_ZERO;blend.BlendOp=D3D12_BLEND_OP_ADD;
            blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.RenderTargetWriteMask=15;}
        pipeline.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pipeline.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pipeline.RasterizerState.DepthClipEnable=TRUE;
        pipeline.DepthStencilState.DepthEnable=TRUE;pipeline.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;
        pipeline.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS;pipeline.DepthStencilState.StencilEnable=TRUE;
        pipeline.DepthStencilState.StencilReadMask=255;pipeline.DepthStencilState.StencilWriteMask=255;
        pipeline.DepthStencilState.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_EQUAL};
        pipeline.DepthStencilState.BackFace=pipeline.DepthStencilState.FrontFace;pipeline.DSVFormat=DXGI_FORMAT_D24_UNORM_S8_UINT;
        ComPtr<ID3D12PipelineState> pso;Check(d->CreateGraphicsPipelineState(&pipeline,IID_PPV_ARGS(&pso)));
        D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=32;texture.Height=32;
        texture.DepthOrArraySize=1;texture.MipLevels=1;texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;texture.SampleDesc.Count=1;texture.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        const D3D12_CLEAR_VALUE color_clear{DXGI_FORMAT_R8G8B8A8_UNORM,{0,0,0,0}};
        std::array<ComPtr<ID3D12Resource>,6> colors;std::array<D3D12_CPU_DESCRIPTOR_HANDLE,6> targets;
        for(UINT i=0;i<6;++i){colors[i]=resource(texture,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET,&color_clear);
            targets[i]=rtvs->GetCPUDescriptorHandleForHeapStart();targets[i].ptr+=i*d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            d->CreateRenderTargetView(colors[i].Get(),nullptr,targets[i]);}
        texture.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;texture.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE depth_clear{};depth_clear.Format=texture.Format;depth_clear.DepthStencil={1,9};
        const auto depth=resource(texture,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_DEPTH_WRITE,&depth_clear);
        d->CreateDepthStencilView(depth.Get(),nullptr,dsv->GetCPUDescriptorHandleForHeapStart());
        const float points[]={99,99,-.75f,-.75f,0,.75f,.75f,-.75f};const uint16_t indices[]={99,0,1,2};
        const auto vertices=buffer(sizeof(points),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
        const auto index=buffer(sizeof(indices),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
        void* mapped{};Check(vertices->Map(0,nullptr,&mapped));std::memcpy(mapped,points,sizeof(points));vertices->Unmap(0,nullptr);
        Check(index->Map(0,nullptr,&mapped));std::memcpy(mapped,indices,sizeof(indices));index->Unmap(0,nullptr);
        const float instance_values[]{0,0,0,0,0,.5f};
        const auto instance=buffer(sizeof(instance_values),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
        Check(instance->Map(0,nullptr,&mapped));std::memcpy(mapped,instance_values,sizeof(instance_values));instance->Unmap(0,nullptr);
        const D3D12_VERTEX_BUFFER_VIEW vbs[]{{vertices->GetGPUVirtualAddress(),sizeof(points),8},
            {instance->GetGPUVirtualAddress(),sizeof(instance_values),4}};
        const D3D12_INDEX_BUFFER_VIEW ib{index->GetGPUVirtualAddress(),sizeof(indices),DXGI_FORMAT_R16_UINT};
        ID3D12DescriptorHeap* heaps[]{bindings.Get()};const D3D12_GPU_DESCRIPTOR_HANDLE tables[]{bindings->GetGPUDescriptorHandleForHeapStart()};
        const GBufferDraw draw{targets,dsv->GetCPUDescriptorHandleForHeapStart(),{0,0,32,32,0,1},{0,0,32,32},root.Get(),pso.Get(),heaps,tables,
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,vbs,ib,3,1,1,5,1};
        FrameRecorder recorder;recorder.Open(d);auto* list=recorder.list.Get();Check(recorder.allocator->Reset());Check(list->Reset(recorder.allocator.Get(),nullptr));
        ClearGBuffer(list,targets,draw.depth,1.f,9);list->OMSetStencilRef(8);
        auto rejected=draw;rejected.start_instance=4;RecordGBufferDraw(list,rejected);
        // Rejected stencil draw must not write depth; the following draw uses
        // nonzero start index, base vertex and first instance on all six MRTs.
        list->OMSetStencilRef(9);RecordGBufferDraw(list,draw);
        // Same depth with LESS must reject the zero-green instance after the
        // admitted draw. Either broken stencil or depth changes final pixels.
        RecordGBufferDraw(list,rejected);
        const auto readback=buffer(6*8192,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
        for(UINT i=0;i<6;++i){D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition={colors[i].Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
            D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=colors[i].Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            to.PlacedFootprint={i*8192ull,{DXGI_FORMAT_R8G8B8A8_UNORM,32,32,1,256}};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
        Check(list->Close());const auto fence=gpu.Execute(list);gpu.Complete(fence);
        Check(readback->Map(0,nullptr,&mapped));bool exact=true;
        for(UINT i=0;i<6;++i){const auto* bytes=static_cast<const uint8_t*>(mapped)+i*8192;const auto* center=bytes+16*256+16*4;
            std::printf("target=%u center=%u,%u,%u,%u outside=%u,%u,%u,%u\n",i,center[0],center[1],center[2],center[3],bytes[0],bytes[1],bytes[2],bytes[3]);
            const UINT red=(i+1)*255/8;exact=exact && center[0]>=red && center[0]<=red+1 && center[1]>=127 && center[1]<=128 && center[2]==255 && center[3]==255;
            exact=exact && !bytes[0] && !bytes[1] && !bytes[2] && !bytes[3];}
        readback->Unmap(0,nullptr);if(!exact)throw std::runtime_error("six-target indexed/stencil output differs");
        if(gpu.diagnostics){for(UINT64 i=0;i<gpu.diagnostics->GetNumStoredMessages();++i){SIZE_T size{};Check(gpu.diagnostics->GetMessage(i,nullptr,&size));
            std::vector<uint8_t> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Check(gpu.diagnostics->GetMessage(i,message,&size));
            if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)throw std::runtime_error("D3D12 debug error");}}
        gpu.Drain();std::printf("PASS %s six_targets=6 indexed_offsets=1 stencil=9 first_instance=5 fence=%llu debug_errors=0 game_started=false\n",argv[1],static_cast<unsigned long long>(fence));
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL %s\n",error.what());return 1;}
}
