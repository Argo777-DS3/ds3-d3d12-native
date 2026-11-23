#include "tonemap_stage.h"
#include "backend.h"
#include <cfloat>
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
// The vertex of vs_2D_Tex2D5 (stride 48, the original's declaration): POSITION and TEXCOORD0..4, float2 each.
struct Vertex {float position[2],uv[5][2];};
static_assert(sizeof(Vertex)==48);
// The four samplers as the original binds them for this draw (i66-reference-1643, PSGetSamplers): s0 point, clamp; s1 trilinear,
// clamp; s2 point, clamp; s3 point, wrap (the dither repeat); MaxLOD 0 for all. (The host's recorded modes 0, 15, 5, 13 read
// through YEBIS-NATIVE 5.2's table give border for s0 and wrap for s1: that table or the record is wrong for these stages.)
D3D12_STATIC_SAMPLER_DESC Sampler(uint32_t reg,D3D12_FILTER filter,D3D12_TEXTURE_ADDRESS_MODE address){
    D3D12_STATIC_SAMPLER_DESC s{};s.Filter=filter;s.AddressU=s.AddressV=address;s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;s.BorderColor=D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    s.MinLOD=0;s.MaxLOD=0;s.ShaderRegister=reg;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;return s;}
}
bool TonemapStage::Open(ID3D12Device* device,const TonemapBlobs& blobs,const std::array<uint8_t,64>& dither,DXGI_FORMAT target_format,
    uint32_t width,uint32_t height,std::string& reason){
    Close();
    try{
        if(!device || blobs.vertex.empty() || blobs.pixel.empty() || target_format==DXGI_FORMAT_UNKNOWN || !width || !height)
            throw std::invalid_argument("tone-map stage inputs");
        device_=device;width_=width;height_=height;
        // Root: b0 $Globals (both stages read it: the VS its position scale), t0..t3 (one table), s0..s3 static.
        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
        const D3D12_DESCRIPTOR_RANGE textures{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0};
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&textures};
        parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_STATIC_SAMPLER_DESC samplers[4]{Sampler(0,D3D12_FILTER_MIN_MAG_MIP_POINT,D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
            Sampler(1,D3D12_FILTER_MIN_MAG_MIP_LINEAR,D3D12_TEXTURE_ADDRESS_MODE_CLAMP),Sampler(2,D3D12_FILTER_MIN_MAG_MIP_POINT,D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
            Sampler(3,D3D12_FILTER_MIN_MAG_MIP_POINT,D3D12_TEXTURE_ADDRESS_MODE_WRAP)};
        const D3D12_ROOT_SIGNATURE_DESC root{2,parameters,4,samplers,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS};
        ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"tone-map root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"tone-map root");
        D3D12_INPUT_ELEMENT_DESC layout[6]{{"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
        for(UINT i=0;i<5;++i)layout[i+1]={"TEXCOORD",i,DXGI_FORMAT_R32G32_FLOAT,0,8+8*i,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0};
        // YEBIS-NATIVE 5.2, render state 0: opaque, RGBA written; rasterizer solid, cull none, front counter-clockwise, depth clip
        // off; depth and stencil off.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.InputLayout={layout,6};
        pso.VS={blobs.vertex.data(),blobs.vertex.size()};pso.PS={blobs.pixel.data(),blobs.pixel.size()};
        auto& target=pso.BlendState.RenderTarget[0];target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
        target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
        pso.SampleMask=UINT_MAX;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
        pso.RasterizerState.FrontCounterClockwise=TRUE;pso.RasterizerState.DepthClipEnable=FALSE;
        pso.DepthStencilState.DepthEnable=FALSE;pso.DepthStencilState.StencilEnable=FALSE;
        pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=target_format;pso.SampleDesc.Count=1;
        Check(Pipelines().Graphics(device,pso,pso_.ReleaseAndGetAddressOf()),"tone-map pipeline");
        if(!blobs.pixel_vignette.empty()){pso.PS={blobs.pixel_vignette.data(),blobs.pixel_vignette.size()};
            if(FAILED(Pipelines().Graphics(device,pso,pso_vignette_.ReleaseAndGetAddressOf())))pso_vignette_.Reset();}   // optional: the stage keeps the plain variant (HasVignette false)
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv_)),"tone-map SRV heap");
        srv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const auto slot=[&](uint32_t i){auto h=srv_->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(srv_stride_)*i;return h;};
        // t1 and t2: null views (sample 0).
        D3D12_SHADER_RESOURCE_VIEW_DESC null{};null.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;null.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        null.Texture2D.MipLevels=1;null.Format=DXGI_FORMAT_R11G11B10_FLOAT;device->CreateShaderResourceView(nullptr,&null,slot(1));
        null.Format=DXGI_FORMAT_R8G8B8A8_UNORM;device->CreateShaderResourceView(nullptr,&null,slot(2));
        // t3: the 4x4 dither texture, filled from an upload buffer by the first Record.
        D3D12_HEAP_PROPERTIES defaults{};defaults.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC texture{};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=4;texture.Height=4;
        texture.DepthOrArraySize=texture.MipLevels=1;texture.Format=DXGI_FORMAT_R8G8B8A8_UNORM;texture.SampleDesc.Count=1;
        Check(CountedResource(device)->CreateCommittedResource(&defaults,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&dither_)),"tone-map dither");
        device->CreateShaderResourceView(dither_.Get(),nullptr,slot(3));
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=4*D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;buffer.Height=1;
        buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Check(CountedResource(device)->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&dither_upload_)),"tone-map dither upload");
        void* memory{};const D3D12_RANGE none{};Check(dither_upload_->Map(0,&none,&memory),"tone-map dither map");
        for(uint32_t row=0;row<4;++row)std::memcpy(static_cast<uint8_t*>(memory)+row*D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,dither.data()+16*row,16);
        dither_upload_->Unmap(0,nullptr);
        MakeVertices();return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
void TonemapStage::MakeVertices(){
    // The original's three vertices at 1280x720 (Native1633 record 33): positions (-1, 1), (-1, -3), (3, 1) scaled by (1, -1) in the
    // VS; uv0 = uv1 (0, 1), (0, -1), (2, 1); uv2 (0, 0); uv4 = uv0 x (width / 4, height / 4), the dither repeat. uv3 (t2's
    // coordinates, which the original jitters) is uv0: a null view samples 0 at any coordinate.
    const float w=float(width_)/4.0f,h=float(height_)/4.0f;
    const float position[3][2]{{-1,1},{-1,-3},{3,1}},uv[3][2]{{0,1},{0,-1},{2,1}};
    Vertex v[3]{};
    for(int i=0;i<3;++i){v[i].position[0]=position[i][0];v[i].position[1]=position[i][1];
        for(int k=0;k<2;++k){v[i].uv[0][k]=v[i].uv[1][k]=v[i].uv[3][k]=uv[i][k];}
        v[i].uv[4][0]=uv[i][0]*w;v[i].uv[4][1]=uv[i][1]*h;}
    D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=sizeof(v);buffer.Height=1;
    buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    vertices_.Reset();
    Check(CountedResource(device_.Get())->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&vertices_)),"tone-map vertices");
    void* memory{};const D3D12_RANGE none{};Check(vertices_->Map(0,&none,&memory),"tone-map vertices map");
    std::memcpy(memory,v,sizeof(v));vertices_->Unmap(0,nullptr);std::memcpy(vertex_words_.data(),v,sizeof(v));
    scene_view_=nullptr;   // the caller's scene image is recreated with the extent: its view is written again at the next frame
}
void TonemapStage::Resize(uint32_t width,uint32_t height){
    if(!IsOpen() || !width || !height)throw std::invalid_argument("tone-map stage resize");
    width_=width;height_=height;MakeVertices();
}
void TonemapStage::Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint64_t scene_generation,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,
    const uint8_t* globals,const TonemapConstantSink& constants,ID3D12Resource* glare,const std::array<std::array<float,2>,3>* vignette,uint64_t glare_generation,
    ID3D12Resource* adapted){
    if(vignette && !pso_vignette_)throw std::logic_error("tone-map vignette without its programme");
    if(!IsOpen() || !list || !scene || !target_rtv.ptr || !globals || !constants)throw std::logic_error("tone-map pass before the stage");
    if(!dither_ready_){
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=dither_.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=dither_upload_.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,4,4,1,D3D12_TEXTURE_DATA_PITCH_ALIGNMENT};
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=dither_.Get();
        barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&barrier);dither_ready_=true;}
    if(scene!=scene_view_ || scene_generation!=scene_view_generation_ || glare!=glare_view_ || glare_generation!=glare_view_generation_ || adapted!=adapted_view_){
        // Optimisation review axis 2: the four views (t0 scene, t1 glare or null, t2 null, t3 dither) into a fresh heap, the former
        // heap retired by ticket (the frames in flight that read it keep it): a glare surface or scene change drains nothing.
        ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,4,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device_.Get())->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&fresh)),"tone-map SRV heap");
        const auto at=[&](uint32_t i){auto h=fresh->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(srv_stride_)*i;return h;};
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;device_->CreateShaderResourceView(scene,&view,at(0));
        view.Format=DXGI_FORMAT_R11G11B10_FLOAT;device_->CreateShaderResourceView(glare,&view,at(1));   // null when there is no glare (samples 0)
        // t2: null (its factor 1 through the Offset words), or I-0132 R11h's automatic exposure factor.
        view.Format=adapted?DXGI_FORMAT_R32G32B32A32_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;device_->CreateShaderResourceView(adapted,&view,at(2));
        device_->CreateShaderResourceView(dither_.Get(),nullptr,at(3));
        if(srv_)RetireAfterCurrentRecording(HoldObject(srv_.Get()));srv_=fresh;
        scene_view_=scene;scene_view_generation_=scene_generation;glare_view_=glare;glare_view_generation_=glare_generation;adapted_view_=adapted;}
    const auto memory=constants(GlobalsBytes);
    if(!memory.cpu || !memory.gpu || memory.gpu%256)throw std::runtime_error("tone-map constants");
    std::memcpy(memory.cpu,globals,GlobalsBytes);
    // I-0132 R11h (R11-PLAN section 9): with the factor bound, the programme's t2 term m = t2 x Offset[2].x + Offset[2].y = t2, kept whole by
    // Offset[3].x = 1 (its desaturation towards the mean of m), x Offset[2].z + Offset[2].w = t2: the scene's and the glare's factor.
    if(adapted){const float words[5]{1.0f,0.0f,1.0f,0.0f,1.0f};std::memcpy(static_cast<uint8_t*>(memory.cpu)+4*752,words,sizeof words);}
    ID3D12DescriptorHeap* heaps[]{srv_.Get()};list->SetDescriptorHeaps(1,heaps);list->SetGraphicsRootSignature(root_.Get());
    list->SetPipelineState(vignette?pso_vignette_.Get():pso_.Get());list->SetGraphicsRootConstantBufferView(0,memory.gpu);
    list->SetGraphicsRootDescriptorTable(1,srv_->GetGPUDescriptorHandleForHeapStart());
    D3D12_VERTEX_BUFFER_VIEW view{vertices_->GetGPUVirtualAddress(),3*sizeof(Vertex),sizeof(Vertex)};
    if(vignette){   // TM-3: this frame's vertices (the static ones with their TEXCOORD2) in the frame's upload memory
        const auto frame_vertices=constants(256);if(!frame_vertices.cpu || !frame_vertices.gpu)throw std::runtime_error("tone-map vignette vertices");
        auto words=vertex_words_;for(size_t i=0;i<3;++i){words[i*12+6]=(*vignette)[i][0];words[i*12+7]=(*vignette)[i][1];}
        std::memcpy(frame_vertices.cpu,words.data(),sizeof(words));view.BufferLocation=frame_vertices.gpu;}
    list->IASetVertexBuffers(0,1,&view);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    list->OMSetRenderTargets(1,&target_rtv,FALSE,nullptr);
    const D3D12_VIEWPORT viewport{0,0,float(width_),float(height_),0,1};const D3D12_RECT scissor{0,0,LONG(width_),LONG(height_)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->DrawInstanced(3,1,0,0);++frames_;
}
void TonemapStage::Close() noexcept {
    root_.Reset();pso_.Reset();pso_vignette_.Reset();srv_.Reset();vertices_.Reset();dither_.Reset();dither_upload_.Reset();device_.Reset();
    scene_view_=nullptr;scene_view_generation_=0;glare_view_=nullptr;glare_view_generation_=0;adapted_view_=nullptr;dither_ready_=false;width_=height_=0;
}
void TonemapStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)pso_.Detach();(void)pso_vignette_.Detach();(void)srv_.Detach();(void)vertices_.Detach();(void)dither_.Detach();(void)dither_upload_.Detach();(void)device_.Detach();
}
}
