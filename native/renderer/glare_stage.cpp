#include "glare_stage.h"
#include "backend.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace ds3native {
using Microsoft::WRL::ComPtr;
namespace {
constexpr uint32_t draws_max=96;   // I-0138 tranche 8f: GlareShape 3 draws 43 at 32000020 (more levels, more arms' passes)
constexpr uint32_t scene_rtv=draws_max-1;   // LS-1: the RTV slot of the scene (the pre draws' target)
// I-0138 tranche 8e (tranche 8d's final review, M1): the pre draws' post tables from the top (95, 94, ...), the main draws' from 0, so
// that a pre draw's table does not move with the main draws' count (Layout keeps both within draws_max).
constexpr uint32_t PreTable(size_t j){return uint32_t(draws_max-1-j);}
DXGI_FORMAT Format(const yebis::GlareSurface& s){return s.half_float?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R11G11B10_FLOAT;}
void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);}
// The sampler set of a draw: s0..s7 as the original binds them for the glare draws (i66-reference-1648): point or bilinear (filter 0
// or 0x14, no mip filtering), border (the source, bright pass, blurs and 2x draws) or clamp (the composite). LS-3: the film grain's
// slot (the shaft mask's s1, Reference1667): trilinear (0x15), wrapped, MaxLOD the float maximum (its CPU mips).
PostSamplers Samplers(const yebis::GlareDraw& g){PostSamplers s{};
    for(size_t i=0;i<s.size();++i){
        if(g.film_grain[i]){s[i].filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;s[i].address=D3D12_TEXTURE_ADDRESS_MODE_WRAP;s[i].max_lod=D3D12_FLOAT32_MAX;continue;}
        s[i].filter=g.point[i]?D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        s[i].address=g.border?D3D12_TEXTURE_ADDRESS_MODE_BORDER:D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        if(g.address[i])s[i].address=D3D12_TEXTURE_ADDRESS_MODE(g.address[i]);}   // DF-1: the slot's own mode (the DoF draws, Reference1676)
    return s;}
// The pipeline layout of a draw: one pair (the generic vertices), two (the vignette bright pass) or the draw's own (the shafts).
uint32_t Texcoords(const yebis::GlareDraw& g){return g.texcoord1?2u:!g.vertices.empty()?uint32_t(g.texcoords):1u;}
// The fields the cached state depends on (the blocks and vertices are written each frame; Texcoords on both sides, so the shape
// keeps whether the draw brings vertices: review of Native1673, a relayout and a drain every frame at 23 frames/s).
bool SameShape(const yebis::GlareDraw& a,const yebis::GlareDraw& b){
    return a.technique==b.technique && a.inputs==b.inputs && a.target==b.target && a.point==b.point && a.border==b.border && a.texcoord1==b.texcoord1 &&
        Texcoords(a)==Texcoords(b) && a.film_grain==b.film_grain && a.write_mask==b.write_mask && a.address==b.address && a.strip==b.strip && a.copy_to==b.copy_to;}
yebis::GlareDraw Shape(const yebis::GlareDraw& g){yebis::GlareDraw s;s.technique=g.technique;s.inputs=g.inputs;s.target=g.target;s.point=g.point;s.border=g.border;
    s.texcoord1=g.texcoord1;s.texcoords=g.texcoords;s.film_grain=g.film_grain;s.write_mask=g.write_mask;s.address=g.address;s.strip=g.strip;s.copy_to=g.copy_to;
    s.vertices.assign(g.vertices.empty()?0u:1u,0.0f);   // a marker: whether the draw brings its own vertices (Texcoords reads it)
    return s;}
bool ReadsFilmGrain(const yebis::GlarePlan& plan){for(const auto& g:plan.draws)for(const auto in:g.inputs)if(in==-3)return true;return false;}
bool ReadsTrim(const yebis::GlarePlan& plan){for(const auto& g:plan.draws)for(const auto in:g.inputs)if(in==-7)return true;return false;}
bool ReadsTable(const yebis::GlarePlan& plan){for(const auto* list:{&plan.draws,&plan.pre})for(const auto& g:*list)for(const auto in:g.inputs)if(in==-5)return true;return false;}
}
bool GlareStage::Open(ID3D12Device* device,std::string& reason){
    Close();
    try{
        if(!device)throw std::invalid_argument("glare stage device");
        device_=device;if(!post_.Open(device,draws_max,reason))return false;
        D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,draws_max,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        if(FAILED(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv_))))throw std::runtime_error("glare RTV heap");
        rtv_stride_=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        return true;
    }catch(const std::exception& e){reason=e.what();Close();return false;}
}
bool GlareStage::NeedsLayout(const yebis::GlarePlan& plan) const {
    // I-0138 tranche 8e (tranche 8d's final review, M1): a plan of pre draws alone (the glare family skipped at shape 0 with the
    // motion blur's Alpha_R) keeps the main layout of the last full plan: only its pre draws are compared, so that the next full
    // plan finds its surfaces (no relayout at each shape 0/1 toggle). A pre draw reading the DoF table (input -5, the DoF stage's
    // own plans, never a glare plan's Alpha_R) keeps the former rule: the table's size is the plan's, so such a plan is laid out.
    if(plan.surfaces.empty() && plan.draws.empty() && !plan.pre.empty() && !layout_.empty()){
        if(plan.pre.size()!=pre_shape_.size() || ReadsTable(plan))return true;
        for(size_t i=0;i<pre_shape_.size();++i)if(!SameShape(plan.pre[i],pre_shape_[i]))return true;
        return false;}
    if(plan.surfaces.size()!=layout_.size() || plan.draws.size()!=shape_.size() || plan.pre.size()!=pre_shape_.size())return true;
    if(ReadsTable(plan) && (!table_ || table_width_!=plan.table_width || table_height_!=plan.table_height))return true;
    for(size_t i=0;i<layout_.size();++i)if(plan.surfaces[i].width!=layout_[i].width || plan.surfaces[i].height!=layout_[i].height || plan.surfaces[i].half_float!=layout_[i].half_float)return true;
    for(size_t i=0;i<shape_.size();++i)if(!SameShape(plan.draws[i],shape_[i]))return true;
    for(size_t i=0;i<pre_shape_.size();++i)if(!SameShape(plan.pre[i],pre_shape_[i]))return true;
    return false;
}
bool GlareStage::Layout(ID3D12GraphicsCommandList* list,const yebis::GlarePlan& plan,const GlarePrograms& programs,
    const std::vector<std::vector<uint8_t>>* film_grain,std::string& reason){
    if(plan.surfaces.size()>=scene_rtv || plan.draws.size()+plan.pre.size()>draws_max){reason="glare plan size";return false;}
    const auto reset=[&]{surfaces_.clear();layout_.clear();techniques_.clear();inputs_.clear();pipelines_.clear();samplers_.clear();shape_.clear();
        pre_pipelines_.clear();pre_samplers_.clear();pre_shape_.clear();};
    // Optimisation review axis 2: the former surfaces are retired by ticket (the frames in flight that read them keep them) and
    // the post tables go into a fresh heap (the former one retired the same way), so a relayout drains nothing.
    // I-0138 tranche 8f (its step review's L1): the afterimage's history outlives a relayout when the new plan's history has the same
    // size and format (the library's pair is not recreated when the techniques or the levels change; INFERRED), so it does not pop.
    Microsoft::WRL::ComPtr<ID3D12Resource> kept_history;
    if(history_>=0 && size_t(history_)<surfaces_.size() && plan.history>=0 && size_t(plan.history)<plan.surfaces.size()){
        const auto& was=layout_[size_t(history_)];const auto& now=plan.surfaces[size_t(plan.history)];
        if(was.width==now.width && was.height==now.height && was.half_float==now.half_float)kept_history=surfaces_[size_t(history_)];}
    for(auto& surface:surfaces_)if(surface)RetireAfterCurrentRecording(HoldObject(surface.Get()));
    reset();history_=-1;scene_view_=nullptr;depth_view_=nullptr;velocity_view_=nullptr;pre_scene_=nullptr;pre_depth_=nullptr;++generation_;
    post_.RenewTables();
    // LS-3: the film grain, once for the session: nine levels of R8G8B8A8 256x256 through one upload buffer, copied in this list.
    if(ReadsFilmGrain(plan) && !film_grain_){
        if(!film_grain || film_grain->size()!=9 || (*film_grain)[0].size()!=256u*256u*4u){reason="film grain levels";return false;}
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=desc.Height=256;desc.DepthOrArraySize=1;desc.MipLevels=9;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        if(FAILED(CountedResource(device_.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&film_grain_)))){
            reason="film grain texture";return false;}
        std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT,9> footprints{};std::array<UINT,9> rows{};std::array<UINT64,9> row_bytes{};UINT64 total{};
        device_->GetCopyableFootprints(&desc,0,9,0,footprints.data(),rows.data(),row_bytes.data(),&total);
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        buffer.Width=total;buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(FAILED(CountedResource(device_.Get())->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&film_grain_upload_)))){
            film_grain_.Reset();reason="film grain upload";return false;}
        uint8_t* memory{};const D3D12_RANGE none{};if(FAILED(film_grain_upload_->Map(0,&none,reinterpret_cast<void**>(&memory)))){film_grain_.Reset();film_grain_upload_.Reset();reason="film grain map";return false;}
        for(UINT m=0;m<9;++m){const UINT w=std::max(256u>>m,1u);for(UINT y=0;y<rows[m];++y)
            std::memcpy(memory+footprints[m].Offset+UINT64(y)*footprints[m].Footprint.RowPitch,(*film_grain)[m].data()+size_t(y)*w*4,size_t(w)*4);}
        film_grain_upload_->Unmap(0,nullptr);
        for(UINT m=0;m<9;++m){D3D12_TEXTURE_COPY_LOCATION to{film_grain_.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{film_grain_upload_.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
            to.SubresourceIndex=m;from.PlacedFootprint=footprints[m];list->CopyTextureRegion(&to,0,0,0,&from,nullptr);}
        Transition(list,film_grain_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
    // I-0138 tranche 8f: the ghost's trim (input -7, yebis::TrimCircle: A8_UNORM 64x64, one level), once for the session.
    if(ReadsTrim(plan) && !trim_){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=desc.Height=64;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_A8_UNORM;
        if(FAILED(CountedResource(device_.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&trim_)))){
            reason="trim texture";return false;}
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows{};UINT64 row_bytes{},total{};device_->GetCopyableFootprints(&desc,0,1,0,&footprint,&rows,&row_bytes,&total);
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        buffer.Width=total;buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if(FAILED(CountedResource(device_.Get())->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&trim_upload_)))){
            trim_.Reset();reason="trim upload";return false;}
        uint8_t* memory{};const D3D12_RANGE none{};if(FAILED(trim_upload_->Map(0,&none,reinterpret_cast<void**>(&memory)))){trim_.Reset();trim_upload_.Reset();reason="trim map";return false;}
        const auto texels=yebis::TrimCircle();for(UINT y=0;y<64;++y)std::memcpy(memory+footprint.Offset+UINT64(y)*footprint.Footprint.RowPitch,texels.data()+size_t(y)*64,64);
        trim_upload_->Unmap(0,nullptr);
        D3D12_TEXTURE_COPY_LOCATION to{trim_.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{trim_upload_.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        to.SubresourceIndex=0;from.PlacedFootprint=footprint;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        Transition(list,trim_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
    // DF-1: the DoF table (R32_FLOAT at the plan's extent, the depth factor's t2), kept while its extent holds; its values are copied
    // in each frame by RecordPre through a ring of upload buffers (more than the frames in flight).
    if(ReadsTable(plan) && (!table_ || table_width_!=plan.table_width || table_height_!=plan.table_height)){
        // Pre-merge review D4: the former table and its uploads are retired by ticket (frames in flight copy and read them).
        if(table_)RetireAfterCurrentRecording(HoldObject(table_.Get()));for(auto& u:table_uploads_)if(u)RetireAfterCurrentRecording(HoldObject(u.Get()));
        table_.Reset();for(auto& u:table_uploads_)u.Reset();
        if(!plan.table_width || !plan.table_height || plan.table_width*4u%D3D12_TEXTURE_DATA_PITCH_ALIGNMENT){reason="DoF table extent";return false;}
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=plan.table_width;desc.Height=plan.table_height;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R32_FLOAT;
        if(FAILED(CountedResource(device_.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,nullptr,IID_PPV_ARGS(&table_)))){
            reason="DoF table texture";return false;}
        D3D12_HEAP_PROPERTIES upload{};upload.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        buffer.Width=UINT64(plan.table_width)*4*plan.table_height;buffer.Height=buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        for(auto& u:table_uploads_)if(FAILED(CountedResource(device_.Get())->CreateCommittedResource(&upload,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&u)))){
            table_.Reset();reason="DoF table upload";return false;}
        table_width_=plan.table_width;table_height_=plan.table_height;}
    for(const auto& s:plan.surfaces){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=s.width;desc.Height=s.height;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=Format(s);desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        ComPtr<ID3D12Resource> r;
        // I-0138 tranche 8f: the history, which ClearHistory clears, is created with its clear value (black), or the debug layer warns at
        // each clear (i138-t8f-c32000020-nat: the renderer's diagnostic stopped the game on that warning at the first shape-3 frame).
        D3D12_CLEAR_VALUE black{};black.Format=desc.Format;const bool history=plan.history>=0 && size_t(plan.history)==surfaces_.size();
        if(history && kept_history)r=kept_history;
        else if(FAILED(CountedResource(device_.Get())->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,history?&black:nullptr,IID_PPV_ARGS(&r)))){
            reason="glare surface";reset();return false;}
        auto at=rtv_->GetCPUDescriptorHandleForHeapStart();at.ptr+=SIZE_T(rtv_stride_)*surfaces_.size();device_->CreateRenderTargetView(r.Get(),nullptr,at);
        surfaces_.push_back(r);layout_.push_back(s);}
    // I-0138 tranche 8f: a new history starts black (no former frame; INFERRED: the library's pair is created cleared, as
    // YebisClearAfterImageHistory_1416c92b0 clears it); a kept one keeps its frame.
    if(plan.history>=0 && size_t(plan.history)>=surfaces_.size()){reason="glare history surface";reset();return false;}
    try{
        const auto prepare=[&](const yebis::GlareDraw& g,uint32_t table,std::vector<uint32_t>& pipelines,std::vector<uint32_t>& samplers,std::vector<yebis::GlareDraw>& shapes){
            const auto found=programs.find(g.technique);
            if(found==programs.end() || found->second.vertex.empty() || found->second.pixel.empty())throw std::runtime_error("glare programme "+g.technique);
            const DXGI_FORMAT target=g.target<0?DXGI_FORMAT_R16G16B16A16_FLOAT:Format(plan.surfaces[size_t(g.target)]);
            pipelines.push_back(post_.Pipeline(found->second.vertex,found->second.pixel,target,Texcoords(g),g.write_mask));
            samplers.push_back(post_.SamplerTable(Samplers(g)));shapes.push_back(Shape(g));(void)table;};
        for(size_t k=0;k<plan.draws.size();++k){prepare(plan.draws[k],uint32_t(k),pipelines_,samplers_,shape_);techniques_.push_back(plan.draws[k].technique);inputs_.push_back(plan.draws[k].inputs);}
        for(size_t j=0;j<plan.pre.size();++j)prepare(plan.pre[j],PreTable(j),pre_pipelines_,pre_samplers_,pre_shape_);
        WriteDrawInputs(plan,nullptr,nullptr,nullptr,nullptr);   // the scene's, the depth's and the velocity's slots are written at Record and RecordPre
    }catch(const std::exception& e){reason=e.what();reset();return false;}
    // Recorded only once the layout holds (its step review's M1: a failure above releases the new surfaces while the list is open).
    history_=plan.history;if(!kept_history)ClearHistory(list,plan);
    return true;
}
void GlareStage::ClearHistory(ID3D12GraphicsCommandList* list,const yebis::GlarePlan& plan){
    if(plan.history<0 || size_t(plan.history)>=surfaces_.size())return;
    auto* h=surfaces_[size_t(plan.history)].Get();auto at=rtv_->GetCPUDescriptorHandleForHeapStart();at.ptr+=SIZE_T(rtv_stride_)*size_t(plan.history);
    constexpr float black[4]{};Transition(list,h,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
    list->ClearRenderTargetView(at,black,0,nullptr);Transition(list,h,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
void GlareStage::WriteDrawInputs(const yebis::GlarePlan& plan,ID3D12Resource* scene,ID3D12Resource* depth,ID3D12Resource* velocity,ID3D12Resource* pre_depth){
    scene_view_=scene;depth_view_=depth;velocity_view_=velocity;pre_depth_=pre_depth;
    // I-0138 tranche 8e: the main draws' inputs from the layout's own record (inputs_, layout_), not the plan's, so that a plan of pre
    // draws alone (NeedsLayout) still writes the kept main draws' tables into a fresh heap.
    const auto write=[&](const std::array<int8_t,8>& inputs,uint32_t table){
        for(uint32_t s=0;s<PostDraws::Inputs;++s){const int8_t in=inputs[s];
            if(in>=0)post_.SetInput(table,s,surfaces_[size_t(in)].Get(),Format(layout_[size_t(in)]));
            else if(in==-3)post_.SetInput(table,s,film_grain_.Get(),DXGI_FORMAT_R8G8B8A8_UNORM);
            else if(in==-5)post_.SetInput(table,s,table_.Get(),DXGI_FORMAT_R32_FLOAT);
            else if(in==-7)post_.SetInput(table,s,trim_.Get(),DXGI_FORMAT_A8_UNORM);
            else post_.SetInput(table,s,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);}};
    for(size_t k=0;k<inputs_.size();++k)write(inputs_[k],uint32_t(k));
    for(size_t j=0;j<plan.pre.size();++j)write(plan.pre[j].inputs,PreTable(j));
    // The frame inputs the stage knows (null until Record and RecordPre name them): -1 the scene, -4 the depth's plane 0 (the
    // original's R32_FLOAT_X8X24 view), -6 the engine's object velocity (MB-2); the pre draws' -4 the depth RecordPre named.
    for(size_t k=0;k<inputs_.size();++k)for(uint32_t s=0;s<PostDraws::Inputs;++s){const int8_t in=inputs_[k][s];
        if(in==-1 && scene_view_)post_.SetInput(uint32_t(k),s,scene_view_,DXGI_FORMAT_R16G16B16A16_FLOAT);
        else if(in==-4 && depth_view_)post_.SetInput(uint32_t(k),s,depth_view_,DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
        else if(in==-6 && velocity_view_)post_.SetInput(uint32_t(k),s,velocity_view_,DXGI_FORMAT_R8G8B8A8_UNORM);}
    for(size_t j=0;j<plan.pre.size();++j)for(uint32_t s=0;s<PostDraws::Inputs;++s)if(plan.pre[j].inputs[s]==-4 && pre_depth_)
        post_.SetInput(PreTable(j),s,pre_depth_,DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
}
bool GlareStage::Draw(ID3D12GraphicsCommandList* list,const yebis::GlareDraw& g,size_t index,uint32_t table,D3D12_CPU_DESCRIPTOR_HANDLE rtv,uint32_t width,uint32_t height,
    const GlareConstantSink& constants,std::string& reason){
    const bool pre=g.target<0;const uint32_t pipeline=pre?pre_pipelines_[index]:pipelines_[index],samplers=pre?pre_samplers_[index]:samplers_[index];
    // I-0138 tranche 8b: our block (yebis::glare_block_registers, 1,456 bytes) as it is (Unplaced refused the plan beforehand).
    const auto memory=constants(yebis::glare_block_bytes);if(!memory.cpu || !memory.gpu || memory.gpu%256){reason="glare constants";return false;}
    std::memcpy(memory.cpu,g.block.data(),yebis::glare_block_bytes);
    // GL-4: a draw with TEXCOORD1 (the vignette bright pass) takes its three vertices (stride 24) from the frame's upload memory;
    // LS-3: a shaft draw its own three vertices (stride 8 + 8 texcoords).
    D3D12_VERTEX_BUFFER_VIEW vertices{};
    if(g.texcoord1){const auto v=constants(vertex_request_bytes);if(!v.cpu || !v.gpu){reason="glare vertices";return false;}
        constexpr float base[3][4]{{-1,1,0,1},{-1,-3,0,-1},{3,1,2,1}};float words[18]{};
        for(size_t i=0;i<3;++i){std::memcpy(words+6*i,base[i],16);words[6*i+4]=g.uv1[i][0];words[6*i+5]=g.uv1[i][1];}
        std::memcpy(v.cpu,words,sizeof(words));vertices={v.gpu,sizeof(words),24};}
    else if(!g.vertices.empty()){const UINT stride=8+8*Texcoords(g);if(g.vertices.size()*4!=3*size_t(stride)){reason="shaft vertices";return false;}
        const auto v=constants(vertex_request_bytes);if(!v.cpu || !v.gpu){reason="glare vertices";return false;}
        std::memcpy(v.cpu,g.vertices.data(),g.vertices.size()*4);vertices={v.gpu,3*stride,stride};}
    post_.Draw(list,pipeline,table,samplers,memory.gpu,rtv,width,height,Texcoords(g)>1?&vertices:nullptr,g.strip);
    return true;
}
// I-0138 tranche 8b (the step review's L1): a draw whose builder put a word our programmes do not read refuses the whole plan
// before any transition or draw is recorded (the module's own contract: the tests' plans never hold one).
static bool Unplaced(const yebis::GlarePlan& plan,std::string& reason){
    for(const auto* list:{&plan.pre,&plan.draws})for(const auto& g:*list)if(g.unplaced){reason="glare word without a slot ("+g.technique+")";return true;}
    return false;}
bool GlareStage::RecordPre(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,ID3D12Resource* depth,uint64_t scene_generation,const yebis::GlarePlan& plan,
    const GlarePrograms& programs,const GlareConstantSink& constants,std::string& reason,const std::vector<std::vector<uint8_t>>* film_grain,
    const std::vector<float>* table){
    if(!IsOpen() || !list || !scene || !depth || !constants || !plan.refused.empty()){reason="glare pre inputs";return false;}
    if(Unplaced(plan,reason))return false;
    if(plan.pre.empty())return true;
    try{
    if(NeedsLayout(plan) && !Layout(list,plan,programs,film_grain,reason))return false;
    // DF-1: this frame's DoF table into the texture (the upload slot of a frame no longer in flight).
    if(ReadsTable(plan)){
        if(!table || table->size()!=size_t(table_width_)*table_height_){reason="DoF table values";return false;}
        auto& upload=table_uploads_[table_next_++%table_uploads_.size()];
        uint8_t* memory{};const D3D12_RANGE none{};if(FAILED(upload->Map(0,&none,reinterpret_cast<void**>(&memory)))){reason="DoF table map";return false;}
        std::memcpy(memory,table->data(),table->size()*4);upload->Unmap(0,nullptr);
        Transition(list,table_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
        D3D12_TEXTURE_COPY_LOCATION to{table_.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R32_FLOAT,table_width_,table_height_,1,table_width_*4};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        Transition(list,table_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
    auto rtv=rtv_->GetCPUDescriptorHandleForHeapStart();rtv.ptr+=SIZE_T(rtv_stride_)*scene_rtv;
    if(scene!=pre_scene_ || depth!=pre_depth_ || scene_generation!=pre_generation_){
        device_->CreateRenderTargetView(scene,nullptr,rtv);
        // Axis 2 (review): every input into a fresh heap: the pre draws' depth (the scene depth's plane 0 as Alpha_R reads
        // it) and the main draws' inputs Record named, forgotten when the targets changed (a new generation).
        if(scene_generation!=pre_generation_){scene_view_=nullptr;depth_view_=nullptr;velocity_view_=nullptr;}
        pre_scene_=scene;pre_generation_=scene_generation;
        post_.RenewTables();WriteDrawInputs(plan,scene_view_,depth_view_,velocity_view_,depth);}
    const auto desc=scene->GetDesc();
    post_.Begin(list);
    for(size_t j=0;j<plan.pre.size();++j)
        if(!Draw(list,plan.pre[j],j,PreTable(j),rtv,uint32_t(desc.Width),desc.Height,constants,reason))return false;
    return true;
    }catch(const std::exception& e){reason=e.what();return false;}
}
ID3D12Resource* GlareStage::Record(ID3D12GraphicsCommandList* list,ID3D12Resource* scene,uint64_t scene_generation,const yebis::GlarePlan& plan,
    const GlarePrograms& programs,const GlareConstantSink& constants,std::string& reason,const std::vector<std::vector<uint8_t>>* film_grain,
    ID3D12Resource* depth,ID3D12Resource* object_velocity,bool clear_history){
    if(!IsOpen() || !list || !scene || !constants || !plan.refused.empty() || plan.output<0){reason="glare stage inputs";return nullptr;}
    if(Unplaced(plan,reason))return nullptr;
    try{
    bool reads_depth=false,reads_velocity=false;
    for(const auto& g:plan.draws)for(const auto in:g.inputs){reads_depth=reads_depth || in==-4;reads_velocity=reads_velocity || in==-6;}
    if((reads_depth && !depth) || (reads_velocity && !object_velocity)){reason="glare stage depth or velocity input";return nullptr;}
    if(NeedsLayout(plan) && !Layout(list,plan,programs,film_grain,reason))return nullptr;
    if(scene!=scene_view_ || depth!=depth_view_ || object_velocity!=velocity_view_ || scene_generation!=scene_view_generation_){
        // Axis 2 (review): every input into a fresh heap (no table rewritten under a frame in flight); the pre draws keep the
        // depth RecordPre named unless the targets changed (a new generation: a released depth is never viewed again).
        if(scene_generation!=scene_view_generation_){pre_scene_=nullptr;pre_depth_=nullptr;}
        scene_view_generation_=scene_generation;post_.RenewTables();WriteDrawInputs(plan,scene,depth,object_velocity,pre_depth_);}
    // I-0138 tranche 8f: at a camera cut the history restarts black (the library: E69A0 1400e6a78..1400e6a93 calls
    // YebisClearAfterImageHistory_1416c92b0 on its display record's cut, READ; natively the velocity's reset, the camera cut the TAA
    // and DLSS also take (they reset on a gap in the frame serials too): the two cuts' equivalence is INFERRED).
    if(clear_history)ClearHistory(list,plan);
    post_.Begin(list);
    for(size_t k=0;k<plan.draws.size();++k){const auto& g=plan.draws[k];auto* target=surfaces_[size_t(g.target)].Get();
        Transition(list,target,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto rtv=rtv_->GetCPUDescriptorHandleForHeapStart();rtv.ptr+=SIZE_T(rtv_stride_)*size_t(g.target);
        if(!Draw(list,g,k,uint32_t(k),rtv,layout_[size_t(g.target)].width,layout_[size_t(g.target)].height,constants,reason))return nullptr;
        if(g.copy_to>=0){   // I-0138 tranche 8f: the afterimage's history for the next frame (the library flips a pair: the same texels)
            const size_t h=size_t(g.copy_to);
            if(h>=surfaces_.size() || layout_[h].width!=layout_[size_t(g.target)].width || layout_[h].height!=layout_[size_t(g.target)].height ||
               layout_[h].half_float!=layout_[size_t(g.target)].half_float){reason="glare history copy";return nullptr;}
            Transition(list,target,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
            Transition(list,surfaces_[h].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
            list->CopyResource(surfaces_[h].Get(),target);
            Transition(list,surfaces_[h].Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Transition(list,target,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        else Transition(list,target,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
    ++frames_;return surfaces_[size_t(plan.output)].Get();
    }catch(const std::exception& e){reason=e.what();return nullptr;}   // the caller keeps the failure for the session
}
void GlareStage::Close() noexcept {
    post_.Close();surfaces_.clear();layout_.clear();techniques_.clear();inputs_.clear();pipelines_.clear();samplers_.clear();shape_.clear();rtv_.Reset();device_.Reset();
    pre_pipelines_.clear();pre_samplers_.clear();pre_shape_.clear();film_grain_.Reset();film_grain_upload_.Reset();trim_.Reset();trim_upload_.Reset();
    table_.Reset();for(auto& u:table_uploads_)u.Reset();table_width_=table_height_=0;
    scene_view_=nullptr;scene_view_generation_=0;pre_scene_=nullptr;pre_depth_=nullptr;pre_generation_=0;depth_view_=nullptr;velocity_view_=nullptr;
}
void GlareStage::QuarantineToProcessExit() noexcept {
    post_.QuarantineToProcessExit();for(auto& s:surfaces_)(void)s.Detach();(void)rtv_.Detach();(void)device_.Detach();(void)film_grain_.Detach();(void)film_grain_upload_.Detach();
    (void)trim_.Detach();(void)trim_upload_.Detach();(void)table_.Detach();for(auto& u:table_uploads_)(void)u.Detach();
}
}
