// The scene renderer's velocity family (the Velo rows, the resolve, the half-size velocity), the frame blocks, the slots'
// readbacks and completed frames, and the scene colour copy (scene_pass_internal.h has the shared helpers).
#include "scene_pass_internal.h"
#include "streamline_host.h"
// I-0132 R12h step 2: the programmes built by fxc (shaders/velocity_resolve.hlsl, velocity_half.hlsl, velocity_occlusion.hlsl,
// depth_out.hlsl and cloth_velocity.hlsl).
#include "velocity_resolve_vs.dxbc.h"
#include "velocity_resolve_ps.dxbc.h"
#include "velocity_half_vs.dxbc.h"
#include "velocity_half_ps.dxbc.h"
#include "velocity_occlusion_vs.dxbc.h"
#include "velocity_occlusion_ps.dxbc.h"
#include "depth_out_vs.dxbc.h"
#include "depth_out_ps.dxbc.h"
#include "cloth_velocity_vs.dxbc.h"

namespace ds3native {
// 1186 velocity family. The resolve turns the velocity target into motion vectors: a pixel a Velo row wrote (alpha 1) decodes
// its NDC delta with the rows' FC_DifColMul; any other pixel (alpha 0, the clear) takes the camera motion of its scene depth
// through 13BFC0's current-to-previous screen matrix. Output: previous minus current position, in pixels.
// The programmes: shaders/velocity_resolve.hlsl; the half-size object velocity (shaders/velocity_half.hlsl), the Velo rows'
// occlusion (velocity_occlusion.hlsl), the output-size depth (depth_out.hlsl) and the cloth Velo rows' vertex programme
// (cloth_velocity.hlsl) each carry their own account.
namespace {
bool SameName(std::string_view a,std::string_view b) noexcept {
    return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin(),[](char x,char y){return (x>='a' && x<='z'?x-32:x)==(y>='a' && y<='z'?y-32:y);});}
// A constant buffer's variable the programme reads (RDEF D3D_SVF_USED); with `bytes`, the buffer at register `slot` and the
// variable at `offset`, `bytes` long (its step review's L2: the native programme reads them there).
bool ReadsVariable(const ds3content::ShaderInterface& shader,std::string_view buffer,std::string_view variable,
    uint32_t slot=0,uint32_t offset=0,uint32_t bytes=0) noexcept {
    if(bytes && std::none_of(shader.resources.begin(),shader.resources.end(),[&](const auto& r){return !r.input_type && r.name==buffer && r.shader_register==slot;}))return false;
    for(const auto& constant:shader.constants)if(constant.name==buffer)
        for(const auto& v:constant.variables)if(v.name==variable)return (v.flags&2u)!=0 && (!bytes || (v.byte_offset==offset && v.bytes==bytes));
    return false;}
bool SintFormat(DXGI_FORMAT format) noexcept {
    return format==DXGI_FORMAT_R16G16_SINT || format==DXGI_FORMAT_R16G16B16A16_SINT || format==DXGI_FORMAT_R32G32_SINT ||
        format==DXGI_FORMAT_R32G32B32_SINT || format==DXGI_FORMAT_R32G32B32A32_SINT || format==DXGI_FORMAT_R8G8B8A8_SINT;}
}
namespace {
ds3content::CpuShaderProgram BuiltClothVelocity() {
    ds3content::CpuShaderProgram program;program.stage=ds3content::ShaderStage::Vertex;program.model_major=5;program.name="native cloth velocity VS";
    program.bytecode.assign(std::begin(cloth_velocity_vs_dxbc),std::end(cloth_velocity_vs_dxbc));return program;}
}
std::shared_ptr<const NativeModelProgramSet> MakeNativeClothVelocitySet(const NativeModelProgramSet& base,const ds3content::ShaderInterface& game_vertex) {
    static const ds3content::CpuShaderProgram vertex=BuiltClothVelocity();
    if(base.pixel.bytecode.empty() || !base.hull.bytecode.empty() || game_vertex.stage!=ds3content::ShaderStage::Vertex)return nullptr;
    if(!ReadsVariable(game_vertex,"cbMatDynParam","VC_isEnableCloth") || !ReadsVariable(game_vertex,"cbvModelParam","VC_World",7,0,48) ||
       !ReadsVariable(game_vertex,"cbvModelParam","VC_PrevWorld",7,48,48) || !ReadsVariable(game_vertex,"cbMatrixWVP","VC_MatrixWVP",0,0,64) ||
       !ReadsVariable(game_vertex,"cbMatrixWVP","VC_PrevMatrixWVP",0,64,64) || !ReadsVariable(game_vertex,"cbTexScroll","VC_TexScroll_0",11,0,16))return nullptr;
    // The game's outputs, which its pixel programme links to: ours are the same registers, semantics and masks.
    const auto native_outputs=ds3content::InspectShaderInterfaceShared(vertex)->outputs;
    if(game_vertex.outputs.size()!=native_outputs.size())return nullptr;
    for(size_t i=0;i<native_outputs.size();++i){const auto &a=game_vertex.outputs[i],&b=native_outputs[i];
        if(!SameName(a.semantic,b.semantic) || a.semantic_index!=b.semantic_index || a.shader_register!=b.shader_register || a.mask!=b.mask ||
           a.system_value!=b.system_value || a.component_type!=b.component_type)return nullptr;}
    // The normal (TEXCOORD2) is written as zero: refused for a pixel programme that reads it (its second final review's L2).
    const auto pixel_interface=ds3content::InspectShaderInterfaceShared(base.pixel);   // held through the loop (its step review's M1)
    for(const auto& input:pixel_interface->inputs)
        if(SameName(input.semantic,"TEXCOORD") && input.semantic_index==2 && input.usage_mask)return nullptr;
    const ds3content::ModelVertexElement* position=nullptr;bool uv=false;
    for(const auto& element:base.elements){
        if(element.input_slot==2 || SameName(element.semantic,"PREVPOSITION"))return nullptr;
        if(SameName(element.semantic,"POSITION") && !element.semantic_index && !element.input_slot &&
           (element.format==DXGI_FORMAT_R32G32B32_FLOAT || element.format==DXGI_FORMAT_R32G32B32A32_FLOAT))position=&element;
        if(SameName(element.semantic,"TEXCOORD") && !element.semantic_index && SintFormat(element.format))uv=true;}
    if(!position || !uv)return nullptr;
    auto elements=base.elements;
    elements.push_back({"PREVPOSITION",0,position->format,2,position->byte_offset,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0});
    return MakeNativeModelProgramSet(vertex,base.pixel,std::move(elements));
}
// I-0140 tranche 40: the resolve's root and pipeline (and the main view's velocity heaps), apart from the main view's targets so the menu
// views' TAA makes them without touching those.
void SceneRenderer::VelocityPipelines() {
    auto* device=gpu_.device.Get();
    // Every object the block makes is checked, so one that a caught menu failure left unmade is made again (the step review's L2).
    if(!velocity_root_ || !velocity_resolve_pso_ || !velocity_rtvs_ || !velocity_half_root_ || !velocity_half_pso_ || !velocity_views_){
        D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].Descriptor.ShaderRegister=0;parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_DESCRIPTOR_RANGE views{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&views};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC desc{2,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"native velocity root serialization");
        Check(ds3native::Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),velocity_root_.ReleaseAndGetAddressOf()),"native velocity root");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=velocity_root_.Get();
        pso.VS={velocity_resolve_vs_dxbc,sizeof(velocity_resolve_vs_dxbc)};pso.PS={velocity_resolve_ps_dxbc,sizeof(velocity_resolve_ps_dxbc)};
        pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.DepthStencilState.DepthEnable=FALSE;
        pso.NumRenderTargets=1;pso.RTVFormats[0]=MotionVectorFormat();
        Check(ds3native::Pipelines().Graphics(device,pso,velocity_resolve_pso_.ReleaseAndGetAddressOf()),"native velocity resolve pipeline");
        D3D12_DESCRIPTOR_HEAP_DESC rtvs{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,3,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};   // MB-4: slot 2, the half-size object velocity
        Check(CountedHeap(device)->CreateDescriptorHeap(&rtvs,IID_PPV_ARGS(&velocity_rtvs_)),"native velocity target views");
        // W10 MB-4: the reduction to the half-size object velocity: the velocity target (the views' first slot) into an R8G8B8A8 target.
        const D3D12_DESCRIPTOR_RANGE half_view{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER half_parameters[2]{};
        half_parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;half_parameters[0].DescriptorTable={1,&half_view};half_parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        half_parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;half_parameters[1].Constants={0,0,2};half_parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;   // I-0111: the ratio
        const D3D12_ROOT_SIGNATURE_DESC half_desc{2,half_parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> half_blob;
        Check(D3D12SerializeRootSignature(&half_desc,D3D_ROOT_SIGNATURE_VERSION_1,&half_blob,&error),"native velocity half root serialization");
        Check(ds3native::Pipelines().Root(device,half_blob->GetBufferPointer(),half_blob->GetBufferSize(),velocity_half_root_.ReleaseAndGetAddressOf()),"native velocity half root");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC half_pso{};Blend(half_pso);half_pso.pRootSignature=velocity_half_root_.Get();
        half_pso.VS={velocity_half_vs_dxbc,sizeof(velocity_half_vs_dxbc)};half_pso.PS={velocity_half_ps_dxbc,sizeof(velocity_half_ps_dxbc)};
        half_pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;half_pso.DepthStencilState.DepthEnable=FALSE;
        half_pso.NumRenderTargets=1;half_pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
        Check(ds3native::Pipelines().Graphics(device,half_pso,velocity_half_pso_.ReleaseAndGetAddressOf()),"native velocity half pipeline");
        D3D12_DESCRIPTOR_HEAP_DESC views_desc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,3,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};   // I-0136: slot 2 the rows' depth
        Check(CountedHeap(device)->CreateDescriptorHeap(&views_desc,IID_PPV_ARGS(&velocity_views_)),"native velocity resolve views");
    }
}
void SceneRenderer::NativeVelocityResources() {
    auto* device=gpu_.device.Get();
    VelocityPipelines();
    // I-0136: the occlusion pass (b0 four constants; t0 the scene depth, t1 the rows' depth: the views' slots 1 and 2) and the rows' DSV.
    if(!velocity_occlusion_pso_ || !velocity_dsv_){   // apart from the block above, so a failure there or here is retried (its step review's L6)
        ComPtr<ID3DBlob> error;D3D12_ROOT_PARAMETER occ[2]{};const D3D12_DESCRIPTOR_RANGE depths{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};
            occ[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;occ[0].Constants={0,0,4};occ[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            occ[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;occ[1].DescriptorTable={1,&depths};occ[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            const D3D12_ROOT_SIGNATURE_DESC occ_desc{2,occ,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> occ_blob;
            Check(D3D12SerializeRootSignature(&occ_desc,D3D_ROOT_SIGNATURE_VERSION_1,&occ_blob,&error),"native velocity occlusion root serialization");
            Check(ds3native::Pipelines().Root(device,occ_blob->GetBufferPointer(),occ_blob->GetBufferSize(),velocity_occlusion_root_.ReleaseAndGetAddressOf()),"native velocity occlusion root");
            D3D12_GRAPHICS_PIPELINE_STATE_DESC occ_pso{};Blend(occ_pso);occ_pso.pRootSignature=velocity_occlusion_root_.Get();
            occ_pso.VS={velocity_occlusion_vs_dxbc,sizeof(velocity_occlusion_vs_dxbc)};occ_pso.PS={velocity_occlusion_ps_dxbc,sizeof(velocity_occlusion_ps_dxbc)};
            occ_pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;occ_pso.DepthStencilState.DepthEnable=FALSE;
            occ_pso.NumRenderTargets=1;occ_pso.RTVFormats[0]=VelocityTargetFormat();
            Check(ds3native::Pipelines().Graphics(device,occ_pso,velocity_occlusion_pso_.ReleaseAndGetAddressOf()),"native velocity occlusion pipeline");
            const D3D12_DESCRIPTOR_HEAP_DESC dsv_desc{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            Check(CountedHeap(device)->CreateDescriptorHeap(&dsv_desc,IID_PPV_ARGS(&velocity_dsv_)),"native velocity rows' depth view heap");}
    if(velocity_generation_==view_->target_generation)return;
    // The targets follow the scene extent (I-0111: the render extent) and depth incarnation (render targets between frames).
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    const DXGI_FORMAT formats[2]{VelocityTargetFormat(),MotionVectorFormat()};
    ComPtr<ID3D12Resource>* targets[2]{&velocity_target_,&motion_vectors_};
    for(unsigned i=0;i<2;++i){
        const auto image=Image(RenderWidth(),RenderHeight(),1,formats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clear{};clear.Format=formats[i];if(!i){clear.Color[0]=clear.Color[1]=0.5f;clear.Color[2]=1.0f;}
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(targets[i]->ReleaseAndGetAddressOf())),"native velocity target");
        device->CreateRenderTargetView(targets[i]->Get(),nullptr,CPU(velocity_rtvs_.Get(),rtv_stride_,i));
    }
    {   // W10 MB-4: the half-size object velocity (the engine's w >> 1, h >> 1: I-0111, the presentation's), shader-readable between frames.
        const auto image=Image(presenter_.width>>1,presenter_.height>>1,1,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(velocity_half_.ReleaseAndGetAddressOf())),"native velocity half target");
        device->CreateRenderTargetView(velocity_half_.Get(),nullptr,CPU(velocity_rtvs_.Get(),rtv_stride_,2));}
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    srv.Format=VelocityTargetFormat();device->CreateShaderResourceView(velocity_target_.Get(),&srv,CPU(velocity_views_.Get(),binding_stride_,0));
    srv.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;device->CreateShaderResourceView(view_->depth.Get(),&srv,CPU(velocity_views_.Get(),binding_stride_,1));
    {   // I-0136: the rows' own depth, the scene depth's format at the render extent (the rows' pipelines keep their depth format).
        const auto scene=view_->depth->GetDesc();
        if(scene.Format!=DXGI_FORMAT_R32G8X24_TYPELESS)throw std::invalid_argument("native velocity rows' depth: the scene depth's format");
        const auto image=Image(RenderWidth(),RenderHeight(),1,scene.Format,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;clear.DepthStencil.Depth=1;
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_DEPTH_WRITE,&clear,IID_PPV_ARGS(velocity_depth_.ReleaseAndGetAddressOf())),"native velocity rows' depth");
        velocity_depth_->SetName(L"native velocity rows' depth");
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};dsv.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;dsv.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(velocity_depth_.Get(),&dsv,velocity_dsv_->GetCPUDescriptorHandleForHeapStart());
        device->CreateShaderResourceView(velocity_depth_.Get(),&srv,CPU(velocity_views_.Get(),binding_stride_,2));}
    velocity_generation_=view_->target_generation;
}
// I-0111: DLSS's output, the lit HDR's format at the presentation's extent (tranche 3: the render extent may be smaller), unordered
// access and render target (DoF and Alpha_R draw into the post chain's source), RENDER_TARGET between frames, as the lit HDR; its
// composition view (binding +2) written with it. Remade when the presentation's extent or the lit HDR's format changes.
void SceneRenderer::NativeDlssOutput() {
    auto* hdr=view_->light_targets[3].Get();if(!hdr){dlss_output_.Reset();return;}
    const auto format=hdr->GetDesc().Format;
    if(dlss_output_){const auto d=dlss_output_->GetDesc();if(d.Width==presenter_.width && d.Height==presenter_.height && d.Format==format)return;}
    if(dlss_output_generation_==UINT64_MAX)throw std::overflow_error("native DLSS output generation exhausted");
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    const auto image=Image(presenter_.width,presenter_.height,1,format,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS|D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    D3D12_CLEAR_VALUE clear{};clear.Format=format;
    Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,
        IID_PPV_ARGS(dlss_output_.ReleaseAndGetAddressOf())),"native DLSS output");
    dlss_output_->SetName(L"native DLSS output");++dlss_output_generation_;
    DlssOutputView();
}
// I-0140 tranche 40 (D-0305): the current menu view context's TAA targets and views (scene_pass.h's ViewContext), made at its first TAA
// recording and again for a new target generation (the depth's view; DepthTarget drained the queue first, so no list in flight reads
// the views rewritten here; the targets, held by each slot that used them, are made once a context: its extent never changes).
void SceneRenderer::MenuTaaResources() {
    auto* device=gpu_.device.Get();VelocityPipelines();
    auto* hdr=view_->light_targets[3].Get();if(!hdr || !view_->depth)throw std::logic_error("native menu TAA: no lit HDR or depth");
    const auto format=hdr->GetDesc().Format;const uint32_t width=view_->width,height=view_->height;
    if(!view_->taa_output || view_->taa_output->GetDesc().Format!=format){
        const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto output=Image(width,height,1,format,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&output,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(view_->taa_output.ReleaseAndGetAddressOf())),"native menu TAA output");
        view_->taa_output->SetName(L"native menu TAA output");view_->taa_made=++menu_aa_creations_;
        const DXGI_FORMAT formats[2]{VelocityTargetFormat(),MotionVectorFormat()};ComPtr<ID3D12Resource>* targets[2]{&view_->taa_velocity,&view_->taa_motion};
        if(!view_->taa_rtvs){const D3D12_DESCRIPTOR_HEAP_DESC rtvs{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
            Check(CountedHeap(device)->CreateDescriptorHeap(&rtvs,IID_PPV_ARGS(&view_->taa_rtvs)),"native menu TAA target views");}
        for(unsigned i=0;i<2;++i){const auto image=Image(width,height,1,formats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            D3D12_CLEAR_VALUE clear{};clear.Format=formats[i];if(!i){clear.Color[0]=clear.Color[1]=0.5f;clear.Color[2]=1.0f;}
            Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(targets[i]->ReleaseAndGetAddressOf())),"native menu TAA velocity target");
            device->CreateRenderTargetView(targets[i]->Get(),nullptr,CPU(view_->taa_rtvs.Get(),rtv_stride_,i));}
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels=1;srv.Format=format;
        device->CreateShaderResourceView(view_->taa_output.Get(),&srv,CPU(view_->bindings.Get(),binding_stride_,composition_binding+2));   // the composition's source
        view_->taa_generation=UINT64_MAX;}
    if(view_->taa_generation==view_->target_generation)return;
    if(!view_->taa_views){const D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&view_->taa_views)),"native menu TAA resolve views");}
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    srv.Format=VelocityTargetFormat();device->CreateShaderResourceView(view_->taa_velocity.Get(),&srv,CPU(view_->taa_views.Get(),binding_stride_,0));
    srv.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;device->CreateShaderResourceView(view_->depth.Get(),&srv,CPU(view_->taa_views.Get(),binding_stride_,1));
    view_->taa_generation=view_->target_generation;
}
// I-0113 tranche 39: the TAA's reactive base (scene_pass.h), the lit HDR's format at the render extent, shader-readable between frames.
void SceneRenderer::NativeTaaBase() {
    auto* hdr=view_->light_targets[3].Get();if(!hdr)throw std::logic_error("native TAA base: no lit HDR");
    const auto format=hdr->GetDesc().Format;
    if(taa_base_){const auto d=taa_base_->GetDesc();if(d.Width==RenderWidth() && d.Height==RenderHeight() && d.Format==format)return;}
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto image=Image(RenderWidth(),RenderHeight(),1,format,D3D12_RESOURCE_FLAG_NONE);
    Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(taa_base_.ReleaseAndGetAddressOf())),"native TAA base");
    taa_base_->SetName(L"native TAA reactive base");
}
void SceneRenderer::CopyTaaBase(ID3D12GraphicsCommandList* list) {
    auto* hdr=view_->light_targets[3].Get();auto* base=taa_base_.Get();const auto read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    Transition(list,hdr,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);Transition(list,base,read,D3D12_RESOURCE_STATE_COPY_DEST);
    if(auto* graph=active_graph_shadow){graph->Pass("taa_base",PassKind::Effect);graph->Use(hdr,ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);
        graph->Use(base,ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);}
    list->CopyResource(base,hdr);
    Transition(list,base,D3D12_RESOURCE_STATE_COPY_DEST,read);Transition(list,hdr,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
}
// I-0140 (gap 23): the TAA's fade mask (scene_pass.h), its view made with it.
static_assert(kFadeMaskFormat==kTaaFadeMaskFormat);
void SceneRenderer::NativeTaaFadeMask() {
    if(taa_fade_mask_){const auto d=taa_fade_mask_->GetDesc();if(d.Width==RenderWidth() && d.Height==RenderHeight())return;}
    if(!taa_fade_mask_rtv_){const D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&taa_fade_mask_rtv_)),"native TAA fade mask view");}
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto image=Image(RenderWidth(),RenderHeight(),1,kFadeMaskFormat,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    D3D12_CLEAR_VALUE clear{};clear.Format=kFadeMaskFormat;clear.Color[0]=clear.Color[1]=clear.Color[2]=clear.Color[3]=1.0f;
    Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,&clear,IID_PPV_ARGS(taa_fade_mask_.ReleaseAndGetAddressOf())),"native TAA fade mask");
    taa_fade_mask_->SetName(L"native TAA fade mask");
    gpu_.device->CreateRenderTargetView(taa_fade_mask_.Get(),nullptr,taa_fade_mask_rtv_->GetCPUDescriptorHandleForHeapStart());
}
// I-0140 (gap 23): cleared to 1, then every row whose model's fade moved (fade_mask_draws) drawn again by its fade-mask pipeline (0 where it draws: the dissolve's
// dots, not its holes), against the scene depth (tested, not written); left shader-readable for the TAA (shaders/taa.hlsl). After the
// forward and effect rows, the depth then in DEPTH_WRITE.
void SceneRenderer::EncodeTaaFadeMask(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,const NativeModelFrameAddresses& frame,
    NativeModelConstantUploads& uploads,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) {
    auto* mask=taa_fade_mask_.Get();const auto read=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    const auto target=taa_fade_mask_rtv_->GetCPUDescriptorHandleForHeapStart();
    Transition(list,mask,read,D3D12_RESOURCE_STATE_RENDER_TARGET);
    if(auto* graph=active_graph_shadow){graph->Pass("taa_fade_mask",PassKind::Material);graph->Use(mask,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
    const float clear[4]{1.0f,1.0f,1.0f,1.0f};list->ClearRenderTargetView(target,clear,0,nullptr);
    const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,1> targets{target};const auto dsv=view_->dsv->GetCPUDescriptorHandleForHeapStart();
    GBufferBindState state;uint64_t rows{};
    for(const auto* draw:input.fade_mask_draws)if(draw && draw->Input().fade_mask){draw->Encode(list,targets,dsv,viewport,scissor,frame,uploads,&state,kFadeMaskPipeline);++rows;}
    taa_fade_rows_.fetch_add(rows,std::memory_order_relaxed);
    Transition(list,mask,D3D12_RESOURCE_STATE_RENDER_TARGET,read);
}
// I-0111 tranche 2 (D-0242): DS3's exposure for DLSS (the tone map's own, K-0448: never automatic in play), one float copied from the
// frame's upload ring into a 1x1 R32_FLOAT, NON_PIXEL_SHADER_RESOURCE between frames (made in COPY_DEST, so its first copy needs no
// transition). Null on a refused creation (latched: DLSS's own auto-exposure then), the frame going on.
// I-0132 R11h: `automatic`, the automatic exposure's last value (the area's exposure x its factor, the previous frame's: DLSS runs
// before this frame's pass) copied on the GPU instead, once that pass has run.
ID3D12Resource* SceneRenderer::RecordDlssExposure(ID3D12GraphicsCommandList* list,float exposure,bool automatic) {
    if(dlss_exposure_failed_)return nullptr;
    if(!(exposure>0.f) || !std::isfinite(exposure))exposure=1.f;   // final review L1: the exposure stays given (sl.dlss fixes its mode at creation)
    const auto npsr=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;bool fresh=false;
    if(!dlss_exposure_){
        const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto image=Image(1,1,1,DXGI_FORMAT_R32_FLOAT,D3D12_RESOURCE_FLAG_NONE);
        if(FAILED(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,
            IID_PPV_ARGS(dlss_exposure_.ReleaseAndGetAddressOf())))){dlss_exposure_failed_=true;dlss_exposure_.Reset();return nullptr;}
        dlss_exposure_->SetName(L"native DLSS exposure");fresh=true;}
    if(!fresh)Transition(list,dlss_exposure_.Get(),npsr,D3D12_RESOURCE_STATE_COPY_DEST);
    if(automatic && !auto_exposure_failed_ && auto_exposure_.CopyDlssExposure(list,dlss_exposure_.Get())){
        Transition(list,dlss_exposure_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,npsr);return dlss_exposure_.Get();}
    const auto range=frame_uploads_.Allocate(gpu_.device.Get(),D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT,false);
    std::memcpy(range.cpu,&exposure,sizeof exposure);
    D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=range.resource;from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    from.PlacedFootprint.Offset=range.offset;from.PlacedFootprint.Footprint={DXGI_FORMAT_R32_FLOAT,1,1,1,D3D12_TEXTURE_DATA_PITCH_ALIGNMENT};
    to.pResource=dlss_exposure_.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=0;
    list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    Transition(list,dlss_exposure_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,npsr);
    return dlss_exposure_.Get();
}
// §3.7: the output-size depth at the presentation's extent, made at the first scaled frame (step review L5: none at DLAA) and with
// each new extent; DEPTH_WRITE between frames. Its generation is DLSS's output's (the post stages key their views on it). Its
// pipeline once per device, made here (final review L2: not while the frame records; a failure latches DLSS off).
void SceneRenderer::NativeDepthOut() {
    auto* device=gpu_.device.Get();
    if(!depth_out_root_){
        D3D12_ROOT_PARAMETER parameters[2]{};const D3D12_DESCRIPTOR_RANGE view{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[0].Constants={0,0,4};parameters[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&view};parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        const D3D12_ROOT_SIGNATURE_DESC desc{2,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"native output-size depth root serialization");
        ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> pipeline;ComPtr<ID3D12DescriptorHeap> views;
        Check(ds3native::Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root.GetAddressOf()),"native output-size depth root");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=root.Get();
        pso.VS={depth_out_vs_dxbc,sizeof(depth_out_vs_dxbc)};pso.PS={depth_out_ps_dxbc,sizeof(depth_out_ps_dxbc)};
        pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.DepthStencilState.DepthEnable=TRUE;
        pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
        pso.NumRenderTargets=0;pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        Check(ds3native::Pipelines().Graphics(device,pso,pipeline.GetAddressOf()),"native output-size depth pipeline");
        const D3D12_DESCRIPTOR_HEAP_DESC views_desc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,1,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&views_desc,IID_PPV_ARGS(&views)),"native output-size depth source view");
        depth_out_pso_=pipeline;depth_out_views_=views;depth_out_view_generation_=UINT64_MAX;depth_out_root_=root;}
    if(depth_out_){const auto d=depth_out_->GetDesc();if(d.Width==presenter_.width && d.Height==presenter_.height)return;}
    if(dlss_output_generation_==UINT64_MAX)throw std::overflow_error("native DLSS output generation exhausted");
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    const auto depth_image=Image(presenter_.width,presenter_.height,1,DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    D3D12_CLEAR_VALUE depth_clear{};depth_clear.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;depth_clear.DepthStencil.Depth=1;
    Check(CountedResource(gpu_.device.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&depth_image,D3D12_RESOURCE_STATE_DEPTH_WRITE,&depth_clear,
        IID_PPV_ARGS(depth_out_.ReleaseAndGetAddressOf())),"native output-size depth");
    depth_out_->SetName(L"native output-size depth");
    if(!depth_out_dsv_){const D3D12_DESCRIPTOR_HEAP_DESC desc{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&depth_out_dsv_)),"native output-size depth view heap");}
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};dsv.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;dsv.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
    gpu_.device->CreateDepthStencilView(depth_out_.Get(),&dsv,depth_out_dsv_->GetCPUDescriptorHandleForHeapStart());
    ++dlss_output_generation_;
}
// §3.7: the output-size depth drawn from the scene depth (DEPTH_WRITE on entry and exit; its plane 0 shader-readable during the draw),
// for this frame's jitter; the scene depth's view renewed with its generation.
void SceneRenderer::EncodeDepthOut(ID3D12GraphicsCommandList* list,float jitter_x,float jitter_y) {
    auto* device=gpu_.device.Get();
    if(depth_out_view_generation_!=view_->target_generation){   // the scene depth remade (a new extent or stencil: the GPU drained)
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels=1;srv.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        device->CreateShaderResourceView(view_->depth.Get(),&srv,depth_out_views_->GetCPUDescriptorHandleForHeapStart());depth_out_view_generation_=view_->target_generation;}
    const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    DepthPlaneTransition(list,view_->depth.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    if(auto* graph=active_graph_shadow){graph->Pass("depth_out",PassKind::PostProcess);graph->Use(view_->depth.Get(),ResourceAccess::Read,depth_read,{0,1});
        graph->Use(depth_out_.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
    const auto target=depth_out_dsv_->GetCPUDescriptorHandleForHeapStart();
    ID3D12DescriptorHeap* heaps[]{depth_out_views_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->OMSetRenderTargets(0,nullptr,FALSE,&target);list->SetPipelineState(depth_out_pso_.Get());list->SetGraphicsRootSignature(depth_out_root_.Get());
    const float map[4]{float(RenderWidth())/float(presenter_.width),float(RenderHeight())/float(presenter_.height),jitter_x,jitter_y};
    list->SetGraphicsRoot32BitConstants(0,4,map,0);list->SetGraphicsRootDescriptorTable(1,depth_out_views_->GetGPUDescriptorHandleForHeapStart());
    const D3D12_VIEWPORT viewport{0,0,float(presenter_.width),float(presenter_.height),0,1};const D3D12_RECT scissor{0,0,LONG(presenter_.width),LONG(presenter_.height)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    DepthPlaneTransition(list,view_->depth.Get(),depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
void SceneRenderer::DlssOutputView() {
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=dlss_output_->GetDesc().Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    gpu_.device->CreateShaderResourceView(dlss_output_.Get(),&srv,CPU(view_->bindings.Get(),binding_stride_,composition_binding+2));
}
void SceneRenderer::LatchDlssOff(uint64_t serial,const std::string& reason) {
    dlss_failed_=true;streamline::Disable();
    if(!upscaler_sink_)return;
    std::string why=reason.substr(0,200);for(auto& c:why)if(c=='"' || c=='\\' || c<32)c=' ';char row[320]{};
    sprintf_s(row,"{\"event\":\"native_dlss_latched_off\",\"serial\":%llu,\"after_frames\":%llu,\"reason\":\"%s\"}\n",static_cast<unsigned long long>(serial),
        static_cast<unsigned long long>(dlss_frames_.load(std::memory_order_relaxed)),why.c_str());upscaler_sink_(row);
}
std::array<D3D12_GPU_VIRTUAL_ADDRESS,FrameBlockKindCount> SceneRenderer::UploadNativeFrameBlocks(const NativeModelFrameInput& input) {
    std::array<D3D12_GPU_VIRTUAL_ADDRESS,FrameBlockKindCount> addresses{};
    for(uint32_t kind=0;kind<FrameBlockKindCount;++kind)if(const auto& block=input.frame_blocks[kind]){
        if(block->empty() || block->size()>65536)throw std::invalid_argument("native frame block extent");
        const auto range=frame_uploads_.Allocate(gpu_.device.Get(),(block->size()+255)&~size_t(255));   // zero padded (B2c)
        std::memcpy(range.cpu,block->data(),block->size());addresses[kind]=range.gpu;}
    return addresses;
}
// Group 0x33 (102380): the velocity target cleared (1400BE8E0: colour only, (0.5, 0.5, 1, 0), alpha 0 = no object velocity), then
// the Velo rows (group 0x34): I-0136, into their own depth, then the occlusion pass against the scene depth (velo-depth-legacy.flag:
// against the scene depth with the former bias). Entered and left with the scene depth in DEPTH_WRITE, the velocity target a render
// target. I-0126 R7c: on NRD's frames the rows are drawn after the decals, before the lighting, so their occlusion is the G-buffer's
// depth: a row behind a forward row that writes depth (a phantom, the water's surface) keeps its object's motion there, which DLSS
// (RR off) and the motion blur also read on those frames (the fixes' step review M-a, left: R7c RESULT); every other frame draws them
// after the forward rows, as the original.
void SceneRenderer::EncodeNativeVelocityRows(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,const NativeModelFrameAddresses& frame,
    NativeModelConstantUploads& uploads,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) {
    const auto target=CPU(velocity_rtvs_.Get(),rtv_stride_,0);
    const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,1> targets{target};
    const bool own=input.velocity->own_depth;   // I-0136: the rows in their own depth, then the occlusion pass
    const auto rows_dsv=own?velocity_dsv_->GetCPUDescriptorHandleForHeapStart():view_->dsv->GetCPUDescriptorHandleForHeapStart();
    // The pass and its uses declared before its clears, so the graph's barriers come first (its second final review's L2).
    if(auto* graph=active_graph_shadow){graph->Pass("velocity",PassKind::Material);graph->Use(velocity_target_.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        // I-0136 (its final review's H1): the rows' depth is cleared first, written (a transient read before any write is refused).
        if(own)graph->Use(velocity_depth_.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_DEPTH_WRITE);
        else graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE);}
    const float clear[4]{0.5f,0.5f,1.0f,0.0f};list->ClearRenderTargetView(target,clear,0,nullptr);
    if(own)list->ClearDepthStencilView(rows_dsv,D3D12_CLEAR_FLAG_DEPTH,1.0f,0,0,nullptr);
    GBufferBindState state;for(const auto& draw:input.velocity_draws)draw->Encode(list,targets,rows_dsv,viewport,scissor,frame,uploads,&state);
    if(!own)return;
    // The occlusion pass: both depths' plane 0 read, the velocity target written where a nearer scene surface hides a row.
    const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    DepthPlaneTransition(list,view_->depth.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    DepthPlaneTransition(list,velocity_depth_.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    if(auto* graph=active_graph_shadow){graph->Pass("velocity_occlusion",PassKind::Effect);graph->Use(view_->depth.Get(),ResourceAccess::Read,depth_read,{0,1});
        graph->Use(velocity_depth_.Get(),ResourceAccess::Read,depth_read,{0,1});graph->Use(velocity_target_.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);}   // keeps the pixels it does not write
    ID3D12DescriptorHeap* heaps[]{velocity_views_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->OMSetRenderTargets(1,&target,FALSE,nullptr);list->SetPipelineState(velocity_occlusion_pso_.Get());list->SetGraphicsRootSignature(velocity_occlusion_root_.Get());
    list->SetGraphicsRoot32BitConstants(0,4,input.velocity->occlusion.data(),0);
    auto depths=velocity_views_->GetGPUDescriptorHandleForHeapStart();depths.ptr+=binding_stride_;list->SetGraphicsRootDescriptorTable(1,depths);
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    DepthPlaneTransition(list,velocity_depth_.Get(),depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
    DepthPlaneTransition(list,view_->depth.Get(),depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
// The resolve (after the forward and effect rows): the rows' pixels decoded, the others the camera motion of the final depth.
void SceneRenderer::EncodeNativeVelocity(ID3D12GraphicsCommandList* list,const NativeModelFrameInput& input,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,
    const SlotReadbacks* sample) {
    const auto& velocity=*input.velocity;
    const auto vectors=CPU(velocity_rtvs_.Get(),rtv_stride_,1);
    const auto resolve_constants=frame_uploads_.Allocate(gpu_.device.Get(),256);   // B2c
    {struct {Matrix4 matrix;std::array<float,4> scale,screen;} constants{velocity.current_to_previous,velocity.scale,
        {float(RenderWidth()),float(RenderHeight()),1.0f/float(RenderWidth()),1.0f/float(RenderHeight())}};
        std::memcpy(resolve_constants.cpu,&constants,sizeof(constants));}
    const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    DepthPlaneTransition(list,view_->depth.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    if(auto* graph=active_graph_shadow){graph->Pass("velocity_resolve",PassKind::Effect);graph->Use(velocity_target_.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        graph->Use(view_->depth.Get(),ResourceAccess::Read,depth_read,{0,1});graph->Use(motion_vectors_.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
    ID3D12DescriptorHeap* heaps[]{velocity_views_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->OMSetRenderTargets(1,&vectors,FALSE,nullptr);list->SetPipelineState(velocity_resolve_pso_.Get());list->SetGraphicsRootSignature(velocity_root_.Get());
    list->SetGraphicsRootConstantBufferView(0,resolve_constants.gpu);
    list->SetGraphicsRootDescriptorTable(1,velocity_views_->GetGPUDescriptorHandleForHeapStart());
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    DepthPlaneTransition(list,view_->depth.Get(),depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
    if(sample){
        // Witness readback of both images (copied out, then back to their between-frame states).
        Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
        Transition(list,motion_vectors_.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
        if(auto* graph=active_graph_shadow){graph->Pass("velocity_readback",PassKind::Readback);
            graph->Use(velocity_target_.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);graph->Use(motion_vectors_.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);}
        for(unsigned i=0;i<2;++i){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=i?motion_vectors_.Get():velocity_target_.Get();
            from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.pResource=i?sample->motion.Get():sample->velocity.Get();
            to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=i?sample->motion_footprint:sample->velocity_footprint;
            list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
        Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        Transition(list,motion_vectors_.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
    }else Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
}
// W10 MB-4: the velocity target (a render target, as EncodeNativeVelocity leaves it) reduced into the half-size object velocity (shader-readable
// on entry and exit); the velocity target is a render target again on exit.
void SceneRenderer::EncodeVelocityHalf(ID3D12GraphicsCommandList* list) {
    Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Transition(list,velocity_half_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
    if(auto* graph=active_graph_shadow){graph->Pass("velocity_half",PassKind::Effect);graph->Use(velocity_target_.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        graph->Use(velocity_half_.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
    const auto target=CPU(velocity_rtvs_.Get(),rtv_stride_,2);ID3D12DescriptorHeap* heaps[]{velocity_views_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->OMSetRenderTargets(1,&target,FALSE,nullptr);list->SetPipelineState(velocity_half_pso_.Get());list->SetGraphicsRootSignature(velocity_half_root_.Get());
    list->SetGraphicsRootDescriptorTable(0,velocity_views_->GetGPUDescriptorHandleForHeapStart());
    {const float ratio[2]{float(RenderWidth())/float(presenter_.width),float(RenderHeight())/float(presenter_.height)};list->SetGraphicsRoot32BitConstants(1,2,ratio,0);}
    const D3D12_VIEWPORT viewport{0,0,float(presenter_.width>>1),float(presenter_.height>>1),0,1};const D3D12_RECT scissor{0,0,LONG(presenter_.width>>1),LONG(presenter_.height>>1)};
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    Transition(list,velocity_half_.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
}
// After completion of a sampled frame (its slot's release): object coverage and motion statistics (pixels) as one JSON
// object, optionally the picture file.
std::string SceneRenderer::ReadNativeVelocity(const SlotReadbacks& readbacks,const std::wstring& velocity_file) {
    const uint32_t width=readbacks.velocity_width,height=readbacks.velocity_height;
    const auto& velocity_readback_=readbacks.velocity;const auto& motion_readback_=readbacks.motion;
    // I-0111 (i111-q-1): the last row is not padded to the row pitch (853 texels at Quality: the full pitch ran past the buffer).
    const D3D12_RANGE velocity_range{0,static_cast<SIZE_T>(readbacks.velocity_footprint.Offset+uint64_t(readbacks.velocity_footprint.Footprint.RowPitch)*(height-1)+uint64_t(width)*8)};
    const D3D12_RANGE motion_range{0,static_cast<SIZE_T>(readbacks.motion_footprint.Offset+uint64_t(readbacks.motion_footprint.Footprint.RowPitch)*(height-1)+uint64_t(width)*4)};
    std::vector<uint8_t> picture(velocity_file.empty()?0:size_t(width)*height*4);   // allocated before the readbacks are mapped
    void* encoded_memory{};void* motion_memory{};
    Check(velocity_readback_->Map(0,&velocity_range,&encoded_memory),"native velocity readback map");
    Check(motion_readback_->Map(0,&motion_range,&motion_memory),"native motion vector readback map");
    const auto half=[](uint16_t h){uint32_t sign=(h>>15)&1,exponent=(h>>10)&0x1F,mantissa=h&0x3FF;float value;
        if(!exponent)value=std::ldexp(float(mantissa),-24);else if(exponent==31)value=mantissa?NAN:INFINITY;else value=std::ldexp(float(mantissa|0x400),int(exponent)-25);
        return sign?-value:value;};
    uint64_t objects{},invalid{};double object_sum{},camera_sum{};float object_max{},camera_max{};
    for(uint32_t y=0;y<height;++y){
        const auto* encoded=reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(encoded_memory)+readbacks.velocity_footprint.Offset+uint64_t(y)*readbacks.velocity_footprint.Footprint.RowPitch);
        const auto* motion=reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(motion_memory)+readbacks.motion_footprint.Offset+uint64_t(y)*readbacks.motion_footprint.Footprint.RowPitch);
        for(uint32_t x=0;x<width;++x){
            const bool object=encoded[4*x+3]>32767;const float mx=half(motion[2*x]),my=half(motion[2*x+1]);
            if(!std::isfinite(mx) || !std::isfinite(my)){++invalid;continue;}
            const float length=std::sqrt(mx*mx+my*my);
            if(object){++objects;object_sum+=length;object_max=std::max(object_max,length);}else{camera_sum+=length;camera_max=std::max(camera_max,length);}
            if(!picture.empty()){auto* p=picture.data()+(size_t(y)*width+x)*4;
                p[0]=static_cast<uint8_t>(std::clamp(128.0f+4.0f*mx,0.0f,255.0f));p[1]=static_cast<uint8_t>(std::clamp(128.0f+4.0f*my,0.0f,255.0f));
                p[2]=object?255:0;p[3]=255;}
        }
    }
    const D3D12_RANGE no_write{};velocity_readback_->Unmap(0,&no_write);motion_readback_->Unmap(0,&no_write);
    const uint64_t pixels=uint64_t(width)*height,cameras=pixels-objects-invalid;
    char row[400]{};sprintf_s(row,"{\"width\":%u,\"height\":%u,\"object_pixels\":%llu,\"invalid\":%llu,\"object_mean_px\":%.4g,\"object_max_px\":%.4g,\"camera_mean_px\":%.4g,\"camera_max_px\":%.4g}",
        width,height,static_cast<unsigned long long>(objects),static_cast<unsigned long long>(invalid),objects?object_sum/double(objects):0.0,object_max,
        cameras?camera_sum/double(cameras):0.0,camera_max);
    if(!picture.empty()){std::ofstream file(velocity_file,std::ios::binary);file.write(reinterpret_cast<const char*>(picture.data()),static_cast<std::streamsize>(picture.size()));
        if(!file)OutputDebugStringA("native velocity picture write failed: picture dropped\n");}   // I-0120 E4: a test picture, not a fault
    return row;
}
// cD4 (G1, G2): the readbacks of one slot, the back buffer's at the presentation extent and the velocity's at the render extent
// (I-0111), created on first use (a size change replaces them; a slot that sampled holds the former ones until its release).
SceneRenderer::SlotReadbacks& SceneRenderer::Readbacks(uint32_t slot,bool velocity) {
    if(slot>=slot_readbacks_.size())throw std::invalid_argument("slot readbacks");
    auto& readbacks=slot_readbacks_[slot];auto* device=gpu_.device.Get();
    if(readbacks.width!=presenter_.width || readbacks.height!=presenter_.height || readbacks.velocity_width!=RenderWidth() || readbacks.velocity_height!=RenderHeight()){
        readbacks={};readbacks.width=presenter_.width;readbacks.height=presenter_.height;readbacks.velocity_width=RenderWidth();readbacks.velocity_height=RenderHeight();}
    readbacks.backbuffer_format=presenter_.format;   // I-0138 tranche 5
    if(!readbacks.backbuffer){const auto image=Image(presenter_.width,presenter_.height,1,DXGI_FORMAT_R8G8B8A8_UNORM);UINT64 total{};
        device->GetCopyableFootprints(&image,0,1,0,nullptr,nullptr,nullptr,&total);
        readbacks.backbuffer=NewBuffer(device,total,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);}
    if(velocity && !readbacks.velocity){const DXGI_FORMAT formats[2]{VelocityTargetFormat(),MotionVectorFormat()};
        for(unsigned i=0;i<2;++i){const auto image=Image(RenderWidth(),RenderHeight(),1,formats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);UINT64 total{};
            device->GetCopyableFootprints(&image,0,1,0,i?&readbacks.motion_footprint:&readbacks.velocity_footprint,nullptr,nullptr,&total);
            (i?readbacks.motion:readbacks.velocity)=NewBuffer(device,total,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);}}
    return readbacks;
}
// cD4: a frame's release-time work, run by its slot's last completion (after the ticket completed): the sampled back-buffer
// hash (and pixel file), the velocity statistics (and picture), then Diagnostics(), which reads the debug-layer messages of
// the work submitted so far; the result goes to the channel and, for the frame functions, to `last_completed_`.
void SceneRenderer::CompleteFrame(uint64_t serial,uint64_t ticket,const SlotReadbacks& readbacks,bool pixels,const std::wstring& pixel_file,
    bool velocity,const std::wstring& velocity_file) {
    CompletedFrame frame;frame.serial=serial;frame.ticket=ticket;frame.pixels_sampled=pixels;
    if(pixels){
        const uint32_t width=readbacks.width,height=readbacks.height;
        const auto image=Image(width,height,1,DXGI_FORMAT_R8G8B8A8_UNORM);D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        gpu_.device->GetCopyableFootprints(&image,0,1,0,&footprint,nullptr,nullptr,nullptr);
        const D3D12_RANGE range{0,static_cast<SIZE_T>(footprint.Offset+uint64_t(footprint.Footprint.RowPitch)*(height-1)+uint64_t(width)*4)};
        void* memory{};Check(readbacks.backbuffer->Map(0,&range,&memory),"native model readback map");
        std::vector<uint8_t> bytes(size_t(width)*height*4);
        for(uint32_t row=0;row<height;++row)std::memcpy(bytes.data()+size_t(row)*width*4,
            static_cast<const uint8_t*>(memory)+footprint.Offset+uint64_t(row)*footprint.Footprint.RowPitch,size_t(width)*4);
        const D3D12_RANGE no_write{};readbacks.backbuffer->Unmap(0,&no_write);frame.pixel_hash=14695981039346656037ULL;
        for(size_t pixel=0;pixel<bytes.size();pixel+=4){
            if(bytes[pixel] || bytes[pixel+1] || bytes[pixel+2])++frame.changed_pixels;
            for(unsigned c=0;c<4;++c){frame.pixel_hash^=bytes[pixel+c];frame.pixel_hash*=1099511628211ULL;}
        }
        if(!pixel_file.empty()){
            // I-0138 tranche 5: an HDR10 back buffer's texels are packed 10-bit ST 2084 codes, not RGBA bytes: written under another
            // extension (investigations/I-0138-modern-post-chain/Decode-Hdr10.py reads it), never under the 8-bit picture's name.
            const std::wstring written=readbacks.backbuffer_format==DXGI_FORMAT_R10G10B10A2_UNORM?std::filesystem::path(pixel_file).replace_extension(L".r10g10b10a2").wstring():pixel_file;
            std::ofstream file(written,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
            if(!file)OutputDebugStringA("native model pixel evidence write failed: picture dropped\n");   // I-0120 E4
        }
    }
    if(velocity)frame.velocity=ReadNativeVelocity(readbacks,velocity_file);
    frame.debug_errors=Diagnostics();
    if(frame.debug_errors){frame.diagnostic=last_diagnostic_;released_errors_+=frame.debug_errors;if(released_diagnostic_.empty())released_diagnostic_=last_diagnostic_;}
    last_completed_=frame;
    const std::lock_guard hold(channel_lock_);completed_.push_back(std::move(frame));
    if(completed_.size()>64)completed_.erase(completed_.begin());   // a caller that never takes them keeps the latest only
}
std::vector<SceneRenderer::CompletedFrame> SceneRenderer::TakeCompleted() {
    const std::lock_guard hold(channel_lock_);auto frames=std::move(completed_);completed_.clear();return frames;
}
void SceneRenderer::RequestVelocityPicture(std::wstring path) {
    const std::lock_guard hold(channel_lock_);velocity_request_=std::move(path);
}
void SceneRenderer::RequireThread(bool worker,const char* what) const {
    const auto thread=render_thread_.load(std::memory_order_acquire);
    if(thread && (GetCurrentThreadId()==thread)!=worker)throw std::logic_error(what);
}
void SceneRenderer::WaitFrameSlot() {
    RequireThread(true,"native frame slot wait off the render thread");
    if(!open_ || faulted_ || frames_.Recording())return;
    try {
        // cD8 (R14): the swap chain's frame-latency object first (DXGI's queue of presents has room), then the slot.
        latency_taken_=false;
        if(display_interval_job_){}   // I-0111 tranche 2: the engine waits the object for this frame (one waiter a frame)
        else if(presenter_.latency && TakeLatencyDebt())latency_taken_=true;   // I-0101 review M1: a count a failed Present kept (final review L2: banked again if this Present fails too)
        else if(presenter_.latency && presenter_.presented && IsWindowVisible(presenter_.presented) && !IsIconic(presenter_.presented)){LARGE_INTEGER frequency{},before{},after{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&before);
            cadence::Mark(cadence::LatencyWaitStart);
            const auto waited=WaitForSingleObjectEx(presenter_.latency,100,FALSE);   // not alertable: every frame consumes one count (review); bounded
            cadence::Mark(cadence::LatencyWaitEnd);
            QueryPerformanceCounter(&after);waitable_wait_ms_.fetch_add(1000.0*double(after.QuadPart-before.QuadPart)/double(frequency.QuadPart));
            if(waited==WAIT_TIMEOUT)++waitable_timeouts_;else if(waited!=WAIT_OBJECT_0)throw std::runtime_error("native frame latency wait");
            latency_taken_=waited==WAIT_OBJECT_0;}
        frames_.WaitNext(gpu_,WaitMainFrame);
    }catch(...){faulted_=true;throw;}
}
// cD4: after a frame function's End. A frame released at its own End (one slot) puts its results in its receipt; the
// errors of any frame released since the last check end the frame (with more slots, an earlier frame's: one frame late).
void SceneRenderer::TakeReleased(FrameReceipt& receipt,const char* failure) {
    if(last_completed_.serial==receipt.serial){receipt.pixel_hash=last_completed_.pixel_hash;receipt.changed_pixels=last_completed_.changed_pixels;
        receipt.debug_errors=last_completed_.debug_errors;}
    if(released_errors_){const auto message=std::string(failure)+released_diagnostic_;released_errors_=0;released_diagnostic_.clear();
        throw std::runtime_error(message);}
}
ComPtr<ID3D12Resource> SceneRenderer::SceneColourCopy() {
    RequireThread(false,"native scene colour copy on the render thread");
    if(!open_ || !gpu_.device || !presenter_.width || !presenter_.height)return {};
    if(scene_colour_copy_){const auto desc=scene_colour_copy_->GetDesc();
        if(desc.Width==RenderWidth() && desc.Height==RenderHeight())return scene_colour_copy_;}
    // A size change (I-0111: the render extent's): the earlier copy stays alive in its holders until their draws complete.
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);const auto image=Image(RenderWidth(),RenderHeight(),1,ForwardTargetFormat());
    ComPtr<ID3D12Resource> copy;
    Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&copy)),"native scene colour copy");
    scene_colour_copy_=copy;return copy;
}
}
