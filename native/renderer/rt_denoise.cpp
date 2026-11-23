// I-0126 R7c (R7-PLAN §7): NRD's REBLUR_DIFFUSE_SPECULAR (I-0132 R11: the specular with the GI) recorded with the engine's own D3D12 objects (rt_denoise.h). This software contains
// source code provided by NVIDIA Corporation (NRD 4.17.3, linked statically; its programmes compiled by the module's build).
#include "rt_denoise.h"
#include "NRD.h"
#include "nrd_shaders.h"   // Build-GameBootstrap.ps1: nrd_programmes[], NRD's programmes by shaderIdentifier
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace ds3native {
namespace {
using Microsoft::WRL::ComPtr;
constexpr nrd::Identifier kDenoiser=0;   // REBLUR_DIFFUSE_SPECULAR, the instance's one denoiser
// The ring of shader-visible descriptors: three frames' worth (two in flight, D-0187), each frame's dispatches from its third.
constexpr uint32_t kRingThirds=3,kRingThird=2048;
void Check(HRESULT status,const char* what) {
    if(FAILED(status)){char message[160]{};sprintf_s(message,"%s: HRESULT %08lX",what,static_cast<unsigned long>(status));throw std::runtime_error(message);}
}
DXGI_FORMAT FormatOf(nrd::Format format) {
    using F=nrd::Format;
    switch(format){
        case F::R8_UNORM:return DXGI_FORMAT_R8_UNORM;case F::R8_SNORM:return DXGI_FORMAT_R8_SNORM;case F::R8_UINT:return DXGI_FORMAT_R8_UINT;case F::R8_SINT:return DXGI_FORMAT_R8_SINT;
        case F::RG8_UNORM:return DXGI_FORMAT_R8G8_UNORM;case F::RG8_SNORM:return DXGI_FORMAT_R8G8_SNORM;case F::RG8_UINT:return DXGI_FORMAT_R8G8_UINT;case F::RG8_SINT:return DXGI_FORMAT_R8G8_SINT;
        case F::RGBA8_UNORM:return DXGI_FORMAT_R8G8B8A8_UNORM;case F::RGBA8_SNORM:return DXGI_FORMAT_R8G8B8A8_SNORM;case F::RGBA8_UINT:return DXGI_FORMAT_R8G8B8A8_UINT;
        case F::RGBA8_SINT:return DXGI_FORMAT_R8G8B8A8_SINT;case F::RGBA8_SRGB:return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case F::R16_UNORM:return DXGI_FORMAT_R16_UNORM;case F::R16_SNORM:return DXGI_FORMAT_R16_SNORM;case F::R16_UINT:return DXGI_FORMAT_R16_UINT;case F::R16_SINT:return DXGI_FORMAT_R16_SINT;
        case F::R16_SFLOAT:return DXGI_FORMAT_R16_FLOAT;
        case F::RG16_UNORM:return DXGI_FORMAT_R16G16_UNORM;case F::RG16_SNORM:return DXGI_FORMAT_R16G16_SNORM;case F::RG16_UINT:return DXGI_FORMAT_R16G16_UINT;
        case F::RG16_SINT:return DXGI_FORMAT_R16G16_SINT;case F::RG16_SFLOAT:return DXGI_FORMAT_R16G16_FLOAT;
        case F::RGBA16_UNORM:return DXGI_FORMAT_R16G16B16A16_UNORM;case F::RGBA16_SNORM:return DXGI_FORMAT_R16G16B16A16_SNORM;case F::RGBA16_UINT:return DXGI_FORMAT_R16G16B16A16_UINT;
        case F::RGBA16_SINT:return DXGI_FORMAT_R16G16B16A16_SINT;case F::RGBA16_SFLOAT:return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case F::R32_UINT:return DXGI_FORMAT_R32_UINT;case F::R32_SINT:return DXGI_FORMAT_R32_SINT;case F::R32_SFLOAT:return DXGI_FORMAT_R32_FLOAT;
        case F::RG32_UINT:return DXGI_FORMAT_R32G32_UINT;case F::RG32_SINT:return DXGI_FORMAT_R32G32_SINT;case F::RG32_SFLOAT:return DXGI_FORMAT_R32G32_FLOAT;
        case F::RGB32_UINT:return DXGI_FORMAT_R32G32B32_UINT;case F::RGB32_SINT:return DXGI_FORMAT_R32G32B32_SINT;case F::RGB32_SFLOAT:return DXGI_FORMAT_R32G32B32_FLOAT;
        case F::RGBA32_UINT:return DXGI_FORMAT_R32G32B32A32_UINT;case F::RGBA32_SINT:return DXGI_FORMAT_R32G32B32A32_SINT;case F::RGBA32_SFLOAT:return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case F::R10_G10_B10_A2_UNORM:return DXGI_FORMAT_R10G10B10A2_UNORM;case F::R10_G10_B10_A2_UINT:return DXGI_FORMAT_R10G10B10A2_UINT;
        case F::R11_G11_B10_UFLOAT:return DXGI_FORMAT_R11G11B10_FLOAT;case F::R9_G9_B9_E5_UFLOAT:return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        default:throw std::invalid_argument("nrd format");}
}
ComPtr<ID3D12Resource> NewTexture(ID3D12Device* device,uint32_t width,uint32_t height,DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,const char* what) {
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;desc.DepthOrArraySize=1;desc.MipLevels=1;
    desc.Format=format;desc.SampleDesc.Count=1;desc.Flags=flags;
    D3D12_CLEAR_VALUE clear{};clear.Format=format;   // the inputs' clear colour: zero
    ComPtr<ID3D12Resource> result;
    Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,(flags&D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)?&clear:nullptr,IID_PPV_ARGS(&result)),what);
    return result;
}
}

RtDenoiser::~RtDenoiser() {Close();}

void RtDenoiser::Close() noexcept {
    if(instance_){nrd::DestroyInstance(*instance_);instance_=nullptr;}settings_set_=false;
    root_.Reset();pipelines_.clear();permanent_.clear();transient_.clear();
    for(auto& input:inputs_)input.Reset();for(auto& output:outputs_)output.Reset();views_.Reset();ring_.Reset();rtvs_.Reset();
    width_=height_=0;frame_index_=0;device_.Reset();
}

// The root (NRD's layout: a root CBV and its static samplers in one space, the textures' SRV and UAV tables in the other) and one
// compute pipeline a pipeline NRD describes, its DXIL found by shaderIdentifier.
void RtDenoiser::MakePipelines(std::string& reason) {
    const auto& desc=*nrd::GetInstanceDesc(*instance_);
    srv_max_=std::max(desc.descriptorPoolDesc.perSetTexturesMaxNum,1u);uav_max_=std::max(desc.descriptorPoolDesc.perSetStorageTexturesMaxNum,1u);
    for(uint32_t i=0;i<desc.pipelinesNum;++i){const auto& p=desc.pipelines[i];uint32_t srvs=0,uavs=0;
        for(uint32_t r=0;r<p.resourceRangesNum;++r)(p.resourceRanges[r].descriptorType==nrd::DescriptorType::TEXTURE?srvs:uavs)+=p.resourceRanges[r].descriptorsNum;
        srv_max_=std::max(srv_max_,srvs);uav_max_=std::max(uav_max_,uavs);}
    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[0].NumDescriptors=srv_max_;ranges[0].BaseShaderRegister=desc.resourcesBaseRegisterIndex;ranges[0].RegisterSpace=desc.resourcesSpaceIndex;
    ranges[1].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_UAV;ranges[1].NumDescriptors=uav_max_;ranges[1].BaseShaderRegister=desc.resourcesBaseRegisterIndex;ranges[1].RegisterSpace=desc.resourcesSpaceIndex;
    D3D12_ROOT_PARAMETER params[3]{};
    params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[0].Descriptor.ShaderRegister=desc.constantBufferRegisterIndex;params[0].Descriptor.RegisterSpace=desc.constantBufferAndSamplersSpaceIndex;
    for(uint32_t i=0;i<2;++i){params[1+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[1+i].DescriptorTable.NumDescriptorRanges=1;params[1+i].DescriptorTable.pDescriptorRanges=&ranges[i];}
    std::vector<D3D12_STATIC_SAMPLER_DESC> samplers(desc.samplersNum);
    for(uint32_t i=0;i<desc.samplersNum;++i){auto& s=samplers[i];
        s.Filter=desc.samplers[i]==nrd::Sampler::LINEAR_CLAMP?D3D12_FILTER_MIN_MAG_MIP_LINEAR:D3D12_FILTER_MIN_MAG_MIP_POINT;
        s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.MaxLOD=D3D12_FLOAT32_MAX;s.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;
        s.ShaderRegister=desc.samplersBaseRegisterIndex+i;s.RegisterSpace=desc.constantBufferAndSamplersSpaceIndex;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;}
    D3D12_ROOT_SIGNATURE_DESC root{3,params,UINT(samplers.size()),samplers.data(),D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;
    Check(D3D12SerializeRootSignature(&root,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"nrd root serialize");
    Check(Pipelines().Root(device_.Get(),blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"nrd root");
    pipelines_.resize(desc.pipelinesNum);
    for(uint32_t i=0;i<desc.pipelinesNum;++i){
        const char* const id=desc.pipelines[i].shaderIdentifier;
        const auto found=std::find_if(std::begin(nrd_programmes),std::end(nrd_programmes),[&](const NrdProgramme& p){return std::strcmp(p.identifier,id)==0;});
        if(found==std::end(nrd_programmes)){reason=std::string("nrd programme not built: ")+id;throw std::runtime_error(reason);}
        D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root_.Get();pso.CS={found->bytes,found->size};
        Check(Pipelines().Compute(device_.Get(),pso,pipelines_[i].ReleaseAndGetAddressOf()),"nrd pipeline");}
}

// The pools at their downsampled sizes (NRD's DIV_UP), the inputs (render targets and shader-readable) and the outputs, their SRV
// and UAV in the views heap (two a texture: the inputs, the outputs, the permanent pool, the transient pool), the inputs' RTVs.
void RtDenoiser::MakeTextures(uint32_t width,uint32_t height) {
    const auto& desc=*nrd::GetInstanceDesc(*instance_);
    auto* const device=device_.Get();const auto uav=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    std::array<ComPtr<ID3D12Resource>,kRtDenoiseInputs> inputs;std::array<ComPtr<ID3D12Resource>,kRtDenoiseOutputs> outputs;
    for(uint32_t i=0;i<kRtDenoiseInputs;++i)inputs[i]=NewTexture(device,width,height,kRtDenoiseInputFormats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET|uav,D3D12_RESOURCE_STATE_RENDER_TARGET,"nrd input");
    for(auto& output:outputs)output=NewTexture(device,width,height,kRtDenoiseOutputFormat,uav,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,"nrd output");
    const auto pool=[&](const nrd::TextureDesc* textures,uint32_t count){std::vector<Texture> made(count);
        for(uint32_t i=0;i<count;++i){const uint32_t f=std::max<uint32_t>(textures[i].downsampleFactor,1);
            made[i]={NewTexture(device,(width+f-1)/f,(height+f-1)/f,FormatOf(textures[i].format),uav,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,"nrd pool"),D3D12_RESOURCE_STATE_UNORDERED_ACCESS};}
        return made;};
    auto permanent=pool(desc.permanentPool,desc.permanentPoolSize);auto transient=pool(desc.transientPool,desc.transientPoolSize);
    const uint32_t textures=kRtDenoiseInputs+kRtDenoiseOutputs+desc.permanentPoolSize+desc.transientPoolSize;
    ComPtr<ID3D12DescriptorHeap> views,rtvs;
    {D3D12_DESCRIPTOR_HEAP_DESC h{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2*textures,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};Check(CountedHeap(device)->CreateDescriptorHeap(&h,IID_PPV_ARGS(&views)),"nrd views");}
    {D3D12_DESCRIPTOR_HEAP_DESC h{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,kRtDenoiseInputs,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};Check(CountedHeap(device)->CreateDescriptorHeap(&h,IID_PPV_ARGS(&rtvs)),"nrd rtvs");}
    view_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);rtv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    uint32_t at=0;
    const auto views_of=[&](ID3D12Resource* r){const auto d=r->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC s{};s.Format=d.Format;s.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;s.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;s.Texture2D.MipLevels=1;
        D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=d.Format;u.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        auto cpu=views->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=size_t(at)*view_stride_;device->CreateShaderResourceView(r,&s,cpu);
        cpu.ptr+=view_stride_;device->CreateUnorderedAccessView(r,nullptr,&u,cpu);at+=2;};
    for(auto& input:inputs)views_of(input.Get());
    for(auto& output:outputs)views_of(output.Get());for(auto& t:permanent)views_of(t.resource.Get());for(auto& t:transient)views_of(t.resource.Get());
    for(uint32_t i=0;i<kRtDenoiseInputs;++i){auto cpu=rtvs->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=size_t(i)*rtv_stride_;device->CreateRenderTargetView(inputs[i].Get(),nullptr,cpu);}
    inputs_=inputs;input_state_=D3D12_RESOURCE_STATE_RENDER_TARGET;outputs_=outputs;permanent_=std::move(permanent);transient_=std::move(transient);
    views_=views;rtvs_=rtvs;width_=width;height_=height;++generation_;
}

bool RtDenoiser::Open(ID3D12Device* device,uint32_t width,uint32_t height,std::string& reason) {
    if(IsOpen() && width==width_ && height==height_)return true;
    try{
        if(!device || !width || !height || width>UINT16_MAX || height>UINT16_MAX)throw std::invalid_argument("nrd extent");
        if(!instance_){
            device_=device;
            const nrd::DenoiserDesc denoisers[]{{kDenoiser,nrd::Denoiser::REBLUR_DIFFUSE_SPECULAR}};
            nrd::InstanceCreationDesc creation{};creation.denoisers=denoisers;creation.denoisersNum=1;
            nrd::Instance* made{};
            if(nrd::CreateInstance(creation,made)!=nrd::Result::SUCCESS || !made)throw std::runtime_error("nrd instance");
            instance_=made;
            MakePipelines(reason);
            {D3D12_DESCRIPTOR_HEAP_DESC h{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kRingThirds*kRingThird,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
                Check(CountedHeap(device)->CreateDescriptorHeap(&h,IID_PPV_ARGS(&ring_)),"nrd ring");}}
        MakeTextures(width,height);
        frame_index_=0;   // a new size: NRD's history restarts (Record clears it)
        return true;
    }catch(const std::exception& error){if(reason.empty())reason=error.what();Close();return false;}
}

D3D12_CPU_DESCRIPTOR_HANDLE RtDenoiser::InputRtv(uint32_t index) const noexcept {
    if(!rtvs_ || index>=kRtDenoiseInputs)return {};
    auto cpu=rtvs_->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=size_t(index)*rtv_stride_;return cpu;
}

void RtDenoiser::Hold(std::vector<std::shared_ptr<const void>>& kept) const {
    for(const auto& input:inputs_)if(input)kept.push_back(HoldResource(input.Get()));
    for(const auto& output:outputs_)if(output)kept.push_back(HoldResource(output.Get()));
    for(const auto& t:permanent_)kept.push_back(HoldResource(t.resource.Get()));
    for(const auto& t:transient_)kept.push_back(HoldResource(t.resource.Get()));
    if(views_)kept.push_back(HoldObject(views_.Get()));
    if(ring_)kept.push_back(HoldObject(ring_.Get()));   // the step review's L1: a failed resize's Close drops it while a list still binds it
}

bool RtDenoiser::Record(ID3D12GraphicsCommandList* list,const RtDenoiseCamera& camera,const std::function<UploadArena::Range(uint64_t)>& upload,std::string& reason) {
    if(!IsOpen() || !list){reason="nrd record before open";return false;}
    const auto& desc=*nrd::GetInstanceDesc(*instance_);
    // The common settings: the camera this frame and the previous one (NRD keeps no matrices of its own), the motion in pixels.
    nrd::CommonSettings common{};
    static_assert(sizeof common.viewToClipMatrix==sizeof camera.view_to_clip && sizeof common.worldToViewMatrix==sizeof camera.world_to_view);
    std::memcpy(common.viewToClipMatrix,camera.view_to_clip.data(),sizeof common.viewToClipMatrix);
    std::memcpy(common.worldToViewMatrix,camera.world_to_view.data(),sizeof common.worldToViewMatrix);
    const bool restart=camera.restart || frame_index_==0;
    std::memcpy(common.viewToClipMatrixPrev,(restart?camera.view_to_clip:previous_.view_to_clip).data(),sizeof common.viewToClipMatrixPrev);
    std::memcpy(common.worldToViewMatrixPrev,(restart?camera.world_to_view:previous_.world_to_view).data(),sizeof common.worldToViewMatrixPrev);
    common.motionVectorScale[0]=1.0f/float(width_);common.motionVectorScale[1]=1.0f/float(height_);common.motionVectorScale[2]=0;
    common.cameraJitter[0]=camera.jitter[0];common.cameraJitter[1]=camera.jitter[1];
    common.cameraJitterPrev[0]=restart?camera.jitter[0]:previous_.jitter[0];common.cameraJitterPrev[1]=restart?camera.jitter[1]:previous_.jitter[1];
    common.resourceSize[0]=common.resourceSizePrev[0]=common.rectSize[0]=common.rectSizePrev[0]=uint16_t(width_);
    common.resourceSize[1]=common.resourceSizePrev[1]=common.rectSize[1]=common.rectSizePrev[1]=uint16_t(height_);
    common.denoisingRange=kRtDenoiseRange;
    common.frameIndex=frame_index_;
    common.accumulationMode=frame_index_==0?nrd::AccumulationMode::CLEAR_AND_RESTART:restart?nrd::AccumulationMode::RESTART:nrd::AccumulationMode::CONTINUE;
    if(nrd::SetCommonSettings(*instance_,common)!=nrd::Result::SUCCESS){reason="nrd common settings";return false;}
    if(!settings_set_){   // constant: given once an instance (the final review's L6)
        nrd::ReblurSettings settings{};
        settings.hitDistanceParameters.A=kRtDenoiseHitDistance[0];settings.hitDistanceParameters.B=kRtDenoiseHitDistance[1];settings.hitDistanceParameters.C=kRtDenoiseHitDistance[2];
        if(nrd::SetDenoiserSettings(*instance_,kDenoiser,&settings)!=nrd::Result::SUCCESS){reason="nrd denoiser settings";return false;}
        settings_set_=true;}
    const nrd::DispatchDesc* dispatches{};uint32_t count{};
    if(nrd::GetComputeDispatches(*instance_,&kDenoiser,1,dispatches,count)!=nrd::Result::SUCCESS){reason="nrd dispatches";return false;}
    // The textures by NRD's resource type: their views' index (two a texture) and their tracked state.
    struct Bound {uint32_t view;D3D12_RESOURCE_STATES* state;ID3D12Resource* resource;};
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS> constant_ranges;   // each dispatch's constants (filled by the checks below)
    std::array<D3D12_RESOURCE_STATES,kRtDenoiseInputs> input_states;input_states.fill(input_state_);
    std::array<D3D12_RESOURCE_STATES,kRtDenoiseOutputs> output_states;output_states.fill(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    constexpr uint32_t pools=2*(kRtDenoiseInputs+kRtDenoiseOutputs);   // the permanent pool's first view (MakeTextures' order)
    const auto bound=[&](const nrd::ResourceDesc& r)->Bound{
        switch(r.type){
            case nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST:return {0,&input_states[0],inputs_[0].Get()};
            case nrd::ResourceType::IN_NORMAL_ROUGHNESS:return {2,&input_states[1],inputs_[1].Get()};
            case nrd::ResourceType::IN_VIEWZ:return {4,&input_states[2],inputs_[2].Get()};
            case nrd::ResourceType::IN_MV:return {6,&input_states[3],inputs_[3].Get()};
            case nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST:return {8,&input_states[4],inputs_[4].Get()};
            case nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST:return {10,&output_states[0],outputs_[0].Get()};
            case nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST:return {12,&output_states[1],outputs_[1].Get()};
            case nrd::ResourceType::PERMANENT_POOL:if(r.indexInPool<permanent_.size())return {pools+2u*r.indexInPool,&permanent_[r.indexInPool].state,permanent_[r.indexInPool].resource.Get()};break;
            case nrd::ResourceType::TRANSIENT_POOL:if(r.indexInPool<transient_.size())return {pools+2u*uint32_t(permanent_.size()+r.indexInPool),&transient_[r.indexInPool].state,transient_[r.indexInPool].resource.Get()};break;
            default:break;}
        throw std::runtime_error(std::string("nrd resource not provided: ")+nrd::GetResourceTypeString(r.type));};
    // Every dispatch checked before anything is recorded (its pipeline, its ranges against its resources and the root's tables, each
    // resource provided, the frame's descriptors within its third of the ring): a refusal records nothing.
    try{uint32_t descriptors=0;
        for(uint32_t d=0;d<count;++d){const auto& dispatch=dispatches[d];
            if(dispatch.pipelineIndex>=pipelines_.size() || dispatch.pipelineIndex>=desc.pipelinesNum)throw std::runtime_error("nrd pipeline index");
            const auto& pipeline=desc.pipelines[dispatch.pipelineIndex];uint32_t srvs=0,uavs=0;
            for(uint32_t r=0;r<pipeline.resourceRangesNum;++r)(pipeline.resourceRanges[r].descriptorType==nrd::DescriptorType::TEXTURE?srvs:uavs)+=pipeline.resourceRanges[r].descriptorsNum;
            if(srvs+uavs!=dispatch.resourcesNum || srvs>srv_max_ || uavs>uav_max_)throw std::runtime_error("nrd dispatch ranges");
            for(uint32_t k=0;k<dispatch.resourcesNum;++k)(void)bound(dispatch.resources[k]);
            descriptors+=srv_max_+uav_max_;}
        if(descriptors>kRingThird)throw std::runtime_error("nrd descriptors");
        // The step review's L2: every dispatch's constants uploaded before anything is recorded (an upload that throws records nothing).
        D3D12_GPU_VIRTUAL_ADDRESS last{};constant_ranges.resize(count);
        for(uint32_t d=0;d<count;++d){const auto& dispatch=dispatches[d];
            if(dispatch.constantBufferDataSize && (!dispatch.constantBufferDataMatchesPreviousDispatch || !last)){
                const auto range=upload((dispatch.constantBufferDataSize+255)&~255u);std::memcpy(range.cpu,dispatch.constantBufferData,dispatch.constantBufferDataSize);last=range.gpu;}
            if(!last){const auto range=upload(256);std::memset(range.cpu,0,256);last=range.gpu;}   // a root CBV always bound
            constant_ranges[d]=last;}
    }catch(const std::exception& error){reason=error.what();return false;}
    ID3D12DescriptorHeap* heaps[]{ring_.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root_.Get());
    const uint32_t third=uint32_t(frames_%kRingThirds);uint32_t next=third*kRingThird;   // the frame's descriptors checked to fit above
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    const auto cpu_ring=ring_->GetCPUDescriptorHandleForHeapStart();const auto gpu_ring=ring_->GetGPUDescriptorHandleForHeapStart();
    const auto cpu_views=views_->GetCPUDescriptorHandleForHeapStart();
    for(uint32_t d=0;d<count;++d){
        const auto& dispatch=dispatches[d];
        // The states this dispatch asks, then one UAV barrier (the previous dispatch's writes seen by this one).
        barriers.clear();
        for(uint32_t k=0;k<dispatch.resourcesNum;++k){const auto& r=dispatch.resources[k];const Bound b=bound(r);
            const auto want=r.descriptorType==nrd::DescriptorType::TEXTURE?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            if(*b.state!=want){D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;t.Transition.pResource=b.resource;
                t.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;t.Transition.StateBefore=*b.state;t.Transition.StateAfter=want;barriers.push_back(t);*b.state=want;}}
        {D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barriers.push_back(u);}
        list->ResourceBarrier(UINT(barriers.size()),barriers.data());
        // The dispatch's descriptors (its SRVs then its UAVs, in its ranges' order) copied into this frame's third of the ring.
        const uint32_t srv_base=next,uav_base=next+srv_max_;uint32_t srv_at=srv_base,uav_at=uav_base;
        for(uint32_t k=0;k<dispatch.resourcesNum;++k){const auto& r=dispatch.resources[k];const Bound b=bound(r);const bool srv=r.descriptorType==nrd::DescriptorType::TEXTURE;
            D3D12_CPU_DESCRIPTOR_HANDLE to=cpu_ring;to.ptr+=size_t(srv?srv_at++:uav_at++)*view_stride_;
            D3D12_CPU_DESCRIPTOR_HANDLE from=cpu_views;from.ptr+=size_t(b.view+(srv?0:1))*view_stride_;
            device_->CopyDescriptorsSimple(1,to,from,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
        next+=srv_max_+uav_max_;
        D3D12_GPU_DESCRIPTOR_HANDLE srv_table=gpu_ring,uav_table=gpu_ring;srv_table.ptr+=uint64_t(srv_base)*view_stride_;uav_table.ptr+=uint64_t(uav_base)*view_stride_;
        list->SetPipelineState(pipelines_[dispatch.pipelineIndex].Get());
        list->SetComputeRootConstantBufferView(0,constant_ranges[d]);list->SetComputeRootDescriptorTable(1,srv_table);list->SetComputeRootDescriptorTable(2,uav_table);
        list->Dispatch(dispatch.gridWidth,dispatch.gridHeight,1);}
    // Back to the between-frame states: the inputs render targets, the outputs shader-readable (the pools stay as NRD left them).
    barriers.clear();
    {D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;barriers.push_back(u);}
    const auto back=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){if(from==to)return;D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        t.Transition.pResource=r;t.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;t.Transition.StateBefore=from;t.Transition.StateAfter=to;barriers.push_back(t);};
    for(uint32_t i=0;i<kRtDenoiseInputs;++i)back(inputs_[i].Get(),input_states[i],D3D12_RESOURCE_STATE_RENDER_TARGET);
    for(uint32_t i=0;i<kRtDenoiseOutputs;++i)back(outputs_[i].Get(),output_states[i],D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    list->ResourceBarrier(UINT(barriers.size()),barriers.data());
    previous_=camera;++frame_index_;++frames_;
    return true;
}
}
