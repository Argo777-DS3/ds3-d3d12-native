// PREPARE-REDUCTION host controls T-P3, T-P5 and T-P6 (PREPARE-REDUCTION-IMPLEMENTATION-SHEET 8.1): prepared native model
// draws on a real device with the debug layer, drawn and read back. Synthetic shaders and geometry only; nothing here is
// installed in the game or admitted as its content.
#include "../renderer/model_draw.h"
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
template<class F> void Refuses(F action,const char* message){bool refused{};try{action();}catch(const std::invalid_argument&){refused=true;}Require(refused,message);}
ds3content::CpuShaderProgram Compile(ds3content::ShaderStage stage,const char* source){
    ComPtr<ID3DBlob> code,errors;
    const auto status=D3DCompile(source,std::strlen(source),nullptr,nullptr,nullptr,"main",
        stage==ds3content::ShaderStage::Vertex?"vs_5_0":stage==ds3content::ShaderStage::Hull?"hs_5_0":stage==ds3content::ShaderStage::Domain?"ds_5_0":"ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&errors);
    if(FAILED(status) && errors)std::fprintf(stderr,"%s\n",static_cast<const char*>(errors->GetBufferPointer()));
    Require(SUCCEEDED(status),"control shader compilation");const auto* bytes=static_cast<const uint8_t*>(code->GetBufferPointer());
    return {0,"model-draw-control",stage,5,0,{bytes,bytes+code->GetBufferSize()}};
}
constexpr const char* vertex_source=
    "cbuffer cbOffset:register(b0){float4 offset;};"
    "float4 main(float2 p:POSITION):SV_Position{return float4(p+offset.xy,0,1);}";
// cbColour declares 64 bytes: the colour, the sampling coordinate and the texel weight.
constexpr const char* pixel_source=
    "cbuffer cbColour:register(b1){float4 colour;float4 coordinate;float4 unused;float4 weight;};"
    "Texture2D texel:register(t0);SamplerState sampler0:register(s0);"
    "float4 main(float4 position:SV_Position):SV_Target{return colour+texel.SampleLevel(sampler0,coordinate.xy,0)*weight.x;}";
// C4-1b review: a pixel programme indexing PS b4 dynamically (the index from b1).
constexpr const char* bounded_pixel_source=
    "cbuffer cbIndex:register(b1){float4 index;};cbuffer cbInstanceData:register(b4){float4 tint[2];};"
    "float4 main(float4 position:SV_Position):SV_Target{return tint[(uint)index.x]+float4(.5,0,0,1);}";
// W0a: a pixel programme writing two targets (the Forward2 envelope): the colour, and half of it.
constexpr const char* two_target_pixel_source=
    "cbuffer cbColour:register(b1){float4 colour;};struct Out{float4 a:SV_Target0;float4 b:SV_Target1;};"
    "Out main(float4 position:SV_Position){Out o;o.a=colour;o.b=colour*.5;return o;}";
// W7 (TS-3): a tessellated control: the vertex programme passes the position on, the hull takes its factors from HS b6
// (cbPNTriangles' register), the domain interpolates the patch and adds DS b5 (cbTessScene's) and DS b8 (cbSceneParam's).
constexpr const char* tess_vertex_source=
    "cbuffer cbOffset:register(b0){float4 offset;};"
    "float4 main(float2 p:POSITION):POS{return float4(p+offset.xy,0,1);}";
constexpr const char* tess_hull_source=
    "cbuffer cbPNTriangles:register(b6){float4 factors;};struct V{float4 p:POS;};"
    "struct C{float e[3]:SV_TessFactor;float i:SV_InsideTessFactor;};"
    "C pc(InputPatch<V,3> ip){C c;c.e[0]=c.e[1]=c.e[2]=factors.x;c.i=factors.y;return c;}"
    "[domain(\"tri\")][partitioning(\"fractional_odd\")][outputtopology(\"triangle_cw\")][outputcontrolpoints(3)][patchconstantfunc(\"pc\")]"
    "V main(InputPatch<V,3> ip,uint i:SV_OutputControlPointID){return ip[i];}";
constexpr const char* tess_domain_source=
    "cbuffer cbTessScene:register(b5){float4 shift;};cbuffer cbSceneParam:register(b8){float4 extra;};struct V{float4 p:POS;};"
    "struct C{float e[3]:SV_TessFactor;float i:SV_InsideTessFactor;};"
    "[domain(\"tri\")]float4 main(C c,float3 b:SV_DomainLocation,const OutputPatch<V,3> p):SV_Position{return p[0].p*b.x+p[1].p*b.y+p[2].p*b.z+shift+extra;}";
std::vector<uint8_t> Floats(std::initializer_list<float> values){
    std::vector<uint8_t> bytes(values.size()*4);std::memcpy(bytes.data(),values.begin(),bytes.size());return bytes;}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;
}
void Write(ID3D12Resource* buffer,const void* bytes,size_t size){
    void* memory{};const D3D12_RANGE none{};Check(buffer->Map(0,&none,&memory),"control map");std::memcpy(memory,bytes,size);buffer->Unmap(0,nullptr);}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}

// One device with a queue, a 4x4 target and its depth, the control geometry and a 1x1 texture of value `texel`.
struct Rig {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;uint64_t fence_value{};HANDLE event{};
    ComPtr<ID3D12Resource> target,target2,depth,readback,vertices,indices,texture,frame_block,frame_far,constants;ComPtr<ID3D12DescriptorHeap> rtv,rtv2,dsv,srv;
    std::shared_ptr<const void> srv_owner;
    explicit Rig(IDXGIAdapter1* adapter){
        Check(D3D12CreateDevice(adapter,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"control device");
        const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)),"control queue");
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"control allocator");
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");list->Close();
        Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"control fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=desc.Height=4;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target)),"control target");
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target2)),"control second target");   // W0a T4
        desc.Format=DXGI_FORMAT_D32_FLOAT;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;D3D12_CLEAR_VALUE clear{DXGI_FORMAT_D32_FLOAT};clear.DepthStencil.Depth=1;
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(&depth)),"control depth");
        desc.Width=desc.Height=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.Flags=D3D12_RESOURCE_FLAG_NONE;
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&texture)),"control texture");
        D3D12_DESCRIPTOR_HEAP_DESC heaps{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&rtv)),"control rtv heap");
        Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&rtv2)),"control second rtv heap");
        heaps.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&dsv)),"control dsv heap");
        heaps.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&srv)),"control srv heap");
        device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());device->CreateRenderTargetView(target2.Get(),nullptr,rtv2->GetCPUDescriptorHandleForHeapStart());
        device->CreateDepthStencilView(depth.Get(),nullptr,dsv->GetCPUDescriptorHandleForHeapStart());
        device->CreateShaderResourceView(texture.Get(),nullptr,srv->GetCPUDescriptorHandleForHeapStart());
        srv_owner=std::shared_ptr<const void>(srv.Get(),[keep=srv](const void*){});
        readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,256*4*2,D3D12_RESOURCE_STATE_COPY_DEST);
        const float corners[]{-1,-1,-1,3,3,-1};const uint16_t order[]{0,1,2};
        vertices=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,sizeof(corners),D3D12_RESOURCE_STATE_GENERIC_READ);Write(vertices.Get(),corners,sizeof(corners));
        indices=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256,D3D12_RESOURCE_STATE_GENERIC_READ);Write(indices.Get(),order,sizeof(order));
        // C4-1b: the constant uploads of one Draw (bump allocation in a 64 KB upload buffer, zeroed per draw).
        constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,65536,D3D12_RESOURCE_STATE_GENERIC_READ);
        // C4-1: the VS b0 offset block (zero), bound as the pass's frame root CBV.
        frame_block=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256,D3D12_RESOURCE_STATE_GENERIC_READ);const float zero[4]{};Write(frame_block.Get(),zero,sizeof(zero));
        // review: a second block whose offset moves the triangle off the target, so the image proves the root CBV is read.
        frame_far=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256,D3D12_RESOURCE_STATE_GENERIC_READ);const float away[4]{4,4,0,0};Write(frame_far.Get(),away,sizeof(away));
        // The texel (0, 128, 0, 255), uploaded once.
        const auto upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256,D3D12_RESOURCE_STATE_GENERIC_READ);const uint8_t value[4]{0,128,0,255};Write(upload.Get(),value,4);
        Begin();D3D12_TEXTURE_COPY_LOCATION to{texture.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,1,1,1,256};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        const auto barrier=Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);Finish();
    }
    ~Rig(){if(event)CloseHandle(event);}
    void Begin(){Check(allocator->Reset(),"control allocator reset");Check(list->Reset(allocator.Get(),nullptr),"control list reset");}
    void Finish(){Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        Check(queue->Signal(fence.Get(),++fence_value),"control signal");Check(fence->SetEventOnCompletion(fence_value,event),"control wait");WaitForSingleObject(event,INFINITE);}
    ModelGeometryBinding Geometry() const {
        ModelGeometryBinding geometry;geometry.storage_generation=1;geometry.vertex_resource=vertices;geometry.index_resource=indices;
        geometry.vertices={vertices->GetGPUVirtualAddress(),24,8};geometry.indices={indices->GetGPUVirtualAddress(),6,DXGI_FORMAT_R16_UINT};geometry.index_count=3;return geometry;}
    // Draws one prepared draw over the cleared target and returns the RGBA8 of pixel (1, 1).
    std::array<uint8_t,4> Draw(const OwnedNativeModelDraw& draw,bool away=false,uint64_t frame_number=0,uint64_t completed=0){
        Begin();const auto target_view=rtv->GetCPUDescriptorHandleForHeapStart(),depth_view=dsv->GetCPUDescriptorHandleForHeapStart();
        const float black[4]{};list->ClearRenderTargetView(target_view,black,0,nullptr);list->ClearDepthStencilView(depth_view,D3D12_CLEAR_FLAG_DEPTH,1,0,0,nullptr);
        NativeModelFrameAddresses frame{};frame[FrameVsView]=(away?frame_far:frame_block)->GetGPUVirtualAddress();
        uint8_t* constant_memory{};const D3D12_RANGE unread{};Check(constants->Map(0,&unread,reinterpret_cast<void**>(&constant_memory)),"control constant map");for(size_t at=0;at<65536;at+=4){const float one=1.f;std::memcpy(constant_memory+at,&one,4);}   // review: a sentinel, so only a bounded read gives zero
        uint64_t used{};NativeModelConstantUploads uploads([&](uint64_t bytes){Require(used+bytes<=65536,"control constant space");
            const NativeModelConstantUploads::Range range{constant_memory+used,constants->GetGPUVirtualAddress()+used};used+=bytes;return range;},frame_number,completed);
        draw.Encode(list.Get(),{&target_view,1},depth_view,{0,0,4,4,0,1},{0,0,4,4},frame,uploads);constants->Unmap(0,nullptr);
        auto barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION to{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},from{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        to.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,4,4,1,256};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&barrier);Finish();
        void* memory{};const D3D12_RANGE read{0,256*4};Check(readback->Map(0,&read,&memory),"control readback");
        std::array<uint8_t,4> pixel{};std::memcpy(pixel.data(),static_cast<const uint8_t*>(memory)+256+4,4);const D3D12_RANGE none{};readback->Unmap(0,&none);return pixel;}
    // W0a: encodes one draw into `targets` (with the cleared depth) through a fresh uploads object, then `after` records
    // the readback copies; returns nothing (the caller maps the readback).
    void EncodeInto(const OwnedNativeModelDraw& draw,std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> targets,const std::function<void()>& after){
        Begin();const auto depth_view=dsv->GetCPUDescriptorHandleForHeapStart();const float black[4]{};
        for(const auto target_view:targets)list->ClearRenderTargetView(target_view,black,0,nullptr);
        ClearDepth(list.Get(),depth_view,1,0,false);
        NativeModelFrameAddresses frame{};frame[FrameVsView]=frame_block->GetGPUVirtualAddress();
        uint8_t* memory{};const D3D12_RANGE none{};Check(constants->Map(0,&none,reinterpret_cast<void**>(&memory)),"control constant map");
        uint64_t used{};NativeModelConstantUploads uploads([&](uint64_t bytes){Require(used+bytes<=65536,"control constant space");
            const NativeModelConstantUploads::Range range{memory+used,constants->GetGPUVirtualAddress()+used};used+=bytes;return range;});
        draw.Encode(list.Get(),targets,depth_view,{0,0,4,4,0,1},{0,0,4,4},frame,uploads);constants->Unmap(0,nullptr);after();Finish();}
    // Copies a 4x4 texture (state `state` around the copy) to readback row block `block` (1 KB each).
    void CopyOut(ID3D12Resource* resource,DXGI_FORMAT format,D3D12_RESOURCE_STATES state,UINT block){
        auto barrier=Transition(resource,state,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION to{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},from{resource,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        to.PlacedFootprint.Offset=UINT64(block)*256*4;to.PlacedFootprint.Footprint={format,4,4,1,256};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        barrier=Transition(resource,D3D12_RESOURCE_STATE_COPY_SOURCE,state);list->ResourceBarrier(1,&barrier);}
    // The 4 bytes of pixel (1, 1) in readback block `block`.
    std::array<uint8_t,4> Pixel(UINT block){void* memory{};const D3D12_RANGE read{0,256*4*2};Check(readback->Map(0,&read,&memory),"control readback");
        std::array<uint8_t,4> pixel{};std::memcpy(pixel.data(),static_cast<const uint8_t*>(memory)+block*256*4+256+4,4);const D3D12_RANGE none{};readback->Unmap(0,&none);return pixel;}
    // W0a T3: the depth a depth-only draw leaves at pixel (1, 1) (cleared to 1 first).
    float DepthAt(const OwnedNativeModelDraw& draw){
        EncodeInto(draw,{},[&]{CopyOut(depth.Get(),DXGI_FORMAT_D32_FLOAT,D3D12_RESOURCE_STATE_DEPTH_WRITE,0);});
        const auto bytes=Pixel(0);float value{};std::memcpy(&value,bytes.data(),4);return value;}
    // W0a T4: pixel (1, 1) of both targets of a two-target draw.
    std::array<std::array<uint8_t,4>,2> DrawTwo(const OwnedNativeModelDraw& draw){
        const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> views{rtv->GetCPUDescriptorHandleForHeapStart(),rtv2->GetCPUDescriptorHandleForHeapStart()};
        EncodeInto(draw,views,[&]{CopyOut(target.Get(),DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET,0);
            CopyOut(target2.Get(),DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_STATE_RENDER_TARGET,1);});
        return {Pixel(0),Pixel(1)};}
    // C4-1b: the constant payloads copied when several draws are recorded in one view (one uploads object).
    uint64_t UploadsFor(std::initializer_list<const OwnedNativeModelDraw*> draws,uint64_t frame_number=0,uint64_t completed=0){   // I-0090: numbered
        Begin();const auto target_view=rtv->GetCPUDescriptorHandleForHeapStart(),depth_view=dsv->GetCPUDescriptorHandleForHeapStart();
        NativeModelFrameAddresses frame{};frame[FrameVsView]=frame_block->GetGPUVirtualAddress();
        uint8_t* memory{};const D3D12_RANGE none{};Check(constants->Map(0,&none,reinterpret_cast<void**>(&memory)),"control constant map");
        uint64_t used{};NativeModelConstantUploads uploads([&](uint64_t bytes){Require(used+bytes<=65536,"control constant space");
            const NativeModelConstantUploads::Range range{memory+used,constants->GetGPUVirtualAddress()+used};used+=bytes;return range;},frame_number,completed);
        for(const auto* draw:draws)draw->Encode(list.Get(),{&target_view,1},depth_view,{0,0,4,4,0,1},{0,0,4,4},frame,uploads);
        constants->Unmap(0,nullptr);Finish();return uploads.Blocks();}
    uint64_t DebugErrors() const {
        ComPtr<ID3D12InfoQueue> info;if(FAILED(device.As(&info)))return 0;uint64_t errors{};
        for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size{};info->GetMessage(index,nullptr,&size);std::vector<uint8_t> bytes(size);
            auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());if(SUCCEEDED(info->GetMessage(index,message,&size)) &&
            (message->Severity==D3D12_MESSAGE_SEVERITY_ERROR || message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION ||
            (message->Severity==D3D12_MESSAGE_SEVERITY_WARNING && message->ID!=D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE &&
             message->ID!=D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE && message->ID!=D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET))){++errors;std::fprintf(stderr,"%s\n",message->pDescription);}}
        return errors;}
};
D3D12_SAMPLER_DESC Sampler(D3D12_TEXTURE_ADDRESS_MODE address,float bias=0){
    D3D12_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_POINT;sampler.AddressU=sampler.AddressV=sampler.AddressW=address;
    sampler.MipLODBias=bias;sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MaxLOD=D3D12_FLOAT32_MAX;return sampler;}
struct Bank {int identity{};uint64_t generation{1};D3D12_SAMPLER_DESC sampler=Sampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP);};
// A complete input on the set: colour (0.2, 0, 0, 1) plus the texel sampled at `u` with weight 1.
NativeModelDrawInput Input(const Rig& rig,const std::shared_ptr<const NativeModelProgramSet>& set,const Bank& bank,float u=.5f){
    NativeModelDrawInput input;input.program_set=set;input.sampler_bank=&bank.identity;input.sampler_bank_generation=bank.generation;input.geometry=rig.Geometry();
    input.rasterizer.FillMode=D3D12_FILL_MODE_SOLID;input.rasterizer.CullMode=D3D12_CULL_MODE_NONE;input.rasterizer.DepthClipEnable=TRUE;
    input.depth_stencil.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;input.blend.RenderTarget[0].SrcBlend=input.blend.RenderTarget[0].SrcBlendAlpha=D3D12_BLEND_ONE;
    input.blend.RenderTarget[0].DestBlend=input.blend.RenderTarget[0].DestBlendAlpha=D3D12_BLEND_ZERO;input.blend.RenderTarget[0].BlendOp=input.blend.RenderTarget[0].BlendOpAlpha=D3D12_BLEND_OP_ADD;
    input.blend.RenderTarget[0].LogicOp=D3D12_LOGIC_OP_NOOP;input.blend.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    input.shape.envelope=ModelPassEnvelope::Forward1;   // W0a: one target
    input.render_target_count=1;input.render_target_formats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;input.depth_format=DXGI_FORMAT_D32_FLOAT;input.sample_mask=UINT_MAX;
    input.constants={{ds3content::ShaderStage::Vertex,0,Floats({0,0,0,0}),{},true},{ds3content::ShaderStage::Pixel,1,Floats({.2f,0,0,1, u,.5f,0,0, 0,0,0,0, 1,0,0,0})}};
    input.srvs={{ds3content::ShaderStage::Pixel,0,rig.srv->GetCPUDescriptorHandleForHeapStart(),rig.texture,rig.srv_owner}};
    input.samplers={{ds3content::ShaderStage::Pixel,0,bank.sampler}};return input;}
bool Near(const std::array<uint8_t,4>& pixel,std::array<int,4> expected){
    for(size_t i=0;i<4;++i)if(std::abs(int(pixel[i])-expected[i])>1)return false;return true;}
}

int main(int argc,char** argv){
    try{
        Require(argc==2 || argc==3,"warp or hardware required (and the programme cache)");const bool warp=!std::strcmp(argv[1],"warp");Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
        ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
        ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");
        ComPtr<IDXGIAdapter1> warp_adapter,hardware_adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)),"control WARP adapter");
        factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&hardware_adapter));
        DXGI_ADAPTER_DESC1 hardware_desc{};if(hardware_adapter)hardware_adapter->GetDesc1(&hardware_desc);
        if(hardware_desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)hardware_adapter.Reset();
        Require(warp || hardware_adapter,"no hardware adapter");
        Rig rig(warp?warp_adapter.Get():hardware_adapter.Get());auto* device=rig.device.Get();
        const auto set=MakeNativeModelProgramSet(Compile(ds3content::ShaderStage::Vertex,vertex_source),Compile(ds3content::ShaderStage::Pixel,pixel_source),
            {{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}});
        Bank bank;const std::array<int,4> lit{51,128,0,255};   // 0.2 red + the texel (0, 128, 0, 255)
        // Images use the values the set keeps (shadow off). Agrees() draws the input with the kept values, then again with the
        // shadow on (the former programme, pipeline and sampler-table paths run and are compared with the kept values, and the
        // former values are drawn): no mismatch and the same image.
        const auto Agrees=[&](Rig& on,const NativeModelDrawInput& input){
            const auto before=ModelDrawShadowMismatches();const auto kept=on.Draw(*PrepareNativeModelDraw(on.device.Get(),input));
            SetModelDrawShadowPeriod(1);const auto former=on.Draw(*PrepareNativeModelDraw(on.device.Get(),input));SetModelDrawShadowPeriod(0);
            Require(ModelDrawShadowMismatches()==before && kept==former,"kept values differ from the former paths");return former;};

        // T-P3: a shared payload binds the same bytes as a per-draw copy (same image), is not copied into the input, stays
        // alive while any draw holding it lives, and is refused when shorter than the declaration.
        auto copied=Input(rig,set,bank);const auto reference=PrepareNativeModelDraw(device,copied);
        const auto reference_pixel=rig.Draw(*reference);Require(Near(reference_pixel,lit),"T-P3 per-draw copy image");
        auto payload=std::make_shared<const std::vector<uint8_t>>(copied.constants[1].bytes);std::weak_ptr<const std::vector<uint8_t>> watched=payload;
        auto shared_input=Input(rig,set,bank);shared_input.constants[1].bytes.clear();shared_input.constants[1].shared=payload;
        auto first=PrepareNativeModelDraw(device,shared_input);auto second=PrepareNativeModelDraw(device,shared_input);payload.reset();
        Require(first->Input().constants[1].shared.get()==second->Input().constants[1].shared.get() && first->Input().constants[1].bytes.empty(),"T-P3 shared payload not copied");
        Require(rig.Draw(*first)==reference_pixel && rig.Draw(*second)==reference_pixel,"T-P3 shared payload image equals the per-draw copy");
        // C4-1b: a payload object two draws share is copied once per view; the per-draw copies of `reference` are its own.
        Require(rig.UploadsFor({first.get(),second.get()})==1 && rig.UploadsFor({first.get(),reference.get()})==2,"C4-1b shared payload copied once per view");
        first.reset();Require(!watched.expired(),"T-P3 payload kept by the remaining draw");second.reset();shared_input={};
        Require(watched.expired(),"T-P3 payload released with its last draw");
        auto different=Input(rig,set,bank);different.constants[1].shared=std::make_shared<const std::vector<uint8_t>>(Floats({.6f,0,0,1, .5f,.5f,0,0, 0,0,0,0, 1,0,0,0}));
        different.constants[1].bytes.clear();Require(Near(rig.Draw(*PrepareNativeModelDraw(device,different)),{153,128,0,255}),"T-P3 differing payload image");
        auto shorter=Input(rig,set,bank);shorter.constants[1].shared=std::make_shared<const std::vector<uint8_t>>(Floats({.2f,0,0,1}));shorter.constants[1].bytes.clear();
        Refuses([&]{PrepareNativeModelDraw(device,shorter);},"T-P3 shared payload shorter than the declaration accepted");

        // T-P5: the set's sampler tables follow the bank (its object and version). Sampling at u = 1.5 reads the texel under
        // WRAP and the transparent black border under BORDER, so a stale table shows in the image.
        const auto wrapped=Input(rig,set,bank,1.5f);Require(Near(rig.Draw(*PrepareNativeModelDraw(device,wrapped)),lit),"T-P5 wrap image");
        Require(Near(rig.Draw(*PrepareNativeModelDraw(device,wrapped)),lit),"T-P5 kept table image");
        Bank versioned;   // one bank object: a new version with other descriptions drops the kept tables
        Require(Near(rig.Draw(*PrepareNativeModelDraw(device,Input(rig,set,versioned,1.5f))),lit),"T-P5 first version image");
        versioned.generation=2;versioned.sampler=Sampler(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
        Require(Near(rig.Draw(*PrepareNativeModelDraw(device,Input(rig,set,versioned,1.5f))),{51,0,0,255}),"T-P5 new bank version image");
        Bank other;other.sampler=Sampler(D3D12_TEXTURE_ADDRESS_MODE_BORDER);   // another bank at the same version number
        Require(Near(rig.Draw(*PrepareNativeModelDraw(device,wrapped)),lit),"T-P5 wrap image again");
        Require(Near(rig.Draw(*PrepareNativeModelDraw(device,Input(rig,set,other,1.5f))),{51,0,0,255}),"T-P5 other bank image");
        Require(Near(Agrees(rig,wrapped),lit) && Near(Agrees(rig,Input(rig,set,versioned,1.5f)),{51,0,0,255}) && Near(Agrees(rig,Input(rig,set,other,1.5f)),{51,0,0,255}),"T-P5 shadow images");
        // A contract breach (descriptions changed without a new bank version) draws the kept table; the shadow detects it and
        // draws the former one.
        Require(Near(rig.Draw(*PrepareNativeModelDraw(device,Input(rig,set,versioned,1.5f))),{51,0,0,255}),"T-P5 version 2 again");
        versioned.sampler=Sampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP);   // the same bank object and version, other descriptions
        Require(Near(rig.Draw(*PrepareNativeModelDraw(device,Input(rig,set,versioned,1.5f))),{51,0,0,255}),"T-P5 breach draws the kept table");
        SetModelDrawShadowPeriod(1);Require(Near(rig.Draw(*PrepareNativeModelDraw(device,Input(rig,set,versioned,1.5f))),lit),"T-P5 shadow draws the former table");SetModelDrawShadowPeriod(0);
        Require(ModelDrawShadowMismatches()==1,"T-P5 stale table not detected");

        // T-P6: with every cache of the set warm, a missing or short binding is refused exactly as ValidateBindings refuses it,
        // the root shape is the declared one, a state change gets its own pipeline and an equal state reuses one.
        const auto warm=Input(rig,set,bank);
        {auto input=warm;input.constants.erase(input.constants.begin()+1);Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 missing PS constant accepted");}
        {auto input=warm;input.constants.erase(input.constants.begin());Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 missing VS constant accepted");}
        {auto input=warm;input.srvs.clear();Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 missing SRV accepted");}
        {auto input=warm;input.samplers.clear();Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 missing sampler accepted");}
        {auto input=warm;input.constants[1].bytes.resize(48);Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 short constant accepted");}
        {auto input=warm;input.constants.push_back(input.constants[0]);Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 duplicate constant accepted");}
        {auto input=warm;input.geometry.index_count=4;Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 partial triangle accepted");}
        // C4-1: VS b0 is a frame register (a root CBV): a table binding there, or a frame binding elsewhere, is refused, and a
        // draw that binds a frame root refuses to encode without its address.
        {auto input=warm;input.constants[0].frame=false;Refuses([&]{PrepareNativeModelDraw(device,input);},"C4-1 table binding at a frame register accepted");}
        {auto input=warm;input.constants[1].frame=true;Refuses([&]{PrepareNativeModelDraw(device,input);},"C4-1 frame binding at a table register accepted");}
        {const auto draw=PrepareNativeModelDraw(device,warm);Require(draw->FrameMask()==1u<<FrameVsView,"C4-1 frame mask");rig.Begin();
            const auto target_view=rig.rtv->GetCPUDescriptorHandleForHeapStart(),depth_view=rig.dsv->GetCPUDescriptorHandleForHeapStart();
            NativeModelConstantUploads unused([](uint64_t)->NativeModelConstantUploads::Range{throw std::logic_error("control uploads unused");});
            Refuses([&]{draw->Encode(rig.list.Get(),{&target_view,1},depth_view,{0,0,4,4,0,1},{0,0,4,4},NativeModelFrameAddresses{},unused);},"C4-1 encode without the frame address accepted");rig.Finish();
            // The root CBV reaches the shader: the off-target offset leaves the pixel cleared; the legacy per-draw tables
            // (split CBV ranges) draw the same image as the global ones.
            Require(Near(rig.Draw(*draw,true),{0,0,0,0}),"C4-1 frame root CBV not read by the shader");
            SetModelDrawLegacyTables(true);Require(Near(rig.Draw(*PrepareNativeModelDraw(device,warm)),lit),"C4-1 legacy tables image");SetModelDrawLegacyTables(false);}
        // C4-1b review: PS b4 is a bounded descriptor CBV written at encode: an index inside the payload reads it, one in its
        // 256-byte zero tail or past its view reads zero (the D3D11 rule the palettes and cbInstanceData rely on).
        {const auto bounded_set=MakeNativeModelProgramSet(Compile(ds3content::ShaderStage::Vertex,vertex_source),Compile(ds3content::ShaderStage::Pixel,bounded_pixel_source),
                {{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}});
            const auto bounded_at=[&](float index){auto input=Input(rig,bounded_set,bank);input.srvs.clear();input.samplers.clear();
                input.constants[1]=NativeModelConstantBinding{ds3content::ShaderStage::Pixel,1,Floats({index,0,0,0})};
                input.constants.push_back({ds3content::ShaderStage::Pixel,4,Floats({0,0,1,1, 0,1,0,1})});
                return rig.Draw(*PrepareNativeModelDraw(device,input));};
            // The programme adds (0.5, 0, 0, 1); the upload buffer is pre-filled with 1.0, so an unbounded read past the view
            // would not give zero, and a skipped draw would leave the black clear.
            const std::array<int,4> blue{128,0,255,255},green{128,255,0,255},zero{128,0,0,255};
            Require(Near(bounded_at(0),blue) && Near(bounded_at(1),green),"C4-1b bounded CBV payload image");
            Require(Near(bounded_at(5),zero) && Near(bounded_at(100),zero),"C4-1b bounded CBV reads zero past the payload");
            SetModelDrawLegacyTables(true);const bool legacy_ok=Near(bounded_at(1),green) && Near(bounded_at(100),zero);SetModelDrawLegacyTables(false);
            Require(legacy_ok,"C4-1b bounded CBV on the legacy tables");
            {auto input=Input(rig,bounded_set,bank);input.srvs.clear();input.samplers.clear();
                input.constants[1]=NativeModelConstantBinding{ds3content::ShaderStage::Pixel,1,Floats({0,0,0,0})};
                input.constants.push_back({ds3content::ShaderStage::Pixel,4,Floats({0,0,1,1, 0,1,0,1})});
                const auto once=PrepareNativeModelDraw(device,input);Require(Near(rig.Draw(*once),blue),"C4-1b bounded draw image");
                Refuses([&]{rig.Draw(*once);},"C4-1b second encode of a bounded draw accepted");rig.list->Close();
                // C4-2a: a draw kept across frames rewrites its bounded CBVs in each later numbered frame once the previous is finished.
                const auto kept=PrepareNativeModelDraw(device,input);
                Require(Near(rig.Draw(*kept,false,1,0),blue) && Near(rig.Draw(*kept,false,2,1),blue) && Near(rig.Draw(*kept,false,3,2),blue),"C4-2a bounded draw kept across frames");
                Refuses([&]{rig.Draw(*kept,false,5,2);},"C4-2a encode while the previous frame may run accepted");rig.list->Close();
                Refuses([&]{rig.Draw(*kept,false,3,3);},"C4-2a encode in the same frame accepted");rig.list->Close();
                Refuses([&]{rig.Draw(*kept);},"C4-2a unnumbered encode after numbered ones accepted");rig.list->Close();
                // C4-2 with frame slots: two table copies, frame F writes copy F % 2, so frames 1 and 2 encode with nothing finished,
                // frame 3 once frame 1 is; frame 5 while frame 3 (the same copy) may run is refused.
                Require(!kept->ReEncodable(2) && kept->ReEncodable(1),"C4-2 one copy re-encodable at one slot only");
                auto two_copies=input;two_copies.table_copies=2;const auto slots2=PrepareNativeModelDraw(device,two_copies);
                Require(slots2->ReEncodable(2) && !slots2->ReEncodable(3),"C4-2 two copies re-encodable at two slots");
                Require(Near(rig.Draw(*slots2,false,1,0),blue) && Near(rig.Draw(*slots2,false,2,0),blue) && Near(rig.Draw(*slots2,false,3,1),blue) &&
                    Near(rig.Draw(*slots2,false,4,2),blue),"C4-2 bounded draw with two copies, frames in flight");
                Refuses([&]{rig.Draw(*slots2,false,5,2);},"C4-2 encode into a copy whose frame may run accepted");rig.list->Close();
                // I-0090: a rebuilt row's draw takes its predecessor's prepared state when only its constant payloads differ: the
                // new payload is drawn, the predecessor's C4-2a frames stand (a copy whose frame may run is refused, two draws of
                // one state in one frame are refused), the shadow agrees, and a state input or a payload size change is prepared in full.
                {auto base_input=input;base_input.table_copies=2;const auto base=PrepareNativeModelDraw(device,base_input);
                    Require(!base->ReusedState() && Near(rig.Draw(*base,false,1,0),blue),"I-0090 base draw");
                    auto next_input=base_input;next_input.constants.back()=NativeModelConstantBinding{ds3content::ShaderStage::Pixel,4,Floats({0,1,0,1, 0,0,1,1})};
                    const auto next=PrepareNativeModelDraw(device,next_input,base.get());
                    Require(next->ReusedState() && Near(rig.Draw(*next,false,2,0),green) && Near(rig.Draw(*next,false,3,1),green),"I-0090 reused state draws the new payload");
                    Refuses([&]{rig.Draw(*base,false,5,2);},"I-0090 a shared copy whose frame may run accepted");rig.list->Close();
                    Require(Near(rig.Draw(*base,false,4,2),blue),"I-0090 predecessor drawn again in a later frame");
                    Refuses([&]{rig.Draw(*next,false,4,2);},"I-0090 a second frame-4 encode of the state (other uploads) accepted");rig.list->Close();
                    Refuses([&]{rig.UploadsFor({base.get(),next.get()},6,4);},"I-0090 two draws of one state in one frame (one uploads) accepted");rig.list->Close();
                    {const auto before=ModelDrawShadowMismatches();SetModelDrawShadowPeriod(1);const auto shadowed=PrepareNativeModelDraw(device,next_input,base.get());SetModelDrawShadowPeriod(0);
                        Require(shadowed->ReusedState() && ModelDrawShadowMismatches()==before,"I-0090 shadow of a reused state");}
                    {auto changed=next_input;changed.blend.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;changed.rasterizer.CullMode=D3D12_CULL_MODE_BACK;
                        Require(!PrepareNativeModelDraw(device,changed,base.get())->ReusedState(),"I-0090 state change reused");}
                    {auto longer=next_input;longer.constants.back()=NativeModelConstantBinding{ds3content::ShaderStage::Pixel,4,Floats({0,1,0,1, 0,0,1,1, 0,0,0,0})};
                        Require(!PrepareNativeModelDraw(device,longer,base.get())->ReusedState(),"I-0090 payload size change reused");}
                    {auto other_count=next_input;other_count.table_copies=1;Require(!PrepareNativeModelDraw(device,other_count,base.get())->ReusedState(),"I-0090 table copies change reused");}}
                Refuses([&]{two_copies.table_copies=4;PrepareNativeModelDraw(device,two_copies);},"C4-2 four table copies accepted");}}
        // W0a (MODEL-DRAW-ENVELOPE-READY 8.1). T2: the envelope agrees with the producer's count or Prepare refuses the draw,
        // and EnvelopeMatches checks envelope, count, formats and depth together.
        {const std::array<DXGI_FORMAT,1> rgba{DXGI_FORMAT_R8G8B8A8_UNORM};const std::array<DXGI_FORMAT,2> two{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM};
            Require(EnvelopeMatches(warm,ModelPassEnvelope::Forward1,rgba,DXGI_FORMAT_D32_FLOAT) && !EnvelopeMatches(warm,ModelPassEnvelope::Velocity1,rgba,DXGI_FORMAT_D32_FLOAT) &&
                !EnvelopeMatches(warm,ModelPassEnvelope::Forward1,rgba,DXGI_FORMAT_D16_UNORM) && !EnvelopeMatches(warm,ModelPassEnvelope::Forward2,two,DXGI_FORMAT_D32_FLOAT) &&
                !EnvelopeMatches(warm,ModelPassEnvelope::Forward1,two,DXGI_FORMAT_D32_FLOAT),"T2 EnvelopeMatches");
            {auto input=warm;input.shape.envelope=ModelPassEnvelope::GBuffer6;Refuses([&]{PrepareNativeModelDraw(device,input);},"T2 one target under GBuffer6 accepted");}
            {auto input=warm;input.shape.envelope=ModelPassEnvelope::Forward2;Refuses([&]{PrepareNativeModelDraw(device,input);},"T2 one target under Forward2 accepted");}
            {auto input=warm;input.shape.family=ModelRootFamily::ModelTess;Refuses([&]{PrepareNativeModelDraw(device,input);},"T2 tessellation family accepted without its patch topology and programmes");}
            // W7 TS-3: a ModelTess draw (3-control-point patches through hull and domain, root CBVs HS b6, DS b5, b8) draws the
            // same image as the triangle draw; a hull/domain register outside the tess root, a hull SRV, a triangle family
            // with hull programmes and a hull register on a triangle draw are refused.
            {const auto tess_set=MakeNativeModelProgramSet(Compile(ds3content::ShaderStage::Vertex,tess_vertex_source),Compile(ds3content::ShaderStage::Pixel,pixel_source),
                    {{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}},
                    Compile(ds3content::ShaderStage::Hull,tess_hull_source),Compile(ds3content::ShaderStage::Domain,tess_domain_source));
                auto tess=Input(rig,tess_set,bank);tess.shape.family=ModelRootFamily::ModelTess;tess.shape.topology=ModelTopologyClass::Patch3;
                tess.constants.push_back({ds3content::ShaderStage::Hull,6,Floats({3,3,0,0})});
                tess.constants.push_back({ds3content::ShaderStage::Domain,5,Floats({0,0,0,0})});
                tess.constants.push_back({ds3content::ShaderStage::Domain,8,Floats({0,0,0,0})});
                Require(Near(rig.Draw(*PrepareNativeModelDraw(device,tess)),lit),"W7 tessellated draw image");
                SetModelDrawLegacyTables(true);Require(Near(rig.Draw(*PrepareNativeModelDraw(device,tess)),lit),"W7 tessellated draw on the legacy tables");SetModelDrawLegacyTables(false);
                {const auto before=ModelDrawShadowMismatches();SetModelDrawShadowPeriod(1);Require(Near(rig.Draw(*PrepareNativeModelDraw(device,tess)),lit),"W7 tessellated draw under the shadow");
                    SetModelDrawShadowPeriod(0);Require(ModelDrawShadowMismatches()==before,"W7 shadow mismatch on a tessellated draw");}
                {auto input=tess;input.constants.push_back({ds3content::ShaderStage::Hull,7,Floats({0,0,0,0})});Refuses([&]{PrepareNativeModelDraw(device,input);},"W7 hull register outside the tess root accepted");}
                {auto input=tess;input.srvs.push_back(input.srvs[0]);input.srvs.back().stage=ds3content::ShaderStage::Hull;Refuses([&]{PrepareNativeModelDraw(device,input);},"W7 hull SRV accepted");}
                {auto input=tess;input.shape.family=ModelRootFamily::Model;input.shape.topology=ModelTopologyClass::Triangle;Refuses([&]{PrepareNativeModelDraw(device,input);},"W7 triangle family with hull programmes accepted");}
                {auto input=warm;input.constants.push_back({ds3content::ShaderStage::Domain,5,Floats({0,0,0,0})});Refuses([&]{PrepareNativeModelDraw(device,input);},"W7 domain register on a triangle draw accepted");}
                {auto input=tess;input.constants.pop_back();Refuses([&]{PrepareNativeModelDraw(device,input);},"W7 missing DS b8 accepted");}}
            // T4: a Forward2 draw writes both targets (RT1 takes RT0's blend, IndependentBlendEnable off).
            const auto two_set=MakeNativeModelProgramSet(Compile(ds3content::ShaderStage::Vertex,vertex_source),Compile(ds3content::ShaderStage::Pixel,two_target_pixel_source),
                {{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}});
            auto forward2=Input(rig,two_set,bank);forward2.shape.envelope=ModelPassEnvelope::Forward2;forward2.render_target_count=2;
            forward2.render_target_formats[1]=DXGI_FORMAT_R8G8B8A8_UNORM;forward2.srvs.clear();forward2.samplers.clear();
            forward2.constants[1]=NativeModelConstantBinding{ds3content::ShaderStage::Pixel,1,Floats({.4f,0,0,1})};
            Require(EnvelopeMatches(forward2,ModelPassEnvelope::Forward2,two,DXGI_FORMAT_D32_FLOAT),"T4 Forward2 envelope");
            const auto both=rig.DrawTwo(*PrepareNativeModelDraw(device,forward2));
            Require(Near(both[0],{102,0,0,255}) && Near(both[1],{51,0,0,128}),"T4 two-target draw writes both targets");
            // T3: a depth-only draw (no colour target, no pixel programme) writes the depth (VS z = 0 over the cleared 1).
            const auto depth_set=MakeNativeModelProgramSet(Compile(ds3content::ShaderStage::Vertex,vertex_source),{},
                {{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}});
            auto depth_only=Input(rig,depth_set,bank);depth_only.shape.envelope=ModelPassEnvelope::DepthOnly;depth_only.render_target_count=0;
            depth_only.render_target_formats={};depth_only.constants.resize(1);depth_only.srvs.clear();depth_only.samplers.clear();
            depth_only.depth_stencil.DepthEnable=TRUE;depth_only.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;
            Require(EnvelopeMatches(depth_only,ModelPassEnvelope::DepthOnly,{},DXGI_FORMAT_D32_FLOAT),"T3 DepthOnly envelope");
            Require(rig.DepthAt(*PrepareNativeModelDraw(device,depth_only))==0.f,"T3 depth-only draw writes the depth");
            // Review: the same draw under the shadow (no pixel entry on either path) and on the legacy per-draw tables.
            {const auto before=ModelDrawShadowMismatches();SetModelDrawShadowPeriod(1);const auto shadowed=rig.DepthAt(*PrepareNativeModelDraw(device,depth_only));SetModelDrawShadowPeriod(0);
                Require(shadowed==0.f && ModelDrawShadowMismatches()==before,"T3 depth-only draw under the shadow");
                SetModelDrawLegacyTables(true);const auto legacy_depth=rig.DepthAt(*PrepareNativeModelDraw(device,depth_only));SetModelDrawLegacyTables(false);
                Require(legacy_depth==0.f,"T3 depth-only draw on the legacy tables");}
            {auto input=depth_only;input.depth_stencil.DepthEnable=FALSE;Refuses([&]{PrepareNativeModelDraw(device,input);},"T3 depth-only draw without depth accepted");}
            // Review: the G-buffer envelope's positive and negative cases (formats in order, the depth format).
            {NativeModelDrawInput g;const std::array<DXGI_FORMAT,6> six{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                    DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
                std::copy(six.begin(),six.end(),g.render_target_formats.begin());g.depth_format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
                auto swapped=six;std::swap(swapped[1],swapped[2]);
                Require(EnvelopeMatches(g,ModelPassEnvelope::GBuffer6,six,DXGI_FORMAT_D32_FLOAT_S8X24_UINT) && !EnvelopeMatches(g,ModelPassEnvelope::GBuffer6,swapped,DXGI_FORMAT_D32_FLOAT_S8X24_UINT) &&
                    !EnvelopeMatches(g,ModelPassEnvelope::GBuffer6,six,DXGI_FORMAT_D32_FLOAT) && !EnvelopeMatches(g,ModelPassEnvelope::Forward1,std::span(six).first(1),DXGI_FORMAT_D32_FLOAT_S8X24_UINT),"T2 G-buffer envelope");}
            {auto input=depth_only;input.constants=warm.constants;Refuses([&]{PrepareNativeModelDraw(device,input);},"T3 pixel binding without a pixel programme accepted");}
            {auto input=depth_only;input.render_target_count=1;input.render_target_formats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
                Refuses([&]{PrepareNativeModelDraw(device,input);},"T3 a target under DepthOnly accepted");}
            {auto input=warm;input.program_set=depth_set;input.constants.resize(1);input.srvs.clear();input.samplers.clear();
                Refuses([&]{PrepareNativeModelDraw(device,input);},"T3 Forward1 without a pixel programme accepted");}}
        const std::array<uint16_t,6> shape{1,0,0,2,1,1};const auto shapes=NativeModelRootShapes();
        Require(std::find(shapes.begin(),shapes.end(),shape)!=shapes.end(),"T-P6 root shape");
        const auto created=NativeModelDrawCounters().pso_created;Require(Near(rig.Draw(*PrepareNativeModelDraw(device,warm)),lit),"T-P6 warm image");
        Require(NativeModelDrawCounters().pso_created==created,"T-P6 equal state created a pipeline");
        {auto input=warm;input.blend.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_RED|D3D12_COLOR_WRITE_ENABLE_ALPHA;
            Require(Near(rig.Draw(*PrepareNativeModelDraw(device,input)),{51,0,0,255}),"T-P6 state change image");
            Require(NativeModelDrawCounters().pso_created==created+1,"T-P6 state change shared a pipeline");
            Require(Near(rig.Draw(*PrepareNativeModelDraw(device,warm)),lit),"T-P6 first state after the change");}
        {auto input=warm;input.blend.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_RED|D3D12_COLOR_WRITE_ENABLE_ALPHA;
            Require(Near(Agrees(rig,input),{51,0,0,255}) && Near(Agrees(rig,warm),lit),"T-P6 shadow images");}

        // I-0134 (the cloth's own velocity): a game-like cloth Velo vertex programme (the VC_isEnableCloth branch placing one
        // POSITION by VC_World and VC_PrevWorld) and a Velo-like pixel programme (the NDC delta, TEXCOORD3.x in blue). The native
        // set keeps the pixel programme, adds PREVPOSITION in slot 2 and draws the previous copy's motion; the game's draws none.
        bool cloth_checked{},real_checked{};
        {constexpr const char* game_cloth_vertex=
            "cbuffer cbMatrixWVP:register(b0){float4 VC_MatrixWVP[4];float4 VC_PrevMatrixWVP[4];};"
            "cbuffer cbMatDynParam:register(b2){float4 pad[8];bool VC_isEnableCloth;};"
            "cbuffer cbvModelParam:register(b7){float4 VC_World[3];float4 VC_PrevWorld[3];};"
            "cbuffer cbTexScroll:register(b11){float4 VC_TexScroll_0;};"
            "struct O{float4 p:SV_Position;float4 c:TEXCOORD0;float4 q:TEXCOORD1;float3 n:TEXCOORD2;float2 uv:TEXCOORD3;};"
            "O main(float3 p:POSITION0,int2 uv:TEXCOORD0){const float4 x=float4(p,1);"
            "const float4 w=VC_isEnableCloth?float4(dot(x,VC_World[0]),dot(x,VC_World[1]),dot(x,VC_World[2]),1):x;"
            "const float4 v=VC_isEnableCloth?float4(dot(x,VC_PrevWorld[0]),dot(x,VC_PrevWorld[1]),dot(x,VC_PrevWorld[2]),1):x;O o;"
            "o.p=float4(dot(w,VC_MatrixWVP[0]),dot(w,VC_MatrixWVP[1]),dot(w,VC_MatrixWVP[2]),dot(w,VC_MatrixWVP[3]));o.c=o.p;"
            "o.q=float4(dot(v,VC_PrevMatrixWVP[0]),dot(v,VC_PrevMatrixWVP[1]),dot(v,VC_PrevMatrixWVP[2]),dot(v,VC_PrevMatrixWVP[3]));"
            "o.n=0;o.uv=float2(uv)*VC_TexScroll_0.xy*(1.0/2048.0)+VC_TexScroll_0.zw;return o;}";
        constexpr const char* velo_pixel=
            "float4 main(float4 p:SV_Position,float4 c:TEXCOORD0,float4 q:TEXCOORD1,float3 n:TEXCOORD2,float2 uv:TEXCOORD3):SV_Target"
            "{return float4(saturate(.5+.5*(c.xy/c.w-q.xy/q.w)),uv.x,1);}";
        const auto game_vertex=Compile(ds3content::ShaderStage::Vertex,game_cloth_vertex);const auto game_interface=ds3content::InspectShaderInterface(game_vertex);
        const std::vector<ds3content::ModelVertexElement> layout{{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R16G16_SINT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
        const auto game_set=MakeNativeModelProgramSet(game_vertex,Compile(ds3content::ShaderStage::Pixel,velo_pixel),layout);
        const auto cloth_set=MakeNativeClothVelocitySet(*game_set,game_interface);
        Require(cloth_set && cloth_set->pixel.bytecode==game_set->pixel.bytecode && cloth_set->vertex.bytecode!=game_set->vertex.bytecode &&
            cloth_set->elements.size()==3 && cloth_set->elements[2]==ds3content::ModelVertexElement{"PREVPOSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,2,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            "I-0134 cloth velocity set");
        {auto unread=game_interface;for(auto& constant:unread.constants)for(auto& variable:constant.variables)if(variable.name=="VC_isEnableCloth")variable.flags&=~2u;
            Require(!MakeNativeClothVelocitySet(*game_set,unread),"I-0134 a programme without the cloth branch taken");
            auto moved=game_interface;for(auto& constant:moved.constants)for(auto& variable:constant.variables)if(variable.name=="VC_PrevWorld")variable.byte_offset=64;
            Require(!MakeNativeClothVelocitySet(*game_set,moved),"I-0134 VC_PrevWorld elsewhere taken");
            auto outputs=game_interface;outputs.outputs.pop_back();
            Require(!MakeNativeClothVelocitySet(*game_set,outputs),"I-0134 other outputs taken");
            // A pixel programme that reads TEXCOORD2 (the normal the native programme writes as zero) is refused.
            const auto normal_pixel=Compile(ds3content::ShaderStage::Pixel,
                "float4 main(float4 p:SV_Position,float4 c:TEXCOORD0,float4 q:TEXCOORD1,float3 n:TEXCOORD2,float2 uv:TEXCOORD3):SV_Target"
                "{return float4(saturate(.5+.5*(c.xy/c.w-q.xy/q.w)),uv.x+n.x,1);}");
            Require(!MakeNativeClothVelocitySet(*MakeNativeModelProgramSet(game_vertex,normal_pixel,layout),game_interface),"I-0134 a pixel programme reading TEXCOORD2 taken");
            auto snorm=layout;snorm[1].format=DXGI_FORMAT_R16G16_SNORM;
            Require(!MakeNativeClothVelocitySet(*MakeNativeModelProgramSet(game_vertex,game_set->pixel,snorm),game_interface),"I-0134 a float texture coordinate taken");
            auto slot_one=layout;slot_one[0].input_slot=1;
            Require(!MakeNativeClothVelocitySet(*MakeNativeModelProgramSet(game_vertex,game_set->pixel,slot_one),game_interface),"I-0134 POSITION outside slot 0 taken");}
        // The current copy: a triangle over the target; the previous one 0.5 to the right (NDC); u = 1024 (0.5 once scaled). Its step
        // review's L3: VC_PrevWorld moves 0.25 more to the right, VC_PrevMatrixWVP 0.5 up and VC_TexScroll_0.z adds 0.25, so each
        // input shows in the image: native (0.5 + 0.5 x (-0.75), 0.5 + 0.5 x (-0.5), 0.75) = (32, 64, 191); the game-like
        // programme, the same position twice, (0.5 + 0.5 x (-0.25), 64, 191) = (96, 64, 191).
        struct ClothVertex {float x,y,z;int16_t u,v;};
        const ClothVertex current[3]{{-1,-1,.5f,1024,0},{-1,3,.5f,1024,0},{3,-1,.5f,1024,0}},previous[3]{{-.5f,-1,.5f,1024,0},{-.5f,3,.5f,1024,0},{3.5f,-1,.5f,1024,0}};
        const auto current_buffer=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,sizeof(current),D3D12_RESOURCE_STATE_GENERIC_READ);Write(current_buffer.Get(),current,sizeof(current));
        const auto previous_buffer=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,sizeof(previous),D3D12_RESOURCE_STATE_GENERIC_READ);Write(previous_buffer.Get(),previous,sizeof(previous));
        auto geometry=rig.Geometry();geometry.vertex_resource=current_buffer;geometry.vertices={current_buffer->GetGPUVirtualAddress(),sizeof(current),sizeof(ClothVertex)};
        Require(!BindModelPreviousStream(geometry,previous_buffer),"I-0134 a previous stream bound to authored storage");
        geometry.stream_generation=7;geometry.stream_owner=std::make_shared<int>(0);
        Require(BindModelPreviousStream(geometry,previous_buffer) && geometry.previous.StrideInBytes==sizeof(ClothVertex),"I-0134 previous stream");
        const auto identity=Floats({1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1, 1,0,0,0, 0,1,0,.5f, 0,0,1,0, 0,0,0,1});
        Write(rig.frame_block.Get(),identity.data(),identity.size());   // VS b0: VC_MatrixWVP and VC_PrevMatrixWVP the identity
        std::vector<float> matdyn(36,0.f);matdyn[32]=1.f;std::vector<uint8_t> matdyn_bytes(matdyn.size()*4);std::memcpy(matdyn_bytes.data(),matdyn.data(),matdyn_bytes.size());
        auto cloth_input=Input(rig,game_set,bank);cloth_input.geometry=geometry;cloth_input.srvs.clear();cloth_input.samplers.clear();
        cloth_input.constants={{ds3content::ShaderStage::Vertex,0,identity,{},true},{ds3content::ShaderStage::Vertex,2,matdyn_bytes},
            {ds3content::ShaderStage::Vertex,7,Floats({1,0,0,0, 0,1,0,0, 0,0,1,0, 1,0,0,.25f, 0,1,0,0, 0,0,1,0})},{ds3content::ShaderStage::Vertex,11,Floats({1,1,.25f,0})}};
        const auto game_pixel=rig.Draw(*PrepareNativeModelDraw(device,cloth_input));
        cloth_input.program_set=cloth_set;const auto native_pixel=rig.Draw(*PrepareNativeModelDraw(device,cloth_input));
        {auto unbound=cloth_input;unbound.geometry.previous={};unbound.geometry.previous_resource.Reset();
            Refuses([&]{PrepareNativeModelDraw(device,unbound);},"I-0134 slot 2 read without a previous stream");}
        // The final review's M2: against the game's own programme. With the programme cache (argv[2], the I-0063 cache, read in place,
        // never copied), SAT_Equip_cloth_Velo_Skin (the run's cape) draws the same triangle, every input its signature declares, the
        // same constants at its declared offsets; the native set with the current copy as its previous one must draw its image, and
        // with the previous copy the native image above. Without the cache: not checked (said in the PASS line).
        if(argc>2){
            std::ifstream file(std::string(argv[2])+"/SAT_Equip_cloth__SAT_Equip_cloth_Velo_Skin.vpo",std::ios::binary);
            const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};Require(!bytes.empty(),"I-0134 the game's cloth Velo programme read");
            const ds3content::CpuShaderProgram real{0,"SAT_Equip_cloth_Velo_Skin",ds3content::ShaderStage::Vertex,5,0,bytes};const auto real_interface=ds3content::InspectShaderInterface(real);
            struct RealVertex {float x,y,z;uint8_t normal[4],tangent[4],binormal[4],indices[4];float weights[4];int16_t uv[4];float uv1[4];};
            const auto real_vertex=[](const ClothVertex& v){RealVertex r{v.x,v.y,v.z,{127,127,255,0},{255,127,127,0},{0,0,0,0},{0,0,0,0},{1,0,0,0},{v.u,v.v,0,0},{0,0,0,0}};return r;};
            const RealVertex real_current[3]{real_vertex(current[0]),real_vertex(current[1]),real_vertex(current[2])},real_previous[3]{real_vertex(previous[0]),real_vertex(previous[1]),real_vertex(previous[2])};
            const auto real_current_buffer=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,sizeof(real_current),D3D12_RESOURCE_STATE_GENERIC_READ);Write(real_current_buffer.Get(),real_current,sizeof(real_current));
            const auto real_previous_buffer=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,sizeof(real_previous),D3D12_RESOURCE_STATE_GENERIC_READ);Write(real_previous_buffer.Get(),real_previous,sizeof(real_previous));
            const std::vector<ds3content::ModelVertexElement> real_layout{{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
                {"NORMAL",0,DXGI_FORMAT_R8G8B8A8_UINT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"TANGENT",0,DXGI_FORMAT_R8G8B8A8_UINT,0,16,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
                {"BINORMAL",0,DXGI_FORMAT_R8G8B8A8_UINT,0,20,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"BLENDINDICES",0,DXGI_FORMAT_R8G8B8A8_UINT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
                {"BLENDWEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,28,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R16G16B16A16_SINT,0,44,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
                {"TEXCOORD",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,52,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
            const auto real_set=MakeNativeModelProgramSet(real,game_set->pixel,real_layout);const auto real_cloth=MakeNativeClothVelocitySet(*real_set,real_interface);
            Require(bool(real_cloth),"I-0134 the native set made for the game's cloth Velo programme");
            // Each constant buffer the programme declares, zero at its declared size, the variables written by name.
            auto real_input=cloth_input;real_input.constants.clear();
            const auto put=[&](std::vector<uint8_t>& block,const ds3content::ShaderConstantDeclaration& buffer,const char* name,std::initializer_list<float> values){
                for(const auto& v:buffer.variables)if(v.name==name){Require(v.byte_offset+values.size()*4<=block.size(),"I-0134 a test value past its buffer");
                    std::memcpy(block.data()+v.byte_offset,values.begin(),values.size()*4);return;}
                throw std::runtime_error("I-0134 a variable missing from the game's programme");};
            for(const auto& resource:real_interface.resources){if(resource.input_type)continue;
                const ds3content::ShaderConstantDeclaration* buffer{};for(const auto& c:real_interface.constants)if(c.name==resource.name)buffer=&c;Require(buffer,"I-0134 a declared buffer");
                std::vector<uint8_t> block(buffer->declared_bytes,0);
                if(buffer->name=="cbMatrixWVP"){std::memcpy(block.data(),identity.data(),identity.size());}
                else if(buffer->name=="cbMatDynParam"){const uint32_t one=1;for(const auto& v:buffer->variables)if(v.name=="VC_isEnableCloth")std::memcpy(block.data()+v.byte_offset,&one,4);}
                else if(buffer->name=="cbvModelParam"){put(block,*buffer,"VC_World",{1,0,0,0, 0,1,0,0, 0,0,1,0});put(block,*buffer,"VC_PrevWorld",{1,0,0,.25f, 0,1,0,0, 0,0,1,0});}
                else if(buffer->name=="cbTexScroll")put(block,*buffer,"VC_TexScroll_0",{1,1,.25f,0});
                real_input.constants.push_back({ds3content::ShaderStage::Vertex,resource.shader_register,std::move(block),{},resource.shader_register==0});}
            auto real_geometry=geometry;real_geometry.vertex_resource=real_current_buffer;real_geometry.vertices={real_current_buffer->GetGPUVirtualAddress(),sizeof(real_current),sizeof(RealVertex)};
            real_geometry.previous={};real_geometry.previous_resource.Reset();
            real_input.geometry=real_geometry;real_input.program_set=real_set;const auto real_pixel=rig.Draw(*PrepareNativeModelDraw(device,real_input));
            Require(BindModelPreviousStream(real_geometry,real_current_buffer),"I-0134 the real layout's current copy as slot 2");
            real_input.geometry=real_geometry;real_input.program_set=real_cloth;const auto same_pixel=rig.Draw(*PrepareNativeModelDraw(device,real_input));
            real_geometry.previous={};real_geometry.previous_resource.Reset();Require(BindModelPreviousStream(real_geometry,real_previous_buffer),"I-0134 the real layout's previous copy");
            real_input.geometry=real_geometry;const auto moved_pixel=rig.Draw(*PrepareNativeModelDraw(device,real_input));
            Require(Near(real_pixel,{96,64,191,255}) && Near(same_pixel,{96,64,191,255}),"I-0134 the native set equals the game's programme with no motion of its own");
            Require(Near(moved_pixel,{32,64,191,255}),"I-0134 the native set on the game's layout draws the previous copy's motion");real_checked=true;}
        const float zero[4]{};Write(rig.frame_block.Get(),zero,sizeof(zero));
        Require(Near(game_pixel,{96,64,191,255}),"I-0134 the game-like programme's image (the frames' motion only)");
        Require(Near(native_pixel,{32,64,191,255}),"I-0134 the native programme's image (the previous copy's motion)");cloth_checked=true;}

        // Device epoch: another device drops the set's values (programme entries, pipelines, sampler tables); its geometry
        // and resources are refused on the first device and accepted on their own.
        bool epoch_checked{};
        if(const auto other_adapter=warp?hardware_adapter:warp_adapter){
            Rig foreign(other_adapter.Get());
            {auto input=warm;input.geometry=foreign.Geometry();Refuses([&]{PrepareNativeModelDraw(device,input);},"T-P6 foreign-device geometry accepted");}
            Require(Near(foreign.Draw(*PrepareNativeModelDraw(foreign.device.Get(),Input(foreign,set,bank))),lit),"T-P5 other device image");
            Require(Near(rig.Draw(*PrepareNativeModelDraw(device,warm)),lit),"T-P5 first device again");
            Require(Near(Agrees(foreign,Input(foreign,set,bank)),lit) && Near(Agrees(rig,warm),lit),"T-P5 shadow images across a device change");
            Require(foreign.DebugErrors()==0,"debug layer errors on the other device");epoch_checked=true;}

        // T-P5: a full sampler cache (I-0074: tables of the extent their bindings reach in the 2048-sampler heap) still draws,
        // on the per-draw tables; every 256th filler and the last ones are drawn.
        const auto legacy=NativeModelDrawCounters().legacy_table_draws;
        for(int index=0;index<2100;++index){Bank filler=bank;filler.generation=10+index;filler.sampler=Sampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP);filler.sampler.MaxLOD=16.0f+float(index);   // distinct, within the bias bounds
            const auto draw=PrepareNativeModelDraw(device,Input(rig,set,filler));
            if(index%256==0 || index>=2090)Require(Near(rig.Draw(*draw),lit),"T-P5 sampler cache filling image");}
        Require(NativeModelDrawCounters().legacy_table_draws>legacy,"T-P5 full sampler cache did not fall back to per-draw tables");
        Require(Near(Agrees(rig,Input(rig,set,bank)),lit),"T-P5 shadow with a full cache");
        Require(rig.DebugErrors()==0,"debug layer errors");
        std::printf("PASS model draw %s: W0a envelopes (count agreement, two targets, depth only without a pixel programme), C4-2a bounded draw re-encoded per finished frame, C4-2 two table copies re-encoded with a frame in flight, I-0090 prepared state reused by a rebuilt draw (new payload drawn, frames kept, shadow, changes prepared in full), T-P3 shared payload image equals the per-draw copy, kept alive by its draws and released with the last, "
            "short payload refused; T-P5 sampler tables by bank version (wrap/border images), other bank, shadow detects a stale table, "
            "full cache falls back to per-draw tables%s; T-P6 missing/short/duplicate bindings refused with the set warm, root shape, "
            "pipeline by state words; %sD3D12 validation\n",argv[1],epoch_checked?", device epoch and foreign-device geometry refused":" (no second adapter: epoch not checked)",
            cloth_checked?(real_checked?"I-0134 cloth Velo set (the previous copy's motion drawn, the game-like programme's none, slot 2 refused unbound; against the game's SAT_Equip_cloth_Velo_Skin: equal without motion, the motion drawn); ":
                "I-0134 cloth Velo set (the previous copy's motion drawn, the game-like programme's none, slot 2 refused unbound; the game's programme not checked: no cache); "):"");
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL %s\n",error.what());return 1;}
}
