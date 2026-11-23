// I-0117 (R2 tranche T0): the slices a BLAS takes (pure), and on the hardware device with the debug layer when present: one model's
// BLAS built from a storage-like buffer (three slices, one ineligible kept as an empty geometry), compacted once its build
// completed, ready once its copy completed; its counters; T1 a top level of its instances and rays through it; no debug-layer
// warning or error.
#include "../renderer/rt_scene.h"
#include "../renderer/raytracing_base.h"
#include "../renderer/gpu_zones.h"   // I-0128 R8a
#include "../renderer/rt_light_grid.h"   // I-0128 R8c
#include "rt_probe.dxil.h"   // I-0117 T1 (the build folder)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ds3native;
namespace {
void Require(bool condition,const char* what){if(!condition)throw std::runtime_error(what);}

void Eligibility() {
    MeshSlice s{};s.vertex_count=4;s.stride=16;s.index_count=6;s.index_width=2;
    Require(RtSliceEligible(s),"a quad of R16 indices");
    auto t=s;t.index_count=5;Require(!RtSliceEligible(t),"not a whole number of triangles");
    t=s;t.stride=10;Require(!RtSliceEligible(t),"a stride too small for float3");
    t=s;t.stride=14;Require(!RtSliceEligible(t),"a stride not a multiple of 4");
    t=s;t.vertex_offset=2;Require(!RtSliceEligible(t),"a vertex start not 4-byte aligned");
    t=s;t.index_width=4;t.index_offset=6;Require(!RtSliceEligible(t),"an R32 start not 4-byte aligned");
    t=s;t.index_width=3;Require(!RtSliceEligible(t),"an index width other than 2 or 4");
    t=s;t.vertex_count=0;Require(!RtSliceEligible(t),"no vertex");
    Require(RtAlign(1)==256 && RtAlign(256)==256 && RtAlign(257)==512,"256-byte alignment");
    const auto g=RtSliceGeometry(0x10000,s);
    Require(g.Triangles.VertexBuffer.StartAddress==0x10000 && g.Triangles.VertexBuffer.StrideInBytes==16 && g.Triangles.IndexFormat==DXGI_FORMAT_R16_UINT &&
        g.Triangles.IndexBuffer==0x10000 && g.Triangles.IndexCount==6 && g.Flags==D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,"the slice's geometry");
}

uint64_t DebugMessages(DeviceQueues& gpu) {
    if(!gpu.diagnostics)return 0;uint64_t count{};
    for(UINT64 index=0;index<gpu.diagnostics->GetNumStoredMessages();++index){SIZE_T size{};gpu.diagnostics->GetMessage(index,nullptr,&size);
        std::vector<uint8_t> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(gpu.diagnostics->GetMessage(index,message,&size)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++count;std::printf("debug layer: %s\n",message->pDescription);}}
    return count;
}

// I-0117 T1: the instance records, then a top level of two instances of the model's BLAS (its quad at z = 0 and its triangle at
// z = 1), the second moved 10 along x and masked as a sun caster, built twice on one list (the second build grows it), and rays
// through it in a DXIL compute programme (rt_probe.hlsl): the instance, the geometry (the slice) and the distance each one hits.
D3D12_RESOURCE_DESC BufferOf(uint64_t bytes,D3D12_RESOURCE_FLAGS flags) {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;
    d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;return d;}
Microsoft::WRL::ComPtr<ID3D12Resource> Committed(DeviceQueues& gpu,D3D12_HEAP_TYPE type,uint64_t bytes,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE) {
    D3D12_HEAP_PROPERTIES heap{};heap.Type=type;const auto desc=BufferOf(bytes,flags);Microsoft::WRL::ComPtr<ID3D12Resource> r;
    if(FAILED(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r))))throw std::runtime_error("test buffer");
    return r;}
// T1 final review M1: the moved instance is a model built just before, its build list submitted with the top level's in one
// ExecuteCommandLists call (the case the barrier before the top level's build covers; a smoke test: the debug layer without GPU-based
// validation does not report hazards across lists).
void Tlas(DeviceQueues& gpu,const std::shared_ptr<RtModelBlas>& blas,RtBlasRegistry& registry,ID3D12Resource* buffer,std::span<const MeshSlice> slices) {
    const auto fresh=registry.Build(gpu,buffer,slices);Require(fresh && fresh->Structure(),"a model built in the same submission");
    std::vector<RtInstance> instances(2);
    instances[0].transform={1,0,0,0, 0,1,0,0, 0,0,1,0};instances[0].blas=blas->Structure();instances[0].id=7;instances[0].mask=RtMaskView;
    instances[1].transform={1,0,0,10, 0,1,0,0, 0,0,1,0};instances[1].blas=fresh->Structure();instances[1].id=0x1000009;instances[1].mask=RtMaskSun;
    D3D12_RAYTRACING_INSTANCE_DESC records[2]{};RtFillInstanceDescs(instances,records);
    Require(records[1].Transform[0][3]==10 && records[1].InstanceID==9 && records[1].InstanceMask==RtMaskSun && records[0].AccelerationStructure==blas->Structure()->GetGPUVirtualAddress(),
        "the instance records (24-bit ids, the placement's translation in the fourth column)");
    struct Ray {float origin[3],tmax,direction[3];uint32_t mask;};
    const Ray rays[5]{{{0.5f,0.5f,-1},10,{0,0,1},0xFF},     // the first instance's quad (slice 0) at t = 1
                      {{0.25f,0.25f,2},10,{0,0,-1},0xFF},   // its triangle (slice 1) at z = 1, before the quad
                      {{10.5f,0.5f,-1},10,{0,0,1},0xFF},    // the moved instance's quad
                      {{10.5f,0.5f,-1},10,{0,0,1},RtMaskView},   // the same ray masked to the view: no hit
                      {{5,5,-1},10,{0,0,1},0xFF}};          // between them: no hit
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,512,D3D12_RESOURCE_STATE_GENERIC_READ);
    {void* mapped{};if(FAILED(upload->Map(0,nullptr,&mapped)))throw std::runtime_error("test map");
        std::memcpy(mapped,records,sizeof records);std::memcpy(static_cast<uint8_t*>(mapped)+256,rays,sizeof rays);upload->Unmap(0,nullptr);}
    auto answers=Committed(gpu,D3D12_HEAP_TYPE_DEFAULT,256,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,256,D3D12_RESOURCE_STATE_COPY_DEST);
    BindlessHeap heap;if(FAILED(heap.Open(gpu.device.Get(),16)))throw std::runtime_error("test bindless heap");
    const uint32_t scene=heap.Allocate(gpu.CompletedValue()),ray_slot=heap.Allocate(gpu.CompletedValue()),answer_slot=heap.Allocate(gpu.CompletedValue());
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    if(FAILED(gpu.device->CreateRootSignature(0,rt_probe_dxil,sizeof rt_probe_dxil,IID_PPV_ARGS(&root))))throw std::runtime_error("probe root");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={rt_probe_dxil,sizeof rt_probe_dxil};
    if(FAILED(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline))))throw std::runtime_error("probe pipeline");
    RtTlas tlas;
    {auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        Require(tlas.Build(gpu.device5.Get(),list.Get(),upload->GetGPUVirtualAddress(),0)==0,"no instance, no build");
        (void)tlas.Build(gpu.device5.Get(),list.Get(),upload->GetGPUVirtualAddress(),1);const auto first=tlas.Bytes();
        const auto address=tlas.Build(gpu.device5.Get(),list.Get(),upload->GetGPUVirtualAddress(),2);
        Require(address && tlas.Bytes()>=first,"built for two");
        D3D12_SHADER_RESOURCE_VIEW_DESC sv{DXGI_FORMAT_UNKNOWN,D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        sv.RaytracingAccelerationStructure.Location=address;gpu.device->CreateShaderResourceView(nullptr,&sv,heap.Cpu(scene));
        D3D12_SHADER_RESOURCE_VIEW_DESC rv{DXGI_FORMAT_R32_TYPELESS,D3D12_SRV_DIMENSION_BUFFER,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        rv.Buffer.FirstElement=64;rv.Buffer.NumElements=64;rv.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;gpu.device->CreateShaderResourceView(upload.Get(),&rv,heap.Cpu(ray_slot));
        D3D12_UNORDERED_ACCESS_VIEW_DESC av{DXGI_FORMAT_R32_TYPELESS,D3D12_UAV_DIMENSION_BUFFER};av.Buffer.NumElements=64;av.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
        gpu.device->CreateUnorderedAccessView(answers.Get(),nullptr,&av,heap.Cpu(answer_slot));
        ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetPipelineState(pipeline.Get());
        const uint32_t constants[6]{scene,ray_slot,answer_slot,5,BindlessIndices::none,BindlessIndices::none};list->SetComputeRoot32BitConstants(0,6,constants,0);list->Dispatch(1,1,1);
        D3D12_RESOURCE_BARRIER copy{};copy.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        copy.Transition={answers.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&copy);
        list->CopyBufferRegion(readback.Get(),0,answers.Get(),0,80);(void)recording.Commit();}
    gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
    uint32_t a[20]{};{void* mapped{};const D3D12_RANGE read{0,80};if(FAILED(readback->Map(0,&read,&mapped)))throw std::runtime_error("test readback");
        std::memcpy(a,mapped,sizeof a);const D3D12_RANGE none{0,0};readback->Unmap(0,&none);}
    const auto t=[&](int i){float v{};std::memcpy(&v,&a[4*i+1],4);return v;};
    const auto near1=[&](int i){return t(i)>0.999f && t(i)<1.001f;};
    Require(a[0]==1 && near1(0) && a[2]==7 && a[3]==0,"the first instance's quad: instance 7, slice 0, t = 1");
    Require(a[4]==1 && near1(1) && a[6]==7 && a[7]==1,"its triangle first from above: slice 1");
    Require(a[8]==1 && near1(2) && a[10]==9 && a[11]==0,"the moved instance's quad: instance 9 (24 bits)");
    Require(a[12]==0 && a[16]==0,"the masked ray and the ray between them miss");
    std::printf("rt scene: top level %llu bytes with its scratch, five rays answered\n",static_cast<unsigned long long>(tlas.Bytes()));
    tlas.Close();
}

// I-0117 T2: the records of a skinned instance's slices (pure): the excluded slice keeps its place with no vertex, the first
// output vertices follow one another, an element past the vertex or misaligned excludes its slice.
void SkinRecords() {
    std::vector<RtSkinSlice> s(4);
    s[0].slice={0,4,16,64,6,0,0,0,1,2};s[0].mode=1;s[0].index_offset=12;
    s[1]=s[0];s[1].mode=0;
    s[2].slice={128,3,24,200,3,0,0,0,1,4};s[2].mode=2;s[2].index_offset=12;s[2].weight_offset=16;
    s[3]=s[2];s[3].weight_offset=22;   // not 4-byte aligned
    std::vector<RtSkinRecord> r;
    const uint64_t whole[]{512},short_source[]{150};
    Require(RtSkinRecords(s,whole,r)==7 && r.size()==4,"seven vertices over four records");
    Require(r[0].vertices==4 && r[0].first==0 && r[0].mode==1 && r[1].vertices==0 && r[1].first==4 && r[2].vertices==3 && r[2].first==4 && r[2].mode==2,"the records' places");
    Require(r[3].vertices==0,"a misaligned weight excludes its slice");
    Require(RtSkinRecords(s,short_source,r)==4 && r[2].vertices==0,"a slice past the source's bytes excluded");
    // T2b: a slice read from a physics stream (source 1) at its own offset; a source not given excludes its slice.
    auto streamed=s;streamed[2].source=1;streamed[2].source_offset=0;const uint64_t two[]{512,72};
    Require(RtSkinRecords(streamed,two,r)==7 && r[2].source==1 && r[2].source_offset==0 && r[2].vertices==3,"a stream's slice at its offset");
    Require(RtSkinRecords(streamed,whole,r)==4 && r[2].vertices==0,"a stream not given excludes its slice");
    // T5: a blended slice whose indices and weights are in its second stream (at 256, 8 bytes a vertex: the face's FG_A models):
    // read there; with a physics stream, or the second stream past the storage, excluded.
    auto second=s;second[2].slice.vertex_offset2=256;second[2].slice.stride2=8;second[2].blend_stream=1;second[2].index_offset=0;second[2].weight_offset=4;
    Require(RtSkinRecords(second,whole,r)==7 && r[2].vertices==3 && r[2].blend_offset==256 && r[2].blend_stride==8 && r[0].blend_stride==0,"the blend elements in the second stream");
    auto second_streamed=second;second_streamed[2].source=1;
    Require(RtSkinRecords(second_streamed,two,r)==4 && r[2].vertices==0,"the second stream with a physics stream: excluded");
    const uint64_t short_second[]{270};
    Require(RtSkinRecords(second,short_second,r)==4 && r[2].vertices==0,"the second stream past the storage: excluded");
    // I-0117 T6: a world-row slice moved by the wind, TEXCOORD1 at 12 of 24 bytes as float3 (or 16 as half4); past the vertex, or
    // misaligned, or of no known format: the slice kept without its wind.
    auto wind=s;wind[2].mode=3;wind[2].wind=RtWindFloat3;wind[2].wind_offset=12;
    Require(RtSkinRecords(wind,whole,r)==7 && r[2].vertices==3 && r[2].wind==RtWindFloat3 && r[2].wind_offset==12 && r[0].wind==0,"the wind's TEXCOORD1 at 12");
    wind[2].wind_offset=16;Require(RtSkinRecords(wind,whole,r)==7 && r[2].vertices==3 && r[2].wind==0 && r[2].wind_offset==0,"a float3 past the vertex: no wind");
    wind[2].wind=RtWindHalf4;Require(RtSkinRecords(wind,whole,r)==7 && r[2].wind==RtWindHalf4 && r[2].wind_offset==16,"a half4 at 16");
    wind[2].wind=RtWindUnorm8;wind[2].wind_offset=20;Require(RtSkinRecords(wind,whole,r)==7 && r[2].wind==RtWindUnorm8 && r[2].wind_offset==20,"a UNORM8 at 20");
    wind[2].wind=RtWindHalf4;
    wind[2].wind_offset=14;Require(RtSkinRecords(wind,whole,r)==7 && r[2].vertices==3 && r[2].wind==0,"a misaligned TEXCOORD1: no wind");
    wind[2].wind=7;wind[2].wind_offset=12;Require(RtSkinRecords(wind,whole,r)==7 && r[2].wind==0,"an unknown format: no wind");
}

// I-0117 T6: the Grass programmes' wind on the CPU, transcribed from GXFlver_GrassSpcBumpDOL_GBuf's DXBC instruction by
// instruction apart from rt_skin.hlsl: the world position w moved by TEXCOORD1 t and the block's winds.
std::array<float,3> WindReference(const RtWindBlock& block,const float w[3],const float t[3]) {
    const auto row=[&](int k){std::array<float,4> r{};std::memcpy(r.data(),block.data()+16*(k-8),16);return r;};
    const auto bits=[](uint32_t b){float f{};std::memcpy(&f,&b,4);return f;};
    const float k442=bits(0x39E7C943u),tau=bits(0x40C90FDBu),pi=bits(0x40490FDBu),inv_tau=bits(0x3E22F983u);
    const auto sfrac=[](float x){const float a=std::fabs(x),f=a-std::floor(a);return x>=-x?f:-f;};
    std::array<float,4> size{};std::memcpy(size.data(),block.data()+kRtWindForceBytes,16);
    int32_t n[2]{};std::memcpy(n,block.data(),8);const int nd=(std::min)(n[0],8),np=(std::min)(n[1],8);
    const float r0x=1.f/std::sqrt(size[1]),dist=std::sqrt(w[0]*w[0]+w[2]*w[2]);
    const auto gust=[&](const std::array<float,4>& noise,float time){const float a=sfrac((time*noise[2]+dist*noise[3])*k442)*tau-pi;
        float s=std::sin(a)*0.5f+0.5f;const float s3=s*s*s;s=(s*(s*6-15)+10)*s3;return s*(noise[1]-noise[0])+noise[0];};
    float ax=0,az=0;
    for(int i=0;i<nd;++i){const auto F=row(9+i),P=row(33+i),N=row(41+i);
        const float k=gust(N,F[3]),f[3]{k*F[0],k*F[1],k*F[2]};
        const float d[3]{w[0]-P[0],w[1]-P[1],w[2]-P[2]},d2=d[0]*d[0]+d[1]*d[1]+d[2]*d[2],len=d2>0?std::sqrt(d2):0;
        const float fade=P[3]>0?(std::max)(1-len/(std::max)(bits(0x38D1B717u),P[3]),0.f):1;
        const float f2=f[0]*f[0]+f[1]*f[1]+f[2]*f[2],inv=f2>0?1/std::sqrt(f2):0;
        const float ph=t[2]*F[3]*10+(t[0]*tau-(f[0]*inv*w[0]+f[1]*inv*w[1]+f[2]*inv*w[2]));
        const float c=std::sin(sfrac(ph*inv_tau)*tau);
        if(F[3]>0){ax+=(c*f[0]*fade+fade*f[0]*t[1]*10)*r0x;az+=(c*f[2]*fade+fade*f[2]*t[1]*10)*r0x;}}
    for(int j=0;j<np;++j){const auto PP=row(17+j),PF=row(25+j),NP=row(49+j),DE=row(57+j),AT=row(65+j);
        const float k=gust(NP,PP[3]),g[3]{k*PF[0],k*PF[1],k*PF[2]};
        const float d[3]{w[0]-PP[0],w[1]-PP[1],w[2]-PP[2]},d2=d[0]*d[0]+d[1]*d[1]+d[2]*d[2];const bool apart=d2>0;
        float fx=0,fz=0,rl=0,rx=d[0],rz=d[2];
        if(apart){const float r=1/std::sqrt(d[0]*d[0]+d[2]*d[2]);fx=d[0]*r;fz=d[2]*r;rl=std::sqrt(d2);rx=d[0]/rl;rz=d[2]/rl;}
        float ex=0,ez=0;if(DE[0]*DE[0]+DE[1]*DE[1]+DE[2]*DE[2]>0){const float r=1/std::sqrt(DE[0]*DE[0]+DE[2]*DE[2]);ex=DE[0]*r;ez=DE[2]*r;}
        const bool aimed=DE[3]>0;const float cone=(std::clamp)((ex*fx+ez*fz)*bits(0x3F4CCCCDu)+bits(0x3E4CCCCDu),0.f,1.f);
        const float px=aimed?ex:fx,pz=aimed?ez:fz,lx=aimed?ex:rx,lz=aimed?ez:rz,lw=aimed?cone:1;
        const float fade=lw*(1-(std::min)(rl/(std::max)(g[1],bits(0x3A83126Fu)),1.f));
        const float c=std::sin(sfrac((10*PP[3]-2*rl)*inv_tau)*tau),m=g[2]*(1-c)+c,bx=lx*10*t[1],bz=lz*10*t[1];
        const float ox=bx*AT[0]*fade+(px*(g[0]*fade)*m+g[0]*bx*fade),oz=bz*AT[0]*fade+(pz*(g[0]*fade)*m+g[0]*bz*fade);
        if(fade>0 && apart && PP[3]>0){ax+=ox*r0x;az+=oz*r0x;}}
    const float vx=ax/std::sqrt(ax*ax+1),vz=az/std::sqrt(az*az+1),y=std::sqrt((std::max)(1-(vx*vx+vz*vz),0.f))-1,h=t[1]*size[1];
    return {w[0]+vx*h,w[1]+y*h,w[2]+vz*h};
}

// I-0117 T6: a placed instance moved by the wind on the hardware device: four quads placed by the world row (lifted by 1 in z),
// their TEXCOORD1 as float3, half4, R8G8B8A8_UNORM (the grass's FLVER UV1) and B8G8R8A8_UNORM, one directional and one point wind; the skinned positions read back equal the CPU's
// transcription, the BLAS built over them; the same wind again skins nothing; with no wind block the placed positions.
void Wind(DeviceQueues& gpu) {
    std::vector<uint8_t> bytes(512,0);
    const float phases[4]{0.25f,0.5f,0.125f,0.375f},weights[4]{0.25f,0.5f,0.75f,0.125f};
    const uint16_t halves[4][2]{{0x3400,0x3400},{0x3800,0x3800},{0x3000,0x3A00},{0x3600,0x3000}};   // the same phases and weights
    for(int v=0;v<4;++v){const float p[3]{float(v==1 || v==2),float(v>=2),0};
        std::memcpy(bytes.data()+24*v,p,12);const float t[3]{phases[v],weights[v],0.5f};std::memcpy(bytes.data()+24*v+12,t,12);
        std::memcpy(bytes.data()+128+24*v,p,12);const uint16_t h[4]{halves[v][0],halves[v][1],0x3800,0};std::memcpy(bytes.data()+128+24*v+12,h,8);}
    // The third quad (stride 16): TEXCOORD1 as bytes over 255.
    const uint8_t unorm[4][4]{{64,128,128,0},{128,64,128,0},{32,191,128,0},{96,32,128,0}};
    for(int v=0;v<4;++v){const float p[3]{float(v==1 || v==2),float(v>=2),0};std::memcpy(bytes.data()+256+16*v,p,12);std::memcpy(bytes.data()+256+16*v+12,unorm[v],4);
        // The fourth (stride 16, at 336) the same bytes as B8G8R8A8_UNORM: x the third byte, z the first.
        std::memcpy(bytes.data()+336+16*v,p,12);std::memcpy(bytes.data()+336+16*v+12,unorm[v],4);}
    const uint16_t quad[6]{0,1,2,0,2,3};std::memcpy(bytes.data()+96,quad,sizeof quad);std::memcpy(bytes.data()+224,quad,sizeof quad);std::memcpy(bytes.data()+320,quad,sizeof quad);std::memcpy(bytes.data()+400,quad,sizeof quad);
    const auto buffer=gpu.CreateStreamed(BufferOf(bytes.size(),D3D12_RESOURCE_FLAG_NONE),D3D12_RESOURCE_STATE_COMMON,PlacedKind::Buffer);
    {auto staging=gpu.AllocateStaging(bytes.size(),4);std::memcpy(staging.cpu,bytes.data(),bytes.size());
        auto recording=gpu.RecordIngress();recording.List()->CopyBufferRegion(buffer.Get(),0,staging.resource,staging.offset,bytes.size());
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={buffer.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,
            static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER|D3D12_RESOURCE_STATE_INDEX_BUFFER|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
        recording.List()->ResourceBarrier(1,&barrier);recording.Keep(std::move(staging.lease));(void)recording.Commit();}
    auto slices=std::make_shared<std::vector<RtSkinSlice>>(4);
    (*slices)[0].slice={0,4,24,96,6,0,0,0,1,2};(*slices)[0].mode=3;(*slices)[0].wind=RtWindFloat3;(*slices)[0].wind_offset=12;
    (*slices)[1].slice={128,4,24,224,6,0,0,0,1,2};(*slices)[1].mode=3;(*slices)[1].wind=RtWindHalf4;(*slices)[1].wind_offset=12;
    (*slices)[2].slice={256,4,16,320,6,0,0,0,1,2};(*slices)[2].mode=3;(*slices)[2].wind=RtWindUnorm8;(*slices)[2].wind_offset=12;
    (*slices)[3].slice={336,4,16,400,6,0,0,0,1,2};(*slices)[3].mode=3;(*slices)[3].wind=RtWindBgra8;(*slices)[3].wind_offset=12;
    auto block=std::make_shared<RtWindBlock>();
    const auto put=[&](int row,std::array<float,4> v){std::memcpy(block->data()+16*(row-8),v.data(),16);};
    {const int32_t loops[4]{1,1,0,0};std::memcpy(block->data(),loops,16);}
    put(9,{0.3f,0,0.5f,12.5f});put(33,{0,0,0,0});put(41,{0.4f,0.9f,0.7f,0.05f});                  // a directional wind, no range
    put(17,{0.5f,0,-2,3});put(25,{0.6f,10,0.3f,0});put(49,{0.5f,1,0.2f,0.1f});put(57,{0,0,0,0});put(65,{0.2f,0,0,0});   // a point wind, radial
    {const float size[4]{0,1.5f,0,0};std::memcpy(block->data()+kRtWindForceBytes,size,16);}
    RtSkinnedInstance item;item.blas=std::make_shared<RtSkinnedBlas>();item.source=buffer;item.slices=slices;item.id=(1u<<23)|7;item.mask=RtMaskView;
    item.world={1,0,0,0, 0,1,0,0, 0,0,1,1};item.wind=block;item.generation={1,0};
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,64*1024,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* mapped{};
    if(FAILED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))))throw std::runtime_error("test map");
    uint64_t used{};
    const RtSkinner::Upload allocate=[&](uint64_t size){const uint64_t at=(used+255)&~uint64_t(255);used=at+size;if(used>64*1024)throw std::runtime_error("test upload");
        UploadArena::Range r;r.cpu=mapped+at;r.gpu=upload->GetGPUVirtualAddress()+at;r.bytes=size;return r;};
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,192,D3D12_RESOURCE_STATE_COPY_DEST);
    RtSkinner skinner;
    // One frame: the instance skinned (or not); its sixteen positions read back.
    const auto frame=[&](RtSkinCounters& counters){
        used=0;std::vector<RtInstance> out;
        auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        skinner.Encode(gpu,list.Get(),allocate,std::span<const RtSkinnedInstance>(&item,1),out,counters);
        Require(out.size()==1 && out[0].blas && out[0].id==item.id,"one instance");
        D3D12_RESOURCE_BARRIER copy{};copy.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;   // the positions decay to COMMON between submissions
        copy.Transition={item.blas->positions.Get(),0,counters.vertices?D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE};
        if(counters.vertices)list->ResourceBarrier(1,&copy);
        list->CopyBufferRegion(readback.Get(),0,item.blas->positions.Get(),0,192);(void)recording.Commit();
        gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
        std::array<float,48> p{};void* read{};const D3D12_RANGE range{0,192};if(FAILED(readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
        std::memcpy(p.data(),read,192);const D3D12_RANGE none{0,0};readback->Unmap(0,&none);return p;};
    const auto rest=[&](int v){return std::array<float,3>{float(v%4==1 || v%4==2),float(v%4>=2),1};};
    RtSkinCounters c1{};auto p=frame(c1);
    Require(c1.builds==1 && c1.vertices==16 && c1.winds==1 && c1.wind_vertices==16 && c1.failed==0,"built once, sixteen vertices moved by the wind");
    float largest{};
    for(int v=0;v<16;++v){const auto w=rest(v);float t[3]{phases[v%4],weights[v%4],0.5f};
        if(v>=8)for(int k=0;k<3;++k)t[k]=float(unorm[v%4][v>=12?2-k:k])/255.f;
        const auto want=WindReference(*block,w.data(),t);
        for(int k=0;k<3;++k){Require(std::fabs(p[3*v+k]-want[k])<2e-3f,"the GPU's wind is the CPU's transcription");largest=(std::max)(largest,std::fabs(want[k]-w[k]));}}
    Require(largest>0.01f,"the wind moves the quads");
    RtSkinCounters c2{};p=frame(c2);Require(c2.vertices==0 && c2.refits==0,"the same wind: nothing skinned again");
    // No wind block: the placed positions (a new generation), refitted.
    item.wind=nullptr;item.generation={2,0};RtSkinCounters c3{};p=frame(c3);
    Require(c3.refits==1 && c3.winds==0,"refitted without the wind");
    for(int v=0;v<16;++v){const auto w=rest(v);for(int k=0;k<3;++k)Require(std::fabs(p[3*v+k]-w[k])<1e-5f,"the placed positions without a wind block");}
    // The final review's M1: an instance that cannot be skinned (its BLAS failed) with its storage's BLAS is traced at its placement
    // in its rest pose; without one, left out.
    {item.rest=readback;item.blas->failed=true;std::vector<RtInstance> out;RtSkinCounters c{};used=0;
        auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        skinner.Encode(gpu,list.Get(),allocate,std::span<const RtSkinnedInstance>(&item,1),out,c);
        Require(out.size()==1 && out[0].blas==item.rest && !out[0].positions && out[0].transform==item.world && out[0].id==item.id && c.failed==1,"the instance in its rest pose");
        item.rest=nullptr;out.clear();skinner.Encode(gpu,list.Get(),allocate,std::span<const RtSkinnedInstance>(&item,1),out,c);
        Require(out.empty(),"left out without a rest structure");(void)recording.Commit();gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();}
    std::printf("rt scene: the wind's largest move %.3f m, matched by the GPU\n",largest);
    upload->Unmap(0,nullptr);skinner.Close();
}

// I-0117 T3 (T2 step review, carried, L5): the palette rows a skinned instance's slices index, read from the storage's bytes as the
// shader reads them: a rigid slice's NORMAL.w, a blended slice's four indices (into the cancel's role too with the cancel), a
// streamed slice every row of its role, a world-row slice none, a slice past the bytes every row.
void SkinRows() {
    std::vector<uint8_t> bytes(256,0);
    for(int v=0;v<4;++v){const uint8_t n[4]{9,9,9,uint8_t(v<2?3:5)};std::memcpy(bytes.data()+16*v+12,n,4);}   // rigid: rows 3 and 5
    for(int v=0;v<3;++v){const uint16_t i[4]{uint16_t(7+v),2,2,0};std::memcpy(bytes.data()+128+24*v+12,i,8);}   // blended R16: 7, 8, 9, 2, 0
    std::vector<RtSkinSlice> s(4);
    s[0].slice={0,4,16,64,6,0,0,0,1,2};s[0].mode=1;s[0].index_offset=12;
    s[1].slice={128,3,24,200,3,0,0,0,1,4};s[1].mode=2;s[1].index_offset=12;s[1].index_format=1;s[1].cancel=1;
    s[2]=s[0];s[2].mode=1;s[2].palette=1;s[2].source=1;   // streamed, on the auxiliary rows
    s[3]=s[0];s[3].mode=3;
    const auto r=RtSkinRowsOf(bytes,s);
    Require(r.rows[0]==std::vector<uint32_t>({0,2,3,5,7,8,9}) && !r.all[0],"the current rows: the rigid slice's NORMAL.w and the blended slice's indices");
    Require(r.rows[2]==std::vector<uint32_t>({0,2,7,8,9}) && !r.all[2],"the cancel's rows: the blended slice's");
    Require(r.all[1] && r.rows[1].empty(),"a streamed slice: every row of its role");
    auto past=s;past[0].slice.vertex_count=40;Require(RtSkinRowsOf(bytes,past).all[0],"a slice past the bytes: every row");
    // T5: a blended slice's indices in its second stream (R8 at 200, 4 bytes a vertex: rows 11, 12, 13 and 1).
    for(int v=0;v<3;++v){const uint8_t i[4]{uint8_t(11+v),1,1,1};std::memcpy(bytes.data()+200+4*v,i,4);}
    std::vector<RtSkinSlice> face(1);face[0].slice={128,3,24,200,3,0,200,4,1,4};face[0].mode=2;face[0].blend_stream=1;
    Require(RtSkinRowsOf(bytes,face).rows[0]==std::vector<uint32_t>({1,11,12,13}),"the blend indices read in the second stream");
}

// I-0117 T2: a skinned instance on the hardware device: a rigid quad (its bone in NORMAL.w), an excluded slice and a blended
// triangle (two bones at half weight) placed by a pose's rows, its BLAS built, then refitted on a new pose, each time an instance of
// a top level traced by rays (rt_probe.hlsl): the distances follow the rows, the cancel rows apply first, GeometryIndex() is the
// slice's index; an unchanged pose is neither skinned nor refitted again.
void Skinned(DeviceQueues& gpu) {
    std::vector<uint8_t> bytes(512,0);
    // Slice 0, stride 16: a quad at z = 0, NORMAL (R8G8B8A8_UINT) at 12 with its w (the bone) 1.
    for(int v=0;v<4;++v){const float p[3]{float(v==1 || v==2),float(v>=2),0};std::memcpy(bytes.data()+16*v,p,12);const uint8_t n[4]{0,0,0,1};std::memcpy(bytes.data()+16*v+12,n,4);}
    const uint16_t quad[6]{0,1,2,0,2,3};std::memcpy(bytes.data()+64,quad,sizeof quad);
    // Slice 2, stride 24: a triangle at x 20..21, z = 0, BLENDINDICES (0, 1) at 12 and BLENDWEIGHT (R8G8B8A8_UNORM, 255, 255: half
    // each once normalised) at 16.
    for(int v=0;v<3;++v){const float p[3]{20.f+float(v==1),float(v==2),0};std::memcpy(bytes.data()+128+24*v,p,12);
        const uint8_t i[4]{0,1,0,0},w[4]{255,255,0,0};std::memcpy(bytes.data()+128+24*v+12,i,4);std::memcpy(bytes.data()+128+24*v+16,w,4);}
    const uint32_t triangle[3]{0,1,2};std::memcpy(bytes.data()+200,triangle,sizeof triangle);
    // I-0117 T5c (T5's final review L4, carried): the same triangle with its blend elements in the second stream (the faces' FG_A
    // layout): its positions at 256 (stride 12), BLENDINDICES (0, 1) at 0 and BLENDWEIGHT (255, 255) at 4 of a stride-8 stream at 304.
    for(int v=0;v<3;++v){const float p[3]{20.f+float(v==1),float(v==2),0};std::memcpy(bytes.data()+256+12*v,p,12);
        const uint8_t i[4]{0,1,0,0},w[4]{255,255,0,0};std::memcpy(bytes.data()+304+8*v,i,4);std::memcpy(bytes.data()+304+8*v+4,w,4);}
    D3D12_RESOURCE_DESC desc=BufferOf(bytes.size(),D3D12_RESOURCE_FLAG_NONE);
    const auto buffer=gpu.CreateStreamed(desc,D3D12_RESOURCE_STATE_COMMON,PlacedKind::Buffer);
    {auto staging=gpu.AllocateStaging(bytes.size(),4);std::memcpy(staging.cpu,bytes.data(),bytes.size());
        auto recording=gpu.RecordIngress();recording.List()->CopyBufferRegion(buffer.Get(),0,staging.resource,staging.offset,bytes.size());
        // Left readable as the storage's upload leaves it with the rays' scene (AdmitNativeStaticBuffer): the skinning's list may be
        // submitted with this one, where nothing decays between them.
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={buffer.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,
            static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER|D3D12_RESOURCE_STATE_INDEX_BUFFER|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
        recording.List()->ResourceBarrier(1,&barrier);recording.Keep(std::move(staging.lease));(void)recording.Commit();}
    auto slices=std::make_shared<std::vector<RtSkinSlice>>(3);
    (*slices)[0].slice={0,4,16,64,6,0,0,0,1,2};(*slices)[0].mode=1;(*slices)[0].index_offset=12;
    (*slices)[1].slice=(*slices)[0].slice;   // excluded (mode 0)
    (*slices)[2].slice={128,3,24,200,3,0,0,0,1,4};(*slices)[2].mode=2;(*slices)[2].index_offset=12;(*slices)[2].weight_offset=16;
    const auto translate=[](float z){return std::array<float,12>{1,0,0,0, 0,1,0,0, 0,0,1,z};};
    RtSkinnedInstance item;item.blas=std::make_shared<RtSkinnedBlas>();item.source=buffer;item.slices=slices;item.id=(1u<<23)|5;item.mask=RtMaskView;
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,64*1024,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* mapped{};
    if(FAILED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))))throw std::runtime_error("test map");
    uint64_t used{};
    const RtSkinner::Upload allocate=[&](uint64_t size){const uint64_t at=(used+255)&~uint64_t(255);used=at+size;if(used>64*1024)throw std::runtime_error("test upload");
        UploadArena::Range r;r.cpu=mapped+at;r.gpu=upload->GetGPUVirtualAddress()+at;r.bytes=size;return r;};
    auto answers=Committed(gpu,D3D12_HEAP_TYPE_DEFAULT,256,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,256,D3D12_RESOURCE_STATE_COPY_DEST);
    BindlessHeap heap;if(FAILED(heap.Open(gpu.device.Get(),16)))throw std::runtime_error("test bindless heap");
    const uint32_t scene=heap.Allocate(gpu.CompletedValue()),ray_slot=heap.Allocate(gpu.CompletedValue()),answer_slot=heap.Allocate(gpu.CompletedValue());
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    if(FAILED(gpu.device->CreateRootSignature(0,rt_probe_dxil,sizeof rt_probe_dxil,IID_PPV_ARGS(&root))))throw std::runtime_error("probe root");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={rt_probe_dxil,sizeof rt_probe_dxil};
    if(FAILED(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline))))throw std::runtime_error("probe pipeline");
    RtSkinner skinner;RtTlas tlas;
    struct Ray {float origin[3],tmax,direction[3];uint32_t mask;};
    const Ray rays[2]{{{0.5f,0.5f,-1},20,{0,0,1},0xFF},{{20.25f,0.25f,-1},20,{0,0,1},0xFF}};
    // One frame: the instance skinned (or not), the top level of what was built, the two rays; their answers.
    const auto frame=[&](RtSkinCounters& counters){
        used=0;std::vector<RtInstance> out;
        auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        skinner.Encode(gpu,list.Get(),allocate,std::span<const RtSkinnedInstance>(&item,1),out,counters);
        Require(out.size()==1 && out[0].transform[0]==1 && out[0].transform[3]==0 && out[0].id==item.id,"one instance, the identity transform");
        const auto descs=allocate(64);RtFillInstanceDescs(out,reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(descs.cpu));
        const auto ray_range=allocate(sizeof rays);std::memcpy(ray_range.cpu,rays,sizeof rays);
        const auto address=tlas.Build(gpu.device5.Get(),list.Get(),descs.gpu,1);
        D3D12_SHADER_RESOURCE_VIEW_DESC sv{DXGI_FORMAT_UNKNOWN,D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        sv.RaytracingAccelerationStructure.Location=address;gpu.device->CreateShaderResourceView(nullptr,&sv,heap.Cpu(scene));
        D3D12_SHADER_RESOURCE_VIEW_DESC rv{DXGI_FORMAT_R32_TYPELESS,D3D12_SRV_DIMENSION_BUFFER,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        rv.Buffer.FirstElement=(ray_range.gpu-upload->GetGPUVirtualAddress())/4;rv.Buffer.NumElements=16;rv.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
        gpu.device->CreateShaderResourceView(upload.Get(),&rv,heap.Cpu(ray_slot));
        D3D12_UNORDERED_ACCESS_VIEW_DESC av{DXGI_FORMAT_R32_TYPELESS,D3D12_UAV_DIMENSION_BUFFER};av.Buffer.NumElements=64;av.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
        gpu.device->CreateUnorderedAccessView(answers.Get(),nullptr,&av,heap.Cpu(answer_slot));
        ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetPipelineState(pipeline.Get());
        const uint32_t constants[6]{scene,ray_slot,answer_slot,2,BindlessIndices::none,BindlessIndices::none};list->SetComputeRoot32BitConstants(0,6,constants,0);list->Dispatch(1,1,1);
        D3D12_RESOURCE_BARRIER copy{};copy.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        copy.Transition={answers.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&copy);
        list->CopyBufferRegion(readback.Get(),0,answers.Get(),0,32);(void)recording.Commit();
        gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
        std::array<uint32_t,8> a{};void* read{};const D3D12_RANGE range{0,32};if(FAILED(readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
        std::memcpy(a.data(),read,32);const D3D12_RANGE none{0,0};readback->Unmap(0,&none);return a;};
    const auto t=[](const std::array<uint32_t,8>& a,int i){float v{};std::memcpy(&v,&a[4*i+1],4);return v;};
    const auto close_to=[](float v,float want){return v>want-0.001f && v<want+0.001f;};
    // Pose 1: row 0 lifts by 1, row 1 by 3; the cancel's row 1 lowers by 1 (slice 0 applies it). Quad: 3 - 1 = 2 (t = 3); the
    // triangle: half of row 0 and half of row 1, 2 (t = 3), no cancel.
    (*slices)[0].cancel=1;
    item.palettes={std::make_shared<RtPoseRows>(RtPoseRows{translate(1),translate(3)}),nullptr,std::make_shared<RtPoseRows>(RtPoseRows{translate(0),translate(-1)})};
    item.generation={1,0};RtSkinCounters c1{};auto a=frame(c1);
    Require(c1.builds==1 && c1.refits==0 && c1.vertices==7 && c1.failed==0,"built once, seven vertices skinned");
    Require(a[0]==1 && close_to(t(a,0),3) && a[2]==item.id && a[3]==0,"the rigid quad by its bone's row after the cancel's: t = 3, slice 0");
    Require(a[4]==1 && close_to(t(a,1),3) && a[7]==2,"the blended triangle at the half-way row: t = 3, slice 2 (the excluded one keeps its place)");
    // The same pose again: nothing skinned. Then pose 2 (row 1 lifts by 7): a refit; the quad 6 (t = 7), the triangle 4 (t = 5).
    RtSkinCounters c2{};a=frame(c2);Require(c2.builds==0 && c2.refits==0 && c2.vertices==0 && close_to(t(a,0),3),"an unchanged pose: neither skinned nor refitted");
    item.palettes[0]=std::make_shared<RtPoseRows>(RtPoseRows{translate(1),translate(7)});item.generation={2,0};
    RtSkinCounters c3{};a=frame(c3);
    Require(c3.refits==1 && c3.builds==0 && close_to(t(a,0),7) && close_to(t(a,1),5),"refitted on the new pose: t = 7 and 5");
    // T2b: the triangle's vertices from a physics stream (its own buffer: the same vertices lifted by 1): t = 6; the quad stays 7.
    {std::vector<uint8_t> stream(bytes.begin()+128,bytes.begin()+200);for(int v=0;v<3;++v){const float z=1;std::memcpy(stream.data()+24*v+8,&z,4);}
        auto streamed=gpu.CreateStreamed(BufferOf(stream.size(),D3D12_RESOURCE_FLAG_NONE),D3D12_RESOURCE_STATE_COMMON,PlacedKind::Buffer);
        {auto staging=gpu.AllocateStaging(stream.size(),4);std::memcpy(staging.cpu,stream.data(),stream.size());
            auto recording=gpu.RecordIngress();recording.List()->CopyBufferRegion(streamed.Get(),0,staging.resource,staging.offset,stream.size());
            D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;   // the game's streams are upload buffers (GENERIC_READ; I-0132 R11f: COMMON in a GPU upload heap, here this test's own copy)
            barrier.Transition={streamed.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
            recording.List()->ResourceBarrier(1,&barrier);recording.Keep(std::move(staging.lease));(void)recording.Commit();}
        auto moved=std::make_shared<std::vector<RtSkinSlice>>(*slices);(*moved)[2].source=1;(*moved)[2].source_offset=0;item.slices=moved;
        item.sources={streamed};item.generation={2,1};
        RtSkinCounters c4{};a=frame(c4);
        Require(c4.refits==1 && close_to(t(a,0),7) && close_to(t(a,1),6) && a[7]==2,"the stream's vertices: t = 6, still slice 2");
        // T2c: the same stream as a cloth copy placed by the world row (lifted by 2, no palette): z = 1 + 2, t = 4.
        auto cloth=std::make_shared<std::vector<RtSkinSlice>>(*moved);(*cloth)[2].mode=3;item.slices=cloth;
        item.world={1,0,0,0, 0,1,0,0, 0,0,1,2};item.generation={2,2};
        RtSkinCounters c5{};a=frame(c5);
        Require(c5.refits==1 && close_to(t(a,0),7) && close_to(t(a,1),4) && a[7]==2,"the cloth copy by the world row: t = 4");}
    // The auxiliary route (the cloth binding): slice 0 on the auxiliary rows (row 1 lifts by 9), its cancel still first: t = 9.
    {auto aux=std::make_shared<std::vector<RtSkinSlice>>(*item.slices);(*aux)[0].palette=1;item.slices=aux;
        item.palettes[1]=std::make_shared<RtPoseRows>(RtPoseRows{translate(0),translate(9)});item.generation={3,2};
        RtSkinCounters c6{};a=frame(c6);Require(c6.refits==1 && close_to(t(a,0),9),"the auxiliary rows: t = 9");
        // A rigid slice reads its one row (the T2 hold's rows): row 0 of its auxiliary and cancel rows not finite changes nothing
        // (row 1 lifts by 11, the cancel's lowers by 1): t = 11.
        const float nan=std::numeric_limits<float>::quiet_NaN();const std::array<float,12> broken{nan,nan,nan,nan, nan,nan,nan,nan, nan,nan,nan,nan};
        item.palettes[1]=std::make_shared<RtPoseRows>(RtPoseRows{broken,translate(11)});
        item.palettes[2]=std::make_shared<RtPoseRows>(RtPoseRows{broken,translate(-1)});item.generation={3,3};
        RtSkinCounters c7{};a=frame(c7);Require(c7.refits==1 && a[0]==1 && close_to(t(a,0),11),"a rigid slice reads its own row alone: t = 11");
        item.palettes[2]=std::make_shared<RtPoseRows>(RtPoseRows{translate(0),translate(-1)});}
    // Every RtSkinner::kRebuildAfter refits a rebuild (5 refits so far): the refits up to it, then a build.
    {uint64_t builds{},refits{};for(uint64_t g=4;builds==0 && g<4+RtSkinner::kRebuildAfter;++g){item.generation={g,2};RtSkinCounters c{};(void)frame(c);builds+=c.builds;refits+=c.refits;}
        Require(builds==1 && refits==RtSkinner::kRebuildAfter-5,"a rebuild after kRebuildAfter refits");}
    // A held instance (a pose row not finite): a new generation, yet neither skinned nor refitted; still traced at its last place.
    {item.hold=true;item.generation={1000,2};RtSkinCounters c{};a=frame(c);
        Require(c.refits==0 && c.builds==0 && c.vertices==0 && close_to(t(a,0),11),"held: its last positions traced");item.hold=false;}
    // T5c: slice 2 from the second stream's blend elements (a new instance: its slices changed): the half-way row of the pose's
    // current rows (1 and 7): z = 4, t = 5, slice 2.
    {auto second=std::make_shared<std::vector<RtSkinSlice>>(*slices);auto& s2=(*second)[2];
        s2.slice={256,3,12,200,3,0,304,8,1,4};s2.mode=2;s2.blend_stream=1;s2.index_offset=0;s2.weight_offset=4;s2.source=0;s2.source_offset=0;
        item.slices=second;item.blas=std::make_shared<RtSkinnedBlas>();item.palettes[1]=nullptr;
        item.palettes[0]=std::make_shared<RtPoseRows>(RtPoseRows{translate(1),translate(7)});item.generation={2000,0};
        RtSkinCounters c{};a=frame(c);
        Require(c.builds==1 && a[4]==1 && close_to(t(a,1),5) && a[7]==2,"the blend elements read in the second stream: t = 5, slice 2");}
    std::printf("rt scene: a skinned instance built and refitted, %llu bytes of positions and structure, %llu of scratch\n",
        static_cast<unsigned long long>(item.blas->positions_bytes+item.blas->structure_bytes),static_cast<unsigned long long>(skinner.Bytes()));
    upload->Unmap(0,nullptr);tlas.Close();skinner.Close();
}

// I-0117 T3, I-0119 R3a: the alpha test and the materials in the rays. The model's quad (slice 0, UV16 in its vertices' last word:
// u = x, v = y) alpha-tested on a 4x1 diffuse texture whose texels are white with alphas 0, 0, 1, 1, with a 1x1 specular and a 1x1
// bump texture (the raster's CPU-only views, copied into the bindless heap by the material table), FC_DifMapMulCol (0.5, 0.25, 1)
// and a gloss scale of 0.5; its triangle (slice 1, z = 1, below it) opaque without a material; a second instance with no set moved
// 10 along x. Rays from z = -1: where the texture's alpha is 0 the quad is cut (the triangle behind it hit, or nothing), where it is 1
// the quad is hit with its material; the instance without a set is opaque. Then a second frame naming the same set writes nothing,
// and the views go once no frame named the set for kRetireFrames frames.
void Materials(DeviceQueues& gpu,const std::shared_ptr<RtModelBlas>& blas,ID3D12Resource* buffer) {
    D3D12_HEAP_PROPERTIES heap_default{};heap_default.Type=D3D12_HEAP_TYPE_DEFAULT;
    // Three textures: the diffuse 4x1, the specular and the bump 1x1 (R8G8B8A8_UNORM, texels as little-endian words).
    const uint32_t texels[3][4]{{0x00FFFFFFu,0x00FFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu},{0xFFC08040u},{0xFFC88080u}};const UINT widths[3]{4,1,1};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> textures;
    auto staging=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,768,D3D12_RESOURCE_STATE_GENERIC_READ);
    {void* mapped{};if(FAILED(staging->Map(0,nullptr,&mapped)))throw std::runtime_error("test map");
        for(int k=0;k<3;++k)std::memcpy(static_cast<uint8_t*>(mapped)+256*k,texels[k],4*widths[k]);staging->Unmap(0,nullptr);}
    D3D12_DESCRIPTOR_HEAP_DESC cd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,3,D3D12_DESCRIPTOR_HEAP_FLAG_NONE};Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpu_heap;
    if(FAILED(gpu.device->CreateDescriptorHeap(&cd,IID_PPV_ARGS(&cpu_heap))))throw std::runtime_error("test cpu heap");
    const UINT increment=gpu.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const auto cpu=[&](int k){auto h=cpu_heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=size_t(k)*increment;return h;};
    for(int k=0;k<3;++k){D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=widths[k];td.Height=1;td.DepthOrArraySize=td.MipLevels=1;
        td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;
        if(FAILED(gpu.device->CreateCommittedResource(&heap_default,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&textures[k]))))throw std::runtime_error("test texture");
        gpu.device->CreateShaderResourceView(textures[k].Get(),nullptr,cpu(k));}
    auto set=std::make_shared<RtMaterialSet>();set->buffer=buffer;set->slices.resize(3);set->non_opaque=true;
    auto& q=set->slices[0];q.index_offset=64;q.index_width=2;q.position_offset=0;q.position_stride=16;q.uv_offset=12;q.uv_stride=16;q.uv_format=RtUvSint16;
    q.material=true;q.alpha=q.alpha_texture=true;q.ref=0.5f;q.albedo={0.5f,0.25f,1,0.5f};q.family=7;
    q.skin=true;set->skin=true;   // R3c: the skin's curve, with the instance's FC_SkinMatchColor below
    const std::array<std::shared_ptr<const void>,3> owners{std::make_shared<int>(1),std::make_shared<int>(2),std::make_shared<int>(3)};
    for(const auto [role,k]:{std::pair{RtTextureDiffuse,0},std::pair{RtTextureSpecular,1},std::pair{RtTextureBump,2}}){q.views[role]=cpu(k);q.textures[role]=owners[k];}
    auto& tri=set->slices[1];tri.index_offset=176;tri.index_width=4;tri.position_offset=128;tri.position_stride=12;
    tri.keep=RtSliceKeep(false,true);   // T5: the triangle an overlay (I-0118 R5c: a transparent caster): never the view's rays; the sun's and the local lights' yes
    set->slices[2].layers.resize(1);   // R3c: the ineligible slice a two-layer blend (its layer record after the slices')
    set->slices[2].position_stride=12;   // I-0120 S1: a slice the collect reached has its position stride (one without takes no view)
    std::vector<RtInstance> instances(2);
    instances[0].transform={1,0,0,0, 0,1,0,0, 0,0,1,0};instances[0].blas=blas->Structure();instances[0].id=3;instances[0].mask=RtMaskView|RtMaskSun;instances[0].materials=set;
    instances[0].skin={-0.5f,-0.5f,-0.5f,0};   // R3c: saturate(sqrt(1) - 0.5)² = 0.25 on the white texel
    instances[1]=instances[0];instances[1].transform[3]=10;instances[1].id=4;instances[1].materials=nullptr;instances[1].skin={};
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,262144,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* mapped{};   // the table made at 256 records (48 KiB)
    if(FAILED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))))throw std::runtime_error("test map");
    uint64_t used{};
    const RtMaterialTable::Upload allocate=[&](uint64_t size){const uint64_t at=(used+255)&~uint64_t(255);used=at+size;if(used>262144)throw std::runtime_error("test upload");
        UploadArena::Range r;r.cpu=mapped+at;r.gpu=upload->GetGPUVirtualAddress()+at;r.bytes=size;r.resource=upload.Get();r.offset=at;return r;};
    BindlessHeap heap;if(FAILED(heap.Open(gpu.device.Get(),16)))throw std::runtime_error("test bindless heap");
    RtMaterialTable table;RtTlas tlas;
    struct Ray {float origin[3],tmax,direction[3];uint32_t mask;};
    const Ray rays[7]{{{0.125f,0.5f,-1},10,{0,0,1},0xFF},   // alpha 0: the quad cut, the triangle behind it (slice 1) at t = 2
                      {{0.625f,0.2f,-1},10,{0,0,1},0xFF},   // alpha 1: the quad at t = 1, its material
                      {{0.875f,0.9f,-1},10,{0,0,1},0xFF},   // alpha 1: the quad at t = 1
                      {{0.375f,0.9f,-1},10,{0,0,1},0xFF},   // alpha 0, past the triangle: no hit
                      {{10.125f,0.5f,-1},10,{0,0,1},0xFF},  // the instance with no set: its quad at t = 1
                      {{0.125f,0.5f,-1},10,{0,0,1},RtMaskView},  // T5: as the first with the view's mask: the overlay passed over, no hit
                      {{0.125f,0.5f,-1},10,{0,0,1},RtMaskSun}};  // R5c: as the first with the sun's mask: the transparent caster hit
    const auto ray_range=allocate(sizeof rays);std::memcpy(ray_range.cpu,rays,sizeof rays);
    auto answers=Committed(gpu,D3D12_HEAP_TYPE_DEFAULT,512,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,512,D3D12_RESOURCE_STATE_COPY_DEST);
    const uint32_t scene=heap.Allocate(gpu.CompletedValue()),ray_slot=heap.Allocate(gpu.CompletedValue()),answer_slot=heap.Allocate(gpu.CompletedValue()),
        table_slot=heap.Allocate(gpu.CompletedValue()),rows_slot=heap.Allocate(gpu.CompletedValue());
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    if(FAILED(gpu.device->CreateRootSignature(0,rt_probe_dxil,sizeof rt_probe_dxil,IID_PPV_ARGS(&root))))throw std::runtime_error("probe root");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={rt_probe_dxil,sizeof rt_probe_dxil};
    if(FAILED(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline))))throw std::runtime_error("probe pipeline");
    RtMaterialTable::Frame frame;
    {auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        std::vector<D3D12_RESOURCE_BARRIER> readable;
        for(int k=0;k<3;++k){D3D12_TEXTURE_COPY_LOCATION dst{textures[k].Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};dst.SubresourceIndex=0;
            D3D12_TEXTURE_COPY_LOCATION src{staging.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};src.PlacedFootprint.Offset=256ull*k;
            src.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,widths[k],1,1,256};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            b.Transition={textures[k].Get(),0,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};readable.push_back(b);}
        list->ResourceBarrier(3,readable.data());
        frame=table.Encode(gpu.device.Get(),list.Get(),heap,allocate,gpu.LastSubmitted(),gpu.CompletedValue(),{std::span<const RtInstance>(instances)});
        Require(frame.table && frame.rows && frame.records==4 && frame.instances==2 && frame.firsts.size()==2 && frame.firsts[0]==0,
            "four records for the instance with a set (three slices, one layer), two rows");
        const auto t=table.Totals();
        Require((table.RecordAt(2).flags&RtRecordLayers) && table.RecordAt(2).extension==3 && !(table.RecordAt(0).flags&RtRecordLayers) &&
            table.RecordAt(3).buffer==table.RecordAt(2).buffer,"R3c: the blend slice names its layer record, written after the slices");
        Require((table.RecordAt(0).flags&RtRecordSkin) && !(table.RecordAt(1).flags&RtRecordSkin),"R3c: the skin slice's record flagged, the triangle's not");
        Require(t.sets==1 && t.buffers==1 && t.textures==3 && t.written==4 && heap.Indices().Live()==9,"one set: a buffer view and three texture views in the heap");
        const auto descs=allocate(2*sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        RtFillInstanceDescs(instances,reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(descs.cpu),frame.firsts);
        const auto* d=reinterpret_cast<const D3D12_RAYTRACING_INSTANCE_DESC*>(descs.cpu);
        Require(d[0].Flags==D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE && d[0].InstanceContributionToHitGroupIndex==0 && d[1].Flags==0,
            "the instance with a non-opaque set forced non-opaque at its first record, the other opaque");
        const auto top=tlas.Build(gpu.device5.Get(),list.Get(),descs.gpu,2);
        D3D12_SHADER_RESOURCE_VIEW_DESC sv{DXGI_FORMAT_UNKNOWN,D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        sv.RaytracingAccelerationStructure.Location=top;gpu.device->CreateShaderResourceView(nullptr,&sv,heap.Cpu(scene));
        D3D12_SHADER_RESOURCE_VIEW_DESC rv{DXGI_FORMAT_R32_TYPELESS,D3D12_SRV_DIMENSION_BUFFER,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        rv.Buffer.FirstElement=ray_range.offset/4;rv.Buffer.NumElements=56;rv.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
        gpu.device->CreateShaderResourceView(upload.Get(),&rv,heap.Cpu(ray_slot));
        // The table and the rows are default buffers the encode made: views of their whole (the records at 224 a record from 0).
        D3D12_SHADER_RESOURCE_VIEW_DESC tv{DXGI_FORMAT_UNKNOWN,D3D12_SRV_DIMENSION_BUFFER,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        tv.Buffer.NumElements=frame.records;tv.Buffer.StructureByteStride=sizeof(RtMaterialRecord);
        gpu.device->CreateShaderResourceView(table.TableResource(),&tv,heap.Cpu(table_slot));
        D3D12_SHADER_RESOURCE_VIEW_DESC wv{DXGI_FORMAT_R32_TYPELESS,D3D12_SRV_DIMENSION_BUFFER,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        wv.Buffer.NumElements=static_cast<UINT>(2*sizeof(RtInstanceRow)/4);wv.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;gpu.device->CreateShaderResourceView(table.RowsResource(),&wv,heap.Cpu(rows_slot));   // two rows (T6; I-0125 R6a; R6d step review L3: by the row's size)
        D3D12_UNORDERED_ACCESS_VIEW_DESC av{DXGI_FORMAT_R32_TYPELESS,D3D12_UAV_DIMENSION_BUFFER};av.Buffer.NumElements=128;av.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
        gpu.device->CreateUnorderedAccessView(answers.Get(),nullptr,&av,heap.Cpu(answer_slot));
        ID3D12DescriptorHeap* heaps[]{heap.Heap()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetPipelineState(pipeline.Get());
        const uint32_t constants[6]{scene,ray_slot,answer_slot,7,table_slot,rows_slot};list->SetComputeRoot32BitConstants(0,6,constants,0);list->Dispatch(1,1,1);
        D3D12_RESOURCE_BARRIER copy{};copy.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        copy.Transition={answers.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&copy);
        list->CopyBufferRegion(readback.Get(),0,answers.Get(),0,112+48*7);(void)recording.Commit();}
    gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
    uint32_t a[28+84]{};{void* read{};const D3D12_RANGE range{0,sizeof a};if(FAILED(readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
        std::memcpy(a,read,sizeof a);const D3D12_RANGE none{0,0};readback->Unmap(0,&none);}
    const auto f=[&](int word){float v{};std::memcpy(&v,&a[word],4);return v;};
    const auto t=[&](int i){return f(4*i+1);};
    const auto near_to=[&](float v,float want){return v>want-0.002f && v<want+0.002f;};
    Require(a[0]==1 && near_to(t(0),2) && a[2]==3 && a[3]==1,"alpha 0: the quad cut, the triangle behind it hit (t = 2, slice 1)");
    Require(a[4]==1 && near_to(t(1),1) && a[7]==0 && a[8]==1 && near_to(t(2),1) && a[11]==0,"alpha 1: the quad hit (t = 1, slice 0)");
    Require(a[12]==0,"alpha 0 past the triangle: no hit");
    Require(a[16]==1 && near_to(t(4),1) && a[18]==4,"the instance with no set: opaque");
    Require(a[20]==0,"T5: the overlay slice passed over by a ray of the view's mask");
    Require(a[24]==1 && near_to(t(6),2) && a[26]==3 && a[27]==1,"R5c: a ray of the sun's mask: the quad cut, the transparent caster behind it hit (t = 2, slice 1)");
    // R3a: the materials, 12 words a ray after the 28 answers: {albedo, gloss} {shading normal, F0's red} {geometric normal, material}.
    const auto m=[&](int ray,int word){return f(28+12*ray+word);};
    Require(near_to(m(1,0),0.125f) && near_to(m(1,1),0.0625f) && near_to(m(1,2),0.25f) && near_to(m(1,3),0.5f*200/255.0f),
        "the quad's albedo the skin curve of the white texel times FC_DifMapMulCol, its gloss the bump's blue times the scale");
    Require(near_to(m(1,7),64/255.0f) && near_to(m(1,4),0) && near_to(m(1,5),0) && near_to(m(1,6),-1) && near_to(m(1,10),-1) && m(1,11)==1,
        "its F0 the specular texel, its normals toward the ray (no vertex frame: the geometric one)");
    Require(m(0,11)==0 && near_to(m(0,10),-1) && near_to(m(0,0),0.5f),"the triangle without a material: grey, its geometric normal toward the ray");
    // A second frame naming the set writes no record; the views and the range go once no frame named it for kRetireFrames frames.
    // I-0125 R6d (its final review L8): both instances sharing one forward block: one block after the two rows (no skinned bases), both
    // rows naming its byte, its colour and fog copied; the rows' upload the encode's last.
    {auto recording=gpu.RecordIngress();
        auto block=std::make_shared<RtForwardRow>();block->colour={0.5f,0.25f,1,1};block->fog[8]=3;block->fog[23]=7;
        auto with=instances;with[0].ambient.forward=block;with[1].ambient.forward=block;
        uint8_t* rows_cpu{};uint64_t rows_bytes{};int uploads{};   // the set not fresh: the rows' upload the only one (its step review's L2)
        const RtMaterialTable::Upload capture=[&](uint64_t size){auto r=allocate(size);rows_cpu=r.cpu;rows_bytes=size;++uploads;return r;};
        const auto again=table.Encode(gpu.device.Get(),recording.List(),heap,capture,gpu.LastSubmitted(),gpu.CompletedValue(),{std::span<const RtInstance>(with)});
        Require(again.firsts.size()==2 && again.firsts[0]==0 && table.Totals().written==4,"the set's records written once");
        RtInstanceRow r0{},r1{};RtForwardRow b{};
        Require(uploads==1 && rows_cpu && rows_bytes>=2*sizeof(RtInstanceRow)+sizeof(RtForwardRow),"the rows' upload, the frame's only one, holds one forward block");
        std::memcpy(&r0,rows_cpu,sizeof r0);std::memcpy(&r1,rows_cpu+sizeof r0,sizeof r1);std::memcpy(&b,rows_cpu+2*sizeof(RtInstanceRow),sizeof b);
        Require(r0.forward==2*sizeof(RtInstanceRow) && r1.forward==r0.forward && b.colour==block->colour && b.fog==block->fog,
            "R6d: one shared forward block after the rows, both rows naming its byte");(void)recording.Commit();}
    gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
    const RtMaterialTable::Upload nothing=[](uint64_t){return UploadArena::Range{};};
    // The sweep runs every kSweepFrames-th frame: kRetireFrames and two sweeps' frames unnamed.
    for(uint64_t n=0;n<RtMaterialTable::kRetireFrames+2*RtMaterialTable::kSweepFrames;++n)(void)table.Encode(gpu.device.Get(),nullptr,heap,nothing,gpu.LastSubmitted(),gpu.CompletedValue(),{});
    const auto gone=table.Totals();
    Require(gone.sets==0 && gone.records==0 && gone.buffers==0 && gone.textures==0 && heap.Indices().Live()==5,"the set's views and range released after kRetireFrames frames unnamed");
    std::printf("rt scene: the alpha test cuts the quad where its texture's alpha is 0, its material read at the hit, %u records\n",frame.records);
    upload->Unmap(0,nullptr);tlas.Close();table.Close(&heap,gpu.LastSubmitted());
}

// I-0117 T4: the control view's camera (pure), then the pass on the hardware device against the model's quad (x, y in [0, 1] at
// z = 0, its triangle hidden behind it). First a camera 2 before its centre and a 4x4 raster depth whose rows are the quad's depth,
// 3 % farther, 50 % farther, and two empty pixels with one nearer: the classes counted (agree, close, nearer, farther, extra), then
// with the sun casters' mask (no hit): missed and sky; the classes picture's empty pixel with a hit in magenta. Then (step review
// M1) an 8x4 view whose corner sees the quad's top right corner, with half a pixel of jitter, over an empty raster: the hit pixels
// are the left three columns of the lower two rows, which a flipped axis, swapped lens scales or the jitter's sign would move.
SceneViewInput ViewCamera() {
    SceneViewInput c{};c.serial=1;c.camera_pose={1,0,0,0, 0,1,0,0, 0,0,1,0, 0.5f,0.5f,-2,1};c.vertical_fov=2*std::atan(0.125f);c.near_plane=0.1f;c.far_plane=100;
    c.width=c.height=4;return c;}
// I-0119 R3a (final review L6): two sets give the same records only when every field, texture and view agrees.
void SameMaterials() {
    RtMaterialSet a;a.slices.resize(2);a.slices[0].material=true;a.slices[0].albedo={0.5f,0.5f,0.5f,0.25f};
    const auto texture=std::make_shared<int>(1);a.slices[1].textures[RtTextureBump]=texture;a.slices[1].views[RtTextureBump].ptr=42;
    auto b=a;Require(RtSameMaterials(a,b),"a copy: the same records");
    b.slices[1].textures[RtTextureBump]=std::make_shared<int>(1);Require(!RtSameMaterials(a,b),"another texture: other records");
    b=a;b.slices[0].albedo[3]=0.5f;Require(!RtSameMaterials(a,b),"another gloss scale: other records");
    b=a;b.non_opaque=true;Require(!RtSameMaterials(a,b),"another opacity: other records");
    b=a;b.slices.pop_back();Require(!RtSameMaterials(a,b),"fewer slices: other records");
    a.slices[0].layers.resize(1);b=a;Require(RtSameMaterials(a,b),"R3c: a blend's copy: the same records");
    b.slices[0].layers[0].tiling[RtTextureDiffuse][0]=2;Require(!RtSameMaterials(a,b),"R3c: another layer's tiling: other records");
    b=a;b.slices[0].mul=true;Require(!RtSameMaterials(a,b),"R3c: a Mul piece's second layer: other records");
    b=a;b.slices[0].mul=b.slices[0].mul_mask=true;Require(!RtSameMaterials(a,b),"R3c: a MulMask piece: other records");
    b=a;b.slices[0].skin=b.skin=true;Require(!RtSameMaterials(a,b),"R3c: the skin's curve: other records");
    b=a;b.slices[0].hair=b.skin=true;Require(!RtSameMaterials(a,b),"R3c: the hair's curve: other records");
    b=a;b.slices[0].mul=b.slices[0].face=true;Require(!RtSameMaterials(a,b),"R3c: the face: other records");
    b=a;b.slices[0].mul=b.slices[0].face=b.slices[0].face_both=true;{auto c=a;c.slices[0].mul=c.slices[0].face=true;Require(!RtSameMaterials(c,b),"R3c: the face image curved: other records");}
}
// T5: an instance's masks by its slices' material classes (RtInstanceMask).
void SurfaceMasks() {
    const uint8_t all=RtMaskView|RtMaskSun|RtMaskLocal|RtMaskReflection;
    Require(RtInstanceMask(all,{})==all && RtInstanceMask(0,{})==RtMaskOther,"not classed: the engine's bits, an empty mask Other");
    Require(RtInstanceMask(all,{true,0x1,true,true})==all && RtInstanceMask(all,{true,0x104,true,true})==all,"a G-buffer slice that casts: every bit kept");
    Require(RtInstanceMask(all,{true,0x82,false})==(RtMaskForward|RtMaskReflection),"a forward-only instance with no caster (the sky's dome): forward and reflection only");
    Require(RtInstanceMask(RtMaskSun,{true,0x80,false})==RtMaskOther,"a forward caster alone: nothing left, Other");
    Require(RtInstanceMask(RtMaskView|RtMaskSun,{true,0x5,false,true})==RtMaskView,"G-buffer slices none of which casts (no-cast, Decal): the view alone");
    Require(RtInstanceMask(all,{true,0x1,false,false})==(RtMaskForward|RtMaskReflection),"a G-buffer class drawn without a depth write (the High Wall's sky): no surface");
    Require(RtInstanceMask(RtMaskView|RtMaskSun,{true,0x82|0x1,true,true,true,true})==(RtMaskView|RtMaskSun|RtMaskForward),"a mixed instance with forward slices: their bit added");
    Require(RtInstanceMask(RtMaskView|RtMaskSun,{true,0x1,true,true,true,false})==(RtMaskView|RtMaskSun),"mixed by casting alone (a Decal slice): no forward bit");
    Require(RtSliceKeep(true,true)==(0xFF&~(RtMaskForward|RtMaskWater)) && RtSliceKeep(true,false)==(0xFF&~(RtMaskForward|RtMaskWater|RtMaskSun|RtMaskLocal)) &&
        RtSliceKeep(false,false)==(RtMaskForward|RtMaskReflection),"a slice's ray bits: a casting surface, a surface, an overlay");
    // I-0125 R6c: a water slice (forward, no caster) answers the water's rays besides the forward and reflection ones; its instance
    // gains the water bit.
    Require(RtSliceKeep(false,false,true)==(RtMaskForward|RtMaskReflection|RtMaskWater) && RtSliceKeep(true,false,true)==(0xFF&~(RtMaskForward|RtMaskSun|RtMaskLocal)),
        "R6c: a water slice's ray bits");
    {RtSurfaceClasses water{true,0x80,false};water.overlays=true;water.water=true;
        Require(RtInstanceMask(RtMaskView|RtMaskReflection,water)==(RtMaskForward|RtMaskReflection|RtMaskWater),"R6c: a water piece's instance: forward, reflection and water");
        Require(RtInstanceMask(RtMaskReflection,water)==RtMaskReflection,"R6c (step review L1): a water piece the eye does not see: no water bit");}
    // I-0118 R5c: the cascade row's casters (class mask 0x107 but a Decal or a no-cast byte): a surface, a transparent slice (the
    // hair's class 2, whatever its depth write); not a forward slice (0x80), a decal class, nor a G-buffer slice without a depth write.
    Require(RtSliceCasts(0x1,true,false,false,false) && RtSliceCasts(0x4,true,false,false,false) && RtSliceCasts(0x100,true,false,false,false) &&
        RtSliceCasts(0x2,false,false,false,false) && !RtSliceCasts(0x80,false,false,false,false) && !RtSliceCasts(0x8,false,false,false,false) &&
        !RtSliceCasts(0x10,false,false,false,false) && !RtSliceCasts(0x1,false,false,false,false) && !RtSliceCasts(0x2,false,true,false,false) &&
        !RtSliceCasts(0x2,false,false,true,false) && !RtSliceCasts(0x1,true,false,true,false) && !RtSliceCasts(0x2,false,false,false,true) &&
        !RtSliceCasts(0x1,true,false,false,true),"R5c: a slice's casting by the cascade row (a surface as the caller finds it, class 2 whatever, no other class without a surface; a water or refraction material refused)");
    Require(RtSliceKeep(false,true)==(RtMaskForward|RtMaskReflection|RtMaskSun|RtMaskLocal),"R5c: a transparent caster answers the casters' rays, never the view's");
    Require(RtInstanceMask(all,{true,0x82,true,false,true,true,true})==(RtMaskForward|RtMaskSun|RtMaskLocal|RtMaskReflection),
        "R5c: the hair (forward and transparent slices, the latter casting): the view's bit forward, the casters' kept");
    Require(RtInstanceMask(RtMaskView|RtMaskReflection,{true,0x2,true,false,false,true,true})==(RtMaskForward|RtMaskReflection),
        "R5c: a transparent caster the engine keeps out of the casters (category 1): no caster bit added");
}
void ViewCameraMaths() {
    auto c=ViewCamera();c.jitter_x=0.25f;c.jitter_y=-0.5f;RtViewConstants v{};
    Require(RtViewCameraOf(c,v),"the camera admitted");
    Require(std::abs(v.position[3]-8)<1e-4f && std::abs(v.forward[3]-8)<1e-4f && v.right[3]==0.1f && v.up[3]==100,"the lens (sy = sx = 1 / tan(fov / 2)) and the planes");
    Require(v.position[0]==0.5f && v.position[2]==-2 && v.forward[2]==1 && v.width==4,"the camera's rows");
    Require(std::abs(v.jitter[0]-0.125f)<1e-6f && std::abs(v.jitter[1]-0.25f)<1e-6f && v.jitter[2]==0.25f,"the jitter in NDC (2 jx / W, -2 jy / H)");
    c.vertical_fov=0;Require(!RtViewCameraOf(c,v),"a refused camera");
}
struct ViewRun {RtViewCounts counts,masked;std::vector<uint8_t> classes;};   // classes: R8G8B8A8 rows of 256 bytes
// The pass over `depth` (view z by pixel, row after row; 0 an empty pixel) from `camera`, once with the view's mask and once with
// the sun casters'; the classes picture of the first.
ViewRun RunView(DeviceQueues& gpu,const std::shared_ptr<RtModelBlas>& blas,const SceneViewInput& camera,const std::vector<float>& z) {
    const uint32_t width=camera.width,height=camera.height;const float n=camera.near_plane,f=camera.far_plane,a=f/(f-n);
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,8192,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* mapped{};
    if(FAILED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))))throw std::runtime_error("test map");
    for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){const float v=z[y*width+x],d=v?a-a*n/v:1.0f;std::memcpy(mapped+256*y+4*x,&d,4);}
    RtInstance instance;instance.transform={1,0,0,0, 0,1,0,0, 0,0,1,0};instance.blas=blas->Structure();instance.id=3;instance.mask=RtMaskView;
    RtFillInstanceDescs(std::span<const RtInstance>(&instance,1),reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mapped+2048));
    uint64_t used=4096;
    const auto allocate=[&](uint64_t size){const uint64_t at=(used+255)&~uint64_t(255);used=at+size;if(used>8192)throw std::runtime_error("test upload");
        UploadArena::Range r;r.cpu=mapped+at;r.gpu=upload->GetGPUVirtualAddress()+at;r.bytes=size;return r;};
    D3D12_HEAP_PROPERTIES heap_type{};heap_type.Type=D3D12_HEAP_TYPE_DEFAULT;
    const auto texture=[&](DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state){D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width=width;d.Height=height;d.DepthOrArraySize=d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;d.Flags=flags;Microsoft::WRL::ComPtr<ID3D12Resource> r;
        if(FAILED(gpu.device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r))))throw std::runtime_error("test texture");return r;};
    auto depth=texture(DXGI_FORMAT_R32_FLOAT,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);
    auto traced=texture(DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto classes=texture(DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto counts=Committed(gpu,D3D12_HEAP_TYPE_DEFAULT,RtView::kCountBytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    // The two passes' counts at 0 and kSecond, the classes picture at kPicture (I-0125 R6a: the counts 20,544 bytes with the families).
    constexpr uint64_t kSecond=20736,kPicture=41472,kBytes=57344;
    static_assert(RtView::kCountBytes<=kSecond && kSecond+RtView::kCountBytes<=kPicture);
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,kBytes,D3D12_RESOURCE_STATE_COPY_DEST);
    BindlessHeap heap;if(FAILED(heap.Open(gpu.device.Get(),16)))throw std::runtime_error("test bindless heap");
    RtViewConstants v{};Require(RtViewCameraOf(camera,v),"the view's camera");
    v.depth=heap.Allocate(gpu.CompletedValue());v.traced=heap.Allocate(gpu.CompletedValue());v.classes=heap.Allocate(gpu.CompletedValue());
    {D3D12_SHADER_RESOURCE_VIEW_DESC sv{DXGI_FORMAT_R32_FLOAT,D3D12_SRV_DIMENSION_TEXTURE2D,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};sv.Texture2D.MipLevels=1;
        gpu.device->CreateShaderResourceView(depth.Get(),&sv,heap.Cpu(v.depth));
        D3D12_UNORDERED_ACCESS_VIEW_DESC uv{DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_UAV_DIMENSION_TEXTURE2D};
        gpu.device->CreateUnorderedAccessView(traced.Get(),nullptr,&uv,heap.Cpu(v.traced));gpu.device->CreateUnorderedAccessView(classes.Get(),nullptr,&uv,heap.Cpu(v.classes));}
    RtTlas tlas;RtView view;
    {auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        D3D12_TEXTURE_COPY_LOCATION to{},from{};to.pResource=depth.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint.Footprint={DXGI_FORMAT_R32_FLOAT,width,height,1,256};
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={depth.Get(),0,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
        list->ResourceBarrier(1,&b);
        const auto scene=tlas.Build(gpu.device5.Get(),list.Get(),upload->GetGPUVirtualAddress()+2048,1);
        const auto copy_counts=[&](uint64_t at){D3D12_RESOURCE_BARRIER c{};c.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            c.Transition={counts.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&c);
            list->CopyBufferRegion(readback.Get(),at,counts.Get(),0,RtView::kCountBytes);
            std::swap(c.Transition.StateBefore,c.Transition.StateAfter);list->ResourceBarrier(1,&c);};
        view.Encode(gpu.device.Get(),list.Get(),heap,allocate,scene,0,0,v,counts.Get());copy_counts(0);
        // The classes picture (row pitch 256) before the second pass rewrites it.
        D3D12_RESOURCE_BARRIER p{};p.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;p.Transition={classes.Get(),0,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};
        list->ResourceBarrier(1,&p);
        D3D12_TEXTURE_COPY_LOCATION pic{},out{};pic.pResource=classes.Get();pic.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        out.pResource=readback.Get();out.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;out.PlacedFootprint={kPicture,{DXGI_FORMAT_R8G8B8A8_UNORM,width,height,1,256}};
        list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);std::swap(p.Transition.StateBefore,p.Transition.StateAfter);list->ResourceBarrier(1,&p);
        auto sun=v;sun.mask=RtMaskSun;view.Encode(gpu.device.Get(),list.Get(),heap,allocate,scene,0,0,sun,counts.Get());copy_counts(kSecond);
        (void)recording.Commit();}
    gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
    std::vector<uint8_t> bytes(kBytes);{void* read{};const D3D12_RANGE range{0,kBytes};if(FAILED(readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
        std::memcpy(bytes.data(),read,bytes.size());const D3D12_RANGE none{0,0};readback->Unmap(0,&none);}
    uint32_t w[16]{},s[16]{};std::memcpy(w,bytes.data(),64);std::memcpy(s,bytes.data()+kSecond,64);
    tlas.Close();view.Close();
    return {RtViewCountsOf(w),RtViewCountsOf(s),std::vector<uint8_t>(bytes.begin()+kPicture,bytes.begin()+kPicture+256*height)};
}
void View(DeviceQueues& gpu,const std::shared_ptr<RtModelBlas>& blas) {
    const auto r=RunView(gpu,blas,ViewCamera(),{2,2,2,2, 2.06f,2.06f,2.06f,2.06f, 3,3,3,3, 0,0,1,2});
    const auto& c=r.counts;const auto& m=r.masked;
    std::printf("rt view: agree %llu close %llu nearer %llu farther %llu missed %llu sky %llu extra %llu\n",static_cast<unsigned long long>(c.agree),static_cast<unsigned long long>(c.close),
        static_cast<unsigned long long>(c.nearer),static_cast<unsigned long long>(c.farther),static_cast<unsigned long long>(c.missed),static_cast<unsigned long long>(c.sky),static_cast<unsigned long long>(c.extra));
    Require(c.agree==5 && c.close==4 && c.nearer==4 && c.farther==1 && c.missed==0 && c.sky==0 && c.extra==2 && c.Surfaces()==14,"the classes of the view");
    Require(m.missed==14 && m.sky==2 && m.agree==0 && m.extra==0,"masked to the sun casters: no hit, missed and sky");
    // T5: the quad (0,0,0), (1,0,0), (1,1,0) is counter-clockwise from the camera before it (z = -2, looking along +z): the raster's
    // front, DXR's back face; none of its nearer pixels is counted clockwise.
    Require(c.nearer_cw==0,"the nearer pixels on a counter-clockwise quad: none clockwise");
    Require(c.alpha_met==0,"no material table: no disagreement met by an alpha test");
    const uint8_t* extra=r.classes.data()+256*3;const uint8_t* agree=r.classes.data();
    Require(extra[0]==255 && extra[1]==0 && extra[2]==255 && agree[0]==agree[1] && agree[1]==agree[2] && agree[0]>0,"the classes picture: an extra hit in magenta, an agreeing pixel grey");
    // M1: 8x4 (aspect 2: sx = sy / 2), the camera at (1.03125, 1, -2): a pixel is 0.125 wide at z = 0, the column centres at
    // x = 1.03125 + 0.125 (c - 3.5), the row centres at y = 1 - 0.125 (r - 1.5). The jitter -0.5 px (the raster drawn half a pixel
    // left) undone moves each ray 0.0625 right: column 3 (0.96875) leaves the quad, columns 0-2 stay; rows 2-3 below y = 1.
    auto edge=ViewCamera();edge.camera_pose[12]=1.03125f;edge.camera_pose[13]=1;edge.width=8;edge.jitter_x=-0.5f;
    const auto e=RunView(gpu,blas,edge,std::vector<float>(32,0.0f));
    Require(e.counts.extra==6 && e.counts.sky==26 && e.counts.Surfaces()==0,"the corner view: six hits over an empty raster");
    for(uint32_t y=0;y<4;++y)for(uint32_t x=0;x<8;++x){const bool hit=x<3 && y>=2;const uint8_t* p=e.classes.data()+256*y+4*x;
        if(hit!=(p[0]==255 && p[2]==255))throw std::runtime_error("the corner view: the hits where the lens, the axes and the jitter's sign put them");}
}

// I-0117 R5: the traced sun shadow (RtSunShadow) over a 4x4 view (ViewCamera: the quad at z = 0 covers it there) whose raster
// surface is at view z 3 (world z = 1, behind the quad) everywhere, with the G-buffer stencil bit 0x08 in rows 0-2 and without it
// in row 3 (traced, its columns 0-1 would read 0: the stencil test is what keeps them lit); pixel (3, 0) has the bit and no
// surface (depth 1). The sun toward (0.5, 0, -1): a ray from the surface meets z = 0 about 0.49 to the right, so columns 0 and 1
// (x 0.22, 0.41) are behind the quad and columns 2 and 3 (x 0.59, 0.78) lit. A second quad seen by the camera alone (RtMaskView),
// moved to x 1..2 and z -0.5, lies on the lit columns' rays (x + 0.75): it shadows nothing unless the rays ignore the sun's mask.
// The mask cleared to grey first: every pixel is written (the fill off the bit, the trace on it), xyz (0, 0, 1) everywhere.
// I-0140 tranche 35: the same trace with `neutral` 2 into a second target (xyz white on the bit, black off it); the pixel (1, 1)
// (beside the shadow's edge) then moved to view z 3.3; two passes of the original's blur (EncodeBlur): to a third target with
// `neutral` 2 at scale 1, then from it to a fourth, the last (xyz white), at scale 1.5 (the taps at floor(centre + tap * scale)).
// Each pass equal to the shader's formula computed here on its source's bytes: the centre at weight 1, the 12 taps clamped to
// the view, G = exp(-r^2 / 2) / sqrt(2 pi) on the taps the source flags, the depth weight
// exp(-100 dz^2 / z0^2): 1 between equal surfaces, between 0 and 1 against the moved pixel, 0 against the pixel without a surface.
void SunShadow(DeviceQueues& gpu,const std::shared_ptr<RtModelBlas>& blas) {
    const auto camera=ViewCamera();const uint32_t width=camera.width,height=camera.height;
    const float n=camera.near_plane,f=camera.far_plane,a=f/(f-n),surface=a-a*n/3.0f;
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,8192,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* mapped{};
    if(FAILED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))))throw std::runtime_error("test map");
    std::array<RtInstance,2> instances;instances[0].transform={1,0,0,0, 0,1,0,0, 0,0,1,0};instances[0].blas=blas->Structure();instances[0].id=4;instances[0].mask=RtMaskView|RtMaskSun;
    instances[1]=instances[0];instances[1].transform={1,0,0,1, 0,1,0,0, 0,0,1,-0.5f};instances[1].id=5;instances[1].mask=RtMaskView;
    RtFillInstanceDescs(std::span<const RtInstance>(instances),reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mapped));
    uint64_t used=1024;
    const auto allocate=[&](uint64_t size){const uint64_t at=(used+255)&~uint64_t(255);used=at+size;if(used>8192)throw std::runtime_error("test upload");
        UploadArena::Range r;r.cpu=mapped+at;r.gpu=upload->GetGPUVirtualAddress()+at;r.bytes=size;return r;};
    D3D12_HEAP_PROPERTIES heap_type{};heap_type.Type=D3D12_HEAP_TYPE_DEFAULT;
    const auto texture=[&](DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear){D3D12_RESOURCE_DESC d{};
        d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;d.Flags=flags;
        Microsoft::WRL::ComPtr<ID3D12Resource> r;
        if(FAILED(gpu.device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&d,state,clear,IID_PPV_ARGS(&r))))throw std::runtime_error("test texture");return r;};
    D3D12_CLEAR_VALUE depth_clear{};depth_clear.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;depth_clear.DepthStencil={1.0f,0};
    auto depth=texture(DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,D3D12_RESOURCE_STATE_DEPTH_WRITE,&depth_clear);
    auto mask=texture(DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);
    auto flagged=texture(DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);   // I-0140 t35
    auto mid=texture(DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);
    auto blurred=texture(DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,4096,D3D12_RESOURCE_STATE_COPY_DEST);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvs,rtvs;
    {D3D12_DESCRIPTOR_HEAP_DESC h{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,2,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        if(FAILED(gpu.device->CreateDescriptorHeap(&h,IID_PPV_ARGS(&dsvs))))throw std::runtime_error("test dsv heap");
        h={D3D12_DESCRIPTOR_HEAP_TYPE_RTV,4,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};if(FAILED(gpu.device->CreateDescriptorHeap(&h,IID_PPV_ARGS(&rtvs))))throw std::runtime_error("test rtv heap");}
    const auto dsv=dsvs->GetCPUDescriptorHandleForHeapStart();auto read_only=dsv;read_only.ptr+=gpu.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    {D3D12_DEPTH_STENCIL_VIEW_DESC d{};d.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;d.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;
        gpu.device->CreateDepthStencilView(depth.Get(),&d,dsv);d.Flags=D3D12_DSV_FLAG_READ_ONLY_DEPTH|D3D12_DSV_FLAG_READ_ONLY_STENCIL;gpu.device->CreateDepthStencilView(depth.Get(),&d,read_only);}
    const auto rtv=rtvs->GetCPUDescriptorHandleForHeapStart();gpu.device->CreateRenderTargetView(mask.Get(),nullptr,rtv);
    const UINT rtv_step=gpu.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto flagged_rtv=rtv;flagged_rtv.ptr+=rtv_step;auto blurred_rtv=flagged_rtv;blurred_rtv.ptr+=rtv_step;auto mid_rtv=blurred_rtv;mid_rtv.ptr+=rtv_step;
    gpu.device->CreateRenderTargetView(flagged.Get(),nullptr,flagged_rtv);gpu.device->CreateRenderTargetView(blurred.Get(),nullptr,blurred_rtv);
    gpu.device->CreateRenderTargetView(mid.Get(),nullptr,mid_rtv);
    BindlessHeap heap;if(FAILED(heap.Open(gpu.device.Get(),16)))throw std::runtime_error("test bindless heap");
    RtViewConstants v{};Require(RtViewCameraOf(camera,v),"the view's camera");
    RtSunConstants constants{};Require(RtSunConstantsOf(v,{0.5f,0,-1,0},constants),"the sun's constants");
    Require(std::abs(constants.sun[0]-0.5f/std::sqrt(1.25f))<1e-6f && constants.sun[3]==0 && constants.width==4,"the sun normalised, the extent kept");
    Require(!RtSunConstantsOf(v,{0,0,0,0},constants) && !RtSunConstantsOf(v,{std::numeric_limits<float>::quiet_NaN(),0,1,0},constants),"a zero or not finite sun refused");
    Require(RtSunConstantsOf(v,{0.5f,0,-1,0},constants),"the sun's constants again");
    constants.depth=heap.Allocate(gpu.CompletedValue());
    {D3D12_SHADER_RESOURCE_VIEW_DESC sv{DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,D3D12_SRV_DIMENSION_TEXTURE2D,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};sv.Texture2D.MipLevels=1;
        gpu.device->CreateShaderResourceView(depth.Get(),&sv,heap.Cpu(constants.depth));}
    const uint32_t flagged_view=heap.Allocate(gpu.CompletedValue()),mid_view=heap.Allocate(gpu.CompletedValue());
    {D3D12_SHADER_RESOURCE_VIEW_DESC sv{DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_SRV_DIMENSION_TEXTURE2D,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};sv.Texture2D.MipLevels=1;
        gpu.device->CreateShaderResourceView(flagged.Get(),&sv,heap.Cpu(flagged_view));gpu.device->CreateShaderResourceView(mid.Get(),&sv,heap.Cpu(mid_view));}
    RtTlas tlas;RtSunShadow sun;
    // The test's own clears (rects of other values than the resources' creation ones) are the debug layer's performance notes,
    // not defects of the pass: those two messages are kept out of the store for this test only.
    if(gpu.diagnostics){D3D12_MESSAGE_ID ids[2]{D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE};
        D3D12_INFO_QUEUE_FILTER filter{};filter.DenyList.NumIDs=2;filter.DenyList.pIDList=ids;gpu.diagnostics->PushStorageFilter(&filter);}
    struct PopFilter {DeviceQueues& gpu;~PopFilter(){if(gpu.diagnostics)gpu.diagnostics->PopStorageFilter();}} const pop{gpu};
    {auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        const D3D12_RECT surfaces{0,0,4,3},empty{3,0,4,1};
        list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL,surface,0,0,nullptr);
        list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_STENCIL,surface,0x08,1,&surfaces);
        list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,1.0f,0,1,&empty);
        const float grey[4]{0.5f,0.5f,0.5f,0.5f};list->ClearRenderTargetView(rtv,grey,0,nullptr);
        list->ClearRenderTargetView(flagged_rtv,grey,0,nullptr);list->ClearRenderTargetView(blurred_rtv,grey,0,nullptr);list->ClearRenderTargetView(mid_rtv,grey,0,nullptr);
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);
        const auto scene=tlas.Build(gpu.device5.Get(),list.Get(),upload->GetGPUVirtualAddress(),2);
        const D3D12_VIEWPORT viewport{0,0,float(width),float(height),0,1};const D3D12_RECT scissor{0,0,LONG(width),LONG(height)};
        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
        sun.Encode(gpu.device.Get(),list.Get(),heap,allocate,scene,0,0,constants,rtv,read_only,DXGI_FORMAT_R8G8B8A8_UNORM);
        // I-0140 t35: the flagged trace; then the pixel (1, 1) moved to view z 3.3 (the depth weight between 0 and 1: the step
        // review's L1) and two blur passes, flagged to mid (`neutral` 2, a further pass's source) and mid to blurred (the last).
        {auto second=constants;second.neutral=2;sun.Encode(gpu.device.Get(),list.Get(),heap,allocate,scene,0,0,second,flagged_rtv,read_only,DXGI_FORMAT_R8G8B8A8_UNORM);
            b.Transition={flagged.Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);
            b.Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_DEPTH_WRITE};list->ResourceBarrier(1,&b);
            const D3D12_RECT moved{1,1,2,2};list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,a-a*n/3.3f,0,1,&moved);
            b.Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);
            second.source=flagged_view;second.blur=1;sun.EncodeBlur(gpu.device.Get(),list.Get(),heap,allocate,scene,second,mid_rtv,read_only);
            b.Transition={mid.Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);
            second.neutral=1;second.source=mid_view;second.blur=1.5f;sun.EncodeBlur(gpu.device.Get(),list.Get(),heap,allocate,scene,second,blurred_rtv,read_only);
            b.Transition={flagged.Get(),0,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
            b.Transition={mid.Get(),0,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
            b.Transition={blurred.Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);}
        b.Transition={mask.Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
        D3D12_TEXTURE_COPY_LOCATION pic{},out{};pic.pResource=mask.Get();pic.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        out.pResource=readback.Get();out.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;out.PlacedFootprint={0,{DXGI_FORMAT_R8G8B8A8_UNORM,width,height,1,256}};
        list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);
        pic.pResource=flagged.Get();out.PlacedFootprint.Offset=1024;list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);
        pic.pResource=blurred.Get();out.PlacedFootprint.Offset=2048;list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);
        pic.pResource=mid.Get();out.PlacedFootprint.Offset=3072;list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);
        b.Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_DEPTH_WRITE};list->ResourceBarrier(1,&b);
        (void)recording.Commit();}
    gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
    std::vector<uint8_t> bytes(4096);{void* read{};const D3D12_RANGE range{0,4096};if(FAILED(readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
        std::memcpy(bytes.data(),read,bytes.size());const D3D12_RANGE none{0,0};readback->Unmap(0,&none);}
    tlas.Close();sun.Close();
    std::string seen;
    for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){const uint8_t* p=bytes.data()+256*y+4*x;seen+=p[3]==0?'0':p[3]==255?'1':'?';
        if(p[0]!=0 || p[1]!=0 || p[2]!=255)throw std::runtime_error("the traced sun shadow: xyz the no-shadow packed depth (0, 0, 1) everywhere");}
    std::printf("rt sun shadow: %s\n",seen.c_str());
    Require(seen=="0011" "0011" "0011" "1111","the traced sun shadow: columns 0-1 behind the quad, 2-3 lit (the camera's quad ignored), the pixel without a surface and the row off the G-buffer lit");
    Require(!RtSunConstantsOf(v,{0,0,0,0},constants),"the sun refused once more after the run");
    // I-0140 t35: the flagged trace (the same w, xyz white on the bit's rows 0-2, black on row 3), then each blur pass against the
    // formula applied to the bytes its source holds: the view depth 3 on the surfaces, 3.3 at (1, 1), the far plane at (3, 0).
    for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){const uint8_t* p=bytes.data()+1024+256*y+4*x;const bool bit=y<3;
        if(p[3]!=bytes[256*y+4*x+3])throw std::runtime_error("the flagged trace: the same shadow term as the first");
        if(bit?(p[0]!=255 || p[1]!=255 || p[2]!=255):(p[0]!=0 || p[1]!=0 || p[2]!=0))throw std::runtime_error("the flagged trace: xyz white on the G-buffer's pixels, black off it");}
    std::array<float,16> z{};z.fill(3.0f);z[4*1+1]=3.3f;z[4*0+3]=f;
    static const int taps[12][2]{{0,-2},{-1,-1},{0,-1},{1,-1},{-2,0},{-1,0},{1,0},{2,0},{-1,1},{0,1},{1,1},{0,2}};
    bool between=false;   // a tap whose depth weight is neither about 0 nor 1 met
    const auto pass=[&](size_t source,size_t target,bool last,float scale,const char* what){std::string seen_pass;
        for(int y=0;y<int(height);++y)for(int x=0;x<int(width);++x){const uint8_t* p=bytes.data()+target+256*y+4*x;const bool bit=y<3;
            const uint8_t flag=last || bit?255:0;
            if(p[0]!=flag || p[1]!=flag || p[2]!=flag)throw std::runtime_error(std::string(what)+": xyz white (the last pass everywhere, a further one's source on the G-buffer), else black");
            const auto term=[&](int qx,int qy){return bytes[source+256*qy+4*qx+3]/255.0f;};
            float expected=1;
            if(bit){const float z0=z[4*y+x];float weights=1,sum=term(x,y);
                for(const auto& t:taps){const int qx=std::clamp(int(std::floor(float(x)+0.5f+float(t[0])*scale)),0,int(width)-1),qy=std::clamp(int(std::floor(float(y)+0.5f+float(t[1])*scale)),0,int(height)-1);
                    if(qy>=3)continue;   // the source's xyz black off the G-buffer: weight 0
                    const float dz=z[4*qy+qx]-z0,depth_weight=std::exp(-100*dz*dz/(z0*z0));between|=depth_weight>0.2f && depth_weight<0.8f;
                    const float wq=std::exp(-0.5f*float(t[0]*t[0]+t[1]*t[1]))*0.39894228f*depth_weight;weights+=wq;sum+=wq*term(qx,qy);}
                expected=sum/weights;}
            const int want=int(std::lround(expected*255.0f));seen_pass+=std::to_string(p[3])+(x+1<int(width)?",":y+1<int(height)?" / ":"");
            if(std::abs(int(p[3])-want)>1)throw std::runtime_error(std::string(what)+": its term off the formula at a pixel by more than one step");}
        std::printf("rt sun shadow %s: %s\n",what,seen_pass.c_str());};
    pass(1024,3072,false,1.0f,"blur pass 1");pass(3072,2048,true,1.5f,"blur pass 2");
    Require(between,"the blur test meets a depth weight between 0 and 1");
    Require(bytes[2048+3]>0 && bytes[2048+3]<255,"the blur: a shadowed pixel beside lit ones softened");
}

// I-0122 R4a: the records of the one list. A point keeps its world position and its row's colours and 1 / R²; a spot's world rows
// give the light-space values its view rows give at the same point (the camera turned and moved); a value not finite refused.
void LightRecords() {
    std::array<float,16> point{};for(size_t i=0;i<16;++i)point[i]=float(i)+0.5f;
    RtLight l{};Require(RtPointLightOf(point,{1,2,3},true,l),"a point's record");
    Require(l.position==std::array<float,4>{1,2,3,3.5f} && l.diffuse==std::array<float,4>{4.5f,5.5f,6.5f,7.5f} && l.specular[0]==8.5f && l.specular[2]==10.5f &&
        l.trans[0]==12.5f && l.trans[1]==13.5f && l.kind==kRtLightLegacy,"the point's world position, colours, 1 / R², translucency and kind");
    point[5]=std::numeric_limits<float>::infinity();Require(!RtPointLightOf(point,{1,2,3},false,l),"a point not finite refused");
    // I-0132 R11h: a point in physical terms: DS3's torch (0.12 grey, R = 2 m) reaches sqrt(0.12 / e_min), its colours times the ratio,
    // the physical bit, DS3's R as a half float in the kind's upper bits (2.0 = 0x4000); a legacy point x R^2 / 16; a zero light refused.
    {std::array<float,16> torch{};torch[3]=0.25f;torch[4]=torch[5]=torch[6]=0.12f;torch[8]=torch[9]=torch[10]=0.06f;
        RtLight t{};Require(RtPointLightOf(torch,{0,0,0},false,t) && RtPhysicalLight(t,0.02f,kRtLocalPhysicalMinimum),"a torch in physical terms");
        const float reach=1.f/std::sqrt(t.position[3]);
        Require(std::abs(reach-std::sqrt(0.12f/kRtLocalPhysicalMinimum))<1e-3f && std::abs(t.diffuse[0]-0.0024f)<1e-6f && (t.kind&0xFFFFu)==(kRtLightPoint|kRtLightPhysical) &&
            (t.kind>>16)==0x4000u,"its reach, its colour, its bits and DS3's range kept");
        RtLight legacy{};Require(RtPointLightOf(torch,{0,0,0},true,legacy) && RtPhysicalLight(legacy,1.f,kRtLocalPhysicalMinimum) && std::abs(legacy.diffuse[0]-0.12f*4.f/16.f)<1e-6f,
            "a legacy point matched at half its range (x R^2 / 16)");
        torch[4]=torch[5]=torch[6]=0;torch[8]=torch[9]=torch[10]=0;RtLight dark{};
        Require(RtPointLightOf(torch,{0,0,0},false,dark) && !RtPhysicalLight(dark,0.02f,kRtLocalPhysicalMinimum) && !(dark.kind&kRtLightPhysical),"a light without colour refused, unchanged");}
    // The pose: a turn of 30 degrees about y, then a move; its inverse the view (row vectors).
    const float c=std::cos(0.5235988f),s=std::sin(0.5235988f);
    const float pose[16]{c,0,-s,0, 0,1,0,0, s,0,c,0, 3,-1,7,1};
    float view[16]{c,0,s,0, 0,1,0,0, -s,0,c,0, 0,0,0,1};
    for(int k=0;k<3;++k)view[12+k]=-(pose[12]*view[k]+pose[13]*view[4+k]+pose[14]*view[8+k]);
    std::array<float,32> spot{};spot[0]=0.25f;spot[1]=-2;spot[2]=5;spot[3]=12;for(int i=4;i<15;++i)spot[i]=float(i);spot[15]=1;
    for(int i=0;i<16;++i)spot[16+i]=0.1f*float((i*7)%11)-0.4f;
    Require(RtSpotLightOf(spot,pose,view,l),"a spot's record");
    const auto to_view=[&](const float* w,float* v){for(int k=0;k<3;++k)v[k]=w[0]*view[k]+w[1]*view[4+k]+w[2]*view[8+k]+view[12+k];};
    {float v[3]{};to_view(l.position.data(),v);Require(std::abs(v[0]-0.25f)<1e-4f && std::abs(v[1]+2)<1e-4f && std::abs(v[2]-5)<1e-4f && l.position[3]==12,"the spot's position back in view space");}
    const float world[3]{-4,2.5f,9};float at_view[3]{};to_view(world,at_view);
    for(int i=0;i<4;++i){const float* r=&spot[16+4*i];const float* w=&l.to_light[4*i];
        const float from_view=r[0]*at_view[0]+r[1]*at_view[1]+r[2]*at_view[2]+r[3],from_world=w[0]*world[0]+w[1]*world[1]+w[2]*world[2]+w[3];
        Require(std::abs(from_view-from_world)<1e-4f,"the spot's light-space value from the world rows");}
    Require(l.kind==kRtLightSpot && l.trans[3]==1 && l.diffuse[3]==7,"the spot's kind, ShadingMode and gloss clamp");
    spot[20]=std::numeric_limits<float>::quiet_NaN();Require(!RtSpotLightOf(spot,pose,view,l),"a spot not finite refused");
}
// I-0128 R8c: the light grid. Whether any of RtLocalLight's falloffs (rt_lights.hlsli) is non-zero at `p`: the point's, the legacy
// point's, the spot's in its ShadingMode and without it (its cone, its range, nothing where q.z <= 0), as the shader computes them.
bool LightReaches(const RtLight& l,const float p[3]) {
    const float to[3]{l.position[0]-p[0],l.position[1]-p[1],l.position[2]-p[2]};const float d2=to[0]*to[0]+to[1]*to[1]+to[2]*to[2];
    const auto sat=[](float v){return std::clamp(v,0.f,1.f);};
    const uint32_t kind=l.kind&0xFFu;
    if(kind==kRtLightSpot){float q[4];for(int i=0;i<4;++i){const float* r=&l.to_light[4*i];q[i]=r[0]*p[0]+r[1]*p[1]+r[2]*p[2]+r[3];}
        if(!(q[2]>0))return false;
        const float qx=q[0]/q[3],qy=q[1]/q[3];const float cone=std::max(1-(qx*qx+qy*qy),0.f);const float range=l.position[3];
        if(l.trans[3]>0){const float inv=1/(range*range);return std::max(1/d2-inv,0.f)*cone>0 || (1-std::min(d2*inv,1.f))*cone>0;}
        const float fall=sat(1-q[3]/range);return fall*fall*cone*cone>0;}
    if(kind==kRtLightLegacy){const float f=1-std::sqrt(sat(d2*l.position[3]));return f*f>0;}
    return std::max(1/d2-l.position[3],0.f)>0 || 1-sat(d2*l.position[3])>0;
}
// A spot at `at` along the unit `forward` (`right` and `up` completing its basis), its cone's tangent `t`, range `range`, its near
// plane `front` (q.z = 3 (q.w − front)); `oblique` shears q.x by q.w (an off-centre cone); `mode` its ShadingMode.
RtLight TestSpot(const float at[3],const float right[3],const float up[3],const float forward[3],float t,float range,float front,float oblique,bool mode) {
    RtLight l{};l.kind=kRtLightSpot;for(int k=0;k<3;++k)l.position[k]=at[k];l.position[3]=range;l.diffuse={1,1,1,1};l.trans={0,1,0,mode?1.f:0.f};
    const auto row=[&](float* r,const float* axis,float scale,float shear){float w3=0;
        for(int k=0;k<3;++k){r[k]=axis[k]*scale+forward[k]*shear;w3-=r[k]*at[k];}r[3]=w3;};
    row(&l.to_light[0],right,1/t,oblique);row(&l.to_light[4],up,1/t,0);row(&l.to_light[8],forward,3.f,0);l.to_light[11]-=3.f*front;row(&l.to_light[12],forward,1,0);
    return l;
}
// Lights of every kind about a camera (some across the grid's faces, one out of it, the collect's inert record; four spots unbounded:
// q.z not along q.w, its zero behind the apex, a z row tilted past the cone, x and y rows parallel), at two cameras (the second over
// 1,000 m from the origin: the margins against the floats' rounding there): every light whose falloff is non-zero at a point is in
// that point's cell, for points drawn about each light, across the grid, and just inside each support's boundary (the step review's
// L6: at R (1 − 10⁻⁶), along a spot's cone's edge and its range's face); the cells in the list's order; no cell outside the grid; the
// figures; a centred spot's reach; the entry budget.
void LightGrid() {
    uint32_t state=12345u;const auto random=[&]{state=state*1664525u+1013904223u;return float(state>>8)/16777216.f;};
    const float x[3]{1,0,0},y[3]{0,1,0},z[3]{0,0,1};
    for(const std::array<float,3> eye:{std::array<float,3>{103.5f,-41.2f,587.9f},std::array<float,3>{1003.5f,-41.2f,-1412.1f}}){
    std::vector<RtLight> lights;
    for(int i=0;i<60;++i){RtLight l{};l.kind=i%3==0?kRtLightLegacy:kRtLightPoint;const float r=0.05f+30*random()*random();
        l.position={eye[0]+(random()-0.5f)*240,eye[1]+(random()-0.5f)*110,eye[2]+(random()-0.5f)*240,1/(r*r)};l.diffuse={1,1,1,1};lights.push_back(l);}
    for(int i=0;i<60;++i){const float at[3]{eye[0]+(random()-0.5f)*240,eye[1]+(random()-0.5f)*110,eye[2]+(random()-0.5f)*240};
        float f[3]{random()-0.5f,random()-0.5f,random()-0.5f};const float n=std::sqrt(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);for(float& v:f)v/=n;
        const float h[3]{std::abs(f[1])<0.9f?0.f:1.f,std::abs(f[1])<0.9f?1.f:0.f,0};
        float r[3]{h[1]*f[2]-h[2]*f[1],h[2]*f[0]-h[0]*f[2],h[0]*f[1]-h[1]*f[0]};const float rn=std::sqrt(r[0]*r[0]+r[1]*r[1]+r[2]*r[2]);for(float& v:r)v/=rn;
        const float u[3]{f[1]*r[2]-f[2]*r[1],f[2]*r[0]-f[0]*r[2],f[0]*r[1]-f[1]*r[0]};
        RtLight spot=TestSpot(at,r,u,f,0.2f+2*random(),0.5f+25*random(),0.05f*random(),i%4==1?random()-0.5f:0.f,i%5==0);
        if(i%7==3)for(int k=0;k<3;++k){spot.to_light[8+k]+=0.01f*spot.to_light[k];spot.to_light[11]-=0.01f*spot.to_light[k]*at[k];}   // q.z leaning on q.x, still bounded
        lights.push_back(spot);}
    {RtLight distant{};distant.kind=kRtLightPoint;distant.position={eye[0]+900,eye[1],eye[2],1/25.f};distant.diffuse={1,1,1,1};lights.push_back(distant);}   // out of the grid
    const float at[3]{eye[0],eye[1],eye[2]};
    {RtLight skew=TestSpot(at,x,y,z,1,10,0.1f,0,false);for(int k=0;k<3;++k)skew.to_light[8+k]=x[k];lights.push_back(skew);}   // q.z not along q.w: every cell
    lights.push_back(TestSpot(at,x,y,z,1,10,-1,0,false));   // q.z's zero behind the apex: every cell
    {RtLight tilt=TestSpot(at,x,y,z,1,10,0.1f,0,false);for(int k=0;k<3;++k)tilt.to_light[8+k]+=4*x[k];lights.push_back(tilt);}   // q.z = 3 q.w + 4 q.x: lit behind, every cell
    {RtLight flat=TestSpot(at,x,y,z,1,10,0.1f,0,false);for(int k=0;k<4;++k)flat.to_light[4+k]=flat.to_light[k];lights.push_back(flat);}   // x and y rows the same: every cell
    {RtLight inert{};inert.position={0,0,0,1e30f};inert.trans={0,1,0,0};lights.push_back(inert);}   // the collect's inert record
    // The fixes' step review L2: q.z's zero behind the apex by half the accepted depth (bounded, its backward sliver lit) and by twice it
    // (every cell).
    const uint32_t sliver=uint32_t(lights.size());lights.push_back(TestSpot(at,x,y,z,1,10,-0.5e-4f*10,0,false));
    lights.push_back(TestSpot(at,x,y,z,1,10,-2e-4f*10,0,false));
    std::vector<uint32_t> words;RtLightGridStats stats{};BuildRtLightGrid(lights,eye,words,stats);
    // I-0132 R11b (its step review's L4): the scratch kept between calls on one thread changes nothing: the same list after a larger one
    // and after one past the budget gives the same words.
    {std::vector<RtLight> larger=lights;larger.insert(larger.end(),lights.begin(),lights.end());std::vector<uint32_t> other,again;RtLightGridStats s2{};
        BuildRtLightGrid(larger,eye,other,s2);const float origin0[3]{0,0,0};RtLight flat=TestSpot(origin0,x,y,z,1,10,0.1f,0,false);for(int k=0;k<4;++k)flat.to_light[4+k]=flat.to_light[k];
        std::vector<RtLight> many(65,flat);BuildRtLightGrid(many,{0,0,0},other,s2);
        Require(s2.dropped && other.empty(),"light grid: the kept scratch's test passes the budget between the two builds");BuildRtLightGrid(lights,eye,again,s2);
        Require(again==words,"light grid: the same words after a larger list and one past the budget (the kept scratch)");}
    Require(!stats.dropped && words.size()==kRtLightGridHeader+2ull*kRtLightGridCells+stats.entries && words[4]==kRtLightGridX && words[5]==kRtLightGridY &&
        words[6]==kRtLightGridZ && words[7]==kRtLightGridCells,"light grid: its header and size");
    std::array<float,3> origin{};for(int a=0;a<3;++a)std::memcpy(&origin[a],&words[a],4);
    {float cell{};std::memcpy(&cell,&words[3],4);Require(cell==kRtLightGridCell,"light grid: the cell's size in the header");}
    for(int a=0;a<3;++a)Require(origin[a]==(std::floor(eye[a]/8)-float(a==1?8:16))*8,"light grid: the origin snapped about the camera");
    Require(RtLightReach(lights[sliver])>0 && stats.everywhere==5 && stats.outside>=1 && stats.filled>0 && stats.longest<=lights.size(),"light grid: five lights unbounded, the sliver's bounded, the far one out of the grid");
    for(uint32_t c=0;c<kRtLightGridCells;++c){const uint32_t first=words[kRtLightGridHeader+2*c],count=words[kRtLightGridHeader+2*c+1];
        for(uint32_t j=1;j<count;++j)Require(words[first+j-1]<words[first+j],"light grid: a cell's lights in the list's order");}
    const auto listed=[&](uint32_t c,uint32_t light){const uint32_t first=words[kRtLightGridHeader+2*c],count=words[kRtLightGridHeader+2*c+1];
        return std::binary_search(words.begin()+first,words.begin()+first+count,light);};
    size_t checked=0,reached=0,edges=0;size_t kinds[4]{};   // the boundary points reached by kind: point, legacy, spot in its mode, cone
    const auto check=[&](uint32_t i,const float p[3]){uint32_t c{};if(!RtLightGridCellOf(origin,p,c))return;++checked;
        if(LightReaches(lights[i],p)){++reached;Require(listed(c,i),"light grid: a light of non-zero falloff in the point's cell");}};
    for(uint32_t i=0;i<lights.size();++i){const RtLight& l=lights[i];
        const float reach=RtLightReach(l);const float span=reach>0 && reach<100?reach*1.2f:20.f;
        for(int k=0;k<4000;++k){float p[3];
            if(k<3000)for(int a=0;a<3;++a)p[a]=l.position[a]+(random()*2-1)*span;
            else for(int a=0;a<3;++a)p[a]=origin[a]+random()*float(a==1?128:256);
            check(i,p);}
        // Just inside the support's boundary.
        const bool cone=(l.kind&0xFFu)==kRtLightSpot && !(l.trans[3]>0.f);
        // The fixes' step review L3: inset by a millimetre (several of a float's steps at 1,000 m, a tenth of the absolute margin): no
        // light left out at its boundary (not a proof that the margins are needed: those points lie inside the unmargined sphere too).
        const uint32_t kind=(l.kind&0xFFu)==kRtLightSpot?(cone?3u:2u):(l.kind&0xFFu)==kRtLightLegacy?1u:0u;
        for(int k=0;k<400;++k){double p[3];
            if(!cone){const double radius=(l.kind&0xFFu)==kRtLightSpot?l.position[3]:1/std::sqrt(double(l.position[3]));
                double d[3]{random()-0.5,random()-0.5,random()-0.5};const double n=std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
                for(int a=0;a<3;++a)p[a]=l.position[a]+d[a]/n*std::max(radius-1e-3,0.0);}
            else{// q on the cone's edge (|q.xy / q.w| = s) at a depth below R, or on the range's face (q.w = R s) inside the cone.
                const double s=1-1e-3/std::max(double(l.position[3]),1e-3);
                const double angle=6.2831853*random(),depth=k%2?l.position[3]*s:l.position[3]*random(),spread=k%2?s*std::sqrt(random()):s;
                const double q[3]{std::cos(angle)*spread*depth,std::sin(angle)*spread*depth,depth};
                const float* rx=&l.to_light[0];const float* ry=&l.to_light[4];const float* rw=&l.to_light[12];
                const double m[3][3]{{rx[0],rx[1],rx[2]},{ry[0],ry[1],ry[2]},{rw[0],rw[1],rw[2]}},t[3]{q[0]-rx[3],q[1]-ry[3],q[2]-rw[3]};
                const double det=m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])-m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])+m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
                if(!(std::abs(det)>1e-9))break;
                for(int a=0;a<3;++a){double mm[3][3];for(int r=0;r<3;++r)for(int cc=0;cc<3;++cc)mm[r][cc]=cc==a?t[r]:m[r][cc];
                    p[a]=(mm[0][0]*(mm[1][1]*mm[2][2]-mm[1][2]*mm[2][1])-mm[0][1]*(mm[1][0]*mm[2][2]-mm[1][2]*mm[2][0])+mm[0][2]*(mm[1][0]*mm[2][1]-mm[1][1]*mm[2][0]))/det;}}
            const float pf[3]{float(p[0]),float(p[1]),float(p[2])};const size_t before=reached;check(i,pf);edges+=reached>before;kinds[kind]+=reached>before;}}
    // The sliver's lit points (−s₀ < q.w < 0, |q.xy| < |q.w|: q.w = −t s₀, q.xy within it), listed in their cells.
    {const RtLight& l=lights[sliver];size_t lit=0;
        for(int k=0;k<200;++k){const double qw=-0.5e-4*10*(0.05+0.9*random()),qx=qw*(random()*1.6-0.8),qy=qw*(random()*1.6-0.8);
            const float p[3]{float(l.position[0]+qx),float(l.position[1]+qy),float(l.position[2]+qw)};   // the test spot's rows: x, y, z the world's
            const size_t before=reached;check(sliver,p);lit+=reached>before;}
        Require(lit>50,"light grid: the sliver behind the apex lit and listed");}
    Require(reached>10000 && checked>reached && kinds[0]>500 && kinds[1]>500 && kinds[2]>200 && kinds[3]>500,"light grid: enough points reached, at every kind's boundary too");
    float out[3]{origin[0]-0.01f,origin[1]+1,origin[2]+1};uint32_t c{};Require(!RtLightGridCellOf(origin,out,c),"light grid: no cell outside it");
    out[0]=origin[0]+256;Require(!RtLightGridCellOf(origin,out,c),"light grid: no cell past its far face");
    out[0]=std::numeric_limits<float>::quiet_NaN();Require(!RtLightGridCellOf(origin,out,c),"light grid: no cell for a value not finite");
    std::printf("rt light grid at (%.0f, %.0f, %.0f): %zu lights, %u cells filled, the longest %u, %llu entries, %zu points of %zu reached, %zu at the boundaries (%zu, %zu, %zu, %zu by kind)\n",
        eye[0],eye[1],eye[2],lights.size(),stats.filled,stats.longest,static_cast<unsigned long long>(stats.entries),reached,checked,edges,kinds[0],kinds[1],kinds[2],kinds[3]);}
    // A centred spot's reach: its pyramid's corners' sphere, R sqrt(1 + 2 t²) (|w| 1), with the margins.
    {const float at[3]{0,0,0};const RtLight spot=TestSpot(at,x,y,z,0.5f,10,0.1f,0,false);const float expect=10*std::sqrt(1+2*0.25f)*kRtLightGridMarginShare+kRtLightGridMarginMetres;
        Require(std::abs(RtLightReach(spot)-expect)<1e-3f,"light grid: a spot's reach its pyramid's corners");}
    // The budget: 65 unbounded lights' indices are more than it: no grid (the whole list), counted.
    {const float at[3]{0,0,0};RtLight flat=TestSpot(at,x,y,z,1,10,0.1f,0,false);for(int k=0;k<4;++k)flat.to_light[4+k]=flat.to_light[k];
        std::vector<RtLight> many(65,flat);std::vector<uint32_t> words;RtLightGridStats stats{};BuildRtLightGrid(many,{0,0,0},words,stats);
        Require(stats.dropped && words.empty() && stats.entries==65ull*kRtLightGridCells,"light grid: past its budget, none");
        many.pop_back();BuildRtLightGrid(many,{0,0,0},words,stats);   // the fixes' step review L5: at the budget, kept
        Require(!stats.dropped && stats.entries==kRtLightGridEntriesMost && words.size()==kRtLightGridHeader+2ull*kRtLightGridCells+kRtLightGridEntriesMost,"light grid: at its budget, kept");}
}
// R11G11B10_FLOAT's channel of `bits` (5 exponent bits, `mantissa` bits).
float SmallFloat(uint32_t bits,uint32_t mantissa) {
    const uint32_t m=bits&((1u<<mantissa)-1),e=bits>>mantissa;
    return e?std::ldexp(1.0f+float(m)/float(1u<<mantissa),int(e)-15):std::ldexp(float(m)/float(1u<<mantissa),-14);}
std::array<float,3> R11G11B10(uint32_t w) {return {SmallFloat(w&0x7FF,6),SmallFloat((w>>11)&0x7FF,6),SmallFloat(w>>22,5)};}
// I-0122 R4a: the native local lights' pass on the hardware device. The view camera (ViewCamera) over a plane at view z 3 (world
// z 1) facing it, one pixel without a surface; GB0 the plane's normal, GB1 gloss 0.2 and no translucency, GB3 sqrt F0 0.5, t33 white;
// a point light 1 m in front of the plane: each pixel's diffuse and specular against the point programme's terms (R4-PLAN §1)
// computed here, within R11G11B10's precision, from targets cleared to zero but at two pixels cleared to 0.25: the one without a
// surface left as it was, the first pixel's terms added to it (the programmes' ONE/ONE blend). R4b: the tile holds the light, the
// same light shadowed (strength 0.8), a shadowed light behind the plane (world z 2), the light shadowed with strength 0 and the light
// unshadowed with a strength word (the flag, not the strength, says shadowed). A local caster (the instance's quad at z 0.5, x
// 0.5..1.5; its triangle at z 1.5) lies on the rays of columns 2 and 3 toward both lights; a quad the camera and the sun alone see
// (x -0.5..0.5) on those of columns 0 and 1 toward the front light; the plane itself a local caster (final review M1: a ray toward the
// light behind must leave from its far side). Row 1 is translucent (GB1.y 0.2): the light behind lights it through the wrapped
// diffuse and the translucency, which its visibility leaves alone. Traced (the first pair): v 1 on columns 0 and 1, 0.2 on columns 2
// and 3 for the shadowed lights of strength 0.8; without a top level (the second pair) the shadowed lights shaded unshadowed (I-0131 R10a: no atlas, no batch).
// Then the near planes (a second top level: local quads at z 0.2, x -0.5..0.5, and z 0.5, x 0.5..1.5): a shadowed point
// of strength 1 whose cube's near plane is 0.3 from it, beside its unshadowed copy, traced (the third pair: the quad within the near
// cube ignored on columns 0 and 1, the other hit on 2 and 3); a spot whose near plane (q.z = 0) is at z 0.3, the same two ways,
// traced (the fourth pair) and without a top level (the fifth: shaded unshadowed, final review L5), compared by ratio.
void LocalLights(DeviceQueues& gpu,const std::shared_ptr<RtModelBlas>& blas) {
    const auto camera=ViewCamera();const uint32_t width=camera.width,height=camera.height;
    const float n=camera.near_plane,f=camera.far_plane,a=f/(f-n),surface=a-a*n/3.0f;
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,8192,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* mapped{};
    if(FAILED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))))throw std::runtime_error("test map");
    std::array<RtInstance,3> instances;instances[0].transform={1,0,0,0.5f, 0,1,0,0, 0,0,1,0.5f};instances[0].blas=blas->Structure();instances[0].id=6;instances[0].mask=RtMaskLocal;
    instances[1]=instances[0];instances[1].transform={1,0,0,-0.5f, 0,1,0,0, 0,0,1,0.5f};instances[1].id=7;instances[1].mask=RtMaskView|RtMaskSun;
    instances[2]=instances[0];instances[2].transform={1,0,0,0, 0,1,0,0, 0,0,1,1};instances[2].id=8;   // the plane (its triangle at z 2, beyond the light behind)
    RtFillInstanceDescs(std::span<const RtInstance>(instances),reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mapped));
    std::array<RtInstance,2> cuts;cuts[0]=instances[0];cuts[1]=instances[0];cuts[1].transform={1,0,0,-0.5f, 0,1,0,0, 0,0,1,0.2f};cuts[1].id=9;
    RtFillInstanceDescs(std::span<const RtInstance>(cuts),reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mapped+512));
    uint64_t used=1024;
    const auto allocate=[&](uint64_t size){const uint64_t at=(used+255)&~uint64_t(255);used=at+size;if(used>8192)throw std::runtime_error("test upload");
        UploadArena::Range r;r.cpu=mapped+at;r.gpu=upload->GetGPUVirtualAddress()+at;r.bytes=size;return r;};
    std::array<RtLight,5> light{};const std::array<float,16> row{0,0,0,0.01f, 1,0.5f,0.25f,100, 0.5f,1,1,0, 0.5f,0.25f,0,0};
    Require(RtPointLightOf(row,{0.5f,0.5f,0},false,light[0]),"the test light");
    // ProjParam {1.001001, -0.01001, 0, 0.8} (the bonfires' words, runs i122-r4a): near 0.01, strength 0.8.
    std::array<float,32> shadow{};shadow[0]=1.001001f;shadow[1]=-0.01001f;shadow[3]=0.8f;
    light[1]=light[0];Require(RtShadowedOf(shadow,light[1]) && std::abs(light[1].trans[3]-0.01f)<1e-6f,"the shadowed light's record");
    light[2]=light[1];light[2].position[2]=2;
    shadow[3]=0;light[3]=light[0];Require(RtShadowedOf(shadow,light[3]),"strength 0, still shadowed");
    light[4]=light[0];light[4].trans[2]=0.5f;   // a strength word without the flag: unshadowed
    // The near planes: a point of strength 1 whose cube's near plane is 0.3 from it, its unshadowed copy; a spot along +z from the
    // light (q = (x - 0.5, y - 0.5, z - 0.3, z): its near plane at z 0.3; range 10, ShadingMode 1), its shadowed copy.
    std::array<RtLight,2> point_cut{light[0],light[0]};shadow[0]=1;shadow[1]=-0.3f;shadow[3]=1;Require(RtShadowedOf(shadow,point_cut[1]),"the near point");
    std::array<RtLight,2> spot_cut{};spot_cut[0].kind=kRtLightSpot;spot_cut[0].position={0.5f,0.5f,0,10};spot_cut[0].diffuse={1,0.5f,0.25f,100};spot_cut[0].specular={0.5f,1,1};
    spot_cut[0].trans={0.5f,0.25f,0,1};   // TransLightParam as the points' (y 0 gives the translucency 0^0: NaN)
    spot_cut[0].to_light={1,0,0,-0.5f, 0,1,0,-0.5f, 0,0,1,-0.3f, 0,0,1,0};
    spot_cut[1]=spot_cut[0];shadow[0]=1;Require(RtShadowedOf(shadow,spot_cut[1]) && spot_cut[1].trans[2]==1 && spot_cut[1].trans[3]==1,"the spot's strength, its ShadingMode kept");
    const auto lights=allocate(sizeof light);std::memcpy(lights.cpu,light.data(),sizeof light);
    const auto point_lights=allocate(sizeof point_cut);std::memcpy(point_lights.cpu,point_cut.data(),sizeof point_cut);
    const auto spot_lights=allocate(sizeof spot_cut);std::memcpy(spot_lights.cpu,spot_cut.data(),sizeof spot_cut);
    const uint32_t tile_words[7]{2,5,0,1,2,3,4};const auto tiles=allocate(sizeof tile_words);std::memcpy(tiles.cpu,tile_words,sizeof tile_words);
    const uint32_t pair_words[4]{2,2,0,1};const auto pair_tiles=allocate(sizeof pair_words);std::memcpy(pair_tiles.cpu,pair_words,sizeof pair_words);
    D3D12_HEAP_PROPERTIES heap_type{};heap_type.Type=D3D12_HEAP_TYPE_DEFAULT;
    const auto texture=[&](DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear){D3D12_RESOURCE_DESC d{};
        d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;d.Flags=flags;
        Microsoft::WRL::ComPtr<ID3D12Resource> r;
        if(FAILED(gpu.device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&d,state,clear,IID_PPV_ARGS(&r))))throw std::runtime_error("test texture");return r;};
    D3D12_CLEAR_VALUE depth_clear{};depth_clear.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;depth_clear.DepthStencil={1.0f,0};
    auto depth=texture(DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,D3D12_RESOURCE_STATE_DEPTH_WRITE,&depth_clear);
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,4> gbuffer;   // GB0, GB1, GB3, t33
    for(auto& g:gbuffer)g=texture(DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,10> pair;   // the five pairs above, each diffuse then specular
    for(auto& t:pair)t=texture(DXGI_FORMAT_R11G11B10_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,10240,D3D12_RESOURCE_STATE_COPY_DEST);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvs,rtvs;
    {D3D12_DESCRIPTOR_HEAP_DESC h{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        if(FAILED(gpu.device->CreateDescriptorHeap(&h,IID_PPV_ARGS(&dsvs))))throw std::runtime_error("test dsv heap");
        h={D3D12_DESCRIPTOR_HEAP_TYPE_RTV,14,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};if(FAILED(gpu.device->CreateDescriptorHeap(&h,IID_PPV_ARGS(&rtvs))))throw std::runtime_error("test rtv heap");}
    const auto dsv=dsvs->GetCPUDescriptorHandleForHeapStart();
    {D3D12_DEPTH_STENCIL_VIEW_DESC d{};d.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;d.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;gpu.device->CreateDepthStencilView(depth.Get(),&d,dsv);}
    const auto rtv_step=gpu.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    const auto rtv=[&](uint32_t i){auto h=rtvs->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*rtv_step;return h;};
    for(uint32_t i=0;i<4;++i)gpu.device->CreateRenderTargetView(gbuffer[i].Get(),nullptr,rtv(i));
    for(uint32_t i=0;i<10;++i)gpu.device->CreateRenderTargetView(pair[i].Get(),nullptr,rtv(4+i));
    BindlessHeap heap;if(FAILED(heap.Open(gpu.device.Get(),16)))throw std::runtime_error("test bindless heap");
    RtViewConstants v{};Require(RtViewCameraOf(camera,v),"the view's camera");
    RtLightsConstants constants{};constants.right=v.right;constants.up=v.up;constants.forward=v.forward;constants.position=v.position;constants.jitter=v.jitter;
    constants.width=width;constants.height=height;constants.columns=1;constants.tile=128;
    uint32_t* const slots[5]{&constants.depth,&constants.gb0,&constants.gb1,&constants.gb3,&constants.pad};   // pad: t33's former slot (I-0126 R7c: no AO term; the white image stays bound, unread)
    for(uint32_t i=0;i<5;++i){*slots[i]=heap.Allocate(gpu.CompletedValue());
        D3D12_SHADER_RESOURCE_VIEW_DESC sv{i?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,D3D12_SRV_DIMENSION_TEXTURE2D,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        sv.Texture2D.MipLevels=1;gpu.device->CreateShaderResourceView(i?gbuffer[i-1].Get():depth.Get(),&sv,heap.Cpu(*slots[i]));}
    RtLocalLights pass;RtTlas tlas,cut_tlas;
    if(gpu.diagnostics){D3D12_MESSAGE_ID ids[2]{D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE};
        D3D12_INFO_QUEUE_FILTER filter{};filter.DenyList.NumIDs=2;filter.DenyList.pIDList=ids;gpu.diagnostics->PushStorageFilter(&filter);}
    struct PopFilter {DeviceQueues& gpu;~PopFilter(){if(gpu.diagnostics)gpu.diagnostics->PopStorageFilter();}} const pop{gpu};
    // The stored values as the pass reads them (8-bit): the normal (128, 128, 0) / 255 toward the camera, gloss 0.2, sqrt F0 128 / 255.
    // 128 / 255, not 0.5: a value halfway between two 8-bit steps may round either way (the clear's conversion), and the normal's
    // tilt and F0 with it.
    const float half=128.0f/255;const float gb_values[4][4]{{half,half,0,1},{0.2f,0,0,0},{half,half,half,1},{1,1,1,1}};
    D3D12_GPU_VIRTUAL_ADDRESS scene{},cut_scene{};   // R4c: the PT frames below trace them too
    {auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
        if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
        const D3D12_RECT empty{3,3,4,4};
        list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,surface,0,0,nullptr);list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,1.0f,0,1,&empty);
        for(uint32_t i=0;i<4;++i)list->ClearRenderTargetView(rtv(i),gb_values[i],0,nullptr);
        const float translucent[4]{0.2f,0.2f,0,0};const D3D12_RECT row1{0,1,4,2};list->ClearRenderTargetView(rtv(1),translucent,1,&row1);
        const float start[4]{},quarter[4]{0.25f,0.25f,0.25f,0.25f};const D3D12_RECT kept[2]{{0,0,1,1},{3,3,4,4}};
        for(uint32_t i=0;i<10;++i){list->ClearRenderTargetView(rtv(4+i),start,0,nullptr);list->ClearRenderTargetView(rtv(4+i),quarter,2,kept);}
        D3D12_RESOURCE_BARRIER b[5]{};for(auto& x:b)x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b[0].Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
        for(uint32_t i=0;i<4;++i)b[1+i].Transition={gbuffer[i].Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
        list->ResourceBarrier(5,b);
        const D3D12_VIEWPORT viewport{0,0,float(width),float(height),0,1};const D3D12_RECT scissor{0,0,LONG(width),LONG(height)};
        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
        scene=tlas.Build(gpu.device5.Get(),list.Get(),upload->GetGPUVirtualAddress(),3);
        cut_scene=cut_tlas.Build(gpu.device5.Get(),list.Get(),upload->GetGPUVirtualAddress()+512,2);
        const D3D12_CPU_DESCRIPTOR_HANDLE targets[5][2]{{rtv(4),rtv(5)},{rtv(6),rtv(7)},{rtv(8),rtv(9)},{rtv(10),rtv(11)},{rtv(12),rtv(13)}};
        pass.Encode(gpu.device.Get(),list.Get(),heap,allocate,lights.gpu,tiles.gpu,scene,0,0,constants,targets[0]);
        pass.Encode(gpu.device.Get(),list.Get(),heap,allocate,lights.gpu,tiles.gpu,0,0,0,constants,targets[1]);
        pass.Encode(gpu.device.Get(),list.Get(),heap,allocate,point_lights.gpu,pair_tiles.gpu,cut_scene,0,0,constants,targets[2]);
        pass.Encode(gpu.device.Get(),list.Get(),heap,allocate,spot_lights.gpu,pair_tiles.gpu,cut_scene,0,0,constants,targets[3]);
        pass.Encode(gpu.device.Get(),list.Get(),heap,allocate,spot_lights.gpu,pair_tiles.gpu,0,0,0,constants,targets[4]);
        for(uint32_t i=0;i<10;++i){D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            t.Transition={pair[i].Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&t);
            D3D12_TEXTURE_COPY_LOCATION pic{},out{};pic.pResource=pair[i].Get();pic.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            out.pResource=readback.Get();out.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;out.PlacedFootprint={1024ull*i,{DXGI_FORMAT_R11G11B10_FLOAT,width,height,1,256}};
            list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);}
        b[0].Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_DEPTH_WRITE};list->ResourceBarrier(1,b);
        (void)recording.Commit();}
    gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
    std::vector<uint8_t> bytes(10240);{void* read{};const D3D12_RANGE range{0,10240};if(FAILED(readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
        std::memcpy(bytes.data(),read,bytes.size());const D3D12_RANGE none{0,0};readback->Unmap(0,&none);}
    // The point programme's terms at each pixel (its centre's ray to world z 1), with the 8-bit values as stored.
    const auto q=[](float x){return std::round(x*255)/255;};
    const float nxy=2*q(half)-1;const float nl_len=std::sqrt(nxy*nxy*2+1);const float N[3]{nxy/nl_len,nxy/nl_len,-1/nl_len};
    const float g=q(0.2f),f0=q(half)*q(half);double worst=0;
    // One light at z `lz`: its shadowed diffuse factor (the Lambert, or the wrapped diffuse where t > 0), its translucency factor and
    // its specular factor at P, before the colours (the programmes' common factors applied: diffuse × (1 − F0 / 2), specular × F0).
    struct Terms {float diffuse,translucent,specular;};
    const auto terms=[&](const float* P,float lz,float t){
        float to[3]{0.5f-P[0],0.5f-P[1],lz-P[2]};const float d2=to[0]*to[0]+to[1]*to[1]+to[2]*to[2],dl=std::sqrt(d2);for(auto& c2:to)c2/=dl;
        float E[3]{P[0]-0.5f,P[1]-0.5f,P[2]+2};const float el=std::sqrt(E[0]*E[0]+E[1]*E[1]+E[2]*E[2]);for(auto& c2:E)c2/=el;
        const float nl=N[0]*to[0]+N[1]*to[1]+N[2]*to[2];
        float H[3]{to[0]-E[0],to[1]-E[1],to[2]-E[2]};const float hl=std::sqrt(H[0]*H[0]+H[1]*H[1]+H[2]*H[2]);for(auto& c2:H)c2/=hl;
        const float nh=std::clamp(N[0]*H[0]+N[1]*H[1]+N[2]*H[2],0.f,1.f);
        const float fd=std::max(1/d2-0.01f,0.f),fs=1-std::clamp(d2*0.01f,0.f,1.f);
        const float lobe_n=std::exp2(13*g),lobe=std::max(lobe_n*100/(lobe_n*0.01f+100),1.f),norm=lobe*0.125f+0.25f;
        const float wrapped=t==0?std::clamp(nl,0.f,1.f):std::max(nl+10*t,0.f)/(20*t+1);
        const float back=std::max(1-(E[0]*N[0]+E[1]*N[1]+E[2]*N[2]),0.f)*0.5f;
        const float trans=t==0?0:std::pow(std::clamp(to[0]*E[0]+to[1]*E[1]+to[2]*E[2],0.f,1.f),0.25f*32)*back*back*0.5f*10*t;
        return Terms{wrapped*fd*(1-f0*0.5f),trans*fd*(1-f0*0.5f),std::pow(nh,lobe)*std::pow(std::clamp(nl,0.f,1.f),0.1f)*fs*norm*f0};};
    bool translucency_seen{};
    for(uint32_t pass_index=0;pass_index<3;++pass_index)for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){
        uint32_t d{},s{};std::memcpy(&d,bytes.data()+2048*pass_index+256*y+4*x,4);std::memcpy(&s,bytes.data()+2048*pass_index+1024+256*y+4*x,4);
        const auto diffuse=R11G11B10(d),specular=R11G11B10(s);
        if(x==3 && y==3){Require(diffuse[0]==0.25f && diffuse[2]==0.25f && specular[1]==0.25f,"the pixel without a surface left as it was");continue;}
        const float ndc_x=(x+0.5f)/2-1,ndc_y=1-(y+0.5f)/2;const float P[3]{0.5f+3*ndc_x/8,0.5f+3*ndc_y/8,1};
        const float t=y==1?q(0.2f):0;
        // R4b: the light and its unflagged copy always; traced, the shadowed ones: seen on columns 0 and 1 (the camera's quad and
        // the plane behind the ray's origin ignored), behind the local caster on 2 and 3 (v = 0.2), their translucency unshadowed; the
        // one of strength 0 whole. The near point (pass 2): its copy whole, itself seen through the quad within its near cube on
        // columns 0 and 1, hidden (strength 1) on 2 and 3.
        const float seen=x<2?1.0f:0.2f;
        const auto front=terms(P,0,t),behind=terms(P,2,t);
        float dif=2*front.diffuse,tra=2*front.translucent,spe=2*front.specular;
        if(pass_index<2){const float see=pass_index==0?seen:1.0f;   // pass 1: no top level, every light unshadowed
            dif+=(front.diffuse+behind.diffuse)*see+front.diffuse;tra+=2*front.translucent+behind.translucent;
            spe+=(front.specular+behind.specular)*see+front.specular;}
        if(pass_index==2){const float near_seen=x<2?1.0f:0.0f;dif=front.diffuse*(1+near_seen);tra=2*front.translucent;spe=front.specular*(1+near_seen);}
        if(pass_index==0 && x>=2 && t>0 && behind.translucent>0.05f*(dif+tra))translucency_seen=true;   // where v is not 1
        const float base=x==0 && y==0?0.25f:0;   // the pixel cleared to 0.25: its terms added to it
        const float want_d[3]{base+(dif+tra)*1,base+(dif+tra)*0.5f,base+(dif+tra)*0.25f},want_s[3]{base+spe*0.5f,base+spe,base+spe};
        for(int k=0;k<3;++k){const double ed=std::abs(diffuse[k]-want_d[k])/want_d[k],es=std::abs(specular[k]-want_s[k])/want_s[k];worst=std::max({worst,ed,es});
            const double step=k==2?1.0/32:1.0/64;   // one step of the channel's mantissa (rounding toward zero); 5 % margin for the GPU's approximate maths
            if(ed>step*1.05 || es>step*1.05){std::printf("rt local lights: pass %u pixel %u,%u channel %d diffuse %g (%g) specular %g (%g)\n",pass_index,x,y,k,diffuse[k],want_d[k],specular[k],want_s[k]);
                throw std::runtime_error("the native local lights: a pixel off the point programme's terms");}}}
    Require(translucency_seen,"the test reaches the translucency of the light behind the plane where its visibility is not 1");
    // The spot (pairs 4 and 5): traced over shaded unshadowed, its diffuse: 1 where the quad within its near plane is the only caster
    // (columns 0 and 1), 1 / 2 where the quad beyond it hides the shadowed copy (strength 1). The ratio of two R11G11B10 values: 3 steps.
    for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){if(x==3 && y==3)continue;
        uint32_t lit{},plain_bits{};std::memcpy(&lit,bytes.data()+6144+256*y+4*x,4);std::memcpy(&plain_bits,bytes.data()+8192+256*y+4*x,4);
        const auto traced=R11G11B10(lit),plain=R11G11B10(plain_bits);const float base=x==0 && y==0?0.25f:0;
        Require(plain[0]-base>0.05f && plain[0]<16 && traced[0]<16,"the spot lights the plane (a NaN reads back 2^16 or more)");
        const float ratio=(traced[0]-base)/(plain[0]-base),want=x<2?1.0f:0.5f;
        if(std::abs(ratio-want)>3.0f/64){std::printf("rt local lights: spot pixel %u,%u ratio %g (%g)\n",x,y,ratio,want);
            throw std::runtime_error("the native local lights: the spot's shadow ray not cut at its near plane");}}
    std::printf("rt local lights: worst relative error %.4f (R11G11B10)\n",worst);
    // I-0122 R4c: the PT level over the first tile's five lights (pair 0's), 256 frames of their own seeds, each into its own cleared
    // pair. The mean over the frames against the terms with every light casting whole: v 1 on columns 0 and 1, 0 behind the local caster
    // on 2 and 3 (the shadowed lights' strengths not kept, the unshadowed ones traced to 0.1 m short of their centre), the translucency
    // unshadowed. The four lights in front share one place and colour (their estimate exact: rows 0, 2 and 3 within the format's steps);
    // the one behind lights the translucent row (sampled: 6 %). Then, into pairs 2 and 3, the point and its shadowed copy of the RT
    // test's pass 2 (the step review's M1: a pixel whose lights differ in visibility): on columns 0 and 1 the quad at z 0.2 lies within
    // the copy's near cube (seen) and beyond the point's 0.1 m stop (hidden), on 2 and 3 the quad at z 0.5 hides both; each copy chosen
    // half the time with weight 2, the mean the one seen light's terms (20 %: 256 draws of 0 or twice the value). Not covered: the
    // lights' spheres (offset.w 0).
    {   constexpr uint32_t frames=256;
        RtLightsConstants pt_constants=constants;
        auto pt_readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,4096ull*frames,D3D12_RESOURCE_STATE_COPY_DEST);
        const uint64_t mark=used;
        for(uint32_t frame=0;frame<frames;++frame){used=mark;pt_constants.seed=frame*7919u+1;
            {auto recording=gpu.RecordIngress();Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list;
                if(FAILED(recording.List()->QueryInterface(IID_PPV_ARGS(&list))))throw std::runtime_error("test list4");
                D3D12_RESOURCE_BARRIER b[5]{};for(auto& x:b)x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                b[0].Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
                for(uint32_t i=0;i<4;++i)b[1+i].Transition={pair[i].Get(),0,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET};
                list->ResourceBarrier(5,b);
                const float start[4]{},quarter[4]{0.25f,0.25f,0.25f,0.25f};const D3D12_RECT kept[2]{{0,0,1,1},{3,3,4,4}};
                for(uint32_t i=0;i<4;++i){list->ClearRenderTargetView(rtv(4+i),start,0,nullptr);list->ClearRenderTargetView(rtv(4+i),quarter,2,kept);}
                const D3D12_VIEWPORT viewport{0,0,float(width),float(height),0,1};const D3D12_RECT scissor{0,0,LONG(width),LONG(height)};
                list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
                const D3D12_CPU_DESCRIPTOR_HANDLE targets[2]{rtv(4),rtv(5)},cut_targets[2]{rtv(6),rtv(7)};
                pass.Encode(gpu.device.Get(),list.Get(),heap,allocate,lights.gpu,tiles.gpu,scene,0,0,pt_constants,targets,true);
                pass.Encode(gpu.device.Get(),list.Get(),heap,allocate,point_lights.gpu,pair_tiles.gpu,cut_scene,0,0,pt_constants,cut_targets,true);
                for(uint32_t i=0;i<4;++i){D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    t.Transition={pair[i].Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&t);
                    D3D12_TEXTURE_COPY_LOCATION pic{},out{};pic.pResource=pair[i].Get();pic.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    out.pResource=pt_readback.Get();out.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;out.PlacedFootprint={4096ull*frame+1024ull*i,{DXGI_FORMAT_R11G11B10_FLOAT,width,height,1,256}};
                    list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);}
                b[0].Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_DEPTH_WRITE};list->ResourceBarrier(1,b);
                (void)recording.Commit();}
            gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();}
        std::vector<uint8_t> pt_bytes(4096ull*frames);{void* read{};const D3D12_RANGE range{0,pt_bytes.size()};if(FAILED(pt_readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
            std::memcpy(pt_bytes.data(),read,pt_bytes.size());const D3D12_RANGE none{0,0};pt_readback->Unmap(0,&none);}
        double pt_worst=0,pt_worst_sampled=0;
        for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){
            std::array<double,3> mean_d{},mean_s{};
            for(uint32_t frame=0;frame<frames;++frame){uint32_t d{},s{};std::memcpy(&d,pt_bytes.data()+4096ull*frame+256*y+4*x,4);std::memcpy(&s,pt_bytes.data()+4096ull*frame+1024+256*y+4*x,4);
                const auto diffuse=R11G11B10(d),specular=R11G11B10(s);for(int k=0;k<3;++k){mean_d[k]+=double(diffuse[k])/frames;mean_s[k]+=double(specular[k])/frames;}}
            if(x==3 && y==3){Require(mean_d[0]==0.25 && mean_s[1]==0.25,"PT: the pixel without a surface left as it was");continue;}
            const float ndc_x=(x+0.5f)/2-1,ndc_y=1-(y+0.5f)/2;const float P[3]{0.5f+3*ndc_x/8,0.5f+3*ndc_y/8,1};
            const float t=y==1?q(0.2f):0,seen=x<2?1.0f:0.0f;
            const auto front=terms(P,0,t),behind=terms(P,2,t);
            const float dif=seen*(4*front.diffuse+behind.diffuse),tra=4*front.translucent+behind.translucent,spe=seen*(4*front.specular+behind.specular);
            const float base=x==0 && y==0?0.25f:0;
            const float want_d[3]{base+(dif+tra)*1,base+(dif+tra)*0.5f,base+(dif+tra)*0.25f},want_s[3]{base+spe*0.5f,base+spe,base+spe};
            // The exact rows: one step of the channel's mantissa as the RT test (5 % margin); the sampled row: 6 %. Both plus 1e-3 absolute
            // (a term that is 0 reads back 0).
            for(int k=0;k<3;++k){const double ed=std::abs(mean_d[k]-want_d[k]),es=std::abs(mean_s[k]-want_s[k]);
                const double share=y==1?0.06:(k==2?1.0/32:1.0/64)*1.05;
                double& worst_of=y==1?pt_worst_sampled:pt_worst;
                if(want_d[k]>0.01)worst_of=std::max(worst_of,ed/want_d[k]);if(want_s[k]>0.01)worst_of=std::max(worst_of,es/want_s[k]);
                if(ed>share*want_d[k]+1e-3 || es>share*want_s[k]+1e-3){std::printf("rt local lights PT: pixel %u,%u channel %d diffuse %g (%g) specular %g (%g)\n",x,y,k,mean_d[k],want_d[k],mean_s[k],want_s[k]);
                    throw std::runtime_error("the PT level: a pixel's mean off the terms with every light casting");}}}
        double cut_worst=0;
        for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){if(x==3 && y==3)continue;
            std::array<double,3> mean_d{},mean_s{};
            for(uint32_t frame=0;frame<frames;++frame){uint32_t d{},s{};std::memcpy(&d,pt_bytes.data()+4096ull*frame+2048+256*y+4*x,4);std::memcpy(&s,pt_bytes.data()+4096ull*frame+3072+256*y+4*x,4);
                const auto diffuse=R11G11B10(d),specular=R11G11B10(s);for(int k=0;k<3;++k){mean_d[k]+=double(diffuse[k])/frames;mean_s[k]+=double(specular[k])/frames;}}
            const float ndc_x=(x+0.5f)/2-1,ndc_y=1-(y+0.5f)/2;const float P[3]{0.5f+3*ndc_x/8,0.5f+3*ndc_y/8,1};
            const float t=y==1?q(0.2f):0,seen=x<2?1.0f:0.0f;const auto front=terms(P,0,t);
            const float dif=seen*front.diffuse,tra=2*front.translucent,spe=seen*front.specular,base=x==0 && y==0?0.25f:0;
            const float want_d[3]{base+(dif+tra)*1,base+(dif+tra)*0.5f,base+(dif+tra)*0.25f},want_s[3]{base+spe*0.5f,base+spe,base+spe};
            for(int k=0;k<3;++k){const double ed=std::abs(mean_d[k]-want_d[k]),es=std::abs(mean_s[k]-want_s[k]);
                if(want_d[k]>0.01)cut_worst=std::max(cut_worst,ed/want_d[k]);if(want_s[k]>0.01)cut_worst=std::max(cut_worst,es/want_s[k]);
                if(ed>0.2*want_d[k]+1e-3 || es>0.2*want_s[k]+1e-3){std::printf("rt local lights PT near cut: pixel %u,%u channel %d diffuse %g (%g) specular %g (%g)\n",x,y,k,mean_d[k],want_d[k],mean_s[k],want_s[k]);
                    throw std::runtime_error("the PT level: a light's own visibility not the one applied");}}}
        std::printf("rt local lights PT: worst relative error of the mean over %u frames %.4f (exact rows), %.4f (the sampled row), %.4f (the near cut)\n",frames,pt_worst,pt_worst_sampled,cut_worst);}
    pass.Close();tlas.Close();cut_tlas.Close();
}

// I-0123 RR-a: Ray Reconstruction's guide pass on the hardware device. The view camera (ViewCamera) over the plane at view z 3, one
// pixel without a surface; GB0 the plane's normal tilted (R16G16B16A16_FLOAT, as the production's), GB1 gloss 0.2 (row 1: 0.6), GB2 an albedo through its sRGB view, GB3 sqrt
// F0 128 / 255: each pixel's diffuse albedo, specular albedo (rr_guides.hlsl's EnvBRDFApprox2, ported here) and normal with the
// roughness (2 / (2^(13 g) + 2))^¼ against the values computed here, within the targets' precision (R11G11B10: 1 / 32 relative,
// RGBA16F: 1e-3); the pixel without a surface the guide's sky values.
float Half(uint16_t h) {
    const uint32_t e=(h>>10)&0x1F,m=h&0x3FF;const float s=(h&0x8000)?-1.f:1.f;
    if(e==0)return s*std::ldexp(float(m),-24);if(e==31)return s*std::numeric_limits<float>::infinity();
    return s*std::ldexp(float(m|0x400),int(e)-25);}
std::array<float,3> EnvBrdfApprox2(const std::array<float,3>& f0,float alpha,float nv) {
    nv=std::abs(nv);const float x[4]{1,nv,nv*nv,nv*nv*nv},y[4]{1,alpha,alpha*alpha,alpha*alpha*alpha};
    // HLSL's mul(matrix, vector): row i of the matrix (as written, row-major) times the vector.
    const float m1[2][2]{{0.99044f,-1.28514f},{1.29678f,-0.755907f}},m3[2][2]{{0.0365463f,3.32707f},{9.0632f,-9.04756f}};
    const float m2[3][3]{{1,2.92338f,59.4188f},{20.3225f,-27.0302f,222.592f},{121.563f,626.13f,316.627f}};
    const float m4[3][3]{{1,3.59685f,-1.36772f},{9.04401f,-16.3174f,9.22949f},{5.56589f,19.7886f,-20.2123f}};
    const auto mul2=[](const float m[2][2],float a,float b,float ya,float yb){return (m[0][0]*a+m[0][1]*b)*ya+(m[1][0]*a+m[1][1]*b)*yb;};
    const auto mul3=[](const float m[3][3],float a,float b,float c,float ya,float yb,float yc){
        return (m[0][0]*a+m[0][1]*b+m[0][2]*c)*ya+(m[1][0]*a+m[1][1]*b+m[1][2]*c)*yb+(m[2][0]*a+m[2][1]*b+m[2][2]*c)*yc;};
    float bias=mul2(m1,x[0],x[1],y[0],y[1])/mul3(m2,x[0],x[1],x[3],y[0],y[1],y[3]);
    const float scale=mul2(m3,x[0],x[1],y[0],y[1])/mul3(m4,x[0],x[2],x[3],y[0],y[1],y[3]);
    bias*=std::clamp(f0[1]*50,0.f,1.f);
    return {f0[0]*std::max(0.f,scale)+std::max(0.f,bias),f0[1]*std::max(0.f,scale)+std::max(0.f,bias),f0[2]*std::max(0.f,scale)+std::max(0.f,bias)};
}
void RrGuidePass(DeviceQueues& gpu) {
    const auto camera=ViewCamera();const uint32_t width=camera.width,height=camera.height;
    const float n=camera.near_plane,f=camera.far_plane,a=f/(f-n),surface=a-a*n/3.0f;
    auto upload=Committed(gpu,D3D12_HEAP_TYPE_UPLOAD,4096,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* mapped{};
    if(FAILED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))))throw std::runtime_error("test map");
    uint64_t used=0;
    const auto allocate=[&](uint64_t size){const uint64_t at=(used+255)&~uint64_t(255);used=at+size;if(used>4096)throw std::runtime_error("test upload");
        UploadArena::Range r;r.cpu=mapped+at;r.gpu=upload->GetGPUVirtualAddress()+at;r.bytes=size;return r;};
    D3D12_HEAP_PROPERTIES heap_type{};heap_type.Type=D3D12_HEAP_TYPE_DEFAULT;
    const auto texture=[&](DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear){D3D12_RESOURCE_DESC d{};
        d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=width;d.Height=height;d.DepthOrArraySize=d.MipLevels=1;d.Format=format;d.SampleDesc.Count=1;d.Flags=flags;
        Microsoft::WRL::ComPtr<ID3D12Resource> r;
        if(FAILED(gpu.device->CreateCommittedResource(&heap_type,D3D12_HEAP_FLAG_NONE,&d,state,clear,IID_PPV_ARGS(&r))))throw std::runtime_error("test texture");return r;};
    D3D12_CLEAR_VALUE depth_clear{};depth_clear.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;depth_clear.DepthStencil={1.0f,0};
    auto depth=texture(DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,D3D12_RESOURCE_STATE_DEPTH_WRITE,&depth_clear);
    // Step review L7: GB0 in the production format (scene_pass_internal.h: R16G16B16A16_FLOAT), its values exact in half floats.
    const DXGI_FORMAT gb_formats[4]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_R8G8B8A8_UNORM};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,4> gbuffer;
    for(uint32_t i=0;i<4;++i)gbuffer[i]=texture(gb_formats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> guides;
    for(uint32_t i=0;i<3;++i)guides[i]=texture(kRrGuideFormats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr);
    auto readback=Committed(gpu,D3D12_HEAP_TYPE_READBACK,3072,D3D12_RESOURCE_STATE_COPY_DEST);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvs,rtvs;
    {D3D12_DESCRIPTOR_HEAP_DESC h{D3D12_DESCRIPTOR_HEAP_TYPE_DSV,1,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        if(FAILED(gpu.device->CreateDescriptorHeap(&h,IID_PPV_ARGS(&dsvs))))throw std::runtime_error("test dsv heap");
        h={D3D12_DESCRIPTOR_HEAP_TYPE_RTV,7,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};if(FAILED(gpu.device->CreateDescriptorHeap(&h,IID_PPV_ARGS(&rtvs))))throw std::runtime_error("test rtv heap");}
    const auto dsv=dsvs->GetCPUDescriptorHandleForHeapStart();
    {D3D12_DEPTH_STENCIL_VIEW_DESC d{};d.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;d.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;gpu.device->CreateDepthStencilView(depth.Get(),&d,dsv);}
    const auto rtv_step=gpu.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    const auto rtv=[&](uint32_t i){auto h=rtvs->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*rtv_step;return h;};
    for(uint32_t i=0;i<4;++i)gpu.device->CreateRenderTargetView(gbuffer[i].Get(),nullptr,rtv(i));
    for(uint32_t i=0;i<3;++i)gpu.device->CreateRenderTargetView(guides[i].Get(),nullptr,rtv(4+i));
    BindlessHeap heap;if(FAILED(heap.Open(gpu.device.Get(),8)))throw std::runtime_error("test bindless heap");
    RtViewConstants v{};Require(RtViewCameraOf(camera,v),"the view's camera");
    RrGuidesConstants constants{};constants.right=v.right;constants.up=v.up;constants.forward=v.forward;constants.position=v.position;constants.jitter=v.jitter;
    constants.width=width;constants.height=height;
    uint32_t* const slots[5]{&constants.depth,&constants.gb0,&constants.gb1,&constants.gb2,&constants.gb3};
    for(uint32_t i=0;i<5;++i){*slots[i]=heap.Allocate(gpu.CompletedValue());
        D3D12_SHADER_RESOURCE_VIEW_DESC sv{i?gb_formats[i-1]:DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,D3D12_SRV_DIMENSION_TEXTURE2D,D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
        sv.Texture2D.MipLevels=1;gpu.device->CreateShaderResourceView(i?gbuffer[i-1].Get():depth.Get(),&sv,heap.Cpu(*slots[i]));}
    RrGuides pass;
    if(gpu.diagnostics){D3D12_MESSAGE_ID ids[2]{D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE};
        D3D12_INFO_QUEUE_FILTER filter{};filter.DenyList.NumIDs=2;filter.DenyList.pIDList=ids;gpu.diagnostics->PushStorageFilter(&filter);}
    struct PopFilter {DeviceQueues& gpu;~PopFilter(){if(gpu.diagnostics)gpu.diagnostics->PopStorageFilter();}} const pop{gpu};
    const float half=128.0f/255;const float albedo[3]{0.5f,0.25f,0.1f};
    const float gb_values[4][4]{{0.5f,0.75f,0,1},{0.2f,0,0,0},{albedo[0],albedo[1],albedo[2],1},{half,half,half,1}};
    {auto recording=gpu.RecordIngress();auto* list=recording.List();
        const D3D12_RECT empty{3,3,4,4};
        list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,surface,0,0,nullptr);list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,1.0f,0,1,&empty);
        for(uint32_t i=0;i<4;++i)list->ClearRenderTargetView(rtv(i),gb_values[i],0,nullptr);
        const float rough[4]{0.6f,0,0,0};const D3D12_RECT row1{0,1,4,2};list->ClearRenderTargetView(rtv(1),rough,1,&row1);
        D3D12_RESOURCE_BARRIER b[5]{};for(auto& x:b)x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b[0].Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
        for(uint32_t i=0;i<4;++i)b[1+i].Transition={gbuffer[i].Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
        list->ResourceBarrier(5,b);
        const D3D12_VIEWPORT viewport{0,0,float(width),float(height),0,1};const D3D12_RECT scissor{0,0,LONG(width),LONG(height)};
        list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
        const D3D12_CPU_DESCRIPTOR_HANDLE targets[3]{rtv(4),rtv(5),rtv(6)};
        pass.Encode(gpu.device.Get(),list,heap,allocate,constants,targets);
        for(uint32_t i=0;i<3;++i){D3D12_RESOURCE_BARRIER t{};t.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            t.Transition={guides[i].Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&t);
            D3D12_TEXTURE_COPY_LOCATION pic{},out{};pic.pResource=guides[i].Get();pic.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            out.pResource=readback.Get();out.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;out.PlacedFootprint={1024ull*i,{kRrGuideFormats[i],width,height,1,256}};
            list->CopyTextureRegion(&out,0,0,0,&pic,nullptr);}
        b[0].Transition={depth.Get(),0,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_DEPTH_WRITE};list->ResourceBarrier(1,b);
        (void)recording.Commit();}
    gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();
    std::vector<uint8_t> bytes(3072);{void* read{};const D3D12_RANGE range{0,3072};if(FAILED(readback->Map(0,&range,&read)))throw std::runtime_error("test readback");
        std::memcpy(bytes.data(),read,bytes.size());const D3D12_RANGE none{0,0};readback->Unmap(0,&none);}
    pass.Close();
    const auto q=[](float x){return std::round(x*255)/255;};
    const auto srgb=[](float c){return c<=0.0031308f?12.92f*c:1.055f*std::pow(c,1/2.4f)-0.055f;};
    const auto linear=[](float c){return c<=0.04045f?c/12.92f:std::pow((c+0.055f)/1.055f,2.4f);};
    float raw[3]{0,0.5f,-1};const float length=std::sqrt(raw[0]*raw[0]+raw[1]*raw[1]+raw[2]*raw[2]);for(auto& c:raw)c/=length;
    const float fq=q(half)*q(half);double worst=0;
    const auto close=[&](float got,float want,double relative,double absolute,const char* what){const double e=std::abs(double(got)-want);
        if(e>absolute+relative*std::abs(want)){std::printf("rr guides: %s %g (%g)\n",what,got,want);throw std::runtime_error("the RR guides: a value off");}
        if(want!=0)worst=std::max(worst,e/std::abs(want));};
    for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x){
        uint32_t ab{},sb{};std::memcpy(&ab,bytes.data()+256*y+4*x,4);std::memcpy(&sb,bytes.data()+1024+256*y+4*x,4);
        uint16_t nb[4]{};std::memcpy(nb,bytes.data()+2048+256*y+8*x,8);
        const auto got_albedo=R11G11B10(ab),got_specular=R11G11B10(sb);const float got_normal[4]{Half(nb[0]),Half(nb[1]),Half(nb[2]),Half(nb[3])};
        if(x==3 && y==3){Require(got_albedo[0]==0.5f && got_albedo[1]==0.5f && got_albedo[2]==0.5f && got_specular[0]==0 && got_specular[1]==0 && got_specular[2]==0 &&
            got_normal[0]==0 && got_normal[1]==0 && got_normal[2]==0 && got_normal[3]==0,"the pixel without a surface: the sky's guides");continue;}
        // The eye's ray through the pixel's centre (rt_view.hlsl's), the camera without a jitter.
        const float ndc_x=(x+0.5f)/2-1,ndc_y=1-(y+0.5f)/2;float eye[3]{ndc_x/v.forward[3],ndc_y/v.position[3],1};
        const float el=std::sqrt(eye[0]*eye[0]+eye[1]*eye[1]+eye[2]*eye[2]);for(auto& c:eye)c/=el;
        const float g=q(y==1?0.6f:0.2f),roughness=std::pow(2/(std::exp2(13*g)+2),0.25f);
        const auto want_specular=EnvBrdfApprox2({fq,fq,fq},roughness*roughness,-(raw[0]*eye[0]+raw[1]*eye[1]+raw[2]*eye[2]));
        for(int k=0;k<3;++k){close(got_albedo[k],linear(q(srgb(albedo[k]))),1.0/32,1e-3,"diffuse albedo");
            close(got_specular[k],want_specular[k],1.0/32,1e-3,"specular albedo");close(got_normal[k],raw[k],0,1e-3,"normal");}
        close(got_normal[3],roughness,0,1e-3,"roughness");}
    std::printf("rr guides: worst relative error %.4f\n",worst);
}

void Hardware() {
    DeviceQueues gpu;gpu.Open(false,true);
    if(!gpu.device5 || gpu.Capabilities().raytracing<D3D12_RAYTRACING_TIER_1_1){std::printf("rt scene: no DXR 1.1, skipped\n");gpu.Close();return;}
    // A storage-like buffer: slice 0 a quad (4 vertices of 16 bytes, 6 R16 indices), slice 1 a triangle (3 vertices of 12 bytes,
    // 3 R32 indices), slice 2 ineligible (5 indices).
    std::vector<uint8_t> bytes(512,0);
    const float quad[16]{0,0,0,0, 1,0,0,0, 1,1,0,0, 0,1,0,0};std::memcpy(bytes.data(),quad,sizeof quad);
    for(int v=0;v<4;++v){const int16_t uv[2]{int16_t(v==1 || v==2?2048:0),int16_t(v>=2?2048:0)};std::memcpy(bytes.data()+16*v+12,uv,4);}   // T3: UV16 = x, y
    const uint16_t quad_indices[6]{0,1,2,0,2,3};std::memcpy(bytes.data()+64,quad_indices,sizeof quad_indices);
    const float triangle[9]{0,0,1, 1,0,1, 0,1,1};std::memcpy(bytes.data()+128,triangle,sizeof triangle);
    const uint32_t triangle_indices[3]{0,1,2};std::memcpy(bytes.data()+176,triangle_indices,sizeof triangle_indices);
    std::vector<MeshSlice> slices(3);
    slices[0].vertex_offset=0;slices[0].vertex_count=4;slices[0].stride=16;slices[0].index_offset=64;slices[0].index_count=6;slices[0].index_width=2;
    slices[1].vertex_offset=128;slices[1].vertex_count=3;slices[1].stride=12;slices[1].index_offset=176;slices[1].index_count=3;slices[1].index_width=4;
    slices[2]=slices[0];slices[2].index_count=5;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes.size();desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const auto buffer=gpu.CreateStreamed(desc,D3D12_RESOURCE_STATE_COMMON,PlacedKind::Buffer);
    {auto staging=gpu.AllocateStaging(bytes.size(),4);std::memcpy(staging.cpu,bytes.data(),bytes.size());
        auto recording=gpu.RecordIngress();recording.List()->CopyBufferRegion(buffer.Get(),0,staging.resource,staging.offset,bytes.size());
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={buffer.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,
            static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER|D3D12_RESOURCE_STATE_INDEX_BUFFER|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
        recording.List()->ResourceBarrier(1,&barrier);recording.Keep(std::move(staging.lease));(void)recording.Commit();}
    RtBlasRegistry registry;
    const auto blas=registry.Build(gpu,buffer.Get(),slices);
    Require(blas && blas->geometries==3 && blas->skipped==1 && blas->triangles==3,"three geometries, one empty, three triangles");
    const auto settle=[&]{gpu.Complete(gpu.SubmitIngress());gpu.CollectCompleted();};
    settle();
    Require(blas->state.load()==RtModelBlas::State::built,"built once its list completed");
    Require(registry.Advance(gpu)==1 && blas->state.load()==RtModelBlas::State::compacting,"its compaction recorded");
    settle();
    Require(blas->state.load()==RtModelBlas::State::ready && blas->compacted_bytes>0 && blas->compacted_bytes<=blas->built_bytes,"ready, compacted");
    Require(registry.Advance(gpu)==1,"the ready model leaves the pending list");
    const auto c=registry.Counters();
    Require(c.models==1 && c.geometries==3 && c.skipped==1 && c.triangles==3 && c.ready==1 && c.pending==0 && c.compacted_bytes==blas->compacted_bytes,"counters");
    Require(c.slots_free==RtBlasRegistry::kPostbuildSlots,"its postbuild slot given back");
    Require(c.live_models==1 && c.live_placed_bytes==RtPlaced(blas->compacted_bytes),"alive: one model, its compacted structure placed");
    const auto placed_before=gpu.StreamedMemory().placed_live_bytes;
    std::printf("rt scene: built %llu bytes, compacted %llu bytes\n",static_cast<unsigned long long>(blas->built_bytes),static_cast<unsigned long long>(blas->compacted_bytes));
    // Step review L4: two builds in one submission (the sizes buffer enters the second list in UNORDERED_ACCESS), the second
    // model destroyed before that submission (a storage retired early: its lists keep what they write, its slot comes back).
    {auto second=registry.Build(gpu,buffer.Get(),slices);auto third=registry.Build(gpu,buffer.Get(),slices);
        Require(second && third && registry.Counters().slots_free==RtBlasRegistry::kPostbuildSlots-2,"two slots taken");
        third.reset();settle();
        Require(second->state.load()==RtModelBlas::State::built,"the second built");
        Require(registry.Advance(gpu)==1 && second->state.load()==RtModelBlas::State::compacting,"the second compacting, the destroyed one's slot back");
        settle();(void)registry.Advance(gpu);
        const auto d=registry.Counters();
        Require(second->state.load()==RtModelBlas::State::ready && d.models==3 && d.ready==2 && d.failed==0 && d.pending==0 && d.slots_free==RtBlasRegistry::kPostbuildSlots,"both slots back");
        Require(d.live_models==2 && second->Structure(),"alive: two models (the destroyed one not counted)");}
    // T0 final review L9: the structures of models gone are released (their placed bytes back once their lists completed).
    settle();
    Require(registry.Counters().live_models==1 && gpu.StreamedMemory().placed_live_bytes==placed_before,"the gone models' placed bytes released");
    Tlas(gpu,blas,registry,buffer.Get(),slices);settle();(void)registry.Advance(gpu);settle();(void)registry.Advance(gpu);
    Skinned(gpu);settle();
    Wind(gpu);settle();   // T6
    Materials(gpu,blas,buffer.Get());settle();
    View(gpu,blas);settle();
    SunShadow(gpu,blas);settle();   // R5
    LocalLights(gpu,blas);settle();   // I-0122 R4a, R4b
    RrGuidePass(gpu);settle();   // I-0123 RR-a
    std::printf("rt scene: debug layer %s\n",gpu.DebugLayerActive()?"active":"absent (no validation)");
    Require(DebugMessages(gpu)==0,"no debug-layer warning or error");
    registry.Close();gpu.Drain();gpu.CollectCompleted();gpu.Close();
}
// I-0128 R8a: a resolved slot's pairs added to the window: only the zones of the mask, a pair whose end does not follow its start
// dropped, the sums and maxima over two frames.
void GpuZoneSums() {
    GpuZoneWindow w;uint64_t stamps[2*kGpuZones]{};
    stamps[0]=100;stamps[1]=400;stamps[2]=500;stamps[3]=450;stamps[4]=10;stamps[5]=30;
    AccumulateGpuZones(w,0b011u,stamps);   // zone 2 written but not in the mask; zone 1 backwards
    Require(w.frames==1 && w.count[0]==1 && w.ticks[0]==300 && w.count[1]==0 && w.count[2]==0,"gpu zones: the first frame");
    stamps[0]=0;stamps[1]=500;stamps[2]=0;stamps[3]=20;
    AccumulateGpuZones(w,0b011u,stamps);
    Require(w.frames==2 && w.count[0]==2 && w.ticks[0]==800 && w.ticks_max[0]==500 && w.count[1]==1 && w.ticks[1]==20,"gpu zones: two frames");
}
}

int main() try {
    Eligibility();GpuZoneSums();LightGrid();SkinRecords();SkinRows();SurfaceMasks();SameMaterials();ViewCameraMaths();LightRecords();Hardware();
    std::printf("PASS rt scene: the slices a BLAS takes, a model's BLAS built, compacted, alive and released, a top level of its instances traced by inline rays on the hardware device, a skinned instance built and refitted, the wind's grass, the alpha test and the materials of its records, the control view, the traced sun shadow, the local lights' records and pass with their traced shadows, Ray Reconstruction's guides, the GPU zones' sums, the light grid\n");
    return 0;
} catch(const std::exception& e){std::printf("FAIL rt scene: %s\n",e.what());return 1;}
