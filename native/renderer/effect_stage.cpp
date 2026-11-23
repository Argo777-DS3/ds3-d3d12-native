// FFX-DRAW N6: see effect_stage.h. The bank tables are the original's descriptors as its D3D11 objects describe themselves
// (Reference1956, reference_state_bank rows; D3D12's enumerations have the D3D11 values).
#include "effect_stage.h"
#include "backend.h"   // axis 1: the pipeline cache
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace ds3native {
namespace {
using Microsoft::WRL::ComPtr;
struct Rt {uint8_t enable,src,dst,op,src_alpha,dst_alpha,op_alpha,mask;};
struct BlendRow {uint32_t offset;uint8_t a2c,independent;Rt rt0,rt1;};
constexpr Rt R(uint8_t e,uint8_t s,uint8_t d,uint8_t o,uint8_t sa,uint8_t da,uint8_t oa,uint8_t m) {return {e,s,d,o,sa,da,oa,m};}
// +0x08..+0x80: FFX types 0 off, 1 alpha, 4 multiply, 2 additive, 3 reverse subtract (alpha ONE/ZERO), then their mask-7 copies and
// the alpha ZERO/ZERO set; +0x88..+0xC8 independent; +0xD0..+0x190 the alpha-to-coverage copies; +0x198..+0x1D0 the twins.
constexpr std::array<BlendRow,15> base_blends{{
    {0x08,0,0,R(0,2,1,1,2,1,1,15),R(0,2,1,1,2,1,1,15)},{0x10,0,0,R(1,5,6,1,2,1,1,15),R(1,5,6,1,2,1,1,15)},
    {0x18,0,0,R(1,9,1,1,2,1,1,15),R(1,9,1,1,2,1,1,15)},{0x20,0,0,R(1,5,2,1,2,1,1,15),R(1,5,2,1,2,1,1,15)},
    {0x28,0,0,R(1,5,2,3,2,1,1,15),R(1,5,2,3,2,1,1,15)},{0x30,0,0,R(0,2,1,1,2,1,1,7),R(0,2,1,1,2,1,1,7)},
    {0x38,0,0,R(1,5,6,1,2,1,1,7),R(1,5,6,1,2,1,1,7)},{0x40,0,0,R(1,9,1,1,2,1,1,7),R(1,9,1,1,2,1,1,7)},
    {0x48,0,0,R(1,5,2,1,2,1,1,7),R(1,5,2,1,2,1,1,7)},{0x50,0,0,R(1,5,2,3,2,1,1,7),R(1,5,2,3,2,1,1,7)},
    {0x58,0,0,R(0,2,1,1,2,1,1,7),R(0,2,1,1,2,1,1,7)},{0x60,0,0,R(1,5,6,1,1,1,1,15),R(1,5,6,1,1,1,1,15)},
    {0x68,0,0,R(1,9,1,1,1,1,1,15),R(1,9,1,1,1,1,1,15)},{0x70,0,0,R(1,5,2,1,1,1,1,15),R(1,5,2,1,1,1,1,15)},
    {0x78,0,0,R(1,5,2,3,1,1,1,15),R(1,5,2,3,1,1,1,15)}}};
constexpr std::array<BlendRow,10> independent_blends{{
    {0x80,0,0,R(0,2,1,1,2,1,1,15),R(0,2,1,1,2,1,1,15)},{0x88,0,1,R(1,5,6,1,2,1,1,7),R(1,5,6,1,2,1,1,7)},
    {0x90,0,1,R(1,9,1,1,2,1,1,7),R(1,9,1,1,2,1,1,7)},{0x98,0,1,R(1,5,2,1,2,1,1,7),R(1,5,2,1,2,1,1,7)},
    {0xA0,0,1,R(1,5,2,3,2,1,1,7),R(1,5,2,3,2,1,1,7)},{0xA8,0,1,R(0,2,1,1,2,1,1,0),R(0,2,1,1,2,1,1,15)},
    {0xB0,0,1,R(1,5,6,1,2,1,1,0),R(1,5,6,1,2,1,1,7)},{0xB8,0,1,R(1,9,1,1,2,1,1,0),R(1,9,1,1,2,1,1,7)},
    {0xC0,0,1,R(1,5,2,1,2,1,1,0),R(1,5,2,1,2,1,1,7)},{0xC8,0,1,R(1,5,2,3,2,1,1,0),R(1,5,2,3,2,1,1,7)}}};
constexpr std::array<BlendRow,8> twin_blends{{
    {0x198,0,0,R(0,2,1,1,2,1,1,0),R(0,2,1,1,2,1,1,0)},{0x1A0,0,0,R(1,2,1,1,1,1,1,15),R(1,2,1,1,1,1,1,15)},
    {0x1A8,0,0,R(1,5,6,1,1,6,1,15),R(1,5,6,1,1,6,1,15)},{0x1B0,0,0,R(1,9,1,1,1,6,1,15),R(1,9,1,1,1,6,1,15)},
    {0x1B8,0,0,R(1,5,2,1,1,2,1,15),R(1,5,2,1,1,2,1,15)},{0x1C0,0,0,R(1,2,5,1,1,5,1,15),R(1,2,5,1,1,5,1,15)},
    {0x1C8,0,0,R(1,2,1,3,2,2,3,15),R(1,2,1,3,2,2,3,15)},{0x1D0,0,0,R(1,2,5,1,1,2,1,15),R(1,2,5,1,1,2,1,15)}}};
bool FindBlend(uint32_t offset,BlendRow& out) {
    for(const auto& r:base_blends)if(r.offset==offset){out=r;return true;}
    for(const auto& r:independent_blends)if(r.offset==offset){out=r;return true;}
    for(const auto& r:twin_blends)if(r.offset==offset){out=r;return true;}
    // +0xD0..+0x190: the alpha-to-coverage copies of +0x08..+0xC8 (0xC8 apart).
    if(offset>=0xD0 && offset<=0x190 && FindBlend(offset-0xC8,out)){out.offset=offset;out.a2c=1;return true;}
    return false;}
D3D12_RENDER_TARGET_BLEND_DESC Target(const Rt& r) {
    D3D12_RENDER_TARGET_BLEND_DESC d{};d.BlendEnable=r.enable;d.SrcBlend=D3D12_BLEND(r.src);d.DestBlend=D3D12_BLEND(r.dst);d.BlendOp=D3D12_BLEND_OP(r.op);
    d.SrcBlendAlpha=D3D12_BLEND(r.src_alpha);d.DestBlendAlpha=D3D12_BLEND(r.dst_alpha);d.BlendOpAlpha=D3D12_BLEND_OP(r.op_alpha);
    d.LogicOp=D3D12_LOGIC_OP_NOOP;d.RenderTargetWriteMask=r.mask;return d;}
// Depth +0x1D8..+0x200: {enable, write, function, stencil}; +0x200 marks the stencil (pass REPLACE, ALWAYS, read 0).
bool FindDepth(uint32_t offset,D3D12_DEPTH_STENCIL_DESC& d) {
    struct Row {uint32_t offset;uint8_t enable,write,func,stencil;};
    constexpr Row rows[]{{0x1D8,0,1,2,0},{0x1E0,1,1,4,0},{0x1E8,1,0,4,0},{0x1F0,1,0,2,0},{0x1F8,1,1,8,0},{0x200,0,1,2,1}};
    for(const auto& r:rows)if(r.offset==offset){
        d={};d.DepthEnable=r.enable;d.DepthWriteMask=D3D12_DEPTH_WRITE_MASK(r.write);d.DepthFunc=D3D12_COMPARISON_FUNC(r.func);
        d.StencilEnable=r.stencil;d.StencilReadMask=r.stencil?0:0xFF;d.StencilWriteMask=0xFF;
        const D3D12_DEPTH_STENCILOP_DESC op{D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,r.stencil?D3D12_STENCIL_OP_REPLACE:D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};
        d.FrontFace=op;d.BackFace=op;return true;}
    return false;}
// Raster +0x208..+0x2C0: 24 entries in four groups of six (solid, wireframe, then both multisampled), each {cull none, back,
// front} without and with scissor; counter-clockwise front faces, depth clip on, no bias.
bool FindRaster(uint32_t offset,D3D12_RASTERIZER_DESC& d) {
    if(offset<0x208 || offset>0x2C0 || (offset-0x208)%8)return false;
    const uint32_t k=(offset-0x208)/8,group=k/6,cull=(k%6)/2;
    d={};d.FillMode=(group%2)?D3D12_FILL_MODE_WIREFRAME:D3D12_FILL_MODE_SOLID;
    d.CullMode=cull==0?D3D12_CULL_MODE_NONE:cull==1?D3D12_CULL_MODE_BACK:D3D12_CULL_MODE_FRONT;
    d.FrontCounterClockwise=TRUE;d.DepthClipEnable=TRUE;d.MultisampleEnable=group>=2;
    d.ConservativeRaster=D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;return true;}
// I-0140 tranche 10: whether a raster entry enables the scissor (the second of each pair above).
bool RasterScissor(uint32_t offset) {return offset>=0x208 && offset<=0x2C0 && !((offset-0x208)%8) && ((offset-0x208)/8%6)%2==1;}
// Samplers +0x2C8..+0x2F8: point, linear, linear with mip point and max LOD 0 (wrap), the same three clamped, point border white.
bool FindSampler(uint32_t offset,D3D12_SAMPLER_DESC& d) {
    if(offset<0x2C8 || offset>0x2F8 || (offset-0x2C8)%8)return false;
    const uint32_t k=(offset-0x2C8)/8;
    constexpr D3D12_FILTER filters[]{D3D12_FILTER_MIN_MAG_MIP_POINT,D3D12_FILTER_MIN_MAG_MIP_LINEAR,D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT};
    d={};d.Filter=filters[k%3];
    const auto address=k<3?D3D12_TEXTURE_ADDRESS_MODE_WRAP:k<6?D3D12_TEXTURE_ADDRESS_MODE_CLAMP:D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    d.AddressU=d.AddressV=d.AddressW=address;d.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;
    if(k==6){d.Filter=D3D12_FILTER_MIN_MAG_MIP_POINT;for(auto& c:d.BorderColor)c=1.0f;}
    d.MinLOD=0;d.MaxLOD=(k%3==2 && k<6)?0.0f:D3D12_FLOAT32_MAX;return true;}
void Check(HRESULT hr,const char* what) {if(FAILED(hr))throw std::runtime_error(what);}
D3D12_CPU_DESCRIPTOR_HANDLE At(ID3D12DescriptorHeap* heap,uint32_t stride,uint32_t index) {auto h=heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=size_t(stride)*index;return h;}
D3D12_GPU_DESCRIPTOR_HANDLE AtGpu(ID3D12DescriptorHeap* heap,uint32_t stride,uint32_t index) {auto h=heap->GetGPUDescriptorHandleForHeapStart();h.ptr+=uint64_t(stride)*index;return h;}
D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType(uint32_t topology) {
    if(topology==D3D_PRIMITIVE_TOPOLOGY_POINTLIST)return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    if(topology==D3D_PRIMITIVE_TOPOLOGY_LINELIST || topology==D3D_PRIMITIVE_TOPOLOGY_LINESTRIP ||
       topology==D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ || topology==D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ)return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    if(topology>=D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST)return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;}
// I-0104: a constant block's root view, a range of the frame's uploaded arena (256-aligned; {0, 0} the arena's zero block).
template<class Set> bool Constants(const NativeEffectFrame& frame,D3D12_GPU_VIRTUAL_ADDRESS arena,const NativeEffectRange& range,Set&& set) {
    if(range.offset%256 || uint64_t(range.offset)+(range.size?range.size:effect_arena_zero)>frame.arena.size())return false;
    set(arena+range.offset);return true;}
}

bool NativeEffectBankBlend(uint32_t offset,D3D12_BLEND_DESC& out) {
    BlendRow row{};if(!FindBlend(offset,row))return false;
    out={};out.AlphaToCoverageEnable=row.a2c;out.IndependentBlendEnable=row.independent;
    out.RenderTarget[0]=Target(row.rt0);for(int i=1;i<8;++i)out.RenderTarget[i]=Target(row.rt1);return true;}
bool NativeEffectBankDepth(uint32_t offset,D3D12_DEPTH_STENCIL_DESC& out) {return FindDepth(offset,out);}

void EffectStage::Prepare(ID3D12Device* device) {
    if(prepared_)return;   // review: set last, so a failed preparation is retried, never half-used
    // Root: 0 VS b0, 1 DS b0, 2 PS b0, 3 PS b1, 4 PS b6 (root views); 5 PS t0..t4; 6 PS s9 (one descriptor); s1 static anisotropic wrap
    // (INFERRED: I-0063 saw an anisotropic wrap sampler left bound at s1 by an earlier pass; the bank has none; the E01 programmes
    // declare no s1).
    D3D12_ROOT_PARAMETER params[7]{};
    const auto cbv=[&](int i,uint32_t reg,D3D12_SHADER_VISIBILITY v){params[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;params[i].Descriptor.ShaderRegister=reg;params[i].ShaderVisibility=v;};
    cbv(0,0,D3D12_SHADER_VISIBILITY_VERTEX);cbv(1,0,D3D12_SHADER_VISIBILITY_DOMAIN);cbv(2,0,D3D12_SHADER_VISIBILITY_PIXEL);
    cbv(3,1,D3D12_SHADER_VISIBILITY_PIXEL);cbv(4,6,D3D12_SHADER_VISIBILITY_PIXEL);
    D3D12_DESCRIPTOR_RANGE srv{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,5,0,0,0};
    params[5].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[5].DescriptorTable={1,&srv};params[5].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_DESCRIPTOR_RANGE sampler{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,1,9,0,0};
    params[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[6].DescriptorTable={1,&sampler};params[6].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC wrap{};wrap.Filter=D3D12_FILTER_ANISOTROPIC;wrap.AddressU=wrap.AddressV=wrap.AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    wrap.MaxAnisotropy=16;wrap.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;wrap.MaxLOD=D3D12_FLOAT32_MAX;wrap.ShaderRegister=1;wrap.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC desc{7,params,1,&wrap,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
    ComPtr<ID3DBlob> blob,error;
    Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"effect root signature serialization");
    Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),root_.ReleaseAndGetAddressOf()),"effect root signature");
    D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,views_per_slot*3,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    Check(device->CreateDescriptorHeap(&views,IID_PPV_ARGS(&views_)),"effect view heap");
    D3D12_DESCRIPTOR_HEAP_DESC samplers{D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,7,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    Check(device->CreateDescriptorHeap(&samplers,IID_PPV_ARGS(&samplers_)),"effect sampler heap");
    view_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    sampler_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    for(uint32_t i=0;i<7;++i){D3D12_SAMPLER_DESC d{};FindSampler(0x2C8+8*i,d);device->CreateSampler(&d,At(samplers_.Get(),sampler_stride_,i));}
    D3D12_DESCRIPTOR_HEAP_DESC cpu{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,5,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};   // the depth copy, the scene copy, the layer's depth, the two effect targets
    Check(device->CreateDescriptorHeap(&cpu,IID_PPV_ARGS(&cpu_views_)),"effect CPU view heap");
    prepared_=true;
}

size_t EffectStage::PipelineKeyHash::operator()(const PipelineKey& key) const noexcept {
    uint64_t h=0xCBF29CE484222325ull;   // FNV-1a over the key's words
    const auto mix=[&](uint64_t v){for(int i=0;i<8;++i){h^=(v>>(8*i))&0xFF;h*=0x100000001B3ull;}};
    for(const uint64_t v:{key.vs,key.hs,key.ds,key.gs,key.ps,key.cs,key.layout})mix(v);
    for(const uint64_t v:{uint64_t(key.blend),uint64_t(key.depth),uint64_t(key.raster),uint64_t(key.topology_type),uint64_t(key.targets),uint64_t(key.kind),
        uint64_t(key.hdr),uint64_t(key.gb5),uint64_t(key.depth_format)})mix(v);
    return static_cast<size_t>(h);}

ID3D12PipelineState* EffectStage::Pipeline(ID3D12Device* device,const NativeEffectDraw& draw,const Targets& targets) {
    const PipelineKey key{draw.vs.key,draw.hs.key,draw.ds.key,0,draw.ps.key,0,draw.layout?draw.layout->key:0,draw.blend,draw.depth,draw.raster,
        static_cast<uint32_t>(TopologyType(draw.topology)),draw.targets,draw.pass==0x21?3u:0u,targets.hdr_format,targets.gb5_format,targets.depth_format};
    if(const auto found=pipelines_.find(key);found!=pipelines_.end())return found->second.Get();
    if(const auto failed=failed_.find(key);failed!=failed_.end()){stats_.last_failure=failed->second;return nullptr;}
    const auto refuse=[&](const std::string& why)->ID3D12PipelineState*{failed_[key]=why;stats_.last_failure=why;++stats_.pipeline_failures;return nullptr;};
    if(!draw.vs.bytes || !draw.ps.bytes || !draw.layout)return refuse("programme or layout missing");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};d.pRootSignature=root_.Get();
    d.VS={draw.vs.bytes->data(),draw.vs.bytes->size()};d.PS={draw.ps.bytes->data(),draw.ps.bytes->size()};
    if(draw.hs.bytes)d.HS={draw.hs.bytes->data(),draw.hs.bytes->size()};
    if(draw.ds.bytes)d.DS={draw.ds.bytes->data(),draw.ds.bytes->size()};
    if(!NativeEffectBankBlend(draw.blend,d.BlendState))return refuse("blend entry unknown");
    d.SampleMask=UINT_MAX;
    if(!FindRaster(draw.raster,d.RasterizerState))return refuse("raster entry unknown");
    if(!FindDepth(draw.depth,d.DepthStencilState))return refuse("depth entry unknown");
    // The bank entry's test, its write mask zero (explicit policy: whether the original's effect pass binds the depth writable is not
    // read, I-0104 DESIGN §2; I-0113 tranche 15 binds the writable view for the 0x29 rows between the effects, t3 reading a copy).
    // I-0140 (STUDY-EFFECT-PASSES-20-21): pass 0x21's draws (kind 1 entities, mode 1 of 140299E40) are opaque and write the depth
    // (the commit 140264230 binds B+0x08 and B+0x1E0; i140-e5a: every 0x21 draw at the Sister Friede bonfire binds 8 and 1E0): the
    // bank's write mask kept for them (key kind 3).
    if(draw.pass!=0x21)d.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
    d.DepthStencilState.StencilEnable=FALSE;
    if(targets.depth_format==DXGI_FORMAT_UNKNOWN)d.DepthStencilState.DepthEnable=FALSE;   // I-0140 tranche 10: a half-size effect target has no depth
    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    for(const auto& e:draw.layout->elements)elements.push_back({e.semantic.c_str(),e.index,e.format,0,e.offset,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0});
    d.InputLayout={elements.data(),static_cast<UINT>(elements.size())};
    d.PrimitiveTopologyType=TopologyType(draw.topology);
    d.NumRenderTargets=draw.targets;d.RTVFormats[0]=targets.hdr_format;if(draw.targets>1)d.RTVFormats[1]=targets.gb5_format;
    d.DSVFormat=targets.depth_format;d.SampleDesc.Count=1;
    // I-0072: the device's pipeline cache, named by the whole description (the key holds engine object addresses).
    ComPtr<ID3D12PipelineState> pipeline;const auto hr=Pipelines().Graphics(device,d,&pipeline);
    if(FAILED(hr)){char why[96]{};sprintf_s(why,"pipeline creation 0x%08X",static_cast<unsigned>(hr));return refuse(why);}
    ++stats_.pipelines;auto* raw=pipeline.Get();pipelines_.emplace(key,std::move(pipeline));return raw;
}

void EffectStage::Record(ID3D12Device* device,ID3D12GraphicsCommandList* list,const NativeEffectFrame& frame,const std::vector<int32_t>& passes,
    const Targets& all,const D3D12_VIEWPORT& full_viewport,const D3D12_RECT& full_scissor,uint32_t slot,UploadSink& uploads) {
    // I-0127 (effect-measure.flag): the frame's measure, its time written however Record returns; the callers' rows and scene
    // copy (before_pass, before_draw) timed apart.
    const bool measured=frame.measure;auto& m=stats_.measure;m.recorded=measured;   // reset by Encode
    struct Clock {EffectFrameMeasure* m{};LARGE_INTEGER start{};
        ~Clock(){if(!m)return;LARGE_INTEGER end{};QueryPerformanceCounter(&end);const uint64_t all=uint64_t(end.QuadPart-start.QuadPart);m->ticks=all>m->callback_ticks?all-m->callback_ticks:0;}};
    Clock clock{measured?&m:nullptr};if(measured){QueryPerformanceCounter(&clock.start);m.arena_bytes=frame.arena.size();}
    const auto timed=[&](auto&& call)->bool{if(!measured)return call();LARGE_INTEGER a{},b{};QueryPerformanceCounter(&a);const bool r=call();
        QueryPerformanceCounter(&b);m.callback_ticks+=uint64_t(b.QuadPart-a.QuadPart);return r;};
    // The state this list has from the stage's last calls (unknown after a bind, a hook's rows or a GPU-particle statement).
    struct Held {ID3D12PipelineState* pipeline{};uint32_t topology{UINT32_MAX},targets{},sampler{UINT32_MAX};D3D12_CPU_DESCRIPTOR_HANDLE rtv0{},rtv1{},dsv{};
        D3D12_VIEWPORT viewport{};D3D12_RECT scissor{};bool rects{};   // I-0140 tranche 10: as bound (rects: known; bind sets the pass's)
        uint32_t table{UINT32_MAX};std::array<D3D12_GPU_VIRTUAL_ADDRESS,5> constants{};
        D3D12_VERTEX_BUFFER_VIEW vertices{};D3D12_INDEX_BUFFER_VIEW indices{};} held;
    // `filtered`: a call not made, the list already holding that state (counted in `repeats`); else a call made.
    const auto note=[&](EffectCall kind,bool filtered,uint32_t n=1){if(!measured)return;(filtered?m.repeats:m.calls)[kind]+=n;};
    tables_.clear();   // I-0127 step B: this frame's texture tables
    // E07 (review): the records first, before any early return, so releases and creation bytes are never lost.
    if(!frame.objects.empty() || !frame.released.empty() || !pending_released_.empty() || std::any_of(frame.draws.begin(),frame.draws.end(),[](const NativeEffectDraw& d){return d.op!=NativeEffectOp::Draw && d.op!=NativeEffectOp::CopyRect && d.op!=NativeEffectOp::ClearTarget && d.op!=NativeEffectOp::CopyTarget;})){   // L8: a copy needs no compute root
        PrepareGpu(device);Objects(device,list,frame,uploads);}
    if(frame.draws.empty())return;
    for(const auto& draw:frame.draws)if(std::find(passes.begin(),passes.end(),draw.pass)==passes.end())++stats_.other_passes;   // 0x20 (not reached, I-0140): captured, not drawn
    // The t3 and t4 substitutes: views of the depth copy (its depth plane; I-0113 tranche 15, null when no statement reads it) and of
    // the scene-colour copy, remade when the resource changes.
    const auto make=[&](ID3D12Resource* resource,DXGI_FORMAT format,ID3D12Resource*& viewed,uint32_t index){
        if(!resource){viewed=nullptr;return;}   // review: an absent view is remade when its resource returns
        if(resource==viewed && all.target_generation==viewed_generation_)return;
        D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=format;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        v.Texture2D.MipLevels=1;device->CreateShaderResourceView(resource,&v,At(cpu_views_.Get(),view_stride_,index));viewed=resource;};
    make(all.depth,all.depth_view_format,viewed_depth_,0);make(all.scene_copy,all.scene_copy_format,viewed_copy_,1);
    // I-0113 tranche 17 (STUDY-PASS-31): the half-size layer's depth, which pass 0x31 tests (read-only) and samples at t3.
    make(all.layer?all.layer->depth:nullptr,all.layer?all.layer->depth_view_format:DXGI_FORMAT_UNKNOWN,viewed_layer_depth_,2);
    viewed_generation_=all.target_generation;
    // The pass's targets: the half-size layer's for 0x31 when the caller gives one, its viewport and scissor with them.
    const Targets* current=&all;const D3D12_VIEWPORT* viewport=&full_viewport;const D3D12_RECT* scissor=&full_scissor;
    // The arena once (the vertex, index and constant views address it), 256-aligned.
    const auto arena=uploads.Get(std::max<uint64_t>(frame.arena.size(),256),false);   // written whole below (its zero block included)
    if(!arena.cpu)return;
    std::memcpy(arena.cpu,frame.arena.data(),frame.arena.size());
    // I-0140 tranche 10: the half-size effect targets, made when the frame uses them (a draw into one, a view of one, a clear, a copy).
    const bool targets_ready=std::any_of(frame.draws.begin(),frame.draws.end(),[](const NativeEffectDraw& d){
            return d.target || d.op==NativeEffectOp::ClearTarget || d.op==NativeEffectOp::CopyTarget ||
                std::any_of(d.textures.begin(),d.textures.end(),[](const NativeEffectTexture& t){return t.kind==NativeEffectTextureKind::Target;});}) &&
        EffectTargets(device,frame.target_size);
    ID3D12DescriptorHeap* heaps[]{views_.Get(),samplers_.Get()};
    const auto bind=[&]{list->SetDescriptorHeaps(2,heaps);list->SetGraphicsRootSignature(root_.Get());list->RSSetViewports(1,viewport);list->RSSetScissorRects(1,scissor);
        held=Held{};held.viewport=*viewport;held.scissor=*scissor;held.rects=true;note(EffectCallHeaps,false);};
    bind();
    uint32_t next=slot%3*views_per_slot;const uint32_t end=next+views_per_slot;
    D3D12_SHADER_RESOURCE_VIEW_DESC null_view{};null_view.Format=DXGI_FORMAT_R8G8B8A8_UNORM;null_view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    null_view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;null_view.Texture2D.MipLevels=1;
    bool e01_root_dirty=false;   // a particle draw set its own graphics root
    // E07 witness (ffx-record.flag only): this frame's readback slot (written 8 frames ago: read first, then reused).
    const uint32_t readback=static_cast<uint32_t>(readback_frames_++%readbacks_.size());bool draw_args_read{},dispatch_args_read{};
    if(!frame.witness){}
    else if(readbacks_[readback]){void* p{};const D3D12_RANGE range{0,672};if(SUCCEEDED(readbacks_[readback]->Map(0,&range,&p))){
        std::memcpy(stats_.args.data(),p,32);std::memcpy(stats_.particles.data(),static_cast<const uint8_t*>(p)+256,sizeof(stats_.particles));std::memcpy(stats_.counters.data(),static_cast<const uint8_t*>(p)+640,32);const D3D12_RANGE none{};readbacks_[readback]->Unmap(0,&none);}}
    else if(compute_root_){D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=1024;d.Height=1;
        d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readbacks_[readback]));}
    // E07 witness: the hidden counters of the first eight counter slots (assigned in first-use order), into the slot at 640, from the
    // previous frame's end state (copied before this frame's statements).
    if(frame.witness && readbacks_[readback] && !counter_pools_.empty()){auto& pool=counter_pools_[0];Require(list,pool,D3D12_RESOURCE_STATE_COPY_SOURCE);
        for(uint32_t i=0;i<8 && i<counter_next_;++i)list->CopyBufferRegion(readbacks_[readback].Get(),640+4*i,pool.resource.Get(),uint64_t(i)*D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT,4);
        Require(list,pool,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}
    const auto witness=[&](GpuBuffer& args,uint64_t offset,bool& done){if(done || !frame.witness || !readbacks_[readback])return;done=true;
        Require(list,args,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readbacks_[readback].Get(),offset,args.resource.Get(),0,16);};
    for(const int32_t pass:passes){
    // I-0113 tranche 15: a hook that drew rows of its own leaves their state bound: the stage's is bound again.
    const auto rebind=[&](bool drew){if(!drew)return;bind();e01_root_dirty=false;++stats_.row_runs;};
    const bool layered=pass==0x31 && all.layer;
    if(all.before_pass && std::any_of(frame.draws.begin(),frame.draws.end(),[&](const NativeEffectDraw& d){return d.pass==pass;}))rebind(timed([&]{return all.before_pass(pass);}));
    {const auto* next_targets=layered?all.layer:&all;   // the targets change between passes: the viewport bound again with them
        if(next_targets!=current){current=next_targets;viewport=layered?&all.layer->viewport:&full_viewport;scissor=layered?&all.layer->scissor:&full_scissor;bind();e01_root_dirty=false;}}
    const Targets& targets=*current;const uint32_t depth_index=layered?2:0;
    NativeEffectDraw single;   // the layer has one target: the original binds one RTV there, a second output is discarded
    for(const auto& listed:frame.draws){
        if(listed.pass!=pass)continue;
        const bool narrowed=layered && listed.targets>1;if(narrowed){single=listed;single.targets=1;}   // rare: a copy for this draw only
        const NativeEffectDraw& draw=narrowed?single:listed;
        if(all.before_draw)rebind(timed([&]{return all.before_draw(pass,draw.key);}));
        if(measured)++m.statements;
        if(draw.op==NativeEffectOp::CopyRect){   // no state of the stage's changes (the target is back to RENDER_TARGET after it)
            if(!layered && all.copy_rect){all.copy_rect(draw);++stats_.copy_rects;}
            continue;}
        if(draw.op==NativeEffectOp::ClearTarget || draw.op==NativeEffectOp::CopyTarget){   // I-0140 tranche 10: no state of the stage's changes
            const bool copy=draw.op==NativeEffectOp::CopyTarget;
            if(!targets_ready || draw.target<1 || draw.target>2 || (copy && (draw.copy_source<1 || draw.copy_source>2 || draw.copy_source==draw.target))){++stats_.skipped;continue;}
            const uint32_t i=draw.target-1u;
            if(!copy){EffectTargetState(list,i,D3D12_RESOURCE_STATE_RENDER_TARGET);
                list->ClearRenderTargetView(At(target_rtvs_.Get(),rtv_stride_,i),draw.clear_colour.data(),0,nullptr);++stats_.target_clears;}
            else{const uint32_t from=draw.copy_source-1u;EffectTargetState(list,i,D3D12_RESOURCE_STATE_COPY_DEST);EffectTargetState(list,from,D3D12_RESOURCE_STATE_COPY_SOURCE);
                list->CopyResource(effect_targets_[i].Get(),effect_targets_[from].Get());++stats_.target_copies;}
            continue;}
        if(draw.op!=NativeEffectOp::Draw){   // E07: the GPU-particle statements, in their place in the stream
            ++stats_.gpu_statements;held=Held{};   // I-0127: their pipelines, roots and targets
            if(!compute_root_ || next+8>end){++stats_.gpu_skipped;continue;}
            const auto source_of=[&](const NativeEffectTexture& t)->D3D12_CPU_DESCRIPTOR_HANDLE{
                if(t.kind==NativeEffectTextureKind::Texture)return t.view;
                if(t.kind==NativeEffectTextureKind::Depth && targets.depth)return At(cpu_views_.Get(),view_stride_,depth_index);
                if(t.kind==NativeEffectTextureKind::SceneCopy && targets.scene_copy)return At(cpu_views_.Get(),view_stride_,1);
                return {};};
            // I-0120 D2: false when a bound view has nothing to read (the depth or copy absent, an effect buffer not made yet: D1's
            // wait for its creation bytes); the statement is then skipped, never run on a null view (D-0197).
            const auto shader_view=[&](const NativeEffectTexture& t,uint32_t index,D3D12_RESOURCE_STATES state){
                const auto dst=At(views_.Get(),view_stride_,index);
                if(t.kind==NativeEffectTextureKind::Object && View(device,t.object,dst,false)){if(auto* b=BufferOf(t.object))Require(list,*b,state);return true;}
                if(const auto src=source_of(t);src.ptr){device->CopyDescriptorsSimple(1,dst,src,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);return true;}
                device->CreateShaderResourceView(nullptr,&null_view,dst);return t.kind==NativeEffectTextureKind::None;};
            const auto root_view=[&](uint32_t slot,auto&& set){return Constants(frame,arena.gpu,draw.constants[slot],set);};
            D3D12_RESOURCE_BARRIER uav_barrier{};uav_barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
            if(draw.op==NativeEffectOp::Dispatch || draw.op==NativeEffectOp::DispatchIndirect){
                auto* pipeline=ComputePipeline(device,draw);auto* args=draw.op==NativeEffectOp::DispatchIndirect?BufferOf(draw.args):nullptr;
                if(!pipeline || (draw.op==NativeEffectOp::DispatchIndirect && !args)){++stats_.gpu_skipped;continue;}
                const uint32_t t=next;next+=8;
                bool bound=true;for(uint32_t i=0;i<5;++i)bound&=shader_view(draw.cs_resources[i],t+i,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                if(!bound){++stats_.missing_textures;++stats_.gpu_skipped;continue;}
                // I-0120 D2 (the final review's L1): every UAV's view first, so a dispatch skipped for one not made yet resets no counter.
                for(uint32_t i=0;i<3 && bound;++i){const auto dst=At(views_.Get(),view_stride_,t+5+i);
                    if(draw.cs_uavs[i])bound=View(device,draw.cs_uavs[i],dst,true,list,&uploads);
                    else{D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R32_UINT;u.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;device->CreateUnorderedAccessView(nullptr,nullptr,&u,dst);}}
                if(!bound){++stats_.gpu_skipped;continue;}
                for(uint32_t i=0;i<3;++i)if(draw.cs_uavs[i]){if(auto* b=BufferOf(draw.cs_uavs[i]))Require(list,*b,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                    // The bind's initial count (not -1): written into the view's counter, as D3D11's CSSetUnorderedAccessViews does.
                    if(draw.cs_uav_counts_set>>i&1)if(const auto counter=counter_slots_.find(draw.cs_uavs[i]);counter!=counter_slots_.end()){
                        auto& pool=counter_pools_[counter->second/64];const auto value=uploads.Get(256);
                        if(value.cpu && value.resource){std::memcpy(value.cpu,&draw.cs_uav_counts[i],4);Require(list,pool,D3D12_RESOURCE_STATE_COPY_DEST);
                            list->CopyBufferRegion(pool.resource.Get(),uint64_t(counter->second%64)*D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT,value.resource,value.offset,4);
                            Require(list,pool,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);++stats_.counter_writes;}}}
                list->SetComputeRootSignature(compute_root_.Get());list->SetPipelineState(pipeline);
                bool constants=true;
                for(const auto [constant,root]:{std::pair{uint32_t(EffectCsB0),0u},{uint32_t(EffectCsB1),1u},{uint32_t(EffectCsB5),2u}})
                    constants&=root_view(constant,[&](D3D12_GPU_VIRTUAL_ADDRESS a){list->SetComputeRootConstantBufferView(root,a);});
                if(!constants){++stats_.gpu_skipped;continue;}
                list->SetComputeRootDescriptorTable(3,AtGpu(views_.Get(),view_stride_,t));list->SetComputeRootDescriptorTable(4,AtGpu(views_.Get(),view_stride_,t+5));
                if(args){witness(*args,16,dispatch_args_read);Require(list,*args,D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);list->ExecuteIndirect(dispatch_signature_.Get(),1,args->resource.Get(),0,nullptr,0);++stats_.indirect_dispatches;}
                else{list->Dispatch(draw.groups[0],draw.groups[1],draw.groups[2]);++stats_.dispatches;}
                list->ResourceBarrier(1,&uav_barrier);}
            else if(draw.op==NativeEffectOp::Copy){
                auto* dst=BufferOf(draw.destination);auto* src=BufferOf(draw.source);if(!dst || !src){++stats_.gpu_skipped;continue;}
                Require(list,*dst,D3D12_RESOURCE_STATE_COPY_DEST);Require(list,*src,D3D12_RESOURCE_STATE_COPY_SOURCE);
                list->CopyBufferRegion(dst->resource.Get(),0,src->resource.Get(),0,std::min(dst->bytes,src->bytes));++stats_.copies;}
            else if(draw.op==NativeEffectOp::ClearUav){
                // D3D12 refuses the clear on a structured view; the original's (D3D11) writes the first value to every 32-bit word of a
                // structured view, the four values to a typed one. Natively: an R32_UINT view of the whole buffer, the first value, for a
                // structured view; the view itself otherwise.
                auto* buffer=BufferOf(draw.clear_view);const auto view=objects_.find(draw.clear_view);const uint32_t t=next++;
                const auto cpu=At(clear_views_.Get(),view_stride_,clear_next_++%64),gpu_at=At(views_.Get(),view_stride_,t);
                if(!buffer || view==objects_.end()){++stats_.gpu_skipped;continue;}
                const bool structured=view->second.description[0]==0;std::array<uint32_t,4> values=draw.clear_values;
                if(structured){D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R32_UINT;u.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
                    // review: the view's elements only (D3D11 clears FirstElement..NumElements), as 32-bit words
                    const auto parent=objects_.find(view->second.parent);const uint64_t stride=parent!=objects_.end()?std::max<uint32_t>(parent->second.description[5],4):4;
                    const uint64_t first=uint64_t(view->second.description[2])*stride/4,words=std::min<uint64_t>(uint64_t(view->second.description[3])*stride/4,buffer->bytes/4>first?buffer->bytes/4-first:0);
                    if(!words){++stats_.gpu_skipped;continue;}
                    u.Buffer.FirstElement=first;u.Buffer.NumElements=static_cast<UINT>(words);device->CreateUnorderedAccessView(buffer->resource.Get(),nullptr,&u,gpu_at);
                    device->CreateUnorderedAccessView(buffer->resource.Get(),nullptr,&u,cpu);values.fill(draw.clear_values[0]);}
                else if(!View(device,draw.clear_view,gpu_at,true) || !View(device,draw.clear_view,cpu,true)){++stats_.gpu_skipped;continue;}
                Require(list,*buffer,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                list->ClearUnorderedAccessViewUint(AtGpu(views_.Get(),view_stride_,t),cpu,buffer->resource.Get(),values.data(),0,nullptr);++stats_.clears;
                list->ResourceBarrier(1,&uav_barrier);}
            else{   // the particles' indirect draw
                auto* pipeline=ParticlePipeline(device,draw,targets);auto* args=BufferOf(draw.args);
                if(!pipeline || !args){++stats_.gpu_skipped;continue;}
                const uint32_t t=next;next+=6;
                // Witness: the first two particles of VS t0 (2 x 176 bytes) into the readback slot at 256.
                if(!draw_args_read && frame.witness && readbacks_[readback] && draw.vs_resources[0].kind==NativeEffectTextureKind::Object)
                    if(auto* particles=BufferOf(draw.vs_resources[0].object);particles && particles->bytes>=sizeof(stats_.particles)){
                        Require(list,*particles,D3D12_RESOURCE_STATE_COPY_SOURCE);
                        list->CopyBufferRegion(readbacks_[readback].Get(),256,particles->resource.Get(),0,sizeof(stats_.particles));}
                bool bound=true;for(uint32_t i=0;i<2;++i)bound&=shader_view(draw.vs_resources[i],t+i,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                for(uint32_t i=0;i<4;++i)bound&=shader_view(draw.textures[i],t+2+i,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                if(!bound){++stats_.missing_textures;++stats_.gpu_skipped;continue;}
                list->SetGraphicsRootSignature(particle_root_.Get());e01_root_dirty=true;
                bool constants=true;
                for(const auto [constant,root]:{std::pair{uint32_t(EffectGsB9),0u},{uint32_t(EffectPsB0),1u},{uint32_t(EffectPsB6),2u},{uint32_t(EffectPsB9),3u},{uint32_t(EffectPsB11),4u}})
                    constants&=root_view(constant,[&](D3D12_GPU_VIRTUAL_ADDRESS a){list->SetGraphicsRootConstantBufferView(root,a);});
                if(!constants){++stats_.gpu_skipped;continue;}
                list->SetGraphicsRootDescriptorTable(5,AtGpu(views_.Get(),view_stride_,t));list->SetGraphicsRootDescriptorTable(6,AtGpu(views_.Get(),view_stride_,t+2));
                const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> rtvs{targets.hdr,targets.gb5};
                list->OMSetRenderTargets(draw.targets,rtvs.data(),FALSE,&targets.dsv);
                list->RSSetViewports(1,viewport);list->RSSetScissorRects(1,scissor);   // the step review's M1: a classic draw may have set its own
                list->SetPipelineState(pipeline);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
                witness(*args,0,draw_args_read);
                Require(list,*args,D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                list->ExecuteIndirect(draw_signature_.Get(),1,args->resource.Get(),0,nullptr,0);++stats_.particle_draws;}
            if(all.after_draw)rebind(timed([&]{return all.after_draw(draw);}));
            continue;}
        if(e01_root_dirty){list->SetGraphicsRootSignature(root_.Get());e01_root_dirty=false;}
        // I-0140 tranche 10: a draw into a half-size effect target: its view alone (one target, no depth), its format.
        Targets half{};
        if(draw.target){if(!targets_ready || draw.target>2 || draw.targets>1){++stats_.skipped;continue;}
            half.hdr=At(target_rtvs_.Get(),rtv_stride_,draw.target-1u);half.hdr_format=DXGI_FORMAT_R8G8B8A8_UNORM;half.depth_format=DXGI_FORMAT_UNKNOWN;}
        const Targets& drawn=draw.target?half:targets;
        auto* pipeline=Pipeline(device,draw,drawn);
        const uint64_t vertex_bytes=(uint64_t(draw.count)+draw.start)*draw.stride;
        if(!pipeline || !draw.stride || draw.vertex_offset+uint64_t(draw.indexed?0:vertex_bytes)>frame.arena.size()){++stats_.skipped;continue;}
        // I-0120 D2: a draw that samples the depth or the scene copy while the frame has none is skipped, not drawn with a null view.
        if(std::any_of(draw.textures.begin(),draw.textures.end(),[&](const NativeEffectTexture& t){
               return (t.kind==NativeEffectTextureKind::Depth && !targets.depth) || (t.kind==NativeEffectTextureKind::SceneCopy && !targets.scene_copy);})){
            ++stats_.missing_textures;++stats_.skipped;continue;}
        // t0..t4 (I-0127 step B): a table per distinct five-view tuple of the frame, written once and bound again by later draws with
        // the same views (the views' sources live as long as the frame's records; the depth and copy views are made above for the
        // frame); an effect record's view (I-0120 D2, the final review's L2) bound as the particles' path binds it, its buffer put
        // in the shader-read state by every draw that reads it, the draw skipped when it has none yet.
        TextureTuple tuple{};
        for(uint32_t t=0;t<5;++t){const auto& x=draw.textures[t];tuple[t]={uint32_t(x.kind),x.kind==NativeEffectTextureKind::Texture?uint64_t(x.view.ptr):
            x.kind==NativeEffectTextureKind::Object || x.kind==NativeEffectTextureKind::Target?x.object:x.kind==NativeEffectTextureKind::Depth?depth_index:0};}
        if(std::any_of(draw.textures.begin(),draw.textures.end(),[&](const NativeEffectTexture& t){return t.kind==NativeEffectTextureKind::Target && (!targets_ready || t.object<1 || t.object>2);})){
            ++stats_.missing_textures;++stats_.skipped;continue;}
        uint32_t table=UINT32_MAX;
        if(const auto found=tables_.find(tuple);found!=tables_.end())table=found->second;
        if(table!=UINT32_MAX)note(EffectCallViews,true,5);
        bool viewed=true;
        if(table==UINT32_MAX){
            if(next+5>end){++stats_.skipped;continue;}   // the ring's room for a new table (step review L5: a table already written needs none)
            for(uint32_t t=0;t<5 && viewed;++t){
                const auto& texture=draw.textures[t];const auto dst=At(views_.Get(),view_stride_,next+t);
                if(texture.kind==NativeEffectTextureKind::Object){viewed=View(device,texture.object,dst,false);if(viewed)++stats_.textures;continue;}
                D3D12_CPU_DESCRIPTOR_HANDLE source{};
                if(texture.kind==NativeEffectTextureKind::Texture)source=texture.view;
                else if(texture.kind==NativeEffectTextureKind::Depth && targets.depth)source=At(cpu_views_.Get(),view_stride_,depth_index);
                else if(texture.kind==NativeEffectTextureKind::SceneCopy && targets.scene_copy)source=At(cpu_views_.Get(),view_stride_,1);
                else if(texture.kind==NativeEffectTextureKind::Target)source=At(cpu_views_.Get(),view_stride_,3+uint32_t(texture.object)-1);
                if(source.ptr){device->CopyDescriptorsSimple(1,dst,source,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);++stats_.textures;}
                else device->CreateShaderResourceView(nullptr,&null_view,dst);}   // None: nothing bound there
            if(!viewed){++stats_.missing_textures;++stats_.skipped;continue;}
            table=next;next+=5;tables_.emplace(tuple,table);note(EffectCallViews,false,5);}
        for(const auto& texture:draw.textures)if(texture.kind==NativeEffectTextureKind::Object)
            if(auto* b=BufferOf(texture.object))Require(list,*b,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        for(const auto& texture:draw.textures)if(texture.kind==NativeEffectTextureKind::Target)EffectTargetState(list,uint32_t(texture.object)-1,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        if(draw.target)EffectTargetState(list,draw.target-1u,D3D12_RESOURCE_STATE_RENDER_TARGET);
        // I-0127 step A: every piece of state set only when it differs from what the list holds since the stage's last bind (ER's
        // dirty-state filtering made exact: ER sets the pipeline and rebuilds a table again for an identical value, ER-STUDY).
        if(held.table!=table){list->SetGraphicsRootDescriptorTable(5,AtGpu(views_.Get(),view_stride_,table));held.table=table;note(EffectCallTextureTable,false);}
        else note(EffectCallTextureTable,true);
        const uint32_t sampler=(draw.sampler>=0x2C8 && draw.sampler<=0x2F8)?(draw.sampler-0x2C8)/8:1;
        if(held.sampler!=sampler){list->SetGraphicsRootDescriptorTable(6,AtGpu(samplers_.Get(),sampler_stride_,sampler));held.sampler=sampler;note(EffectCallSamplerTable,false);}
        else note(EffectCallSamplerTable,true);
        // The constant blocks, ranges of the uploaded arena (I-0104: one upload a frame; an empty one binds the arena's zero block).
        bool constants=true;
        for(uint32_t c=0;c<=EffectPsB6;++c)   // the classic root views (VS b0, DS b0, PS b0, b1, b6)
            constants&=Constants(frame,arena.gpu,draw.constants[c],[&](D3D12_GPU_VIRTUAL_ADDRESS a){
                if(held.constants[c]!=a){list->SetGraphicsRootConstantBufferView(c,a);held.constants[c]=a;note(EffectCallConstants,false);}else note(EffectCallConstants,true);});
        if(!constants){++stats_.skipped;continue;}
        if(held.targets!=draw.targets || held.rtv0.ptr!=drawn.hdr.ptr || (draw.targets>1 && held.rtv1.ptr!=drawn.gb5.ptr) || held.dsv.ptr!=drawn.dsv.ptr){
            const std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> rtvs{drawn.hdr,drawn.gb5};
            list->OMSetRenderTargets(draw.targets,rtvs.data(),FALSE,drawn.dsv.ptr?&drawn.dsv:nullptr);
            held.targets=draw.targets;held.rtv0=rtvs[0];held.rtv1=rtvs[1];held.dsv=drawn.dsv;note(EffectCallTargets,false);}
        else note(EffectCallTargets,true);
        {   // I-0140 tranche 10: a half-size target's viewport (the context's, else the whole target) and scissor; the record's scissor
            // rectangle, clamped to the target, where the raster entry enables the scissor (not in the half-size layer of 0x31, whose
            // pixels are not the scene's); the pass's own viewport and scissor otherwise, as before.
            D3D12_VIEWPORT vp=*viewport;D3D12_RECT sc=*scissor;
            if(draw.target){const auto w=LONG(effect_target_size_[0]),h=LONG(effect_target_size_[1]);sc={0,0,w,h};
                vp=draw.viewport_set?D3D12_VIEWPORT{draw.viewport[0],draw.viewport[1],draw.viewport[2],draw.viewport[3],draw.viewport[4],draw.viewport[5]}:D3D12_VIEWPORT{0,0,float(w),float(h),0,1};}
            if(draw.scissor_set && RasterScissor(draw.raster) && !layered){
                const D3D12_RECT bound=sc;
                sc.left=std::clamp<LONG>(draw.scissor[0],bound.left,bound.right);sc.top=std::clamp<LONG>(draw.scissor[1],bound.top,bound.bottom);
                sc.right=std::clamp<LONG>(draw.scissor[2],sc.left,bound.right);sc.bottom=std::clamp<LONG>(draw.scissor[3],sc.top,bound.bottom);}
            if(!held.rects || std::memcmp(&held.viewport,&vp,sizeof(vp))){list->RSSetViewports(1,&vp);held.viewport=vp;}
            if(!held.rects || std::memcmp(&held.scissor,&sc,sizeof(sc))){list->RSSetScissorRects(1,&sc);held.scissor=sc;}
            held.rects=true;}
        if(held.pipeline!=pipeline){list->SetPipelineState(pipeline);held.pipeline=pipeline;note(EffectCallPipeline,false);if(measured)++m.pipeline_changes;}
        else note(EffectCallPipeline,true);
        if(held.topology!=draw.topology){list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY(draw.topology));held.topology=draw.topology;note(EffectCallTopology,false);}
        else note(EffectCallTopology,true);
        const D3D12_VERTEX_BUFFER_VIEW vertices{arena.gpu+draw.vertex_offset,static_cast<UINT>(frame.arena.size()-draw.vertex_offset),draw.stride};
        if(held.vertices.BufferLocation!=vertices.BufferLocation || held.vertices.SizeInBytes!=vertices.SizeInBytes || held.vertices.StrideInBytes!=vertices.StrideInBytes){
            list->IASetVertexBuffers(0,1,&vertices);held.vertices=vertices;note(EffectCallVertices,false);}
        else note(EffectCallVertices,true);
        if(draw.indexed){
            if(draw.index_offset>=frame.arena.size()){++stats_.skipped;continue;}
            const D3D12_INDEX_BUFFER_VIEW indices{arena.gpu+draw.index_offset,static_cast<UINT>(frame.arena.size()-draw.index_offset),draw.index_format};
            if(held.indices.BufferLocation!=indices.BufferLocation || held.indices.SizeInBytes!=indices.SizeInBytes || held.indices.Format!=indices.Format){
                list->IASetIndexBuffer(&indices);held.indices=indices;note(EffectCallIndices,false);}
            else note(EffectCallIndices,true);
            list->DrawIndexedInstanced(draw.count,1,draw.start,draw.base,0);}
        else list->DrawInstanced(draw.count,1,draw.start,0);
        note(EffectCallDraw,false);if(measured)++m.draws;
        ++stats_.draws;if(draw.target)++stats_.target_draws;
        if(all.after_draw)rebind(timed([&]{return all.after_draw(draw);}));}
}
    // I-0140 tranche 10: the half-size effect targets left in their resting state for the next frame.
    if(targets_ready)for(uint32_t i=0;i<2;++i)EffectTargetState(list,i,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
}
namespace ds3native {
// E07 (EFFECTS-E07 §5, FFX-DRAW §2): the GPU-particle statements. Buffers are made once per effect record (default heap, UAV
// allowed, the creation bytes copied in), views are written straight into the frame's shader-visible ring from the records'
// descriptions (format 0: structured, the buffer's stride; else typed), states are tracked per buffer. The structured UAVs carry a
// hidden counter (the append and consume programmes; EffectStage::Counter). Built into locals, published last (review: a failed
// preparation is retried, never half-used).
void EffectStage::PrepareGpu(ID3D12Device* device) {
    if(compute_root_)return;
    ComPtr<ID3D12RootSignature> compute_root,particle_root;ComPtr<ID3D12CommandSignature> dispatch_signature,draw_signature;ComPtr<ID3D12DescriptorHeap> clear_views;
    {   // compute: b0, b1, b5 root views; t0..t4; u0..u2
        D3D12_ROOT_PARAMETER p[5]{};
        const auto cbv=[&](int i,uint32_t reg){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i].Descriptor.ShaderRegister=reg;p[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;};
        cbv(0,0);cbv(1,1);cbv(2,5);
        D3D12_DESCRIPTOR_RANGE srv{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,5,0,0,0},uav{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,0};
        p[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[3].DescriptorTable={1,&srv};
        p[4].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[4].DescriptorTable={1,&uav};
        D3D12_STATIC_SAMPLER_DESC s[2]{};   // INFERRED: s0 linear clamp, s1 point clamp for the update programmes' depth reads
        for(int i=0;i<2;++i){s[i].Filter=i?D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;s[i].AddressU=s[i].AddressV=s[i].AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            s[i].ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;s[i].MaxLOD=D3D12_FLOAT32_MAX;s[i].ShaderRegister=i;s[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;}
        D3D12_ROOT_SIGNATURE_DESC d{5,p,2,s,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&d,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"effect compute root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),compute_root.ReleaseAndGetAddressOf()),"effect compute root");}
    {   // particles: GS b9; PS b0, b6, b9, b11; VS t0..t1; PS t0..t3; s7 point clamp, s8 linear wrap (EFFECTS-E07 §4)
        D3D12_ROOT_PARAMETER p[7]{};
        const auto cbv=[&](int i,uint32_t reg,D3D12_SHADER_VISIBILITY v){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i].Descriptor.ShaderRegister=reg;p[i].ShaderVisibility=v;};
        cbv(0,9,D3D12_SHADER_VISIBILITY_GEOMETRY);cbv(1,0,D3D12_SHADER_VISIBILITY_PIXEL);cbv(2,6,D3D12_SHADER_VISIBILITY_PIXEL);cbv(3,9,D3D12_SHADER_VISIBILITY_PIXEL);
        cbv(4,11,D3D12_SHADER_VISIBILITY_PIXEL);
        D3D12_DESCRIPTOR_RANGE vs{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0},ps{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0};
        p[5].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[5].DescriptorTable={1,&vs};p[5].ShaderVisibility=D3D12_SHADER_VISIBILITY_VERTEX;
        p[6].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[6].DescriptorTable={1,&ps};p[6].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC s[2]{};D3D12_SAMPLER_DESC bank{};
        for(int i=0;i<2;++i){FindSampler(i?0x2D0:0x2E0,bank);s[i].Filter=bank.Filter;s[i].AddressU=bank.AddressU;s[i].AddressV=bank.AddressV;s[i].AddressW=bank.AddressW;
            s[i].ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;s[i].MaxLOD=bank.MaxLOD;s[i].ShaderRegister=7+i;s[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
        D3D12_ROOT_SIGNATURE_DESC d{7,p,2,s,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&d,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"effect particle root serialization");
        Check(Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),particle_root.ReleaseAndGetAddressOf()),"effect particle root");}
    D3D12_INDIRECT_ARGUMENT_DESC dispatch{D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH},draw{D3D12_INDIRECT_ARGUMENT_TYPE_DRAW};
    D3D12_COMMAND_SIGNATURE_DESC sd{12,1,&dispatch,0};Check(device->CreateCommandSignature(&sd,nullptr,IID_PPV_ARGS(&dispatch_signature)),"effect dispatch signature");
    sd={16,1,&draw,0};Check(device->CreateCommandSignature(&sd,nullptr,IID_PPV_ARGS(&draw_signature)),"effect draw signature");
    D3D12_DESCRIPTOR_HEAP_DESC cpu{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,64,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
    Check(device->CreateDescriptorHeap(&cpu,IID_PPV_ARGS(&clear_views)),"effect clear views");
    particle_root_=std::move(particle_root);dispatch_signature_=std::move(dispatch_signature);draw_signature_=std::move(draw_signature);
    clear_views_=std::move(clear_views);compute_root_=std::move(compute_root);
}
EffectStage::GpuBuffer* EffectStage::BufferOf(uint64_t key) noexcept {
    const auto object=objects_.find(key);if(object==objects_.end())return nullptr;
    const auto buffer=buffers_.find(object->second.kind==1?key:object->second.parent);
    return buffer==buffers_.end()?nullptr:&buffer->second;}
void EffectStage::Require(ID3D12GraphicsCommandList* list,GpuBuffer& buffer,D3D12_RESOURCE_STATES state) {
    if(buffer.state==state)return;
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=buffer.resource.Get();
    b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=buffer.state;b.Transition.StateAfter=state;
    list->ResourceBarrier(1,&b);buffer.state=state;}
void EffectStage::Retain(const NativeEffectFrame& frame) {
    frame.consumed.store(true,std::memory_order_release);
    pending_released_.insert(pending_released_.end(),frame.released.begin(),frame.released.end());
    for(const auto& o:frame.objects)if(o.kind==1 && o.initial)initials_[o.key]=o.initial;}
void EffectStage::Objects(ID3D12Device* device,ID3D12GraphicsCommandList* list,const NativeEffectFrame& frame,UploadSink& uploads) {
    frame.consumed.store(true,std::memory_order_release);
    std::vector<uint64_t> released;released.swap(pending_released_);released.insert(released.end(),frame.released.begin(),frame.released.end());
    for(const auto& o:frame.objects)if(o.kind==1 && o.initial)initials_[o.key]=o.initial;   // kept until released (a reopen recreates from them)
    for(const auto key:released){   // retired with the caller's frame slot
        initials_.erase(key);
        if(const auto found=buffers_.find(key);found!=buffers_.end()){
            retired_.push_back(std::shared_ptr<const void>(found->second.resource.Get(),[keep=found->second.resource](const void*){}));buffers_.erase(found);}
        if(const auto counter=counter_slots_.find(key);counter!=counter_slots_.end()){free_counters_.push_back(counter->second);counter_slots_.erase(counter);}
        objects_.erase(key);}
    for(const auto& o:frame.objects){
        objects_[o.key]=o;
        if(o.kind!=1 || buffers_.count(o.key))continue;
        // I-0120 D1: made only from its creation bytes; a buffer first described in a frame that was dropped waits for the frame
        // that sends them again (its draws skipped meanwhile, as a failed creation's).
        const auto kept=initials_.find(o.key);const auto& initial=kept!=initials_.end()?kept->second:o.initial;
        if(!initial){++stats_.gpu_skipped;continue;}
        const uint64_t bytes=std::max<uint64_t>(o.description[0],4);
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=(bytes+15)&~uint64_t{15};d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;
        d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        GpuBuffer buffer;buffer.bytes=bytes;
        if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&buffer.resource)))){++stats_.gpu_skipped;continue;}
        if(!initial->empty()){
            const auto memory=uploads.Get((initial->size()+255)&~size_t{255});
            if(memory.cpu && memory.resource){std::memcpy(memory.cpu,initial->data(),initial->size());
                Require(list,buffer,D3D12_RESOURCE_STATE_COPY_DEST);
                list->CopyBufferRegion(buffer.resource.Get(),0,memory.resource,memory.offset,std::min<uint64_t>(initial->size(),d.Width));}}
        buffers_.emplace(o.key,std::move(buffer));}
}
EffectStage::CounterSlot EffectStage::Counter(ID3D12Device* device,ID3D12GraphicsCommandList* list,uint64_t view,UploadSink& uploads) {
    constexpr uint32_t per_pool=64;constexpr uint64_t placement=D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
    auto found=counter_slots_.find(view);bool fresh=false;
    if(found==counter_slots_.end()){
        uint32_t slot;if(!free_counters_.empty()){slot=free_counters_.back();free_counters_.pop_back();}else slot=counter_next_++;
        if(slot/per_pool>=counter_pools_.size()){
            D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=per_pool*placement;
            d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            GpuBuffer pool;pool.bytes=d.Width;
            if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&pool.resource)))){free_counters_.push_back(slot);return {};}
            counter_pools_.push_back(std::move(pool));}
        found=counter_slots_.emplace(view,slot).first;fresh=true;}
    auto& pool=counter_pools_[found->second/per_pool];const uint64_t offset=uint64_t(found->second%per_pool)*placement;
    if(fresh){const auto zero=uploads.Get(256);   // a slot starts at zero (a reused one too)
        if(zero.cpu && zero.resource){std::memset(zero.cpu,0,4);Require(list,pool,D3D12_RESOURCE_STATE_COPY_DEST);list->CopyBufferRegion(pool.resource.Get(),offset,zero.resource,zero.offset,4);}}
    Require(list,pool,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    return {pool.resource.Get(),offset};}
bool EffectStage::View(ID3D12Device* device,uint64_t key,D3D12_CPU_DESCRIPTOR_HANDLE at,bool unordered,ID3D12GraphicsCommandList* counter_list,UploadSink* uploads) {
    const auto view=objects_.find(key);if(view==objects_.end() || view->second.kind==1)return false;
    const auto parent=objects_.find(view->second.parent);auto* buffer=BufferOf(key);if(parent==objects_.end() || !buffer)return false;
    const auto& v=view->second.description;const auto& b=parent->second.description;
    const DXGI_FORMAT format=static_cast<DXGI_FORMAT>(v[0]);const uint32_t stride=format==DXGI_FORMAT_UNKNOWN?std::max<uint32_t>(b[5],4):0;
    if(unordered){D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=format;u.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;u.Buffer.FirstElement=v[2];u.Buffer.NumElements=v[3];
        u.Buffer.StructureByteStride=stride;CounterSlot counter{};
        if(format==DXGI_FORMAT_UNKNOWN && counter_list && uploads)counter=Counter(device,counter_list,key,*uploads);
        u.Buffer.CounterOffsetInBytes=counter.offset;device->CreateUnorderedAccessView(buffer->resource.Get(),counter.resource,&u,at);}
    else{D3D12_SHADER_RESOURCE_VIEW_DESC s{};s.Format=format;s.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;s.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        s.Buffer.FirstElement=v[2];s.Buffer.NumElements=v[3];s.Buffer.StructureByteStride=stride;device->CreateShaderResourceView(buffer->resource.Get(),&s,at);}
    return true;}
ID3D12PipelineState* EffectStage::ComputePipeline(ID3D12Device* device,const NativeEffectDraw& draw) {
    PipelineKey key{};key.cs=draw.cs.key;key.kind=2;
    if(const auto found=pipelines_.find(key);found!=pipelines_.end())return found->second.Get();
    if(failed_.count(key) || !draw.cs.bytes)return nullptr;
    D3D12_COMPUTE_PIPELINE_STATE_DESC d{};d.pRootSignature=compute_root_.Get();d.CS={draw.cs.bytes->data(),draw.cs.bytes->size()};
    ComPtr<ID3D12PipelineState> pipeline;const auto hr=Pipelines().Compute(device,d,&pipeline);   // I-0072: the device's pipeline cache
    if(FAILED(hr)){char why[96]{};sprintf_s(why,"compute pipeline 0x%08X",static_cast<unsigned>(hr));failed_[key]=why;stats_.last_failure=why;++stats_.pipeline_failures;return nullptr;}
    ++stats_.pipelines;auto* raw=pipeline.Get();pipelines_.emplace(key,std::move(pipeline));return raw;}
ID3D12PipelineState* EffectStage::ParticlePipeline(ID3D12Device* device,const NativeEffectDraw& draw,const Targets& targets) {
    const PipelineKey key{draw.vs.key,0,0,draw.gs.key,draw.ps.key,0,0,draw.blend,draw.depth,draw.raster,
        static_cast<uint32_t>(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT),draw.targets,draw.pass==0x21?4u:1u,targets.hdr_format,targets.gb5_format,targets.depth_format};
    if(const auto found=pipelines_.find(key);found!=pipelines_.end())return found->second.Get();
    if(failed_.count(key))return nullptr;
    const auto refuse=[&](const std::string& why)->ID3D12PipelineState*{failed_[key]=why;stats_.last_failure=why;++stats_.pipeline_failures;return nullptr;};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};d.pRootSignature=particle_root_.Get();
    d.VS={draw.vs.bytes->data(),draw.vs.bytes->size()};d.GS={draw.gs.bytes->data(),draw.gs.bytes->size()};d.PS={draw.ps.bytes->data(),draw.ps.bytes->size()};
    if(!NativeEffectBankBlend(draw.blend,d.BlendState))return refuse("particle blend entry unknown");
    d.SampleMask=UINT_MAX;
    if(!FindRaster(draw.raster,d.RasterizerState) || !FindDepth(draw.depth,d.DepthStencilState))return refuse("particle state entry unknown");
    if(draw.pass!=0x21)d.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;   // as the classic draws' (I-0140: 0x21's kept, key kind 4)
    d.DepthStencilState.StencilEnable=FALSE;
    d.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    d.NumRenderTargets=draw.targets;d.RTVFormats[0]=targets.hdr_format;if(draw.targets>1)d.RTVFormats[1]=targets.gb5_format;
    d.DSVFormat=targets.depth_format;d.SampleDesc.Count=1;
    ComPtr<ID3D12PipelineState> pipeline;const auto hr=Pipelines().Graphics(device,d,&pipeline);   // I-0072: the device's pipeline cache
    if(FAILED(hr)){char why[96]{};sprintf_s(why,"particle pipeline 0x%08X",static_cast<unsigned>(hr));return refuse(why);}
    ++stats_.pipelines;auto* raw=pipeline.Get();pipelines_.emplace(key,std::move(pipeline));return raw;}
}
namespace ds3native {
// I-0140 tranche 10: the two half-size effect targets (R8G8B8A8_UNORM, render target allowed), their views; false when the size is
// empty or a creation fails (the frame's statements that use them are then skipped).
bool EffectStage::EffectTargets(ID3D12Device* device,const std::array<uint32_t,2>& size) {
    if(!size[0] || !size[1])return false;
    if(effect_targets_[0] && effect_targets_[1] && size==effect_target_size_)return true;
    for(auto& t:effect_targets_)if(t){retired_.push_back(std::shared_ptr<const void>(t.Get(),[keep=t](const void*){}));t.Reset();}   // held by the caller's slot to completion
    effect_target_size_={};
    if(!target_rtvs_){const D3D12_DESCRIPTOR_HEAP_DESC d{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        if(FAILED(device->CreateDescriptorHeap(&d,IID_PPV_ARGS(&target_rtvs_))))return false;
        rtv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);}
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=size[0];desc.Height=size[1];desc.DepthOrArraySize=1;desc.MipLevels=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    for(uint32_t i=0;i<2;++i){
        if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,&clear,IID_PPV_ARGS(&effect_targets_[i])))){
            for(auto& t:effect_targets_)t.Reset();stats_.last_failure="effect target creation";return false;}
        effect_targets_[i]->SetName(i?L"effect target 2 (light shafts)":L"effect target 1 (light shafts)");
        effect_target_states_[i]=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        device->CreateRenderTargetView(effect_targets_[i].Get(),nullptr,At(target_rtvs_.Get(),rtv_stride_,i));
        D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R8G8B8A8_UNORM;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(effect_targets_[i].Get(),&v,At(cpu_views_.Get(),view_stride_,3+i));}
    effect_target_size_=size;return true;}
void EffectStage::EffectTargetState(ID3D12GraphicsCommandList* list,uint32_t index,D3D12_RESOURCE_STATES state) {
    if(index>=2 || !effect_targets_[index] || effect_target_states_[index]==state)return;
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition={effect_targets_[index].Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,effect_target_states_[index],state};
    list->ResourceBarrier(1,&b);effect_target_states_[index]=state;}
void EffectStage::Close() noexcept {
    root_.Reset();views_.Reset();samplers_.Reset();cpu_views_.Reset();pipelines_.clear();failed_.clear();
    viewed_depth_=viewed_copy_=viewed_layer_depth_=nullptr;viewed_generation_=UINT64_MAX;prepared_=false;stats_={};
    counter_pools_.clear();counter_slots_.clear();free_counters_.clear();counter_next_=0;
    // E07 (review): the device's objects go too; the records' creation bytes stay (initials_), so a reopen recreates the buffers.
    compute_root_.Reset();particle_root_.Reset();dispatch_signature_.Reset();draw_signature_.Reset();clear_views_.Reset();clear_next_=0;
    buffers_.clear();objects_.clear();retired_.clear();for(auto& r:readbacks_)r.Reset();readback_frames_=0;
    for(auto& t:effect_targets_)t.Reset();target_rtvs_.Reset();effect_target_size_={};
}
void EffectStage::QuarantineToProcessExit() noexcept {
    (void)root_.Detach();(void)views_.Detach();(void)samplers_.Detach();(void)cpu_views_.Detach();
    for(auto& [key,pipeline]:pipelines_)(void)pipeline.Detach();
    for(auto& pool:counter_pools_)(void)pool.resource.Detach();
    (void)compute_root_.Detach();(void)particle_root_.Detach();(void)dispatch_signature_.Detach();(void)draw_signature_.Detach();(void)clear_views_.Detach();
    for(auto& [key,buffer]:buffers_)(void)buffer.resource.Detach();for(auto& r:readbacks_)(void)r.Detach();
    for(auto& t:effect_targets_)(void)t.Detach();(void)target_rtvs_.Detach();
    prepared_=false;
}
}
