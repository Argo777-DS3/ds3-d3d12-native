// Reference and diagnostic witnesses, second part: material preparation, cascades and the shadow atlas, casters, the local
// lights' shadow slots, decals and paint, effect draws, glow and GPARAM binds. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part
// (04_updates_and_test_tools.inc before it).
#pragma once
#include "04_updates_and_test_tools.inc"
namespace {
// 1114 witness (Reference): GXFlverMaterial vt+10 = 140680 reads the shader pass from draw state +1C and
// passes it to AA280 (pass table 3D4xxxx: 0 Fwd, 1 Velo, 2 Dpt, 3 DptA, 4 FwdSdw, 5 FwdSSdw, 6 Gbuf, 7 Gbuf2,
// ...). Distinct (shader pass, state +18C, caller) tuples with counts, and once the 2D drawer's virtual
// extent [[4961AB0]+88]+F8/FC used by the FaceGen mark layer (2377FA0). Pass-through.
using MaterialPrepare=uint32_t(__fastcall*)(uintptr_t,uint32_t,uintptr_t);
MaterialPrepare material_prepare_original{};
uint32_t __fastcall ObserveMaterialPrepare(uintptr_t material,uint32_t context,uintptr_t state) noexcept {
    static SRWLOCK lock=SRWLOCK_INIT;static std::array<std::array<uint64_t,4>,64> seen{};static size_t used{};static LONG64 calls{};
    const auto n=InterlockedIncrement64(&calls);
    int32_t pass{};uint32_t flags{};Read(state+0x1C,&pass,4);Read(state+0x18C,&flags,4);
    const uint64_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;
    AcquireSRWLockExclusive(&lock);size_t k=0;for(;k<used;++k)if(seen[k][0]==uint64_t(uint32_t(pass)) && seen[k][1]==flags && seen[k][2]==caller)break;
    const bool fresh=k==used && used<seen.size();if(fresh){seen[used++]={uint64_t(uint32_t(pass)),flags,caller,0};}
    if(k<used)++seen[k][3];
    const bool report=n%200000==0;std::string rows;
    if(report)for(size_t i=0;i<used;++i){char cell[96]{};sprintf_s(cell,"%s[%llu,%llu,%llu,%llu]",i?",":"",seen[i][0],seen[i][1],seen[i][2],seen[i][3]);rows+=cell;}
    ReleaseSRWLockExclusive(&lock);
    if(fresh){char row[256]{};sprintf_s(row,"{\"event\":\"reference_material_shader_pass\",\"call\":%lld,\"pass\":%d,\"state_18c\":%u,\"caller\":%llu}\n",n,pass,flags,caller);Log(row);}
    if(report){std::string row="{\"event\":\"reference_material_shader_pass_counts\",\"call\":"+std::to_string(n)+",\"rows\":["+rows+"]}\n";Log(row.c_str());
        uintptr_t display{},record{};uint32_t extent[2]{};
        if(Read(image+0x4961AB0,&display,8) && display && Read(display+0x88,&record,8) && record && Read(record+0xF8,extent,8)){
            char row2[160]{};sprintf_s(row2,"{\"event\":\"reference_2d_virtual_extent\",\"width\":%u,\"height\":%u}\n",extent[0],extent[1]);Log(row2);}}
    return material_prepare_original(material,context,state);
}
// W5a SW-3 live row SL4 (SHADOWS-WAVE1-READY 2.3, 8), Reference with shadow-observe.flag, pass-through: the cascade fit
// FUN_140239610(float* viewProj16, float(*matrix)[4], camera record, float* lightDir, float depthRange, uint size,
// char alignTexel, float blend) (prototype read in the decompile). Per sampled call: the scalar arguments, the camera record
// (0x150 bytes: the pose +0x50..+0x8F and the lens +0x140..+0x14C), the light direction and both returned matrices, the
// oracle of T-W3. Calls 1-8, then four consecutive calls (one per cascade) every 1200th call.
using CascadeFit=void(__fastcall*)(float*,void*,uintptr_t,const float*,float,uint32_t,char,float);
CascadeFit cascade_fit_original{};
void __fastcall ObserveCascadeFit(float* view_projection,void* matrix,uintptr_t camera,const float* light,float depth_range,
    uint32_t size,char align,float blend) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    cascade_fit_original(view_projection,matrix,camera,light,depth_range,size,align,blend);
    if(n>8 && n%1200>=4)return;
    const auto hex=[](const void* data,size_t bytes){std::string out;out.reserve(bytes*2);char cell[3]{};
        for(size_t i=0;i<bytes;++i){sprintf_s(cell,"%02X",static_cast<const uint8_t*>(data)[i]);out+=cell;}return out;};
    std::array<uint8_t,0x150> record{};std::array<uint8_t,16> direction{};
    const bool read=Read(camera,record.data(),record.size()) && Read(reinterpret_cast<uintptr_t>(light),direction.data(),16);
    std::string row="{\"event\":\"reference_cascade_fit\",\"call\":"+std::to_string(n)+",\"caller\":"+
        std::to_string(reinterpret_cast<uintptr_t>(_ReturnAddress())-image)+",\"depth_range\":\""+hex(&depth_range,4)+"\""+
        ",\"size\":"+std::to_string(size)+",\"align\":"+std::to_string(static_cast<int>(align))+",\"blend\":\""+hex(&blend,4)+"\""+
        ",\"read\":"+(read?"true":"false")+",\"camera\":\""+hex(record.data(),record.size())+"\",\"light\":\""+hex(direction.data(),16)+
        "\",\"view_projection\":\""+hex(view_projection,64)+"\",\"matrix\":\""+hex(matrix,64)+"\"}\n";
    Log(row.c_str());
}
// Cascade observers (SHADOWS-NATIVE 1.4 row SL6; SUN-SHADOWS-WAVE1 7), Reference with cascade-observe.flag, pass-through: the keep test
// FUN_1401B5B40(obj, i, slice camera) -> AL (1 keep) and the cascade record FUN_1401B6020(obj, pass, direction, i, flag), counted
// per cascade; every 240th test of cascade 0 (or record when none is tested) a row with the counts since the last row (records
// without a test are the cascades F9B50 found without usable content, +0xC1D), the object's flags (+0xC09 the reprojection mode,
// +0xC19 content, +0xC1D usable), the forced bytes [14496231C+i], the mode byte [14473DAD4] and the slice camera's position.
using CascadeKeep=char(__fastcall*)(uintptr_t,uint64_t,uintptr_t);
using CascadeRecord=void(__fastcall*)(uintptr_t,uintptr_t,const float*,uint32_t,char);
CascadeKeep cascade_keep_original{};CascadeRecord cascade_record_original{};
// The casters: 57A20's model rows recorded in the cascade passes (pass id 1..4 at pass +0x124), by the model's data name's first
// letter (m map piece, c character, o object, other), and the G-buffer passes' (5..8) for scale.
struct ReferenceCascadeCounts {LONG64 tests[4]{},kept[4]{},records[4]{};LONG64 rows{};uintptr_t obj{};float position[3]{};
    LONG64 casters[4][4]{},gbuffer{};
    LONG64 entity_emits[4]{},disp_emits[4]{};std::map<uint32_t,LONG64> entity_bits;};   // D079C0 and D04B90 calls in the cascade passes, D079C0 entity +0x20 low bytes
ReferenceCascadeCounts reference_cascades;std::mutex reference_cascade_lock;
// The original's atlas draws (cascade-observe.flag, Reference): d3d11.dll's DrawIndexed, DrawIndexedInstanced and Draw, taken from a
// scratch device's immediate context (every context of the runtime shares them), pass-through. A draw whose depth view is a square
// depth texture at least 4096 texels wide is counted by viewport: the four quadrants of the sun atlas (edge 8192: cascade by origin),
// or the other square atlases (the local lights' tiles); with how many bind a VS b5 (cbObjMatrix, the skin palette of the Dpt_Skin
// programmes) and their index count. Read by ReferenceCascadeRow.
struct ReferenceAtlasDraws {volatile LONG64 sun[4]{},sun_skinned[4]{},sun_indices[4]{},other{},other_skinned{};};
ReferenceAtlasDraws reference_atlas;volatile LONG reference_atlas_detail{};
// The vertex programmes the game creates, by object, with their DXBC checksum's first four bytes (ID3D11Device::CreateVertexShader,
// slot 12 of the scratch device's table, pass-through).
std::mutex reference_vs_lock;std::unordered_map<ID3D11VertexShader*,uint32_t> reference_vs_checksums;
using DeviceCreateVertexShader=HRESULT(STDMETHODCALLTYPE*)(ID3D11Device*,const void*,SIZE_T,ID3D11ClassLinkage*,ID3D11VertexShader**);
DeviceCreateVertexShader device_create_vs_original{};
HRESULT STDMETHODCALLTYPE ObserveCreateVertexShader(ID3D11Device* device,const void* code,SIZE_T bytes,ID3D11ClassLinkage* linkage,ID3D11VertexShader** out) noexcept {
    const HRESULT result=device_create_vs_original(device,code,bytes,linkage,out);
    if(SUCCEEDED(result) && out && *out && code && bytes>=8){uint32_t checksum{};std::memcpy(&checksum,static_cast<const uint8_t*>(code)+4,4);
        const std::lock_guard hold(reference_vs_lock);if(reference_vs_checksums.size()<65536)reference_vs_checksums[*out]=checksum;}
    return result;}
// I-0113 gap 7 (the named oracle, decal-observe.flag): the pixel programmes likewise (ID3D11Device::CreatePixelShader, slot 15), their
// DXBC checksum's first eight bytes and size, to name the programme the original binds at a named draw.
struct ReferencePs {uint64_t checksum{};size_t bytes{};};
std::mutex reference_ps_lock;std::unordered_map<ID3D11PixelShader*,ReferencePs> reference_ps;
using DeviceCreatePixelShader=HRESULT(STDMETHODCALLTYPE*)(ID3D11Device*,const void*,SIZE_T,ID3D11ClassLinkage*,ID3D11PixelShader**);
DeviceCreatePixelShader device_create_ps_original{};
HRESULT STDMETHODCALLTYPE ObserveCreatePixelShader(ID3D11Device* device,const void* code,SIZE_T bytes,ID3D11ClassLinkage* linkage,ID3D11PixelShader** out) noexcept {
    const HRESULT result=device_create_ps_original(device,code,bytes,linkage,out);
    if(SUCCEEDED(result) && out && *out && code && bytes>=12){uint64_t checksum{};std::memcpy(&checksum,static_cast<const uint8_t*>(code)+4,8);
        const std::lock_guard hold(reference_ps_lock);if(reference_ps.size()<65536)reference_ps[*out]={checksum,bytes};}
    return result;}
using ContextDrawIndexed=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,INT);
using ContextDrawIndexedInstanced=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,UINT,INT,UINT);
using ContextDraw=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT);
ContextDrawIndexed context_draw_indexed_original{},deferred_draw_indexed_original{};
ContextDrawIndexedInstanced context_draw_indexed_instanced_original{},deferred_draw_indexed_instanced_original{};
ContextDraw context_draw_original{},deferred_draw_original{};
void ReferenceAtlasDraw(ID3D11DeviceContext* context,UINT indices) noexcept {
    ID3D11DepthStencilView* dsv{};context->OMGetRenderTargets(0,nullptr,&dsv);if(!dsv)return;
    static std::mutex lock;static std::unordered_map<ID3D11DepthStencilView*,uint32_t> edges;uint32_t edge{};
    {const std::lock_guard hold(lock);if(const auto found=edges.find(dsv);found!=edges.end())edge=found->second;
        else{ID3D11Resource* resource{};dsv->GetResource(&resource);D3D11_TEXTURE2D_DESC desc{};
            if(resource){Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;if(SUCCEEDED(resource->QueryInterface(IID_PPV_ARGS(&texture))))texture->GetDesc(&desc);resource->Release();}
            edge=desc.Width==desc.Height?desc.Width:0;if(edges.size()<4096)edges[dsv]=edge;}}
    dsv->Release();if(edge<4096)return;
    ID3D11Buffer* palette{};context->VSGetConstantBuffers(5,1,&palette);const bool skinned=palette!=nullptr;if(palette)palette->Release();
    UINT count=1;D3D11_VIEWPORT viewport{};context->RSGetViewports(&count,&viewport);
    // Detail rows (a budget ReferenceCascadeRow opens once): the draw's viewport, index range, index buffer and the vertex programme's
    // DXBC checksum (its first 8 hex digits, PROGRAM-FINGERPRINTS programs.tsv names it).
    if(reference_atlas_detail>0 && InterlockedDecrement(&reference_atlas_detail)>=0){ID3D11Buffer* ib{};DXGI_FORMAT format{};UINT offset{};context->IAGetIndexBuffer(&ib,&format,&offset);
        ID3D11VertexShader* vs{};context->VSGetShader(&vs,nullptr,nullptr);uint32_t checksum{};
        {const std::lock_guard hold(reference_vs_lock);if(const auto f=reference_vs_checksums.find(vs);f!=reference_vs_checksums.end())checksum=f->second;}
        char row[256]{};sprintf_s(row,"{\"event\":\"reference_atlas_draw\",\"edge\":%u,\"viewport\":[%g,%g,%g],\"indices\":%u,\"ib\":%llu,\"vs\":\"%08X\",\"context\":%llu}\n",
            edge,viewport.TopLeftX,viewport.TopLeftY,viewport.Width,indices,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(ib)),_byteswap_ulong(checksum),
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(context)));Log(row);
        if(ib)ib->Release();if(vs)vs->Release();}
    if(edge==8192 && count){const unsigned c=(viewport.TopLeftX>=4096.f?1u:0u)|(viewport.TopLeftY>=4096.f?2u:0u);
        InterlockedIncrement64(&reference_atlas.sun[c]);if(skinned)InterlockedIncrement64(&reference_atlas.sun_skinned[c]);InterlockedAdd64(&reference_atlas.sun_indices[c],indices);}
    else{InterlockedIncrement64(&reference_atlas.other);if(skinned)InterlockedIncrement64(&reference_atlas.other_skinned);}
}
void STDMETHODCALLTYPE ObserveContextDrawIndexed(ID3D11DeviceContext* context,UINT indices,UINT start,INT base) noexcept {
    ReferenceAtlasDraw(context,indices);context_draw_indexed_original(context,indices,start,base);}
void STDMETHODCALLTYPE ObserveContextDrawIndexedInstanced(ID3D11DeviceContext* context,UINT indices,UINT instances,UINT start,INT base,UINT first) noexcept {
    ReferenceAtlasDraw(context,indices*instances);context_draw_indexed_instanced_original(context,indices,instances,start,base,first);}
void STDMETHODCALLTYPE ObserveContextDraw(ID3D11DeviceContext* context,UINT vertices,UINT start) noexcept {
    ReferenceAtlasDraw(context,vertices);context_draw_original(context,vertices,start);}
void STDMETHODCALLTYPE ObserveDeferredDrawIndexed(ID3D11DeviceContext* context,UINT indices,UINT start,INT base) noexcept {
    ReferenceAtlasDraw(context,indices);deferred_draw_indexed_original(context,indices,start,base);}
void STDMETHODCALLTYPE ObserveDeferredDrawIndexedInstanced(ID3D11DeviceContext* context,UINT indices,UINT instances,UINT start,INT base,UINT first) noexcept {
    ReferenceAtlasDraw(context,indices*instances);deferred_draw_indexed_instanced_original(context,indices,instances,start,base,first);}
void STDMETHODCALLTYPE ObserveDeferredDraw(ID3D11DeviceContext* context,UINT vertices,UINT start) noexcept {
    ReferenceAtlasDraw(context,vertices);deferred_draw_original(context,vertices,start);}
// The three entries from a scratch hardware device's immediate context (vtable slots 12 DrawIndexed, 13 Draw, 20 DrawIndexedInstanced),
// and a deferred context's where they differ (null otherwise).
bool ReferenceAtlasDrawEntries(void*& indexed,void*& draw,void*& instanced,void*& deferred_indexed,void*& deferred_draw,void*& deferred_instanced,void*& create_vs) noexcept {
    const auto module=LoadLibraryW(L"d3d11.dll");if(!module)return false;   // the game loads it too; kept loaded
    using Create=HRESULT(WINAPI*)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,const D3D_FEATURE_LEVEL*,UINT,UINT,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);
    const auto create=reinterpret_cast<Create>(GetProcAddress(module,"D3D11CreateDevice"));if(!create)return false;
    Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    if(FAILED(create(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)) || !context)return false;
    void** table=*reinterpret_cast<void***>(context.Get());indexed=table[12];draw=table[13];instanced=table[20];create_vs=(*reinterpret_cast<void***>(device.Get()))[12];
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deferred;deferred_indexed=deferred_draw=deferred_instanced=nullptr;
    if(SUCCEEDED(device->CreateDeferredContext(0,&deferred)) && deferred){void** d=*reinterpret_cast<void***>(deferred.Get());
        if(d[12]!=indexed)deferred_indexed=d[12];if(d[13]!=draw)deferred_draw=d[13];if(d[20]!=instanced)deferred_instanced=d[20];}
    return indexed && draw && instanced;
}
// A scratch hardware device's method `slot` (I-0113: CreatePixelShader 15 for the named oracle, CreateTexture2D 5 for the unit
// cubes), or null.
void* ReferenceDeviceEntry(size_t slot) noexcept {
    const auto module=LoadLibraryW(L"d3d11.dll");if(!module)return nullptr;
    using Create=HRESULT(WINAPI*)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,const D3D_FEATURE_LEVEL*,UINT,UINT,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);
    const auto create=reinterpret_cast<Create>(GetProcAddress(module,"D3D11CreateDevice"));if(!create)return nullptr;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    if(FAILED(create(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,nullptr)) || !device)return nullptr;
    return (*reinterpret_cast<void***>(device.Get()))[slot];
}
// I-0113 tranche 10 (decal-observe.flag, Reference): the device's CreateTexture2D: every 1x1 cube made with initial data (the
// default cube a forward key whose id is -1 binds through 75B30 when the global REM is null: 1x1 B8G8R8A8 with six faces,
// i113-t10-crystal-witness3), its format, each face's first texel (its row pitch's bytes, the texel size of a 1x1 face, at most 8) and its caller (image offset).
using CreateTexture2D=HRESULT(STDMETHODCALLTYPE*)(ID3D11Device*,const D3D11_TEXTURE2D_DESC*,const D3D11_SUBRESOURCE_DATA*,ID3D11Texture2D**);
CreateTexture2D device_create_texture2d_original{};
HRESULT STDMETHODCALLTYPE ObserveCreateTexture2D(ID3D11Device* device,const D3D11_TEXTURE2D_DESC* desc,const D3D11_SUBRESOURCE_DATA* data,ID3D11Texture2D** out) noexcept {
    // I-0113 tranche 30: made first, so the row names the texture (the named oracle's @address of a 1x1 view's resource).
    const HRESULT made=device_create_texture2d_original(device,desc,data,out);
    if(SUCCEEDED(made) && desc && data && desc->Width==1 && desc->Height==1 && (desc->MiscFlags&D3D11_RESOURCE_MISC_TEXTURECUBE) && desc->ArraySize>=6){
        const UINT mips=desc->MipLevels?desc->MipLevels:1;
        std::string faces;for(UINT face=0;face<6;++face){uint8_t texel[8]{};const auto& sub=data[face*mips];   // "mips" logs the raw count (0: one level)
            const size_t texel_bytes=sub.SysMemPitch && sub.SysMemPitch<8?sub.SysMemPitch:8;
            if(sub.pSysMem)Read(reinterpret_cast<uintptr_t>(sub.pSysMem),texel,texel_bytes);faces+=(face?",\"":"\"")+HexBytes(texel,texel_bytes)+"\"";}
        char head[240]{};sprintf_s(head,"{\"event\":\"reference_unit_cube\",\"format\":%u,\"mips\":%u,\"array\":%u,\"caller\":\"%llX\",\"texture\":\"%llX\",\"faces\":[",static_cast<unsigned>(desc->Format),
            desc->MipLevels,desc->ArraySize,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image),
            static_cast<unsigned long long>(out?reinterpret_cast<uintptr_t>(*out):0));
        Log((std::string(head)+faces+"]}\n").c_str());}
    // Tranche 31b: a 1x1 2D texture with initial data too (the hair's t33, g_SSAOTexture, 1x1 B8G8R8A8 in the original), 64 rows.
    else if(SUCCEEDED(made) && desc && data && data->pSysMem && desc->Width==1 && desc->Height==1 && !(desc->MiscFlags&D3D11_RESOURCE_MISC_TEXTURECUBE)){
        static std::atomic<int> left{64};
        if(left.fetch_sub(1)>0){uint8_t texel[8]{};const size_t texel_bytes=data->SysMemPitch && data->SysMemPitch<8?data->SysMemPitch:8;
            Read(reinterpret_cast<uintptr_t>(data->pSysMem),texel,texel_bytes);char row[260]{};
            sprintf_s(row,"{\"event\":\"reference_unit_texture\",\"format\":%u,\"array\":%u,\"caller\":\"%llX\",\"texture\":\"%llX\",\"id\":%llu,\"texel\":\"%s\"}\n",static_cast<unsigned>(desc->Format),desc->ArraySize,
                static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image),static_cast<unsigned long long>(out?reinterpret_cast<uintptr_t>(*out):0),
                static_cast<unsigned long long>(out?reinterpret_cast<uintptr_t>(*out):0),HexBytes(texel,texel_bytes).c_str());Log(row);}}
    return made;
}
void ReferenceDepthModelsRow();   // below
void ReferenceCascadeRow() {
    auto& r=reference_cascades;uint8_t flags[0x19]{},forced[4]{},mode{};
    if(r.obj)Read(r.obj+0xC08,flags,sizeof flags);Read(image+0x496231C,forced,4);Read(image+0x473DAD4,&mode,1);
    char row[600]{};sprintf_s(row,"{\"event\":\"reference_cascade_cache\",\"row\":%lld,\"tests\":[%lld,%lld,%lld,%lld],\"kept\":[%lld,%lld,%lld,%lld],"
        "\"records\":[%lld,%lld,%lld,%lld],\"c09\":%u,\"content\":[%u,%u,%u,%u],\"usable\":[%u,%u,%u,%u],\"forced\":[%u,%u,%u,%u],\"mode\":%u,\"position\":[%g,%g,%g],",
        ++r.rows,r.tests[0],r.tests[1],r.tests[2],r.tests[3],r.kept[0],r.kept[1],r.kept[2],r.kept[3],r.records[0],r.records[1],r.records[2],r.records[3],
        flags[1],flags[0x11],flags[0x12],flags[0x13],flags[0x14],flags[0x15],flags[0x16],flags[0x17],flags[0x18],forced[0],forced[1],forced[2],forced[3],mode,
        r.position[0],r.position[1],r.position[2]);
    std::string text=row;text+="\"casters_mcoX\":[";   // (r is reset after the row, the entity bits map with it)
    for(int c=0;c<4;++c){char cell[96]{};sprintf_s(cell,"%s[%lld,%lld,%lld,%lld]",c?",":"",r.casters[c][0],r.casters[c][1],r.casters[c][2],r.casters[c][3]);text+=cell;}
    text+="],\"gbuffer_rows\":"+std::to_string(r.gbuffer)+",\"entity_emits\":[";
    for(int c=0;c<4;++c)text+=(c?",":"")+std::to_string(r.entity_emits[c]);
    text+="],\"disp_emits\":[";for(int c=0;c<4;++c)text+=(c?",":"")+std::to_string(r.disp_emits[c]);
    text+="],\"entity_bits\":{";bool first=true;for(const auto& [bits,count]:r.entity_bits){text+=(first?"\"":",\"")+std::to_string(bits)+"\":"+std::to_string(count);first=false;}
    text+="},\"atlas_sun\":[";   // the atlas draws since the last row (read and cleared)
    auto& a=reference_atlas;const auto take=[](volatile LONG64& v){return std::to_string(InterlockedExchange64(&v,0));};
    for(int c=0;c<4;++c)text+=(c?",":"")+take(a.sun[c]);
    text+="],\"atlas_sun_skinned\":[";for(int c=0;c<4;++c)text+=(c?",":"")+take(a.sun_skinned[c]);
    text+="],\"atlas_sun_indices\":[";for(int c=0;c<4;++c)text+=(c?",":"")+take(a.sun_indices[c]);
    text+="],\"atlas_other\":"+take(a.other)+",\"atlas_other_skinned\":"+take(a.other_skinned)+"}\n";
    if(r.rows==10)InterlockedExchange(&reference_atlas_detail,240);   // one frame or so of atlas draw details
    if(r.rows%10==5)ReferenceDepthModelsRow();   // the depth rows' models over the last 240 frames
    Log(text.c_str());const auto obj=r.obj;const LONG64 rows=r.rows;r=ReferenceCascadeCounts{};r.obj=obj;r.rows=rows;
}
// The entities each pass scene admits: the method +0x50 (142E80) of the pass visitor FUN_140142A50 builds (vtable [143D52988], the
// path C2EC0 takes while [14473CE5D] is set, as shipped; 204C30 otherwise), the pass at
// visitor +0x80), called per entity the world query keeps for the pass's category byte (C2EC0: 0x40 in the cascade passes 1..4)
// and culler. Tallied for the cascade passes (per cascade) and the G-buffer passes 5..8 by the entity's vtable and +0x20 low byte.
using PassAdmit=void(__fastcall*)(uintptr_t,uintptr_t);
PassAdmit pass_admit_original{};
struct ReferencePassAdmits {LONG64 cascade[4]{},gbuffer{};std::map<std::pair<uint64_t,uint32_t>,LONG64> cascade_kinds,gbuffer_kinds;LONG64 rows{};};
ReferencePassAdmits reference_admits;std::mutex reference_admit_lock;
void __fastcall ObservePassAdmit(uintptr_t visitor,uintptr_t entity) noexcept {
    uintptr_t pass{};int32_t id{-1};uintptr_t table{};uint64_t bits{};
    if(Read(visitor+0x80,&pass,8) && pass && Read(pass+0x124,&id,4) && id>=1 && id<=8){Read(entity,&table,8);Read(entity+0x20,&bits,8);
        const std::pair<uint64_t,uint32_t> kind{table>=image?table-image:table,static_cast<uint32_t>(bits&0xFF)};
        const std::lock_guard hold(reference_admit_lock);auto& r=reference_admits;
        if(id<=4){++r.cascade[id-1];if(r.cascade_kinds.size()<256)++r.cascade_kinds[kind];else if(auto f=r.cascade_kinds.find(kind);f!=r.cascade_kinds.end())++f->second;}
        else{++r.gbuffer;if(r.gbuffer_kinds.size()<256)++r.gbuffer_kinds[kind];else if(auto f=r.gbuffer_kinds.find(kind);f!=r.gbuffer_kinds.end())++f->second;}
        if(r.gbuffer>=20000 || r.cascade[0]>=20000){
            std::string row="{\"event\":\"reference_pass_admits\",\"row\":"+std::to_string(++r.rows)+",\"cascade\":["+std::to_string(r.cascade[0])+","+std::to_string(r.cascade[1])+","+
                std::to_string(r.cascade[2])+","+std::to_string(r.cascade[3])+"],\"gbuffer\":"+std::to_string(r.gbuffer)+",\"cascade_kinds\":[";
            bool first=true;for(const auto& [k,n]:r.cascade_kinds){char cell[80]{};sprintf_s(cell,"%s[%llu,%u,%lld]",first?"":",",static_cast<unsigned long long>(k.first),k.second,n);row+=cell;first=false;}
            row+="],\"gbuffer_kinds\":[";first=true;
            for(const auto& [k,n]:r.gbuffer_kinds){char cell[80]{};sprintf_s(cell,"%s[%llu,%u,%lld]",first?"":",",static_cast<unsigned long long>(k.first),k.second,n);row+=cell;first=false;}
            row+="]}\n";Log(row.c_str());const auto rows=r.rows;r=ReferencePassAdmits{};r.rows=rows;}}
    pass_admit_original(visitor,entity);
}
// The depth rows the original executes (63CB0(executor, model, materials, state, ...): its rows of state type 2, the Dpt/DptA
// preparation of the cascade passes and the local lights' tiles), counted per model data name (cascade-observe.flag, Reference).
using RowExecute=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
RowExecute row_execute_original{};
std::string NativeModelDataName(uintptr_t model);
std::mutex reference_depth_lock;std::unordered_map<uintptr_t,LONG64> reference_depth_models;
bool PopinObserve() noexcept;void PopinReferenceShadow(uintptr_t model) noexcept;   // I-0100 tranche 2, below
struct PopinReferenceExec {std::mutex lock;std::unordered_map<uintptr_t,std::array<uint32_t,2>> rows;};   // I-0100 tranche 4: per model, depth and other rows
PopinReferenceExec popin_reference_exec;
// I-0100 tranche 5 (the Settlement's sun casters): the original's depth rows run at render time from its sorted command lists
// (13EEB0 -> 13F470 -> 59220 -> 5CEC0 for rigid models, B76D0 -> B6FA0 -> 5D5C0 for skinned ones; the return chains of
// i100-t5d-route-o1, the FLVER entries of K-0394). Both entries take the pass context first (+0x124, the command's pass from its
// sort key: the four sun cascades are 5..8 there, the player's parts in 6 and 7 at a third-person camera, i100-t5e-places-o1;
// 9..0x38 the local lights' faces), then the model, then data whose world matrix is not established (neither the third argument
// nor the fourth's +0xA0 holds one, i100-t5e/t5f-places-o1). With caster-place.flag (the original-probe Reference, with
// popin-observe; a plain Reference run installs no hook), each (pass, model, fourth argument: not a stable instance, it differs by
// pass and rotates for skinned draws) drawn into a pass 1..0x38 in a game step is kept; every 600 steps one
// reference_caster_places row per key with its pass and the steps it was drawn in, and (tranche 6, the final review's M1 of tranche
// 5) one reference_sun_casters row per model with the steps it was drawn in any sun cascade (passes 5..8, all its instances and
// grouped draws as one: the presence the native's popin_native_shadow rows give). Names are read at the step after the draw.
struct CasterPlace {uintptr_t model{},instance{};int32_t pass{};uint32_t steps{};std::string name;};
struct CasterPlaces {std::mutex lock;std::unordered_map<uint64_t,CasterPlace> current,totals;std::unordered_map<std::string,uint32_t> sun;int64_t since{};};
CasterPlaces caster_places;
using InstanceDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
InstanceDraw rigid_instance_draw_original{},skinned_instance_draw_original{};
void CasterPlaceRecord(uintptr_t context,uintptr_t model,uintptr_t instance) noexcept {
    int32_t pass{};if(!context || !model || !Read(context+0x124,&pass,4) || pass<1 || pass>0x38)return;
    const uint64_t key=(model*1099511628211ull)^instance^(uint64_t(pass)<<56);
    try{const std::lock_guard hold(caster_places.lock);if(caster_places.current.size()<16384 && !caster_places.current.contains(key))caster_places.current[key]={model,instance,pass,0};}catch(...){}
}
void __fastcall ObserveRigidInstanceDraw(uintptr_t context,uintptr_t model,uintptr_t data,uintptr_t instance,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8) noexcept {
    CasterPlaceRecord(context,model,instance);rigid_instance_draw_original(context,model,data,instance,a5,a6,a7,a8);}
void __fastcall ObserveSkinnedInstanceDraw(uintptr_t context,uintptr_t model,uintptr_t data,uintptr_t instance,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8) noexcept {
    CasterPlaceRecord(context,model,instance);skinned_instance_draw_original(context,model,data,instance,a5,a6,a7,a8);}
// I-0100 tranche 6 (caster-place.flag, the original-probe Reference): the culler of each sun cascade pass (5..8), which 1B6020
// makes with 23C1E0(culler = sun manager +0x170 + 0x100 i, view = the pass context +0x130, light direction, two lengths) and sets
// as the pass culler +0x410 that the caster walk (C2EC0, category 0x02) tests: its planes (count +0xF0, plane k at +0x10 (k+1))
// and its box (+0xD0 minimum, +0xE0 maximum), the inputs, and the view's matrix +0x50. Calls 1..8, then four in a row (the four
// cascades) every 9,600.
using CasterCullerBuild=void(__fastcall*)(uintptr_t,uintptr_t,const float*,float,float);
CasterCullerBuild caster_culler_build_original{};
void __fastcall ObserveCasterCullerBuild(uintptr_t culler,uintptr_t view,const float* light,float length,float scale) noexcept {
    const uintptr_t from=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;caster_culler_build_original(culler,view,light,length,scale);
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);if(n>8 && n%9600>=4)return;
    int32_t count{};float planes[12][4]{},box[8]{},direction[4]{},matrix[16]{};
    if(!Read(culler+0xF0,&count,4) || count<0 || count>12 || !Read(culler+0x10,planes,16ull*count) || !Read(culler+0xD0,box,sizeof box))return;
    if(light)Read(reinterpret_cast<uintptr_t>(light),direction,sizeof direction);Read(view+0x50,matrix,sizeof matrix);
    std::string text="{\"event\":\"reference_caster_culler\",\"call\":"+std::to_string(n)+",\"from\":"+std::to_string(from)+",\"culler\":\""+std::to_string(culler)+"\",\"count\":"+std::to_string(count);
    char cell[160]{};
    sprintf_s(cell,",\"light\":[%.4f,%.4f,%.4f,%.4f],\"length\":%.3f,\"scale\":%.3f",direction[0],direction[1],direction[2],direction[3],length,scale);text+=cell;
    sprintf_s(cell,",\"lo\":[%.2f,%.2f,%.2f],\"hi\":[%.2f,%.2f,%.2f],\"planes\":[",box[0],box[1],box[2],box[4],box[5],box[6]);text+=cell;
    for(int k=0;k<count;++k){sprintf_s(cell,"%s[%.4f,%.4f,%.4f,%.3f]",k?",":"",planes[k][0],planes[k][1],planes[k][2],planes[k][3]);text+=cell;}
    text+="],\"view_matrix\":[";for(int k=0;k<16;++k){sprintf_s(cell,"%s%.4f",k?",":"",matrix[k]);text+=cell;}
    text+="]}\n";Log(text.c_str());
}
// I-0100 tranche 6 (caster-place.flag): 57A20, the row recorder, in the sun cascade passes (pass context +0x124 = 5..8): each
// (model, pass) once (at most 600 rows), with the model's row-mask words +0x220/+0x224 (5C0E0 keeps a row whose mask meets them)
// and the draw state's fade bytes +0x84..+0x87 (57A20's category remap), to find where FG_A_2012's depth rows stop.
// The recorded steps by (model, pass) join the drawn places' windows with the pass + 0x40.
using CasterRows=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
CasterRows caster_rows_original{};
uintptr_t ModelOfRowSource(uintptr_t source) noexcept;
std::string PopinName(uintptr_t model);
void __fastcall ObserveCasterRows(uintptr_t a1,uintptr_t view,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10) noexcept {
    int32_t pass{-1};
    if(view && Read(view+0x124,&pass,4) && pass>=5 && pass<=8)if(const auto model=ModelOfRowSource(a9)){
        static std::mutex lock;static std::set<std::pair<uintptr_t,int32_t>> seen;bool fresh{};
        {const std::lock_guard hold(lock);fresh=seen.size()<600 && seen.emplace(model,pass).second;}
        if(fresh){uint32_t words[2]{};Read(model+0x220,words,8);BYTE bytes[4]{};if(a4)Read(a4+0x84,bytes,4);char text[300]{};
            sprintf_s(text,"{\"event\":\"reference_caster_rows\",\"model\":\"%s\",\"pass\":%d,\"flags220\":%u,\"flags224\":%u,\"b84\":[%u,%u,%u,%u]}\n",
                PopinName(model).c_str(),pass,words[0],words[1],bytes[0],bytes[1],bytes[2],bytes[3]);Log(text);}
        // And its steps by pass, as the drawn places: reference_caster_places rows with the pass + 0x40 (0x45..0x48: recorded).
        const int32_t recorded=pass+0x40;const uint64_t key=(model*1099511628211ull)^(uint64_t(recorded)<<56);
        try{const std::lock_guard hold(caster_places.lock);if(caster_places.current.size()<16384 && !caster_places.current.contains(key))caster_places.current[key]={model,0,recorded,0};}catch(...){}}
    caster_rows_original(a1,view,a3,a4,a5,a6,a7,a8,a9,a10);
}
void CasterPlacesWindow(int64_t step) noexcept {   // at each world update gate of the original
    static const bool on=ModuleFlag(L"caster-place.flag");if(!on)return;
    std::unordered_map<uint64_t,CasterPlace> step_places;
    try{const std::lock_guard hold(caster_places.lock);step_places.swap(caster_places.current);}catch(...){return;}
    // The totals and the sun presence are this thread's alone (the hooks fill only `current`).
    // The sun presence by model name (two loaded copies of one model count once, as the native reader merges by name).
    std::vector<CasterPlace> dump;std::vector<std::pair<std::string,uint32_t>> sun_dump;
    try{std::unordered_set<std::string> sun_now;
        for(auto& [key,p]:step_places){auto& t=caster_places.totals[key];if(!t.steps){t=p;t.name=PopinName(p.model);}++t.steps;
            if(p.pass>=5 && p.pass<=8 && sun_now.insert(t.name).second)++caster_places.sun[t.name];}
        if(!caster_places.since)caster_places.since=step;
        if(step-caster_places.since>=600){for(auto& [key,t]:caster_places.totals)dump.push_back(std::move(t));
            for(auto& [name,steps]:caster_places.sun)sun_dump.emplace_back(name,steps);
            caster_places.totals.clear();caster_places.sun.clear();caster_places.since=step;}}catch(...){return;}
    for(const auto& p:dump){char text[240]{};
        sprintf_s(text,"{\"event\":\"reference_caster_places\",\"step\":%lld,\"pass\":%d,\"model\":\"%s\",\"instance\":\"%llx\",\"steps\":%u}\n",
            static_cast<long long>(step),p.pass,p.name.c_str(),static_cast<unsigned long long>(p.instance),p.steps);Log(text);}
    for(const auto& [name,steps]:sun_dump){char text[200]{};
        sprintf_s(text,"{\"event\":\"reference_sun_casters\",\"step\":%lld,\"model\":\"%s\",\"steps\":%u}\n",static_cast<long long>(step),name.c_str(),steps);Log(text);}
}
uintptr_t __fastcall ObserveRowExecute(uintptr_t a1,uintptr_t model,uintptr_t a3,uintptr_t state,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10) noexcept {
    int32_t type{-1};if(state)Read(state+0x1C,&type,4);
    if(model && PopinObserve())try{const std::lock_guard hold(popin_reference_exec.lock);auto& r=popin_reference_exec.rows;if(r.size()<16384 || r.contains(model))++r[model][type==2?0:1];}catch(...){}
    if(type==2 && model){
        if(PopinObserve())PopinReferenceShadow(model);   // review: both, so a cascade-observe run keeps its rows
        static const bool cascades=ModuleFlag(L"cascade-observe.flag");   // review: its rows are drained by the cascade hooks only
        if(cascades)try{const std::lock_guard hold(reference_depth_lock);if(reference_depth_models.size()<8192)++reference_depth_models[model];}catch(...){}}
    return row_execute_original(a1,model,a3,state,a5,a6,a7,a8,a9,a10);
}
void ReferenceDepthModelsRow() {   // the models of the depth rows since the last row, by name (sorted by count), then cleared
    std::unordered_map<uintptr_t,LONG64> models;{const std::lock_guard hold(reference_depth_lock);models.swap(reference_depth_models);}
    std::map<std::string,LONG64> names;for(const auto& [model,count]:models){std::string clean;for(const auto ch:NativeModelDataName(model))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');names[clean]+=count;}
    std::vector<std::pair<std::string,LONG64>> sorted(names.begin(),names.end());std::sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.second>b.second;});
    for(size_t i=0;i<sorted.size();i+=60){std::string row="{\"event\":\"reference_depth_models\",\"from\":"+std::to_string(i)+",\"models\":[";
        for(size_t j=i;j<sorted.size() && j<i+60;++j)row+=std::string(j>i?",":"")+"[\""+sorted[j].first+"\","+std::to_string(sorted[j].second)+"]";
        row+="]}\n";Log(row.c_str());}
}
char __fastcall ObserveCascadeKeep(uintptr_t obj,uint64_t cascade,uintptr_t camera) noexcept {
    const char kept=cascade_keep_original(obj,cascade,camera);const auto i=static_cast<uint32_t>(cascade)&3;
    const std::lock_guard hold(reference_cascade_lock);auto& r=reference_cascades;r.obj=obj;++r.tests[i];r.kept[i]+=kept?1:0;
    Read(camera+0x80,r.position,12);
    if(i==0 && r.tests[0]>=240)ReferenceCascadeRow();
    return kept;
}
void __fastcall ObserveCascadeRecord(uintptr_t obj,uintptr_t pass,const float* direction,uint32_t cascade,char flag) noexcept {
    cascade_record_original(obj,pass,direction,cascade,flag);
    if((cascade&3)==0 && direction){   // I-0140 tranche 7: F90F0's sun direction (its argument 3), a row at each change, at most 2,000
        static std::mutex lock;static std::array<float,3> last{};static int rows{};std::array<float,3> now{};
        if(Read(reinterpret_cast<uintptr_t>(direction),now.data(),12)){bool changed{};
            {const std::lock_guard held(lock);if(rows<2000 && std::memcmp(now.data(),last.data(),12)){last=now;++rows;changed=true;}}
            if(changed){char row[200]{};sprintf_s(row,"{\"event\":\"reference_shadow_sun\",\"obj\":%llu,\"direction\":[%.6g,%.6g,%.6g]}\n",
                static_cast<unsigned long long>(obj),now[0],now[1],now[2]);Log(row);}}}
    const std::lock_guard hold(reference_cascade_lock);auto& r=reference_cascades;r.obj=obj;++r.records[cascade&3];
    if((cascade&3)==0 && r.records[0]>=240 && !r.tests[0])ReferenceCascadeRow();
}
// W5a live row SL5 (Reference, shadow-observe.flag, pass-through): the cbSceneParam image each view uploads (13AA60(this, source):
// this+0xCB0, 960 bytes, after 13A580's copy) and the cascaded-shadow inputs of cbPerScene (1D6D20(pass, state, ...): pass
// +0x50..+0x110, state +0xA60..+0xC00; the pass is the second argument and the state the third, as the decompile reads them).
// Sampled: calls 1-12, then twelve consecutive calls every 3000th.
std::string HexBytes(const void* data,size_t bytes) {std::string out;out.reserve(bytes*2);char cell[3]{};
    for(size_t i=0;i<bytes;++i){sprintf_s(cell,"%02X",static_cast<const uint8_t*>(data)[i]);out+=cell;}return out;}
using SceneImageUpload=void(__fastcall*)(uintptr_t,uintptr_t);
SceneImageUpload scene_image_upload_original{};
void __fastcall ObserveSceneImage(uintptr_t view,uintptr_t source) noexcept {
    scene_image_upload_original(view,source);
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n>12 && n%3000>=12)return;
    std::array<uint8_t,960> image_bytes{};if(!Read(view+0xCB0,image_bytes.data(),image_bytes.size()))return;
    const std::string row="{\"event\":\"reference_scene_image\",\"call\":"+std::to_string(n)+",\"view\":"+std::to_string(view)+",\"source\":"+
        std::to_string(source)+",\"caller\":"+std::to_string(reinterpret_cast<uintptr_t>(_ReturnAddress())-image)+",\"image\":\""+HexBytes(image_bytes.data(),960)+"\"}\n";
    Log(row.c_str());
}
std::string LocalShadowContainerCell();
// FUN_14192F970(GX rasterizer object, GX description 0x1C bytes); SEH-guarded.
__declspec(noinline) static bool GuardedRasterizerDesc(uintptr_t object,uint8_t* desc) noexcept {
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool ok=true;
    __try{reinterpret_cast<void(__fastcall*)(uintptr_t,uint8_t*)>(image+0x192F970)(object,desc);}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
    guarded_copy_active=outer;return ok;
}
using PerSceneBuild=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
PerSceneBuild per_scene_build_original{};
void __fastcall ObservePerScene(uintptr_t a1,uintptr_t pass,uintptr_t state,uintptr_t a4,uintptr_t a5) noexcept {
    per_scene_build_original(a1,pass,state,a4,a5);
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n>12 && n%600>=6)return;
    std::array<uint8_t,0xC0> pass_bytes{};std::array<uint8_t,0x1A0> state_bytes{};
    if(!Read(pass+0x50,pass_bytes.data(),pass_bytes.size()) || !Read(state+0xA60,state_bytes.data(),state_bytes.size()))return;
    // Row SL3: the four cascade records (state +0x8F0 + 0x50 n, 1B5720) and the GX descriptions of their rasterizer states
    // (state +0xA38 + 8 n, made by 1B5390 and re-made by 1B6020; read back by 192F970 as 1B6020 does).
    std::array<uint8_t,0x140> cascade_bytes{};Read(state+0x8F0,cascade_bytes.data(),cascade_bytes.size());
    std::string rasterizers;
    for(unsigned c=0;c<4;++c){uintptr_t object{};std::array<uint8_t,0x1C> desc{};
        if(Read(state+0xA38+8*c,&object,8) && object)GuardedRasterizerDesc(object,desc.data());
        rasterizers+=std::string(c?",\"":"\"")+HexBytes(desc.data(),desc.size())+"\"";}
    std::string row="{\"event\":\"reference_per_scene\",\"call\":"+std::to_string(n)+",\"pass\":"+std::to_string(pass)+",\"state\":"+std::to_string(state)+
        ",\"pass_50\":\""+HexBytes(pass_bytes.data(),pass_bytes.size())+"\",\"state_a60\":\""+HexBytes(state_bytes.data(),state_bytes.size())+
        "\",\"cascades_8f0\":\""+HexBytes(cascade_bytes.data(),cascade_bytes.size())+"\",\"rasterizers\":["+rasterizers+"]"+LocalShadowContainerCell()+"}\n";
    // Rows SL12/SL15 (W5a SW-8): the mask layer's FilterMode word (argument 5 +0x20: 1 -> programme +0x40, 2 -> +0x48, else +0x38)
    // and its three programme objects.
    {int32_t filter_word{};uintptr_t programmes[3]{};Read(a5+0x20,&filter_word,4);Read(a1+0x38,programmes,24);
        char cell[200]{};sprintf_s(cell,",\"filter_word\":%d,\"programmes\":[%d,%d,%d]",filter_word,programmes[0]?1:0,programmes[1]?1:0,programmes[2]?1:0);
        row.pop_back();row.pop_back();row+=cell;row+="}\n";}
    Log(row.c_str());
}
// Rows SL12/SL13/SL15 (W5a SW-8): FUN_1401D7850 (the mask blur; argument 3 the layer's blur block: +0x00/+0x08 the two mask
// targets, +0x10 the third, +0x24 the radius, +0x28 the pass count) and the global factor [[GXRS+0x5368]+0x128].
using MaskBlur=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
MaskBlur mask_blur_original{};
void __fastcall ObserveMaskBlur(uintptr_t a1,uintptr_t pass,uintptr_t block) noexcept {
    static volatile LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=12 || n%300==0){uintptr_t root{},settings{},targets[3]{};float global{-1.f},radius{};int32_t count{};
        if(Read(image+0x4961B50,&root,8) && root && Read(root+0x5368,&settings,8) && settings)Read(settings+0x128,&global,4);
        Read(block,targets,24);Read(block+0x24,&radius,4);Read(block+0x28,&count,4);
        char row[320]{};sprintf_s(row,"{\"event\":\"reference_mask_blur\",\"call\":%lld,\"global\":%g,\"radius\":%g,\"count\":%d,\"targets\":[%llu,%llu,%llu]}\n",
            static_cast<long long>(n),global,radius,count,static_cast<unsigned long long>(targets[0]),static_cast<unsigned long long>(targets[1]),static_cast<unsigned long long>(targets[2]));Log(row);}
    mask_blur_original(a1,pass,block);
}
// Rows SL8/SL10/SL11 (W5a SW-7): the local-light shadow container [GXRS+0x41D8] (1B24E0: +0x1F8 atlas, +0x258 ready, +0x260/+0x268
// item vector of 0x1850-byte items, +0x280 capacity, +0x284 frame counter, +0x288 mode, +0x28C..+0x2B4 the eleven config dwords),
// the root's mode byte +0x4E1A, the quality Lighting byte [[GXRS+0x5360]+0x44] and the globals 144961BF5 (one-pass cube),
// 14473D39C, 14473CE59/5A (job variant).
std::string LocalShadowContainerCell() {
    uintptr_t root{},container{},quality{};uint8_t mode{},lighting{},one_pass{},d39c{},ce59{},ce5a{};
    if(!Read(image+0x4961B50,&root,8) || !root)return ",\"local_shadow\":null";
    Read(root+0x41D8,&container,8);Read(root+0x4E1A,&mode,1);Read(root+0x5360,&quality,8);if(quality)Read(quality+0x44,&lighting,1);
    Read(image+0x4961BF5,&one_pass,1);Read(image+0x473D39C,&d39c,1);Read(image+0x473CE59,&ce59,1);Read(image+0x473CE5A,&ce5a,1);
    char text[256]{};sprintf_s(text,",\"local_shadow\":{\"root_mode\":%u,\"lighting\":%u,\"one_pass\":%u,\"d39c\":%u,\"ce59\":%u,\"ce5a\":%u,\"container\":%s",
        mode,lighting,one_pass,d39c,ce59,ce5a,container?"true":"false");
    std::string cell=text;
    if(container){std::array<uint8_t,0xC0> words{};uintptr_t begin{},end{},atlas{};
        Read(container+0x1F8,&atlas,8);Read(container+0x260,&begin,8);Read(container+0x268,&end,8);Read(container+0x1F8,words.data(),words.size());
        sprintf_s(text,",\"items\":%lld,\"atlas\":%s,\"words_1f8\":\"",static_cast<long long>(end>=begin?(end-begin)/0x1850:-1),atlas?"true":"false");
        cell+=text;cell+=HexBytes(words.data(),words.size())+"\"";}
    return cell+"}";
}
// Row SL9 (W5a SW-7): GXDrawPointLight slot 3 (212C40) and GXDrawSpotLight slot 3 (214350) on their frame items, the item
// words they read before and the result (+0xB0 / +0x1C0 the container item, +0x1C the programme variant; for a spot the
// rectangle +0x268..+0x274) and, on a hit, the container item's +0x378..+0x383 (face index or count, size, clear) and
// +0x17E0..+0x184F (face rectangles and flags). Bounded: the first 64 calls of each, the calls of every 120th world recording, and the first 600 calls
// whose item casts (+0xA4 / +0x1C8).
using ShadowSlot=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
ShadowSlot point_shadow_slot_original{},spot_shadow_slot_original{};
void LogShadowSlot(const char* kind,uintptr_t item,uintptr_t camera,uintptr_t container,const uint8_t* before,size_t bytes,uint64_t result,LONG64 n,uint32_t hit_at) {
    std::vector<uint8_t> after(bytes);Read(item,after.data(),after.size());
    uintptr_t items_begin{},items_end{};if(container){Read(container+0x260,&items_begin,8);Read(container+0x268,&items_end,8);}
    char text[192]{};sprintf_s(text,"{\"event\":\"reference_local_shadow_slot\",\"kind\":\"%s\",\"call\":%lld,\"item\":%llu,\"camera\":%llu,\"result\":%llu,\"items\":%lld,",
        kind,static_cast<long long>(n),static_cast<unsigned long long>(item),static_cast<unsigned long long>(camera),static_cast<unsigned long long>(result&0xFF),
        static_cast<long long>(items_end>=items_begin?(items_end-items_begin)/0x1850:-1));
    std::string row=std::string(text)+"\"before\":\""+HexBytes(before,bytes)+"\",\"after\":\""+HexBytes(after.data(),after.size())+"\"";
    // SW-7: the pass words the item builders (212810, 2136B0) and the slot read: +0x10..+0x4F (the view), +0xD0..+0x10F (passed
    // to 1400B2DC0 as the camera matrix) and the metric scalar +0x280.
    {std::array<uint8_t,0x40> view{},world{};float metric{};
        if(Read(camera+0x10,view.data(),view.size()) && Read(camera+0xD0,world.data(),world.size()) && Read(camera+0x280,&metric,4)){
            char scalar[48]{};sprintf_s(scalar,",\"ctx_280\":%.9g",metric);
            row+=",\"ctx_10\":\""+HexBytes(view.data(),view.size())+"\",\"ctx_d0\":\""+HexBytes(world.data(),world.size())+"\""+scalar;}}
    uintptr_t hit{};std::memcpy(&hit,after.data()+hit_at,8);
    if(hit){std::array<uint8_t,12> head{};std::array<uint8_t,0x70> faces{};Read(hit+0x378,head.data(),head.size());Read(hit+0x17E0,faces.data(),faces.size());
        row+=",\"hit_378\":\""+HexBytes(head.data(),head.size())+"\",\"hit_17e0\":\""+HexBytes(faces.data(),faces.size())+"\"";}
    row+="}\n";Log(row.c_str());
}
// The world-pipeline light recordings seen (ObserveLightRecording): every slot call of a sampled recording is logged, so a zone
// visited late in a run still shows its whole item list (one recording in 120).
volatile LONG64 observed_world_recordings{};
template<size_t Bytes> uint64_t ObserveShadowSlot(ShadowSlot original,const char* kind,uint32_t cast_at,uint32_t hit_at,volatile LONG64& calls,volatile LONG64& cast_rows,
    uintptr_t item,uintptr_t camera,uintptr_t container) noexcept {
    const auto n=InterlockedIncrement64(&calls);std::array<uint8_t,Bytes> before{};const bool read=Read(item,before.data(),before.size());
    const bool log=read && (n<=64 || observed_world_recordings%120==1 || (before[cast_at] && InterlockedIncrement64(&cast_rows)<=600));
    const auto result=original(item,camera,container);
    if(log)LogShadowSlot(kind,item,camera,container,before.data(),before.size(),result,n,hit_at);
    return result;
}
uint64_t __fastcall ObservePointShadowSlot(uintptr_t item,uintptr_t camera,uintptr_t container) noexcept {
    static volatile LONG64 calls{},cast_rows{};return ObserveShadowSlot<0x120>(point_shadow_slot_original,"point",0xA4,0xB0,calls,cast_rows,item,camera,container);
}
uint64_t __fastcall ObserveSpotShadowSlot(uintptr_t item,uintptr_t camera,uintptr_t container) noexcept {
    static volatile LONG64 calls{},cast_rows{};return ObserveShadowSlot<0x280>(spot_shadow_slot_original,"spot",0x1C8,0x1C0,calls,cast_rows,item,camera,container);
}
// Row SL9b (W5a SW-7): FCE30's light-visibility visitor slot 140222FF0 (visitor +0x10 the view's frustum record, +0x18/+0x1C
// the masks against light +0x60/+0x64, +0x20..+0x28 the kept lights): per call the light, its sphere +0x150, its masks and
// whether it was kept; the frustum record (0x300 bytes) on the first call of each window. The first 400 calls, then 60 calls every 3000th, at most
// 40,000 rows in a run.
using LightVisitor=uint64_t(__fastcall*)(uintptr_t,uintptr_t);
LightVisitor light_visitor_original{};
uint64_t __fastcall ObserveLightVisitor(uintptr_t visitor,uintptr_t light) noexcept {
    uintptr_t begin{},end{};Read(visitor+0x20,&begin,8);Read(visitor+0x28,&end,8);
    const auto result=light_visitor_original(visitor,light);
    static volatile LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    static volatile LONG64 rows{};
    if((n<=400 || n%3000<60) && InterlockedIncrement64(&rows)<=40000){uintptr_t after{},vtable{},record{};Read(visitor+0x28,&after,8);Read(light,&vtable,8);Read(visitor+0x10,&record,8);
        std::array<float,4> sphere{};uint32_t masks[4]{};Read(light+0x150,sphere.data(),16);Read(light+0x60,&masks[0],8);Read(visitor+0x18,&masks[2],8);
        char row[400]{};sprintf_s(row,"{\"event\":\"reference_light_visibility\",\"call\":%lld,\"light\":%llu,\"spot\":%d,\"sphere\":[%.7g,%.7g,%.7g,%.7g],\"masks\":[%u,%u,%u,%u],\"kept\":%d,\"before\":%lld",
            static_cast<long long>(n),static_cast<unsigned long long>(light),vtable==image+0x3D4E8E8?1:0,sphere[0],sphere[1],sphere[2],sphere[3],
            masks[0],masks[1],masks[2],masks[3],after>end?1:0,static_cast<long long>(end>=begin?(end-begin)/8:-1));
        std::string text=row;
        if(n==1 || n%3000==0){std::array<uint8_t,0x300> bytes{};if(record && Read(record,bytes.data(),bytes.size()))text+=",\"record\":\""+HexBytes(bytes.data(),bytes.size())+"\"";}
        text+="}\n";Log(text.c_str());}
    return result;
}
// Decals by family (DECALS-WAVE1, Thomas 2026-10-18; decal-observe.flag, both modes): what the original's decal manager holds and
// draws, and the character paint targets. A decal object (vtable 3D5B8D8) in one row: its blend mode +0x208, width scale +0x244,
// cubic +0x248, blood +0x258, +0x259, bump +0x25A, POM +0x25B, blend weight +0x26C, material +0x270, the textures +0x190..+0x200 as a
// bit mask (bit i = +0x190+8i set), emissive +0x80 with +0xFC, the lifetime +0x414/+0x418 (DECALS-NATIVE §2.1-§2.3).
std::string DecalObjectRow(uintptr_t object){
    uintptr_t table{},material{},emissive{};int32_t blend{},cubic{};float width{},emissive_scale{},life[2]{};std::array<uint8_t,0x18> flags{};std::array<uintptr_t,15> texture_slots{};
    Read(object,&table,8);Read(object+0x208,&blend,4);Read(object+0x244,&width,4);Read(object+0x248,&cubic,4);Read(object+0x258,flags.data(),flags.size());
    Read(object+0x270,&material,8);Read(object+0x190,texture_slots.data(),sizeof(texture_slots));Read(object+0x80,&emissive,8);Read(object+0xFC,&emissive_scale,4);Read(object+0x414,life,8);
    uint32_t mask{};for(size_t i=0;i<texture_slots.size();++i)if(texture_slots[i])mask|=1u<<i;
    char row[320]{};sprintf_s(row,"{\"vt\":\"%llX\",\"blend\":%d,\"width\":%g,\"cubic\":%d,\"blood\":%u,\"b259\":%u,\"bump\":%u,\"pom\":%u,\"weight\":%u,\"material\":%s,\"texture_slots\":\"%X\",\"emissive\":%s,\"emissive_scale\":%g,\"life\":[%g,%g]}",
        static_cast<unsigned long long>(table-image),blend,width,cubic,flags[0],flags[1],flags[2],flags[3],flags[0x14],material?"true":"false",mask,emissive?"true":"false",emissive_scale,life[0],life[1]);
    return row;}
namespace chair_hunt {std::wstring Fragment() noexcept;bool Matches(const std::wstring& part,uintptr_t model) noexcept;}   // 08_model_programmes.inc
namespace decal_observe {
std::mutex lock;uint64_t manager_draws{},decal_draws{},paint_patches{},paint_clears{};size_t last_count=SIZE_MAX;std::map<std::string,uint64_t> kinds;
using ManagerDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);ManagerDraw manager_draw_original{};
using DecalDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,int,uintptr_t,uintptr_t);DecalDraw decal_draw_original{};
using PatchCreate=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint8_t);PatchCreate patch_original{};
using PaintClear=void(__fastcall*)(uintptr_t,uintptr_t);PaintClear clear_original{};
// 51AA0 (GXDecalManager::Draw): the list +0x68..+0x70 whenever its length changes and every 600th call, each object in a row.
void __fastcall ManagerDrawObserved(uintptr_t manager,uintptr_t context,uintptr_t depth,uintptr_t attr,uintptr_t gb1) noexcept {
    std::array<uintptr_t,2> list{};Read(manager+0x68,list.data(),16);const size_t count=list[1]>=list[0]?(list[1]-list[0])/8:0;uint8_t option=1;
    {uintptr_t settings{};if(Read(image+0x4961B50,&settings,8) && settings && Read(settings+0x5360,&settings,8) && settings)Read(settings+0x68,&option,1);}
    bool log{};{const std::lock_guard hold(lock);++manager_draws;log=count!=last_count || manager_draws%600==0;last_count=count;}
    // Thomas (2026-10-18): the original's frames in flight. Once, in game: the GX display [4961AB0]'s first swap-chain record
    // (+0x88; +0x0 the state, whose +0x0 is the IDXGISwapChain) and its device record (+0x10, ID3D11Device at +0x0): the swap
    // chain's own description and the device's maximum frame latency (IDXGIDevice1).
    // Review of Reference2116 (a GX object taken for the swap chain): each COM pointer is accepted only when its vtable lies in
    // the expected system module, the swap chain searched one and two levels below the record.
    const auto in_module=[](uintptr_t object,const wchar_t* name){uintptr_t table{};MODULEINFO info{};const HMODULE module=GetModuleHandleW(name);
        return object && module && Read(object,&table,8) && GetModuleInformation(GetCurrentProcess(),module,&info,sizeof(info)) &&
            table>=reinterpret_cast<uintptr_t>(info.lpBaseOfDll) && table<reinterpret_cast<uintptr_t>(info.lpBaseOfDll)+info.SizeOfImage;};
    if(manager_draws==300){uintptr_t display{},records{},level1{},level2{},level3{},chain{},device_record{},device{};
        if(Read(image+0x4961AB0,&display,8) && display && Read(display+0x88,&records,8) && records && Read(records,&level1,8) && level1){
            Read(level1,&level2,8);if(level2)Read(level2,&level3,8);
            for(const auto candidate:{level1,level2,level3})if(!chain && in_module(candidate,L"dxgi.dll"))chain=candidate;}
        if(Read(display+0x10,&device_record,8) && device_record && Read(device_record,&device,8) && !in_module(device,L"d3d11.dll"))device=0;
        if(chain && device){
            DXGI_SWAP_CHAIN_DESC desc{};UINT latency{};const auto described=reinterpret_cast<IDXGISwapChain*>(chain)->GetDesc(&desc);
            IDXGIDevice1* dxgi{};HRESULT queried=reinterpret_cast<IUnknown*>(device)->QueryInterface(__uuidof(IDXGIDevice1),reinterpret_cast<void**>(&dxgi));
            if(SUCCEEDED(queried) && dxgi){queried=dxgi->GetMaximumFrameLatency(&latency);dxgi->Release();}
            char row[256]{};sprintf_s(row,"{\"event\":\"reference_frame_latency\",\"desc\":\"%08lX\",\"buffers\":%u,\"swap_effect\":%u,\"flags\":\"%X\",\"latency\":\"%08lX\",\"maximum_frame_latency\":%u}\n",
                static_cast<unsigned long>(described),desc.BufferCount,static_cast<unsigned>(desc.SwapEffect),desc.Flags,static_cast<unsigned long>(queried),latency);Log(row);}
        else Log("{\"event\":\"reference_frame_latency\",\"unreadable\":true}\n");}
    if(log){std::string row="{\"event\":\"decal_observe_manager\",\"draws\":"+std::to_string(manager_draws)+",\"count\":"+std::to_string(count)+",\"option\":"+std::to_string(option)+",\"decal_draws\":"+std::to_string(decal_draws)+",\"objects\":[";
        for(size_t i=0;i<count && i<24;++i){uintptr_t object{};Read(list[0]+8*i,&object,8);row+=(i?",":"")+(object?DecalObjectRow(object):std::string("null"));}
        Log((row+"]}\n").c_str());}
    manager_draw_original(manager,context,depth,attr,gb1);}
// The oracle of the native decal stage (DECALS-WAVE1 §2 D1): inside 198910 only (this thread), the GX inner wrappers' D3D11
// arguments: each constant buffer's last UpdateSubresource bytes, the VS and PS constant slots, the 18 PS resources (their texture
// descriptions), the samplers, the programmes (their index in the manager's table +0x98, by the GX object's handle +0x10), the
// blend, depth-stencil and rasterizer descriptions with the stencil reference, and the DrawIndexed: one row per oracle draw
// (the first 8 decals, then every 600th).
thread_local bool in_decal{};thread_local uintptr_t oracle_manager{},oracle_decal{},oracle_view{},oracle_batch{};
// The module holding an object's vtable (its file name), or empty (Reference2124: the oracle's objects were refused by the
// d3d11.dll range check; the row now names the modules).
std::wstring VtableModule(uintptr_t object) noexcept {uintptr_t table{};HMODULE module{};wchar_t name[MAX_PATH]{};
    if(!object || !Read(object,&table,8) || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(table),&module) ||
       !GetModuleFileNameW(module,name,MAX_PATH))return {};
    std::wstring path(name);const auto slash=path.find_last_of(L"\\/");return slash==std::wstring::npos?path:path.substr(slash+1);}
// A D3D11 object: its vtable in a module whose name holds "d3d11" (the runtime or its SDK layers), so GetDesc is safe on it (Reference2116).
bool D3d11Object(uintptr_t object) noexcept {auto module=VtableModule(object);for(auto& c:module)c=static_cast<wchar_t>(towlower(c));return module.find(L"d3d11")!=std::wstring::npos;}
std::string NarrowModule(const std::wstring& w){std::string s;for(const auto c:w)s.push_back(c<128 && c!='"' && c!='\\'?static_cast<char>(c):'?');return s;}
// Reference2131: the GX wrappers receive GX objects (vtables in the executable) holding the D3D11 object: the first D3D11 object among
// the object's first 0x80 bytes (the object itself when it is one) that answers QueryInterface for `iid`, else 0.
uintptr_t D3d11As(uintptr_t object,REFIID iid) noexcept {if(!object)return 0;
    for(uint32_t at=0;at<=0x80;at+=8){uintptr_t inner=object;if(at && (!Read(object+at-8,&inner,8) || !inner))continue;if(!D3d11Object(inner))continue;
        void* found{};HRESULT queried=E_FAIL;const bool outer=guarded_copy_active;guarded_copy_active=true;
        __try{queried=reinterpret_cast<IUnknown*>(inner)->QueryInterface(iid,&found);}__except(EXCEPTION_EXECUTE_HANDLER){queried=E_FAIL;}
        guarded_copy_active=outer;if(FAILED(queried) || !found)continue;
        reinterpret_cast<IUnknown*>(found)->Release();return reinterpret_cast<uintptr_t>(found);}return 0;}
struct Oracle {std::unordered_map<uintptr_t,std::vector<uint8_t>> bytes;std::array<uintptr_t,16> vs_cb{},ps_cb{},ps_samplers{};std::array<uintptr_t,48> ps_srv{};
    uintptr_t vs{},ps{},blend{},depth{},raster{};std::array<float,4> blend_factor{};uint32_t blend_mask{},stencil_ref{};
    uintptr_t blend_by{},depth_by{};std::array<uintptr_t,6> blend_stack{};};   // I-0113 gap 17: who set the blend and the depth (the wrapper's caller, image offset; for the blend, six frames)
thread_local Oracle oracle;uint64_t oracle_draws{},paint_draws{};
// I-0113 gap 7 (the Undead Settlement wheels o001279 lighter natively, WHEEL-STUDY §4): with "!name" in hide-texture.txt, the same
// wrappers inside the mesh loop 63CB0 of a model whose data name holds `name` (in_named): one named_oracle row per draw (the first
// named_rows draws of each name, then sparser: NamedOracleRowKept), the PS constant buffers' bytes, SRVs, samplers and states, against the native's native_named_draw_sources.
// A material's constant buffers may be uploaded outside the loop: while a "!name" is set, every buffer update of 4 KB or less is kept
// by buffer (named_bytes) and read when the loop's own bytes lack it (printed "k:": the buffer's last kept upload, possibly not the
// one the draw reads; a slot absent from a row was not bound inside 63CB0, not unbound).
thread_local bool in_named{};thread_local std::string named_model;std::atomic<bool> named_active{};
// The rows kept per name (I-0113 gap 17: the first 96, enough for a few frames of an effect drawn many times a frame, then every
// 10th up to the 2000th draw, to follow an effect through its whole life; tranche 21, gap 21: then every 41st up to the 24000th,
// about half a minute of the hair, so a warp's fade falls inside the window: tranche 18's 2000 draws ran out before it). 41, prime:
// every 40th aliased with the hair's fixed draws a frame, so a run kept the same few positions of each frame (tranche 21's reviews).
constexpr uint64_t named_rows=96,named_draws_dense=2000,named_draws_followed=24000;
constexpr bool NamedOracleRowKept(uint64_t n) noexcept {return n<=named_rows || (n<=named_draws_dense?n%10==0:n%41==0);}
static_assert(NamedOracleRowKept(96) && !NamedOracleRowKept(97) && NamedOracleRowKept(2000) && !NamedOracleRowKept(2010) && !NamedOracleRowKept(2040) && NamedOracleRowKept(2009+41-2009%41));
std::mutex named_lock;std::unordered_map<uintptr_t,std::vector<uint8_t>> named_bytes;
// I-0113 tranche 34 (gap 23): the uploads made outside a named mesh loop also by (GX context, buffer): a deferred context draws with the
// last update recorded on it, so the « ctx » bytes are the draw's when its context uploaded the buffer in this recording, outside the
// loop (inside it the oracle's own bytes hold them; the map is not reset per command list). At most 16384 pairs, dropped with named_bytes.
std::map<std::pair<uintptr_t,uintptr_t>,std::vector<uint8_t>> named_context_bytes;std::map<std::string,uint64_t> named_draws;
// DECALS-PAINT-READY section 6.1: inside 1AAA70 (the character paint draw, Reference) the same wrappers: in_paint, the paint context
// (its pass id +0x1C), the paint request object (the 0x140-byte kind-1 object) and the decal frame 1AAA70 receives (4 rows).
thread_local bool in_paint{};thread_local uintptr_t paint_context{},paint_object{},paint_frame{};
using W2=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);using WArray=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uintptr_t);
using WUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uintptr_t,uint32_t,uint32_t);
using WBlend=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);using WDepth=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t);
using WDrawIndexed=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,int32_t);
WUpdate update_original{};WArray vs_cb_original{},ps_cb_original{},ps_srv_original{},ps_sampler_original{};W2 vs_original{},ps_original{},raster_original{};
WBlend blend_original{};WDepth depth_original{};WDrawIndexed indexed_original{};
void __fastcall OUpdate(uintptr_t c,uintptr_t s,uintptr_t dst,uint32_t sub,uintptr_t box,uintptr_t src,uint32_t pitch,uint32_t slice) noexcept {
    if(const auto inner=in_decal && src && !box?D3d11As(dst,__uuidof(ID3D11Buffer)):0){D3D11_BUFFER_DESC d{};reinterpret_cast<ID3D11Buffer*>(inner)->GetDesc(&d);
        if(d.ByteWidth && d.ByteWidth<=0x10000){auto& b=oracle.bytes[inner];b.resize(d.ByteWidth);if(!Read(src,b.data(),b.size()))b.clear();}}
    else if(const auto any=named_active.load(std::memory_order_relaxed) && src && !box?D3d11As(dst,__uuidof(ID3D11Buffer)):0){D3D11_BUFFER_DESC d{};reinterpret_cast<ID3D11Buffer*>(any)->GetDesc(&d);
        if(d.ByteWidth && d.ByteWidth<=0x1000){std::vector<uint8_t> b(d.ByteWidth);
            if(Read(src,b.data(),b.size())){const std::lock_guard hold(named_lock);
                if(const std::pair key{c,any};named_context_bytes.size()<16384 || named_context_bytes.contains(key))named_context_bytes[key]=b;
                if(named_bytes.size()<8192 || named_bytes.contains(any))named_bytes[any]=std::move(b);}}}
    update_original(c,s,dst,sub,box,src,pitch,slice);}
void Slots(uintptr_t array,uint32_t start,uint32_t count,uintptr_t* slots,size_t size){for(uint32_t i=0;i<count && start+i<size;++i){uintptr_t o{};if(array)Read(array+8*i,&o,8);slots[start+i]=o;}}
void __fastcall OVsCb(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(in_decal)Slots(p,a,n,oracle.vs_cb.data(),16);vs_cb_original(c,s,a,n,p);}
void __fastcall OPsCb(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(in_decal)Slots(p,a,n,oracle.ps_cb.data(),16);ps_cb_original(c,s,a,n,p);}
void __fastcall OPsSrv(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(in_decal)Slots(p,a,n,oracle.ps_srv.data(),48);ps_srv_original(c,s,a,n,p);}
void __fastcall OPsSampler(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(in_decal)Slots(p,a,n,oracle.ps_samplers.data(),16);ps_sampler_original(c,s,a,n,p);}
void __fastcall OVs(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(in_decal)oracle.vs=p;vs_original(c,s,p);}
void __fastcall OPs(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(in_decal)oracle.ps=p;ps_original(c,s,p);}
void __fastcall ORaster(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(in_decal)oracle.raster=p;raster_original(c,s,p);}
void __fastcall OBlend(uintptr_t c,uintptr_t s,uintptr_t p,uintptr_t factor,uint32_t mask) noexcept {
    if(in_decal){oracle.blend=p;oracle.blend_mask=mask;oracle.blend_by=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;
        if(in_named){void* frames[6]{};const auto depth=RtlCaptureStackBackTrace(1,6,frames,nullptr);for(USHORT i=0;i<6;++i)oracle.blend_stack[i]=i<depth?reinterpret_cast<uintptr_t>(frames[i])-image:0;}if(factor)Read(factor,oracle.blend_factor.data(),16);}blend_original(c,s,p,factor,mask);}
void __fastcall ODepth(uintptr_t c,uintptr_t s,uintptr_t p,uint32_t ref) noexcept {if(in_decal){oracle.depth=p;oracle.stencil_ref=ref;oracle.depth_by=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;}depth_original(c,s,p,ref);}
std::string CbHex(uintptr_t buffer){const auto inner=D3d11As(buffer,__uuidof(ID3D11Buffer));const auto found=oracle.bytes.find(inner);
    if(found!=oracle.bytes.end())return "\""+HexBytes(found->second.data(),found->second.size())+"\"";
    if(in_named && inner){const std::lock_guard hold(named_lock);if(const auto kept=named_bytes.find(inner);kept!=named_bytes.end())return "\"k:"+HexBytes(kept->second.data(),kept->second.size())+"\"";}
    return "null";}
// I-0113 tranche 22 (gap 21: the fade's inputs at the original's draw; tranche 21's rows held b4 only as a kept upload and never b7,
// bound outside 63CB0): at a named row, the device context's own bindings, the GX context's first word being its device context
// (EffectDepthWitness; taken only when its vtable lies outside the game's image and it answers QueryInterface: i113-t22-warp-original
// and -warp2-original found none with D3d11Object's module test, INFERRED a wrapping layer outside d3d11.dll): PS b2 +0x0
// (FC_isDitherAlpha), b4 +0x7C (mulCol.w) and +0x100 (instance 0's ditherFadeAlpha) and b7 +0x20 (FC_DitherEnable), each from the
// bound buffer's last kept upload (the mesh draws are recorded on deferred contexts, i113-t22-warp3-original: no read-back is
// possible there; the last update recorded before the row, INFERRED the draw's bytes); the t38 g_DitherTexture's description
// (SAT_HairIbl2_Fwd_[A] samples it at SV_Position x 0.25 with s8 for its discards) and the s8 sampler.
std::string NamedLiveInputs(uintptr_t context) noexcept {
    try{
    uintptr_t first{},table{};if(!context || !Read(context,&first,8) || !first || !Read(first,&table,8) || !table)return ",\"live\":null";
    static const uintptr_t image_end=[]{const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);   // SizeOfImage, as CurrentSetter
        return image+reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew)->OptionalHeader.SizeOfImage;}();
    if(table>=image && table<image_end)return ",\"live\":{\"ctx\":\"engine_object\"}";   // a game object: its slot 0 is not QueryInterface
    void* found{};HRESULT queried=E_FAIL;const bool outer=guarded_copy_active;guarded_copy_active=true;
    [&]{__try{queried=reinterpret_cast<IUnknown*>(first)->QueryInterface(__uuidof(ID3D11DeviceContext),&found);}__except(EXCEPTION_EXECUTE_HANDLER){queried=E_FAIL;}}();
    guarded_copy_active=outer;
    if(FAILED(queried) || !found)return ",\"live\":{\"ctx\":null,\"first_module\":\""+NarrowModule(VtableModule(first))+"\"}";
    auto* d3d=static_cast<ID3D11DeviceContext*>(found);struct Release {ID3D11DeviceContext* p;~Release(){p->Release();}} release{d3d};
    // A buffer's last kept upload.
    // Tranche 34: the draw's own upload inside this mesh loop first (« loop », this thread's oracle), else the last one recorded on this GX
    // context outside the loop (« ctx »: not reset per command list, so possibly an earlier recording's), else the buffer's last from any
    // context (« kept »); `other` the kept one.
    const auto kept_bytes=[&](ID3D11Buffer* b,std::vector<uint8_t>& out,const char*& how,std::vector<uint8_t>* other=nullptr){out.clear();how="none";if(other)other->clear();if(!b)return;
        const auto at=reinterpret_cast<uintptr_t>(b);
        if(const auto loop=oracle.bytes.find(at);loop!=oracle.bytes.end() && !loop->second.empty()){out=loop->second;how="loop";}   // final review M1: uploaded inside this mesh loop
        const std::lock_guard hold(named_lock);const auto kept=named_bytes.find(at);
        if(other && kept!=named_bytes.end())*other=kept->second;
        if(!out.empty())return;
        if(const auto own=named_context_bytes.find({context,at});own!=named_context_bytes.end()){out=own->second;how="ctx";}
        else if(kept!=named_bytes.end()){out=kept->second;how="kept";}};
    const auto word=[](const std::vector<uint8_t>& v,size_t at){float f{};if(at+4>v.size())return std::string("null");std::memcpy(&f,v.data()+at,4);char c[32]{};sprintf_s(c,"%g",f);return std::string(c);};
    const auto dword=[](const std::vector<uint8_t>& v,size_t at){uint32_t u{};if(at+4>v.size())return std::string("null");std::memcpy(&u,v.data()+at,4);return std::to_string(u);};
    std::array<ID3D11Buffer*,8> cb{};d3d->PSGetConstantBuffers(0,8,cb.data());
    std::string row=",\"live\":{\"ctx\":\"first_word\",\"immediate\":"+std::to_string(int(d3d->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE));std::vector<uint8_t> bytes;const char* how{};
    kept_bytes(cb[2],bytes,how);row+=",\"b2\":{\"by\":\""+std::string(how)+"\",\"is_dither_alpha\":"+dword(bytes,0)+"}";
    std::vector<uint8_t> last;
    kept_bytes(cb[4],bytes,how,&last);row+=",\"b4\":{\"by\":\""+std::string(how)+"\",\"bytes\":"+std::to_string(bytes.size())+",\"mulcol_w\":"+word(bytes,0x7C)+",\"dither_fade_alpha\":"+word(bytes,0x100)+
        ",\"kept_mulcol_w\":"+word(last,0x7C)+",\"kept_dither_fade_alpha\":"+word(last,0x100)+"}";
    kept_bytes(cb[7],bytes,how,&last);row+=",\"b7\":{\"by\":\""+std::string(how)+"\",\"dither_enable\":"+word(bytes,0x20)+",\"kept_dither_enable\":"+word(last,0x20)+"}";
    // I-0113 tranche 31 (the portrait's hair, SAT_HairIbl2): b3, b4 and b5 bound at the draw, each with its last kept upload whole
    // (b4's SH rows zero natively; b5 the light set; neither caught inside the mesh loop in tranche 30), the buffer named by address.
    for(const uint32_t slot:{3u,4u,5u}){kept_bytes(cb[slot],bytes,how);char head[96]{};
        sprintf_s(head,",\"b%u_live\":{\"id\":%llu,\"by\":\"%s\",\"hex\":",slot,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(cb[slot])),how);
        row+=head+(bytes.empty()?std::string("null"):"\""+HexBytes(bytes.data(),bytes.size())+"\"")+"}";}
    for(auto* b:cb)if(b)b->Release();
    ID3D11ShaderResourceView* srv{};d3d->PSGetShaderResources(38,1,&srv);std::string t38="null";
    if(srv){ID3D11Resource* r{};srv->GetResource(&r);D3D11_RESOURCE_DIMENSION k{};if(r)r->GetType(&k);char c[160]{};
        if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC d{};static_cast<ID3D11Texture2D*>(r)->GetDesc(&d);
            sprintf_s(c,"{\"id\":%llu,\"w\":%u,\"h\":%u,\"f\":%u,\"mips\":%u,\"array\":%u}",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(r)),d.Width,d.Height,static_cast<unsigned>(d.Format),d.MipLevels,d.ArraySize);}
        else sprintf_s(c,"{\"kind\":%d}",int(k));
        t38=c;if(r)r->Release();srv->Release();}
    row+=",\"t38\":"+t38;
    // Tranche 31: t33 (g_SSAOTexture, SYSTEX_WHITE natively) bound at the draw, its description and address.
    {ID3D11ShaderResourceView* v33{};d3d->PSGetShaderResources(33,1,&v33);std::string t33="null";
        if(v33){ID3D11Resource* r{};v33->GetResource(&r);D3D11_RESOURCE_DIMENSION k{};if(r)r->GetType(&k);char c[160]{};
            if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC d{};static_cast<ID3D11Texture2D*>(r)->GetDesc(&d);
                sprintf_s(c,"{\"id\":%llu,\"w\":%u,\"h\":%u,\"f\":%u,\"mips\":%u,\"array\":%u}",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(r)),d.Width,d.Height,static_cast<unsigned>(d.Format),d.MipLevels,d.ArraySize);}
            else sprintf_s(c,"{\"kind\":%d}",int(k));
            t33=c;if(r)r->Release();v33->Release();}
        row+=",\"t33\":"+t33;}
    // Tranche 31b: the bound render target's size (the Quit Game portrait's 512x256 view told from the world's draws of the same model).
    {ID3D11RenderTargetView* rtv{};d3d->OMGetRenderTargets(1,&rtv,nullptr);std::string target="null";
        if(rtv){ID3D11Resource* r{};rtv->GetResource(&r);D3D11_RESOURCE_DIMENSION k{};if(r)r->GetType(&k);
            if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC d{};static_cast<ID3D11Texture2D*>(r)->GetDesc(&d);char c[64]{};sprintf_s(c,"[%u,%u,%u]",d.Width,d.Height,static_cast<unsigned>(d.Format));target=c;}
            if(r)r->Release();rtv->Release();}
        row+=",\"target\":"+target;}
    ID3D11SamplerState* sampler{};d3d->PSGetSamplers(8,1,&sampler);
    if(sampler){D3D11_SAMPLER_DESC d{};sampler->GetDesc(&d);char c[96]{};sprintf_s(c,",\"s8\":[%u,%u,%u,%u]",static_cast<unsigned>(d.Filter),static_cast<unsigned>(d.AddressU),static_cast<unsigned>(d.AddressV),d.MaxAnisotropy);row+=c;sampler->Release();}
    return row+"}";
    }catch(...){return ",\"live\":\"error\"";}}
// One oracle row per draw (DrawIndexed, or DrawIndexedInstanced: the world's mesh draws, I-0113 i113-s9-wheel-original).
void OracleRow(uint32_t count,uint32_t start,int32_t base,uint32_t instances,uintptr_t context) noexcept {
    // I-0113 tranche 7 (zero-census.flag): one compact row per (model, PS, index count), at most 8000: the PS's DXBC checksum and
    // its b2, b3 and b7 bytes (b4 is read where 5B3D0 writes it: reference_zero_instance).
    static const bool zero_census=ModuleFlag(L"zero-census.flag");
    if(zero_census && in_named){ReferencePs code{};if(const auto object=D3d11As(oracle.ps,__uuidof(ID3D11PixelShader))){const std::lock_guard hold(reference_ps_lock);
            if(const auto f=reference_ps.find(reinterpret_cast<ID3D11PixelShader*>(object));f!=reference_ps.end())code=f->second;}
        static std::mutex census_lock;static std::set<std::tuple<std::string,uint64_t,uint32_t>> seen;bool fresh{};
        {const std::lock_guard hold(census_lock);fresh=code.bytes && seen.size()<8000 && seen.emplace(named_model,code.checksum,count).second;}
        if(fresh){char cell[200]{};sprintf_s(cell,"\",\"ps_dxbc\":\"%016llX\",\"count\":%u,\"start\":%u,\"instances\":%u",
                static_cast<unsigned long long>(_byteswap_uint64(code.checksum)),count,start,instances);
            std::string row="{\"event\":\"reference_zero_census\",\"model\":\""+named_model.substr(0,80)+cell;
            for(const uint32_t slot:{2u,3u,7u})row+=",\"ps_cb"+std::to_string(slot)+"\":"+CbHex(oracle.ps_cb[slot]);
            Log((row+"}\n").c_str());}
        return;}
    if(in_decal){uint64_t n{};{const std::lock_guard hold(lock);n=in_named?++named_draws[named_model]:in_paint?++paint_draws:++oracle_draws;}
        if(in_named?NamedOracleRowKept(n):in_paint?(n<=64 || n%400==0):(n<=8 || n%600==0)){
            std::string row="{\"event\":\""+std::string(in_named?"named_oracle":in_paint?"paint_oracle":"decal_oracle")+"\",\"n\":"+std::to_string(n)+",\"count\":"+std::to_string(count)+",\"start\":"+std::to_string(start)+",\"base\":"+std::to_string(base);
            if(in_named){ReferencePs code{};if(const auto object=D3d11As(oracle.ps,__uuidof(ID3D11PixelShader))){const std::lock_guard hold(reference_ps_lock);
                    if(const auto f=reference_ps.find(reinterpret_cast<ID3D11PixelShader*>(object));f!=reference_ps.end())code=f->second;}
                // I-0113 gap 17: the draw's time in microseconds (QPC), to tell a model's draws within one frame from the next frame's.
                LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
                char cell[360]{};sprintf_s(cell,",\"model\":\"%.80s\",\"ps\":\"%llX\",\"ps_dxbc\":\"%016llX\",\"ps_bytes\":%zu,\"instances\":%u,\"blend_by\":\"%llX\",\"depth_by\":\"%llX\",\"us\":%lld",named_model.c_str(),static_cast<unsigned long long>(oracle.ps),
                    static_cast<unsigned long long>(_byteswap_uint64(code.checksum)),code.bytes,instances,static_cast<unsigned long long>(oracle.blend_by),
                    static_cast<unsigned long long>(oracle.depth_by),frequency.QuadPart?now.QuadPart/frequency.QuadPart*1000000+now.QuadPart%frequency.QuadPart*1000000/frequency.QuadPart:0ll);row+=cell;
                row+=",\"blend_stack\":[";for(size_t i=0;i<oracle.blend_stack.size();++i){char at[24]{};sprintf_s(at,"%s\"%llX\"",i?",":"",static_cast<unsigned long long>(oracle.blend_stack[i]));row+=at;}row+="]";}
            // programmes: VS and PS handles to table indices (the PS's VS of the Am family is the base one)
            int vs_index=-1,ps_index=-1;for(int i=0;i<47 && oracle_manager;++i){uintptr_t object{},h{};if(Read(oracle_manager+0x98+8*i,&object,8) && object && Read(object+0x10,&h,8)){
                if(h==oracle.vs && vs_index<0)vs_index=i;if(h==oracle.ps && ps_index<0)ps_index=i;}}
            // GX shader objects keep both stages? the VS handle may be another object's +0x18: also tried
            if(vs_index<0)for(int i=0;i<47 && oracle_manager;++i){uintptr_t object{},h{};if(Read(oracle_manager+0x98+8*i,&object,8) && object && Read(object+0x18,&h,8) && h==oracle.vs){vs_index=i;break;}}
            if(ps_index<0)for(int i=0;i<47 && oracle_manager;++i){uintptr_t object{},h{};if(Read(oracle_manager+0x98+8*i,&object,8) && object && Read(object+0x18,&h,8) && h==oracle.ps){ps_index=i;break;}}
            row+=",\"vs_index\":"+std::to_string(vs_index)+",\"ps_index\":"+std::to_string(ps_index);
            row+=",\"modules\":[\""+NarrowModule(VtableModule(oracle.vs_cb[10]))+"\",\""+NarrowModule(VtableModule(oracle.ps_srv[0]))+"\",\""+NarrowModule(VtableModule(oracle.blend))+"\",\""+
                NarrowModule(VtableModule(oracle.ps))+"\"]";
            row+=",\"vs_cb10\":"+CbHex(oracle.vs_cb[10]);
            if(in_paint){int pass=-1;if(paint_context)Read(paint_context+0x1C,&pass,4);row+=",\"pass\":"+std::to_string(pass);
                for(const uint32_t slot:{0u,4u,5u,8u})row+=",\"vs_cb"+std::to_string(slot)+"\":"+CbHex(oracle.vs_cb[slot]);}

            if(in_named)for(uint32_t slot=0;slot<14;++slot){row+=",\"ps_cb"+std::to_string(slot)+"\":"+CbHex(oracle.ps_cb[slot]);}
            else for(const uint32_t slot:{2u,3u,4u,7u,8u,9u,10u,11u}){row+=",\"ps_cb"+std::to_string(slot)+"\":"+(!in_paint && (slot==3 || slot==4 || slot==8 || slot==9)?std::string(oracle.ps_cb[slot]?"\"bound\"":"null"):CbHex(oracle.ps_cb[slot]));}
            row+=",\"srv\":[";for(size_t i=0;i<(in_paint || in_named?size_t(48):size_t(18));++i){std::string cell="null";if(const auto v=D3d11As(oracle.ps_srv[i],__uuidof(ID3D11ShaderResourceView));oracle.ps_srv[i] && !v)cell="\"foreign\"";else if(v){ID3D11Resource* r{};reinterpret_cast<ID3D11ShaderResourceView*>(v)->GetResource(&r);
                    if(r){ID3D11Texture2D* t{};if(SUCCEEDED(r->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t))) && t){D3D11_TEXTURE2D_DESC d{};t->GetDesc(&d);char c2[96]{};
                            // I-0113 tranche 30: a 1x1 texture's resource named too (@address), against reference_unit_cube's (which of the three cubes).
                            if(d.Width==1 && d.Height==1)sprintf_s(c2,"\"%ux%u:f%u:m%u:a%u@%llX\"",d.Width,d.Height,static_cast<unsigned>(d.Format),d.MipLevels,d.ArraySize,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(t)));
                            else sprintf_s(c2,"\"%ux%u:f%u:m%u:a%u\"",d.Width,d.Height,static_cast<unsigned>(d.Format),d.MipLevels,d.ArraySize);cell=c2;t->Release();}else cell="\"other\"";r->Release();}}
                row+=(i?",":"")+cell;}
            row+="],\"samplers\":[";for(size_t i=0;i<oracle.ps_samplers.size();++i){std::string cell="null";if(const auto v=D3d11As(oracle.ps_samplers[i],__uuidof(ID3D11SamplerState));oracle.ps_samplers[i] && !v)cell="\"foreign\"";else if(v){D3D11_SAMPLER_DESC d{};reinterpret_cast<ID3D11SamplerState*>(v)->GetDesc(&d);char c2[96]{};
                    sprintf_s(c2,"[%u,%u,%u,%u,%u]",static_cast<unsigned>(d.Filter),static_cast<unsigned>(d.AddressU),static_cast<unsigned>(d.AddressV),static_cast<unsigned>(d.AddressW),d.MaxAnisotropy);cell=c2;}row+=(i?",":"")+cell;}
            row+="]";
            if(const auto blend=D3d11As(oracle.blend,__uuidof(ID3D11BlendState))){D3D11_BLEND_DESC d{};reinterpret_cast<ID3D11BlendState*>(blend)->GetDesc(&d);row+=",\"blend\":{\"a2c\":"+std::to_string(d.AlphaToCoverageEnable)+",\"independent\":"+std::to_string(d.IndependentBlendEnable)+",\"rt\":[";
                for(int i=0;i<8;++i){const auto& t=d.RenderTarget[i];char c2[80]{};sprintf_s(c2,"%s[%u,%u,%u,%u,%u,%u,%u,%u]",i?",":"",t.BlendEnable,t.SrcBlend,t.DestBlend,t.BlendOp,t.SrcBlendAlpha,t.DestBlendAlpha,t.BlendOpAlpha,t.RenderTargetWriteMask);row+=c2;}
                char c3[96]{};sprintf_s(c3,"],\"mask\":\"%X\",\"factor\":[%g,%g,%g,%g]}",oracle.blend_mask,oracle.blend_factor[0],oracle.blend_factor[1],oracle.blend_factor[2],oracle.blend_factor[3]);row+=c3;}
            if(const auto depth=D3d11As(oracle.depth,__uuidof(ID3D11DepthStencilState))){D3D11_DEPTH_STENCIL_DESC d{};reinterpret_cast<ID3D11DepthStencilState*>(depth)->GetDesc(&d);char c2[256]{};
                sprintf_s(c2,",\"depth\":[%u,%u,%u,%u,%u,%u,[%u,%u,%u,%u],[%u,%u,%u,%u]],\"stencil_ref\":%u",d.DepthEnable,d.DepthWriteMask,d.DepthFunc,d.StencilEnable,d.StencilReadMask,d.StencilWriteMask,
                    d.FrontFace.StencilFailOp,d.FrontFace.StencilDepthFailOp,d.FrontFace.StencilPassOp,d.FrontFace.StencilFunc,d.BackFace.StencilFailOp,d.BackFace.StencilDepthFailOp,d.BackFace.StencilPassOp,d.BackFace.StencilFunc,oracle.stencil_ref);row+=c2;}
            if(const auto raster=D3d11As(oracle.raster,__uuidof(ID3D11RasterizerState))){D3D11_RASTERIZER_DESC d{};reinterpret_cast<ID3D11RasterizerState*>(raster)->GetDesc(&d);char c2[160]{};
                sprintf_s(c2,",\"raster\":[%u,%u,%u,%d,%g,%g,%u,%u,%u,%u]",d.FillMode,d.CullMode,d.FrontCounterClockwise,d.DepthBias,d.DepthBiasClamp,d.SlopeScaledDepthBias,d.DepthClipEnable,d.ScissorEnable,d.MultisampleEnable,d.AntialiasedLineEnable);row+=c2;}
            // The inputs of the native producer's test: the decal object (0x420 bytes), the view's first 0x290 bytes and +0x450..+0x470,
            // the manager's +0x400..+0x430 (fade and angle settings) and the batch's 16 bytes.
            if(in_paint){std::array<uint8_t,0x140> object{};std::array<uint8_t,0x40> frame{};Read(paint_object,object.data(),object.size());if(paint_frame)Read(paint_frame,frame.data(),frame.size());
                row+=",\"paint_object\":\""+HexBytes(object.data(),object.size())+"\",\"frame\":\""+HexBytes(frame.data(),frame.size())+"\"";}
            else if(!in_named){std::array<uint8_t,0x420> object{};std::array<uint8_t,0x290> view{};std::array<uint8_t,0x30> manager{};std::array<uint8_t,16> batch{};
                Read(oracle_decal,object.data(),object.size());Read(oracle_view,view.data(),view.size());Read(oracle_manager+0x400,manager.data(),manager.size());if(oracle_batch)Read(oracle_batch,batch.data(),batch.size());
                row+=",\"decal_bytes\":\""+HexBytes(object.data(),object.size())+"\",\"view_bytes\":\""+HexBytes(view.data(),view.size())+"\",\"manager_400\":\""+HexBytes(manager.data(),manager.size())+"\",\"batch\":\""+HexBytes(batch.data(),batch.size())+"\"";}
            if(in_named)row+=NamedLiveInputs(context);
            Log((row+"}\n").c_str());}}
}
void __fastcall ODrawIndexed(uintptr_t c,uintptr_t s,uint32_t count,uint32_t start,int32_t base) noexcept {OracleRow(count,start,base,0,c);indexed_original(c,s,count,start,base);}
// 19213F0: the GX DrawIndexedInstanced wrapper (ID3D11DeviceContext vt+0xA0): index count, instances, start index, base vertex, first instance.
using WDrawIndexedInstanced=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uint32_t,int32_t,uint32_t);WDrawIndexedInstanced indexed_instanced_original{};
void __fastcall ODrawIndexedInstanced(uintptr_t c,uintptr_t s,uint32_t count,uint32_t instances,uint32_t start,int32_t base,uint32_t first) noexcept {
    if(in_named)OracleRow(count,start,base,instances,c);indexed_instanced_original(c,s,count,instances,start,base,first);}
// 198910 (the per-decal draw): counted, the first draw of each kind (blend, cubic, blood, bump, POM, weight, textures) logged.
void __fastcall DecalDrawObserved(uintptr_t drawer,uintptr_t manager,uintptr_t view,int zero,uintptr_t decal,uintptr_t batch) noexcept {
    const auto row=DecalObjectRow(decal);bool fresh{};{const std::lock_guard hold(lock);++decal_draws;
        const auto key=row.substr(row.find("\"blend\""),row.find(",\"material\"")-row.find("\"blend\""))+row.substr(row.find("\"texture_slots\""),24);fresh=++kinds[key]==1 && kinds.size()<=64;}
    if(fresh){uint8_t emissive_batch{};if(batch)Read(batch+8,&emissive_batch,1);
        Log(("{\"event\":\"decal_observe_draw\",\"n\":"+std::to_string(decal_draws)+",\"batch_emissive\":"+std::to_string(emissive_batch)+",\"decal\":"+row+"}\n").c_str());}
    in_decal=true;oracle_manager=manager;oracle_decal=decal;oracle_view=view;oracle_batch=batch;decal_draw_original(drawer,manager,view,zero,decal,batch);in_decal=false;}
// 51F80 (a character's GXBloodyMaterialPatch, from E000E0): the paint flag (param_4) and the model flags +0x224.
uintptr_t __fastcall PatchCreateObserved(uintptr_t manager,uintptr_t model,uintptr_t model_data,uint8_t paint) noexcept {
    const auto patch=patch_original(manager,model,model_data,paint);uint32_t flags{};if(model_data)Read(model_data+0x224,&flags,4);
    uint64_t n{};{const std::lock_guard hold(lock);n=++paint_patches;}
    if(n<=32 || n%100==0){char row[200]{};sprintf_s(row,"{\"event\":\"decal_observe_patch\",\"n\":%llu,\"paint\":%u,\"model_flags\":\"%X\",\"patch\":%s}\n",static_cast<unsigned long long>(n),paint,flags,patch?"true":"false");Log(row);}
    return patch;}
// 4CD70 (GXPaintRenderTarget clear): counted, logged every 50th.
void __fastcall PaintClearObserved(uintptr_t target,uintptr_t context) noexcept {
    uint64_t n{};{const std::lock_guard hold(lock);n=++paint_clears;}
    if(n<=8 || n%50==0){char row[96]{};sprintf_s(row,"{\"event\":\"decal_observe_paint_clear\",\"n\":%llu}\n",static_cast<unsigned long long>(n));Log(row);}
    clear_original(target,context);}
// 1AAA70 (the character paint draw, Reference; DECALS-PAINT-READY section 6.1): the oracle wrappers record its depth and colour
// passes (paint_oracle rows). param_1 is 5FCD0's paint record: +0x28 = the kind-1 object's +0x60 (its descriptor's d+0x48).
using PaintDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);PaintDraw paint_draw_original{};
void __fastcall PaintDrawObserved(uintptr_t record,uintptr_t context,uintptr_t draw,uintptr_t frame) noexcept {
    uintptr_t slots{};Read(record+0x28,&slots,8);
    const bool outer=!in_decal;if(outer){in_decal=true;in_paint=true;paint_context=context;paint_object=slots?slots-0x60:0;paint_frame=frame;oracle_manager=0;}
    paint_draw_original(record,context,draw,frame);
    if(outer){in_decal=false;in_paint=false;}}
// 63CB0 (the FLVER mesh loop: the world's draw items 5CEC0 and 5D5C0, and 636F0's): table, model, material table, args and four
// stack words, passed through. With "!name" in hide-texture.txt and a model whose data name holds it: the named oracle (in_named).
using MeshLoop=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);MeshLoop mesh_loop_original{};
void __fastcall MeshLoopObserved(uintptr_t table,uintptr_t model,uintptr_t material_table,uintptr_t args,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8) noexcept {
    bool named{};
    // I-0113 tranche 7 (ZERO-CONSTANTS-CENSUS §5, zero-census.flag): every model's mesh loop is named, its draws described once per
    // (model, PS, index count) by OracleRow (reference_zero_census); the uploads are kept as for "!name".
    static const bool zero_census=ModuleFlag(L"zero-census.flag");
    if(zero_census && !in_decal && model){named_active.store(true,std::memory_order_relaxed);std::string clean;
        for(const auto ch:NativeModelDataName(model))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
        named=!clean.empty();if(named)named_model=std::move(clean);}
    else if(!in_decal && model)try{const auto part=chair_hunt::Fragment();const bool active=!part.empty() && part[0]==L'!';
        // Step review L2: the kept bytes (at most 8192 buffers of 4 KB) are dropped when "!name" goes, so a reused address never
        // carries an earlier upload into the next one.
        if(named_active.exchange(active,std::memory_order_relaxed) && !active){const std::lock_guard hold(named_lock);named_bytes.clear();named_context_bytes.clear();}
        if(active && chair_hunt::Matches(part,model)){std::string clean;for(const auto ch:NativeModelDataName(model))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
            const std::lock_guard hold(lock);named=(named_draws.contains(clean) || named_draws.size()<16) && named_draws[clean]<named_draws_followed;if(named)named_model=std::move(clean);}}
        catch(...){named=false;}
    if(named){in_decal=true;in_named=true;oracle=Oracle{};oracle_manager=0;}
    // I-0113 tranche 21 (gap 21: which of the hair's meshes 63CB0 keeps for each row during a warp's fade): its mesh filter, a mesh
    // kept when its entry's +0x10 flags hold the bits the args' +0x180 asks (bit 1 asks the mesh's bit 1, bit 2 its bit 0), its LOD
    // bit (+0x14) set at the args' +0x18 (the requested LOD; 63CB0 lowers it to the model's highest loaded one below, model+0xB4) and
    // its material (+0x0) answering the args' masks +0x30, +0x38, +0x40, +0x44 (63CB0.c:53-104). Once per (name, +0x180, +0x18C, shader
    // pass +0x1C, requested LOD), at most 160 rows: those words, the loaded LOD mask and each mesh entry's +0x0, +0x8, +0x10 and +0x14
    // (the model's [0x14] entries of 0x48 bytes at [0x15]; the first 64, "more" past them). The material half of the test (the material's own
    // words +0x8, +0x10, +0x18, +0x1C against those masks) is not read here: named_oracle's index counts show which meshes drew. The
    // 160 keys are shared by every name (the fix review's L2): a run names one model at a time.
    if(named && !zero_census){uint32_t flags180{},pass{},lod{},flags18c{},lod_mask{},mask40{},mask44{};uint64_t mask30{},mask38{};
        Read(args+0x180,&flags180,4);Read(args+0x1C,&pass,4);Read(args+0x18,&lod,4);Read(args+0x18C,&flags18c,4);Read(model+0xB4,&lod_mask,4);
        Read(args+0x30,&mask30,8);Read(args+0x38,&mask38,8);Read(args+0x40,&mask40,4);Read(args+0x44,&mask44,4);
        static std::mutex filter_lock;static std::set<std::tuple<std::string,uint32_t,uint32_t,uint32_t,uint32_t>> filter_seen;bool fresh{};
        {const std::lock_guard hold(filter_lock);fresh=filter_seen.size()<160 && filter_seen.emplace(named_model,flags180,flags18c,pass,lod).second;}
        if(fresh){int32_t count{};uintptr_t meshes{};Read(model+0xA0,&count,4);Read(model+0xA8,&meshes,8);char head[200]{};
            sprintf_s(head,",\"lod_mask\":%u,\"masks\":[\"%llX\",\"%llX\",\"%X\",\"%X\"]",lod_mask,static_cast<unsigned long long>(mask30),static_cast<unsigned long long>(mask38),mask40,mask44);
            std::string row="{\"event\":\"reference_mesh_loop_filter\",\"model\":\""+named_model.substr(0,80)+"\",\"flags180\":"+std::to_string(flags180)+",\"shader_pass\":"+std::to_string(pass)+
                ",\"requested_lod\":"+std::to_string(lod)+",\"flags18c\":"+std::to_string(flags18c)+head+",\"meshes\":[";
            for(int32_t i=0;i<count && i<64 && meshes;++i){uint16_t material{},word14{};int32_t word8{};uint32_t word10{};const uintptr_t e=meshes+0x48*static_cast<uintptr_t>(i);
                Read(e,&material,2);Read(e+8,&word8,4);Read(e+0x10,&word10,4);Read(e+0x14,&word14,2);char cell[72]{};
                sprintf_s(cell,"%s[%u,%d,%u,%u]",i?",":"",static_cast<unsigned>(material),word8,word10,static_cast<unsigned>(word14));row+=cell;}
            Log((row+"]"+(count>64?",\"more\":true":"")+"}\n").c_str());}}
    mesh_loop_original(table,model,material_table,args,a5,a6,a7,a8);
    if(named){in_decal=false;in_named=false;}}
}
// FFX draw N5 (FFX-DRAW §2, Reference runs with ffx-draw-observe.flag): the original's effect draw side, to size and check the
// native record layer. 24E790 (the scene-control draw, sc and the GX pass context): per call the pass context's fields the
// recording reads (+0x124 pass, +0x410 frustum vtable, +0x418 camera and its +0x30..+0x5C, +0x250..+0x25C the sort row,
// +0x4B8/+0x4BC masks, +0x425, +0x426.. the reversal table, +0x4C0..+0x4F0, +0x58C) for the first call of each pass; the commands
// BF4C0 records inside it, the callbacks 299E40 executed and, inside a callback, the GX topology (141920420), Draw
// (141921370) and DrawIndexed (141921390) calls, counted per pass and logged every 3000th call of 24E790 (with its calls per pass).
// The GX effect type registry [1449651D0] (0x48-byte entries, the type in the first word, the count at [1449651D8]): the
// types and their entries' words +0x08..+0x18 (constructor slots), for comparing both modes (FFX-DRAW §2).
std::string FfxTypeRegistry() {
    uintptr_t table{};uint64_t count{};Read(image+0x49651D0,&table,8);Read(image+0x49651D8,&count,8);std::string out;
    for(uint64_t i=0;table && i<count && i<512;++i){uint16_t type{};std::array<uintptr_t,3> words{};if(!Read(table+0x48*i,&type,2))break;Read(table+0x48*i+8,words.data(),24);
        char cell[96]{};sprintf_s(cell,"%s\"%X\":[\"%llX\",\"%llX\",\"%llX\"]",out.empty()?"":",",type,
            static_cast<unsigned long long>(words[0]>image?words[0]-image:words[0]),static_cast<unsigned long long>(words[1]>image?words[1]-image:words[1]),static_cast<unsigned long long>(words[2]>image?words[2]-image:words[2]));out+=cell;}
    return "{\"count\":"+std::to_string(count)+",\"types\":{"+out+"}}";
}
// Witness helper (FFX-DRAW §2): per drawable node, its renderer +0x58's vtable, or for a node without one its +0x68 object's vtable.
void FfxNodeKinds(uintptr_t node,std::map<std::string,uint64_t>& kinds){uintptr_t n58{},n68{},vt{};Read(node+0x58,&n58,8);Read(node+0x68,&n68,8);
    char key[48]{};if(n58 && Read(n58,&vt,8))sprintf_s(key,"r%llX",static_cast<unsigned long long>(vt-image));else if(n68 && Read(n68,&vt,8))sprintf_s(key,"x%llX",static_cast<unsigned long long>(vt-image));else sprintf_s(key,"none");++kinds[key];}
std::string FfxKinds(const std::map<std::string,uint64_t>& kinds){std::string s;for(const auto& [k,v]:kinds){char c[64]{};sprintf_s(c,"%s\"%s\":%llu",s.empty()?"":",",k.c_str(),static_cast<unsigned long long>(v));s+=c;}return "{"+s+"}";}
namespace native_ffx {struct DrawGate{double ms{-1e9};int offers{},nodes{};};DrawGate DrawGateEnter(uintptr_t sc,uintptr_t pass) noexcept;void DrawGateLeave(const DrawGate& gate,uintptr_t sc,uintptr_t pass) noexcept;}   // I-0141 tranche 14, 21_effects.inc
namespace ffx_draw_observe {
// I-0141: a range of memory as hex words (the frustum and camera objects of the effect pass, F9 rows).
std::string HexWords(uintptr_t at,size_t bytes) {std::string s;s.reserve(bytes*2);char w[16]{};for(size_t i=0;i+4<=bytes;i+=4){uint32_t x{};Read(at+i,&x,4);sprintf_s(w,"%08X",x);s+=w;}return s;}
std::atomic<int> frustum_rows{};   // I-0141: F9 asks the effect pass's frustum and camera objects whole (reference_ffx_frustum, native_ffx_frustum)
std::atomic<int> camera_rows{};   // I-0141: F9 (Slot27Observed) asks the next calls' effect camera (reference_ffx_camera, native_ffx_camera)
thread_local int inside_draw{},inside_callback{},callback_pass{};thread_local uint32_t callback_key{};thread_local uintptr_t callback_class{};
// I-0113 gap 5: the last 16 FFX callbacks this thread ran (the pass context, engine pass, key, renderer class and the blends each set),
// printed with the E03 states witness: which commands precede a model particle on its recording, against the native's order.
struct CallbackEntry {uintptr_t pass_context{};int pass{};uint32_t key{};uintptr_t renderer{};uint32_t blends{};};
thread_local std::array<CallbackEntry,16> callback_ring{};thread_local uint64_t callback_count{};
// PASS-CHUNKS-STUDY §5-6: 14013EEB0 runs one chunk (executor, pass context, entries [from, to), flags) from a fresh state; for the
// scene view's sub 0x29 (pass 0x45) its range, count, first and last keys (low 16 bits) and its entries, 120 rows from the first E03 model
// drawn (e03_seen), against the native's native_effect_chunks; since tranche 5 120 more of the chunks around the last flask key
// drawn (e03_key, within 16), for its later particles (i113-h2-flask-native: keys 59556-59558).
using ChunkRun=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uint32_t);ChunkRun chunk_run_original{};
std::atomic<bool> e03_seen{};std::atomic<int> chunk_rows{},chunk_rows_near{};std::atomic<uint32_t> e03_key{};
void __fastcall ChunkRunObserved(uintptr_t executor,uintptr_t pass,uint32_t from,uint32_t to,uint32_t flags) noexcept {
    if(e03_seen.load(std::memory_order_relaxed) && to>from)try{
        uintptr_t entries{};uint32_t total{},first{},last{};Read(executor+8,&entries,8);Read(executor+0x10,&total,4);
        const auto end=std::min(to,total);const uint32_t around=e03_key.load(std::memory_order_relaxed);
        const auto take=[&]{if(chunk_rows.load(std::memory_order_relaxed)<120)return chunk_rows.fetch_add(1)<120;
            return around && (first&0xFFFF)<=around+16 && (last&0xFFFF)+16>=around && chunk_rows_near.fetch_add(1)<120;};
        if(entries && end>from && Read(entries+0x18ull*from,&first,4) && Read(entries+0x18ull*(end-1),&last,4) &&
           (first>>22&0x7F)==0x45 && (first>>16&0x3F)==0x29 && take()){char row[200]{};
            sprintf_s(row,"{\"event\":\"reference_effect_chunk\",\"thread\":%lu,\"from\":%u,\"to\":%u,\"count\":%u,\"first\":%u,\"last\":%u,\"top\":\"%X\",\"entries\":[",
                GetCurrentThreadId(),from,end,end-from,first&0xFFFF,last&0xFFFF,first>>16);std::string out(row);
            // Each entry: its key's top 16 bits and order, its first command's type and, for a type-4 command (a callback), the
            // callback's RVA: which entries of the segment are not the FFX callbacks 299E40 the native records.
            for(uint32_t i=from;i<end && i<from+24;++i){uint32_t key{};uintptr_t command{};uint8_t head[2]{};uintptr_t callback{};
                Read(entries+0x18ull*i,&key,4);Read(entries+0x18ull*i+8,&command,8);if(command)Read(command,head,2);
                if(command && head[0]==4)Read(command+4+head[1],&callback,8);
                char cell[80]{};sprintf_s(cell,"%s[\"%X\",%u,%u,\"%llX\"]",i>from?",":"",key>>16,key&0xFFFF,head[0],static_cast<unsigned long long>(callback>image?callback-image:callback));out+=cell;}
            out+="]}\n";Log(out.c_str());}}catch(...){}
    chunk_run_original(executor,pass,from,to,flags);}
std::mutex lock;   // the counters below
std::map<int,uint64_t> scene_calls,recorded,executed,draws,indexed;std::map<std::pair<int,int>,uint64_t> topologies;
uint64_t frames{},calls{};
using Draw=void(__fastcall*)(uintptr_t,uintptr_t);using Alloc=uintptr_t(__fastcall*)(uintptr_t,uint32_t,uint32_t);
using Callback=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);using Topology=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t);
using DrawCount=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t);using DrawIndexed=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uint32_t);
Draw draw_original{};Alloc alloc_original{};Callback callback_original{};Topology topology_original{};DrawCount count_original{};DrawIndexed indexed_original{};
std::string Counts(const std::map<int,uint64_t>& m){std::string s;for(const auto& [k,v]:m){char c[48]{};sprintf_s(c,"%s\"%X\":%llu",s.empty()?"":",",k,static_cast<unsigned long long>(v));s+=c;}return "{"+s+"}";}
// The update and proceed job bodies (296FF0, 296D80) of the original: the item kinds (vt+8) they run, for comparison with the
// native ItemKinds (FFX-DRAW §2). Logged with the draw counts.
using JobBody=void(__fastcall*)(uintptr_t,const int32_t*);JobBody update_body_original{},proceed_body_original{};
std::map<uint32_t,uint64_t> update_kinds,proceed_kinds;
void __fastcall UpdateBody(uintptr_t pass,const int32_t* range) noexcept {
    {const std::lock_guard hold(lock);uintptr_t items{};Read(pass+0x2D30,&items,8);
        for(int32_t i=range[0];items && i<range[1];++i){uintptr_t item{},vt{},slot{};if(Read(items+0x10*i,&item,8) && item && Read(item,&vt,8) && Read(vt+8,&slot,8))++update_kinds[static_cast<uint32_t>(slot-image)];}}
    update_body_original(pass,range);}
void __fastcall ProceedBody(uintptr_t pass,const int32_t* range) noexcept {
    {const std::lock_guard hold(lock);uintptr_t table{};Read(pass+0x98,&table,8);
        for(int32_t i=range[0];table && i<range[1];++i){uintptr_t head{},link{};if(!Read(table+8*i,&head,8) || !head || !Read(head,&link,8))continue;
            for(int guard=0;link && guard<100000;++guard){uintptr_t item{},vt{},slot{};if(Read(link,&item,8) && item && Read(item,&vt,8) && Read(vt+8,&slot,8))++proceed_kinds[static_cast<uint32_t>(slot-image)];
                if(!Read(link+0x10,&link,8))break;}}}
    proceed_body_original(pass,range);}
// The GX common state bank [[4961AB0]+0x2A0] (FUN_1400D21F0) as the D3D11 objects describe themselves: for each 8-byte slot
// +0x08..+0x2F8, the wrapper's object [slot]+0 classified by QueryInterface (only when its vtable's first entry lies in
// d3d11.dll) and its GetDesc, one row per slot. Once, Reference (the descriptors the native effect PSOs take, FFX-DRAW §2).
// E07 witness (Reference): the GX UAV creation 1925B10(device, out, buffer, description, ...): the first 60 calls, the description's
// eight words and, once created, the D3D11 view's own description (its Buffer.Flags: APPEND 2, COUNTER 4).
using UavCreate=uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);UavCreate uav_create_original{};
uint32_t __fastcall UavCreated(uintptr_t device,uintptr_t out,uintptr_t buffer,uintptr_t description,uintptr_t a5) noexcept {
    const auto result=uav_create_original(device,out,buffer,description,a5);
    std::array<uint32_t,8> d{};if(description)Read(description,d.data(),32);
    static std::atomic<int> n{};if(d[1]==1 && (d[0]==0 || d[0]==42) && (d[3]==4 || d[3]>=100) && n.fetch_add(1)<200){   // structured or counter buffer UAVs
        uintptr_t wrapper{},view{};D3D11_UNORDERED_ACCESS_VIEW_DESC u{};bool got{};
        if(!result && out && Read(out,&wrapper,8) && wrapper && Read(wrapper+0x10,&view,8) && view){reinterpret_cast<ID3D11UnorderedAccessView*>(view)->GetDesc(&u);got=true;}
        char row[300]{};sprintf_s(row,"{\"event\":\"reference_gx_uav\",\"result\":%u,\"description\":[%u,%u,%u,%u,%u,%u,%u,%u],\"d3d11\":%s,\"format\":%d,\"count\":%u,\"flags\":%u}\n",result,
            d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],got?"true":"false",u.Format,u.Buffer.NumElements,u.Buffer.Flags);Log(row);}
    return result;}
void DumpStateBank() {
    uintptr_t display{},bank{};if(!Read(image+0x4961AB0,&display,8) || !display || !Read(display+0x2A0,&bank,8) || !bank)return;
    MODULEINFO d3d{};const auto module=GetModuleHandleW(L"d3d11.dll");if(!module || !GetModuleInformation(GetCurrentProcess(),module,&d3d,sizeof(d3d)))return;
    const auto low=reinterpret_cast<uintptr_t>(d3d.lpBaseOfDll),high=low+d3d.SizeOfImage;
    for(uint32_t off=0x08;off<0x300;off+=8){
        uintptr_t wrapper{},object{},table{},first{};
        if(!Read(bank+off,&wrapper,8) || !wrapper || !Read(wrapper,&object,8) || !object || !Read(object,&table,8) || !Read(table,&first,8) || first<low || first>=high)continue;
        auto* unknown=reinterpret_cast<IUnknown*>(object);char row[700]{};
        Microsoft::WRL::ComPtr<ID3D11BlendState> blend;Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> raster;Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
        if(SUCCEEDED(unknown->QueryInterface(IID_PPV_ARGS(&blend)))){D3D11_BLEND_DESC d{};blend->GetDesc(&d);
            sprintf_s(row,"{\"event\":\"reference_state_bank\",\"offset\":\"%X\",\"kind\":\"blend\",\"a2c\":%d,\"independent\":%d,\"rt\":[[%d,%d,%d,%d,%d,%d,%d,%u],[%d,%d,%d,%d,%d,%d,%d,%u]]}\n",off,d.AlphaToCoverageEnable,d.IndependentBlendEnable,
                d.RenderTarget[0].BlendEnable,d.RenderTarget[0].SrcBlend,d.RenderTarget[0].DestBlend,d.RenderTarget[0].BlendOp,d.RenderTarget[0].SrcBlendAlpha,d.RenderTarget[0].DestBlendAlpha,d.RenderTarget[0].BlendOpAlpha,d.RenderTarget[0].RenderTargetWriteMask,
                d.RenderTarget[1].BlendEnable,d.RenderTarget[1].SrcBlend,d.RenderTarget[1].DestBlend,d.RenderTarget[1].BlendOp,d.RenderTarget[1].SrcBlendAlpha,d.RenderTarget[1].DestBlendAlpha,d.RenderTarget[1].BlendOpAlpha,d.RenderTarget[1].RenderTargetWriteMask);}
        else if(SUCCEEDED(unknown->QueryInterface(IID_PPV_ARGS(&depth)))){D3D11_DEPTH_STENCIL_DESC d{};depth->GetDesc(&d);
            sprintf_s(row,"{\"event\":\"reference_state_bank\",\"offset\":\"%X\",\"kind\":\"depth\",\"enable\":%d,\"write\":%d,\"func\":%d,\"stencil\":%d,\"read\":%u,\"write_mask\":%u,\"front\":[%d,%d,%d,%d],\"back\":[%d,%d,%d,%d]}\n",off,d.DepthEnable,d.DepthWriteMask,d.DepthFunc,d.StencilEnable,d.StencilReadMask,d.StencilWriteMask,
                d.FrontFace.StencilFailOp,d.FrontFace.StencilDepthFailOp,d.FrontFace.StencilPassOp,d.FrontFace.StencilFunc,d.BackFace.StencilFailOp,d.BackFace.StencilDepthFailOp,d.BackFace.StencilPassOp,d.BackFace.StencilFunc);}
        else if(SUCCEEDED(unknown->QueryInterface(IID_PPV_ARGS(&raster)))){D3D11_RASTERIZER_DESC d{};raster->GetDesc(&d);
            sprintf_s(row,"{\"event\":\"reference_state_bank\",\"offset\":\"%X\",\"kind\":\"raster\",\"fill\":%d,\"cull\":%d,\"ccw\":%d,\"bias\":%d,\"clamp\":%g,\"slope\":%g,\"clip\":%d,\"scissor\":%d,\"msaa\":%d,\"aa_line\":%d}\n",off,d.FillMode,d.CullMode,d.FrontCounterClockwise,
                d.DepthBias,d.DepthBiasClamp,d.SlopeScaledDepthBias,d.DepthClipEnable,d.ScissorEnable,d.MultisampleEnable,d.AntialiasedLineEnable);}
        else if(SUCCEEDED(unknown->QueryInterface(IID_PPV_ARGS(&sampler)))){D3D11_SAMPLER_DESC d{};sampler->GetDesc(&d);
            sprintf_s(row,"{\"event\":\"reference_state_bank\",\"offset\":\"%X\",\"kind\":\"sampler\",\"filter\":%d,\"address\":[%d,%d,%d],\"bias\":%g,\"aniso\":%u,\"cmp\":%d,\"border\":[%g,%g,%g,%g],\"lod\":[%g,%g]}\n",off,d.Filter,d.AddressU,d.AddressV,d.AddressW,
                d.MipLODBias,d.MaxAnisotropy,d.ComparisonFunc,d.BorderColor[0],d.BorderColor[1],d.BorderColor[2],d.BorderColor[3],d.MinLOD,d.MaxLOD);}
        else sprintf_s(row,"{\"event\":\"reference_state_bank\",\"offset\":\"%X\",\"kind\":\"other\"}\n",off);
        Log(row);}
}
std::string Kinds(std::map<uint32_t,uint64_t>& m){std::string s;for(const auto& [k,v]:m){char c[48]{};sprintf_s(c,"%s\"%X\":%llu",s.empty()?"":",",k,static_cast<unsigned long long>(v));s+=c;}m.clear();return "{"+s+"}";}
void __fastcall SceneDraw(uintptr_t sc,uintptr_t pass) noexcept {
    int32_t id{};Read(pass+0x124,&id,4);const auto n=InterlockedIncrement64(reinterpret_cast<volatile LONG64*>(&calls));
    bool first_of_pass{};{static std::set<int> passes_seen;const std::lock_guard hold(lock);first_of_pass=passes_seen.insert(id).second;}
    if(n<=4 || first_of_pass){uintptr_t frustum{},vt{},camera{};std::array<float,12> cam{};std::array<float,4> row{};uint32_t masks[2]{},m58c{};uint8_t flags[8]{},reverse[0x40]{};std::array<uintptr_t,7> views{};
        Read(pass+0x410,&frustum,8);if(frustum)Read(frustum,&vt,8);Read(pass+0x418,&camera,8);if(camera)Read(camera+0x30,cam.data(),48);
        Read(pass+0x250,row.data(),16);Read(pass+0x4B8,masks,8);Read(pass+0x420,flags,8);Read(pass+0x426,reverse,0x40);Read(pass+0x4C0,views.data(),56);Read(pass+0x58C,&m58c,4);
        std::string rev;for(int i=0;i<0x40;++i)if(reverse[i]){char c[16]{};sprintf_s(c,"%s%X",rev.empty()?"":",",i);rev+=c;}
        char text[900]{};sprintf_s(text,"{\"event\":\"reference_ffx_scene_draw\",\"call\":%lld,\"pass\":\"%X\",\"frustum_vt\":\"%llX\",\"camera\":\"%llX\",\"camera_30\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g],"
            "\"row\":[%g,%g,%g,%g],\"masks\":[%u,%u],\"flags\":[%u,%u,%u,%u,%u,%u],\"reverse\":\"%s\",\"views\":[\"%llX\",\"%llX\",\"%llX\",\"%llX\",\"%llX\",\"%llX\",\"%llX\"],\"m58c\":%u}\n",
            static_cast<long long>(n),id,static_cast<unsigned long long>(vt?vt-image:0),static_cast<unsigned long long>(camera),cam[0],cam[1],cam[2],cam[3],cam[4],cam[5],cam[6],cam[7],cam[8],cam[9],cam[10],cam[11],
            row[0],row[1],row[2],row[3],masks[0],masks[1],flags[0],flags[1],flags[2],flags[3],flags[4],flags[5],rev.c_str(),
            static_cast<unsigned long long>(views[0]),static_cast<unsigned long long>(views[1]),static_cast<unsigned long long>(views[2]),static_cast<unsigned long long>(views[3]),
            static_cast<unsigned long long>(views[4]),static_cast<unsigned long long>(views[5]),static_cast<unsigned long long>(views[6]),m58c);Log(text);}
    // I-0141: the camera the entity test 29AA10 measures the emitters' draw distances from (+0x418's +0x30..+0x38), on F9.
    if(id==0x45 && camera_rows.load()>0 && camera_rows.fetch_sub(1)>0){uintptr_t camera{};std::array<float,12> cam{};Read(pass+0x418,&camera,8);if(camera)Read(camera+0x30,cam.data(),48);for(auto& x:cam)if(!std::isfinite(x))x=0;
        char text[400]{};sprintf_s(text,"{\"event\":\"reference_ffx_camera\",\"call\":%lld,\"pass\":\"%X\",\"camera\":\"%llX\",\"camera_30\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g]}\n",
            static_cast<long long>(n),id,static_cast<unsigned long long>(camera),cam[0],cam[1],cam[2],cam[3],cam[4],cam[5],cam[6],cam[7],cam[8],cam[9],cam[10],cam[11]);Log(text);}
    if(id==0x45 && frustum_rows.load()>0 && frustum_rows.fetch_sub(1)>0){uintptr_t frustum{},camera{};Read(pass+0x410,&frustum,8);Read(pass+0x418,&camera,8);
        Log(("{\"event\":\"reference_ffx_frustum\",\"call\":"+std::to_string(n)+",\"frustum\":\""+(frustum?HexWords(frustum,0x300):std::string())+"\",\"camera\":\""+(camera?HexWords(camera,0x60):std::string())+"\"}\n").c_str());}
    // The library's draw list (the same walk as native_ffx_entity_slots, FFX-DRAW §2): every 600th call, the active 1AD6440 entities and
    // their nodes' +0x70 / +0x58 / +0x68 (what 1ADDE80 requires).
    if(n==1200)Log(("{\"event\":\"reference_ffx_type_registry\",\"registry\":"+FfxTypeRegistry()+"}\n").c_str());
    if(n==600)DumpStateBank();
    if(n%600==0){uintptr_t mgr{},mgr8{};uint64_t c[6]{};std::map<std::string,uint64_t> kinds;Read(sc+0x18,&mgr,8);if(mgr)Read(mgr+8,&mgr8,8);
        for(auto g=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1AD1630)(mgr8);g && c[0]<100000;g=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x249680)(g))
            for(auto e=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x249690)(g);e;e=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1AC9BD0)(e)){
                uintptr_t vt{},slot{};uint32_t b0{};if(!Read(e,&vt,8) || !vt || !Read(vt+0x38,&slot,8) || slot!=image+0x1AD6440 || !Read(e+0xB0,&b0,4) || !(b0&1))continue;++c[0];
                for(uintptr_t node=e+0xD0,guard=0;node && guard<64;++guard){uint32_t n70{};uintptr_t n58{},n68{};Read(node+0x70,&n70,4);Read(node+0x58,&n58,8);Read(node+0x68,&n68,8);
                    FfxNodeKinds(node,kinds);
                    ++c[1];if(n70)++c[2];if(n58)++c[3];if(n68)++c[4];if(n70 && n58 && n68)++c[5];uintptr_t next{};if(!Read(node+0xB0,&next,8) || next==node)break;node=next;}}
        char text[200]{};sprintf_s(text,"{\"event\":\"reference_ffx_draw_list\",\"call\":%lld,\"active\":%llu,\"nodes\":[%llu,%llu,%llu,%llu,%llu]}\n",static_cast<long long>(n),
            static_cast<unsigned long long>(c[0]),static_cast<unsigned long long>(c[1]),static_cast<unsigned long long>(c[2]),static_cast<unsigned long long>(c[3]),static_cast<unsigned long long>(c[4]),static_cast<unsigned long long>(c[5]));Log(text);
        Log(("{\"event\":\"reference_ffx_node_kinds\",\"kinds\":"+FfxKinds(kinds)+"}\n").c_str());}
    const auto gate=native_ffx::DrawGateEnter(sc,pass);   // I-0141 tranche 14: ffx_draw_gate
    ++inside_draw;draw_original(sc,pass);--inside_draw;
    native_ffx::DrawGateLeave(gate,sc,pass);
    {std::string row;{const std::lock_guard hold(lock);++scene_calls[id];if(++frames%3000==0){
            std::string topo;for(const auto& [k,v]:topologies){char c[48]{};sprintf_s(c,"%s\"%X/%d\":%llu",topo.empty()?"":",",k.first,k.second,static_cast<unsigned long long>(v));topo+=c;}
            row="{\"event\":\"reference_ffx_draw_counts\",\"calls\":"+std::to_string(frames)+",\"scene_calls\":"+Counts(scene_calls)+",\"recorded\":"+Counts(recorded)+",\"executed\":"+Counts(executed)+
                ",\"draws\":"+Counts(draws)+",\"indexed\":"+Counts(indexed)+",\"topologies\":{"+topo+"},\"update_kinds\":"+Kinds(update_kinds)+
                ",\"proceed_kinds\":"+Kinds(proceed_kinds)+"}\n";}}
        if(!row.empty())Log(row.c_str());}
}
uintptr_t __fastcall Command(uintptr_t pass,uint32_t type,uint32_t size) noexcept {
    const auto result=alloc_original(pass,type,size);
    if(inside_draw && type==4){int32_t id{};Read(pass+0x124,&id,4);const std::lock_guard hold(lock);++recorded[id];}
    return result;
}
void __fastcall Execute(uintptr_t pass,uintptr_t entity,uintptr_t data) noexcept {
    int32_t id{};if(data)Read(data+8,&id,4);
    {const std::lock_guard hold(lock);++executed[id];}
    {   // The executing pass context, first two calls per engine pass: its ids, key, GX context (+0x450: [+0], [+8], [+0x21F8]),
        // the context selector +0x458 and 1921AC0's answer, the scene control's default ring sc+0xC8 (its vtable).
        static std::map<int,int> seen;bool show{};{const std::lock_guard hold(lock);show=seen[id]++<2;}
        if(show){int32_t ids[3]{};uintptr_t gx{},gx0{},gx8{},selector{},sc{},ring{},ring_vt{};int32_t index{};Read(pass+0x124,ids,12);Read(pass+0x450,&gx,8);
            if(gx){Read(gx,&gx0,8);Read(gx+8,&gx8,8);Read(gx+0x21F8,&index,4);}Read(pass+0x458,&selector,8);
            const auto deferred=selector?reinterpret_cast<uint64_t(__fastcall*)(uintptr_t)>(image+0x1921AC0)(selector):~0ull;
            if(data)Read(data+0x10,&sc,8);if(sc)Read(sc+0xC8,&ring,8);if(ring)Read(ring,&ring_vt,8);
            char row[420]{};sprintf_s(row,"{\"event\":\"reference_ffx_execute_context\",\"engine_pass\":\"%X\",\"ids\":[\"%X\",\"%X\",\"%X\"],\"gx\":%s,\"gx_0\":%s,\"gx_8\":%s,\"gx_21f8\":%d,"
                "\"selector\":%s,\"deferred\":%lld,\"ring_vt\":\"%llX\"}\n",id,ids[0],ids[1],ids[2],gx?"true":"false",gx0?"true":"false",gx8?"true":"false",index,selector?"true":"false",
                static_cast<long long>(deferred),static_cast<unsigned long long>(ring_vt?ring_vt-image:0));Log(row);}}
    // I-0113 gap 5: the command's key (pass +0x12C, as Execute natively writes it) and renderer class (the node's [entity-0x78+0x58]
    // vtable RVA), for the E03 states witness's origin of a bound state.
    uint32_t key{};uintptr_t node_renderer{},renderer_table{};Read(pass+0x12C,&key,4);
    if(entity && Read(entity-0x78+0x58,&node_renderer,8) && node_renderer && Read(node_renderer,&renderer_table,8) && renderer_table>image)renderer_table-=image;
    const int saved=callback_pass;const uint32_t saved_key=callback_key;const uintptr_t saved_class=callback_class;
    {auto& e=callback_ring[callback_count++%callback_ring.size()];e={pass,id,key,renderer_table,0};}   // I-0113: this thread's callbacks in order
    callback_pass=id;callback_key=key;callback_class=renderer_table;++inside_callback;callback_original(pass,entity,data);--inside_callback;
    callback_pass=saved;callback_key=saved_key;callback_class=saved_class;
}
// I-0105 (Lightning Spear's bolt): the original's tracer draws (GX 11, 2-point patches) to compare with the native's
// native_ffx_trail_draw rows: per GX context its topology and its bound DS b0, PS b0, b1 and b6 (the wrappers 19209D0 and 1920E20),
// the bytes each constant buffer last received through the update wrapper 1920300 (GxUpdateObserved, whole updates only), and for
// the first 8 indexed GX 11 draws inside an effect callback one row with the counts and those blocks as floats. Each block also
// carries the bytes it was printed from (`<label>_bytes`, 0: never captured, its words printed as zeros), as the native row does.
// Once the 8 rows are written the hooks only pass through (`tracer_done`). The bytes are the buffer's last whole update from any
// context (not keyed by context: an effect's per-draw constants, INFERRED written on its own recording thread).
struct TracerBinds {uint32_t topology{};std::array<uintptr_t,16> ds{},ps{};};
std::unordered_map<uintptr_t,TracerBinds> tracer_binds;   // by GX context, under `lock`
std::unordered_map<uintptr_t,std::vector<uint8_t>> tracer_bytes;   // by GX buffer, under `lock`
int tracer_rows{};uint64_t tracer_dropped{};std::atomic<bool> tracer_done{};
// I-0113 gap 12 (Flame Fan's fan): the next n patch draws (GX 10, one control point) after F9 log their DS b0 words 84..111 and PS b0 g_vColorScale
// (reference_ffx_patch_draw, as native_ffx_patch_draw): F9 arms `patch_watch`, which keeps the binds and bytes collected until the
// rows are written (a bind made before the press is missing at first: each block's captured bytes are printed with it).
std::atomic<int> patch_left{};std::atomic<bool> patch_watch{};
bool TracerCollecting() noexcept {return !tracer_done.load(std::memory_order_relaxed) || patch_watch.load(std::memory_order_relaxed);}
using BindArray=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uintptr_t);BindArray ds_cb_original{},ps_cb_original{};
void TracerBind(std::array<uintptr_t,16>& slots,uint32_t start,uint32_t count,uintptr_t buffers) {
    for(uint32_t i=0;i<count && start+i<slots.size();++i){uintptr_t b{};if(buffers)Read(buffers+8*i,&b,8);slots[start+i]=b;}}
void __fastcall DsConstantsObserved(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {
    if(TracerCollecting())try{const std::lock_guard hold(lock);TracerBind(tracer_binds[c].ds,a,n,p);}catch(...){}
    ds_cb_original(c,s,a,n,p);}
void __fastcall PsConstantsObserved(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {
    if(TracerCollecting())try{const std::lock_guard hold(lock);TracerBind(tracer_binds[c].ps,a,n,p);}catch(...){}
    ps_cb_original(c,s,a,n,p);}
// Both called from noexcept hooks: an allocation failure drops the witness, never the game. The bytes are kept only until the
// 8 rows are written, for at most 4096 buffers (the updates refused by that bound counted, `dropped` in each row).
void TracerBytes(uintptr_t buffer,uintptr_t src,size_t bytes) noexcept {   // GxUpdateObserved: a whole update of a constant-sized buffer
    if(!TracerCollecting() || !buffer || !src || !bytes || bytes>0x1000)return;
    try{std::vector<uint8_t> copy(bytes);if(!Read(src,copy.data(),bytes))return;
        const std::lock_guard hold(lock);if(tracer_rows>=8 && !patch_watch.load(std::memory_order_relaxed))return;
        if(tracer_bytes.size()<4096 || tracer_bytes.contains(buffer))tracer_bytes[buffer]=std::move(copy);else ++tracer_dropped;}catch(...){}}
void TracerDraw(uintptr_t c,uint32_t count,uint32_t start,int32_t base) noexcept try {
    if(tracer_done.load(std::memory_order_relaxed))return;
    std::string row;{const std::lock_guard hold(lock);const auto found=tracer_binds.find(c);if(found==tracer_binds.end() || found->second.topology!=11 || tracer_rows>=8)return;
        char cell[200]{};sprintf_s(cell,"{\"event\":\"reference_ffx_tracer_draw\",\"row\":%d,\"pass\":\"%X\",\"count\":%u,\"start\":%u,\"base\":%d,\"dropped\":%llu",tracer_rows,callback_pass,count,start,base,
            static_cast<unsigned long long>(tracer_dropped));row=cell;
        struct Head {const char* label;uintptr_t buffer;uint32_t words;};const auto& b=found->second;
        for(const auto& [label,buffer,words]:{Head{"ds_b0",b.ds[0],112},{"ps_b0",b.ps[0],140},{"ps_b1",b.ps[1],4},{"ps_b6",b.ps[6],56}}){
            const auto bytes=tracer_bytes.find(buffer);const size_t captured=bytes==tracer_bytes.end()?0:bytes->second.size();
            sprintf_s(cell,",\"%s_bytes\":%zu,\"%s\":[",label,captured,label);row+=cell;
            for(uint32_t i=0;i<words;++i){float f{};if(captured>=4u*i+4)std::memcpy(&f,bytes->second.data()+4u*i,4);sprintf_s(cell,"%s%.4g",i?",":"",f);row+=cell;}
            row+="]";}
        if(++tracer_rows>=8){if(!patch_watch.load()){tracer_bytes.clear();tracer_binds.clear();}tracer_done.store(true);}}
    row+="}\n";Log(row.c_str());}catch(...){}
void __fastcall SetTopology(uintptr_t context,uintptr_t a2,uint32_t topology) noexcept {
    // The topology on every call (step review: one set outside a callback would leave a stale 11 for the next draw inside one).
    if(inside_callback || TracerCollecting()){const std::lock_guard hold(lock);if(inside_callback)++topologies[{callback_pass,static_cast<int>(topology)}];
        if(TracerCollecting())try{tracer_binds[context].topology=topology;}catch(...){}}
    topology_original(context,a2,topology);
}
void PatchDraw(uintptr_t c,uint32_t count,bool is_indexed) noexcept try {
    if(patch_left.load(std::memory_order_relaxed)<=0)return;
    std::string row;{const std::lock_guard hold(lock);const auto found=tracer_binds.find(c);if(found==tracer_binds.end() || found->second.topology!=10)return;
        const int left=patch_left.fetch_sub(1);if(left<=0)return;
        const bool last=left==1;if(last)patch_watch=false;
        char cell[96]{};sprintf_s(cell,"{\"event\":\"reference_ffx_patch_draw\",\"count\":%u,\"indexed\":%d,\"ds\":[",count,int(is_indexed));row=cell;
        const auto words=[&](uintptr_t buffer,const char* bytes_key,uint32_t from,uint32_t to){const auto bytes=tracer_bytes.find(buffer);const size_t captured=bytes==tracer_bytes.end()?0:bytes->second.size();
            for(uint32_t i=from;i<to;++i){float f{};if(captured>=4u*i+4)std::memcpy(&f,bytes->second.data()+4u*i,4);sprintf_s(cell,"%s%.5g",i>from?",":"",f);row+=cell;}
            sprintf_s(cell,"],\"%s\":%zu",bytes_key,captured);row+=cell;};
        words(found->second.ds[0],"ds_bytes",84,112);row+=",\"ps_scale\":[";words(found->second.ps[0],"ps_bytes",36,40);
        sprintf_s(cell,",\"dropped\":%llu}\n",static_cast<unsigned long long>(tracer_dropped));row+=cell;
        if(last && tracer_done.load()){tracer_bytes.clear();tracer_binds.clear();}}   // the final review L4: nothing kept past the rows (after `found`'s last use)
    Log(row.c_str());
}catch(...){}
// I-0113 tranche 16 (FINISH-LINE line 7): at an effect draw, the depth the original binds (the GX context's first word is its
// ID3D11DeviceContext, 1921390): the DSV's resource and read-only flags, the depth state's enable, write mask and function, and
// the pixel stage's t3 resource (the same as the DSV's, or another: a copy), once per pass and combination (at most 96 rows).
// I-0113 tranche 16 (the crystals' hue): the scene inputs an E03 model particle's draw binds (636F0 running on this thread): the
// pixel stage's t30, t31 (IBL), t33 (SSAO), t44 (RLR) and t59..t62 (the light grid), each a texture's size and format or a
// buffer's element count, once per model (the first 16 models), against the native's (native_named_draw_sources).
thread_local const std::string* e03_model{};std::atomic<bool> e03_inputs_full{};
void E03SceneInputs(ID3D11DeviceContext* d3d) noexcept {
    if(!e03_model || e03_inputs_full.load(std::memory_order_relaxed))return;
    static std::mutex inputs_lock;static std::set<std::string> inputs_seen;
    {const std::lock_guard hold(inputs_lock);if(inputs_seen.size()>=16){e03_inputs_full.store(true);return;}if(!inputs_seen.insert(*e03_model).second)return;}
    std::string row="{\"event\":\"reference_e03_inputs\",\"model\":\""+e03_model->substr(0,40)+"\",\"srv\":{";bool first=true;
    for(const UINT slot:{30u,31u,33u,44u,59u,60u,61u,62u}){ID3D11ShaderResourceView* srv{};d3d->PSGetShaderResources(slot,1,&srv);char cell[96]{};
        if(!srv)sprintf_s(cell,"%s\"%u\":null",first?"":",",slot);
        else{D3D11_SHADER_RESOURCE_VIEW_DESC desc{};srv->GetDesc(&desc);ID3D11Resource* r{};srv->GetResource(&r);D3D11_RESOURCE_DIMENSION k{};if(r)r->GetType(&k);
            if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC t{};static_cast<ID3D11Texture2D*>(r)->GetDesc(&t);sprintf_s(cell,"%s\"%u\":[\"tex\",%u,%u,%u,%u]",first?"":",",slot,t.Width,t.Height,static_cast<unsigned>(t.Format),t.ArraySize);}
            else if(k==D3D11_RESOURCE_DIMENSION_BUFFER)sprintf_s(cell,"%s\"%u\":[\"buf\",%u]",first?"":",",slot,desc.Buffer.NumElements);
            else sprintf_s(cell,"%s\"%u\":[\"other\",%d]",first?"":",",slot,int(k));
            // I-0113 tranche 17 (STUDY-CRYSTAL-INPUTS, the last section): t44's identity among the engine's 1x1 textures, the common 2D
            // table [[144961AB0]+0x270] + 8 i (i 0..13: WHITE, BLACK, GRAY, SPEC, BLACKSPEC, NORMAL, ZERO, the six colours): the
            // entry whose wrapper's first 0x40 bytes hold the view or its resource, with that word's offset; -1 none.
            if(slot==44){int index{-1},at{-1};uintptr_t display{},table{};
                if(Read(image+0x4961AB0,&display,8) && display && Read(display+0x270,&table,8) && table)
                    for(int i=0;i<14 && index<0;++i){uintptr_t wrapper{};std::array<uintptr_t,8> words{};if(!Read(table+8*i,&wrapper,8) || !wrapper || !Read(wrapper,words.data(),sizeof(words)))continue;
                        const auto same=[&](uintptr_t v){return v && (v==reinterpret_cast<uintptr_t>(srv) || v==reinterpret_cast<uintptr_t>(r));};
                        for(int w=0;w<8 && index<0;++w){if(same(words[w])){index=i;at=w*8;break;}
                            std::array<uintptr_t,8> inner{};if(words[w] && Read(words[w],inner.data(),sizeof(inner)))   // one level down: reported as 0x100 + w*0x40 + v*8
                                for(int v=0;v<8;++v)if(same(inner[v])){index=i;at=0x100+w*0x40+v*8;break;}}}
                const size_t used=std::strlen(cell);if(used && cell[used-1]==']')sprintf_s(cell+used-1,sizeof(cell)-used+1,",\"common\",%d,%d]",index,at);}
            if(r)r->Release();srv->Release();}
        row+=cell;first=false;}
    row+="}}\n";Log(row.c_str());}
std::atomic<bool> effect_depth_full{};
void EffectDepthWitness(uintptr_t context) noexcept {
    ID3D11DeviceContext* d3d{};if(!context || !Read(context,&d3d,8) || !d3d)return;
    E03SceneInputs(d3d);
    // Its 96 rows written, or 200,000 effect draws seen (the rows seldom reach 96: 3 to 5 keys a run): no more queries.
    static std::atomic<uint64_t> draws_seen{};
    if(effect_depth_full.load(std::memory_order_relaxed) || draws_seen.fetch_add(1,std::memory_order_relaxed)>=200000)return;
    ID3D11RenderTargetView* rtv{};ID3D11DepthStencilView* dsv{};d3d->OMGetRenderTargets(1,&rtv,&dsv);D3D11_DEPTH_STENCIL_VIEW_DESC view{};ID3D11Resource* depth{};
    if(dsv){dsv->GetDesc(&view);dsv->GetResource(&depth);dsv->Release();}
    // The depth's and the first target's sizes (0x31: a smaller target and depth, INFERRED from its separate depth).
    const auto size_of=[](ID3D11Resource* r){D3D11_TEXTURE2D_DESC d{};D3D11_RESOURCE_DIMENSION k{};if(r){r->GetType(&k);if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D)static_cast<ID3D11Texture2D*>(r)->GetDesc(&d);}return std::pair{d.Width,d.Height};};
    const auto depth_size=size_of(depth);std::pair<UINT,UINT> target_size{};D3D11_RENDER_TARGET_VIEW_DESC target_view{};if(rtv){rtv->GetDesc(&target_view);ID3D11Resource* r{};rtv->GetResource(&r);target_size=size_of(r);if(r)r->Release();rtv->Release();}
    ID3D11DepthStencilState* state{};UINT stencil_ref{};d3d->OMGetDepthStencilState(&state,&stencil_ref);D3D11_DEPTH_STENCIL_DESC ds{};if(state){state->GetDesc(&ds);state->Release();}
    ID3D11ShaderResourceView* srv{};d3d->PSGetShaderResources(3,1,&srv);ID3D11Resource* t3{};if(srv){srv->GetResource(&t3);srv->Release();}
    const auto id=[](ID3D11Resource* r){return static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(r));};
    const auto key=std::tuple{callback_pass,id(depth),static_cast<int>(view.Flags),static_cast<int>(ds.DepthEnable),static_cast<int>(ds.DepthWriteMask),static_cast<int>(ds.DepthFunc),id(t3)};
    static std::mutex witness_lock;static std::set<std::remove_const_t<decltype(key)>> seen;bool fresh{};
    {const std::lock_guard hold(witness_lock);fresh=seen.size()<96 && seen.insert(key).second;if(seen.size()>=96)effect_depth_full.store(true);}
    if(fresh){char row[420]{};sprintf_s(row,"{\"event\":\"reference_effect_depth\",\"pass\":%d,\"dsv\":%llu,\"dsv_flags\":%u,\"dsv_format\":%u,\"depth_enable\":%d,\"write_mask\":%d,\"func\":%d,\"t3\":%llu,\"t3_is_dsv\":%d,\"depth_size\":[%u,%u],\"target_size\":[%u,%u],\"target_format\":%u}\n",
        callback_pass,id(depth),static_cast<unsigned>(view.Flags),static_cast<unsigned>(view.Format),int(ds.DepthEnable),int(ds.DepthWriteMask),int(ds.DepthFunc),id(t3),int(t3 && t3==depth),
        depth_size.first,depth_size.second,target_size.first,target_size.second,static_cast<unsigned>(target_view.Format));Log(row);}
    if(depth)depth->Release();if(t3)t3->Release();
}
// The order of the depth copies against the effect passes (FINISH-LINE line 7: which depth 0x29, 0x2D and 0x31 sample, copied when):
// after the 20,000th effect draw, 600 rows in call order: each ID3D11DeviceContext CopyResource or CopySubresourceRegion whose
// destination is a depth-format texture (the R32G8X24, R24G8, R32 and R16 families), and each change of effect pass on a thread, with the thread,
// the context and the resources (the ids of reference_effect_depth).
std::atomic<uint64_t> effect_draws_seen{};std::atomic<int> copy_rows_left{600};thread_local int last_effect_pass{-1};
bool DepthFormatTexture(ID3D11Resource* resource) noexcept {
    if(!resource)return false;D3D11_RESOURCE_DIMENSION dimension{};resource->GetType(&dimension);if(dimension!=D3D11_RESOURCE_DIMENSION_TEXTURE2D)return false;
    D3D11_TEXTURE2D_DESC desc{};static_cast<ID3D11Texture2D*>(resource)->GetDesc(&desc);
    return (desc.Format>=DXGI_FORMAT_R32G8X24_TYPELESS && desc.Format<=DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) || (desc.Format>=DXGI_FORMAT_R24G8_TYPELESS && desc.Format<=DXGI_FORMAT_X24_TYPELESS_G8_UINT) ||
        desc.Format==DXGI_FORMAT_R32_TYPELESS || desc.Format==DXGI_FORMAT_D32_FLOAT || desc.Format==DXGI_FORMAT_R16_TYPELESS || desc.Format==DXGI_FORMAT_D16_UNORM;}
void DepthCopyRow(const char* kind,ID3D11DeviceContext* context,ID3D11Resource* dst,ID3D11Resource* src) noexcept {
    if(effect_draws_seen.load(std::memory_order_relaxed)<20000 || copy_rows_left.load(std::memory_order_relaxed)<=0 || !DepthFormatTexture(dst) || copy_rows_left.fetch_sub(1)<=0)return;
    char row[240]{};sprintf_s(row,"{\"event\":\"reference_depth_copy\",\"kind\":\"%s\",\"thread\":%lu,\"context\":%llu,\"dst\":%llu,\"src\":%llu}\n",kind,GetCurrentThreadId(),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(context)),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(dst)),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(src)));Log(row);}
using ContextCopyResource=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,ID3D11Resource*);
using ContextCopyRegion=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Resource*,UINT,UINT,UINT,UINT,ID3D11Resource*,UINT,const D3D11_BOX*);
ContextCopyResource copy_resource_original{},deferred_copy_resource_original{};ContextCopyRegion copy_region_original{},deferred_copy_region_original{};
// I-0113 tranche 23 (gap 22): the distortion series' window (ffx-series.flag; opened by DistortionSeriesRow, 21_effects.inc): the
// milliseconds since ffx-series-go.txt appeared (tranche 31: or, with ffx-series-auto.flag, since a blended quad opened it), -1
// outside its 3 s; and, Reference with ffx-draw-observe.flag, each texture copy of
// those 3 s whose destination is a colour texture (the frame copy is R16G16B16A16), in call order with the draws' rows.
std::atomic<int64_t> series_from{-1};
double SeriesMs() noexcept {const auto from=series_from.load(std::memory_order_relaxed);if(from<0)return -1;LARGE_INTEGER now{},rate{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&rate);
    const double ms=double(now.QuadPart-from)*1000.0/double(rate.QuadPart);return ms<=3000?ms:-1;}
// Tranche 31: a region copy's box and destination point too (the 608 quads' own copies against the native's CopyRect rows).
void SeriesCopyRow(const char* kind,ID3D11DeviceContext* context,ID3D11Resource* dst,ID3D11Resource* src,const D3D11_BOX* box=nullptr,UINT x=0,UINT y=0) noexcept {
    const double ms=SeriesMs();if(ms<0 || !dst)return;static std::atomic<int> rows{};
    D3D11_RESOURCE_DIMENSION dimension{};dst->GetType(&dimension);if(dimension!=D3D11_RESOURCE_DIMENSION_TEXTURE2D || DepthFormatTexture(dst) || rows.fetch_add(1)>=4000)return;
    D3D11_TEXTURE2D_DESC desc{};static_cast<ID3D11Texture2D*>(dst)->GetDesc(&desc);char row[360]{};
    char where[96]{};if(box)sprintf_s(where,",\"box\":[%u,%u,%u,%u],\"at\":[%u,%u]",box->left,box->top,box->right,box->bottom,x,y);
    sprintf_s(row,"{\"event\":\"ffx_distortion_series_copy\",\"ms\":%.3f,\"kind\":\"%s\",\"pass\":%d,\"thread\":%lu,\"context\":%llu,\"dst\":%llu,\"src\":%llu,\"format\":%u,\"size\":[%u,%u]%s}\n",ms,kind,callback_pass,
        GetCurrentThreadId(),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(context)),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(dst)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(src)),unsigned(desc.Format),desc.Width,desc.Height,where);Log(row);}
// I-0113 tranche 30 (gap 16; effect-scan.flag in the Reference with ffx-draw-observe.flag): the original's scene target read where
// the effects copy it. At a region copy into an R16G16B16A16_FLOAT texture of its source's size and format (the frame copy the
// distortion and radial-blur quads sample, taken from the scene target: tranche 23's copies), at most every 100 ms (every 20 ms inside
// ffx-series' window, tranche 31), the whole source is copied on the same context into one of 12 staging textures; the immediate context's ExecuteCommandList, after the
// original, maps each copy at least 250 ms old without waiting (its list executed by then) and scans it as the native's
// effect-scan (effect_scan.h): a row reference_effect_scan for a picture with a NaN, an inf or a negative half, and every 40th
// read a reference_effect_scan_window. Read only for the game: the copies go to the module's own textures, nothing the game binds
// changes. The native's analogue is effect-scan's stage 0 (the scene copy remade before 0x2D).
namespace reference_scan {
struct Slot {Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;D3D11_TEXTURE2D_DESC desc{};LONGLONG at{};bool pending{};uintptr_t source{};int pass{};bool deferred{};};
// Tranche 31: 12 staging pictures (the series window: one every 20 ms, read 250 ms later; about 13 would be needed, so a copy may be
// refused and counted: none in i113-t31-orbseries-near-original). The step review's L5, accepted for a diagnostic: about 200 MB of staging at 1080p, and inside each 3 s window
// the scans run in the immediate context's ExecuteCommandList under `lock`, slowing the frames of the burst they sample.
std::mutex lock;std::array<Slot,12> slots;
std::atomic<LONGLONG> last{};uint64_t taken{},scanned{},flagged{},refused{};int rows_left{400};
using Execute=void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11CommandList*,BOOL);Execute execute_original{};
bool On() noexcept {static const bool on=ModuleFlag(L"effect-scan.flag");return on;}
LONGLONG Now() noexcept {LARGE_INTEGER t{};QueryPerformanceCounter(&t);return t.QuadPart;}
double Ms(LONGLONG ticks) noexcept {static const LONGLONG f=[]{LARGE_INTEGER q{};QueryPerformanceFrequency(&q);return q.QuadPart;}();return double(ticks)*1000.0/double(f);}
bool Texture(ID3D11Resource* r,D3D11_TEXTURE2D_DESC& d) noexcept {
    if(!r)return false;D3D11_RESOURCE_DIMENSION k{};r->GetType(&k);if(k!=D3D11_RESOURCE_DIMENSION_TEXTURE2D)return false;
    static_cast<ID3D11Texture2D*>(r)->GetDesc(&d);return true;}
void Take(ID3D11DeviceContext* context,ID3D11Resource* dst,ID3D11Resource* src,const D3D11_BOX* box) noexcept {
    if(!On() || !context || !box || !dst || !src || dst==src)return;
    try{D3D11_TEXTURE2D_DESC d{},s{};
        if(!Texture(dst,d) || !Texture(src,s) || d.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || s.Format!=d.Format || s.Width!=d.Width || s.Height!=d.Height ||
           s.SampleDesc.Count!=1 || s.ArraySize!=1)return;
        // The final review's L3: the throttle read before the lock, the mip check before any texture is made.
        // Tranche 31: every 20 ms inside ffx-series' 3 s window (the burst's red negatives sampled densely), else every 100 ms.
        const double gap=SeriesMs()>=0?20.0:100.0;
        const auto now=Now();if(const auto before=last.load(std::memory_order_relaxed);before && Ms(now-before)<gap)return;
        if(s.MipLevels!=1)return;   // a mip chain: not the scene target
        const std::lock_guard hold(lock);if(const auto before=last.load(std::memory_order_relaxed);before && Ms(now-before)<gap)return;
        Slot* free{};for(auto& slot:slots)if(!slot.pending){free=&slot;break;}
        if(!free){++refused;return;}
        if(!free->staging || free->desc.Width!=s.Width || free->desc.Height!=s.Height){
            Microsoft::WRL::ComPtr<ID3D11Device> device;context->GetDevice(&device);if(!device){++refused;return;}
            D3D11_TEXTURE2D_DESC t=s;t.MipLevels=1;t.ArraySize=1;t.Usage=D3D11_USAGE_STAGING;t.BindFlags=0;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;t.MiscFlags=0;
            free->staging.Reset();if(FAILED(device->CreateTexture2D(&t,nullptr,&free->staging)) || !free->staging){++refused;return;}free->desc=t;}
        // The final review's L4: the copy through the originals, so the series rows never count it.
        const bool deferred=context->GetType()==D3D11_DEVICE_CONTEXT_DEFERRED;
        (deferred && deferred_copy_resource_original?deferred_copy_resource_original:copy_resource_original)(context,free->staging.Get(),src);
        // L5: the source, its context's kind and the effect pass the copy came in, for the row.
        free->pending=true;free->at=now;free->source=reinterpret_cast<uintptr_t>(src);free->pass=callback_pass;free->deferred=deferred;last.store(now,std::memory_order_relaxed);++taken;}
    catch(...){}}
void STDMETHODCALLTYPE ExecuteObserved(ID3D11DeviceContext* context,ID3D11CommandList* list,BOOL restore) noexcept {
    execute_original(context,list,restore);
    if(!context || context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return;   // the final review's L2: Map on the immediate context only
    try{const std::lock_guard hold(lock);const auto now=Now();
        for(auto& slot:slots){if(!slot.pending || Ms(now-slot.at)<250)continue;
            D3D11_MAPPED_SUBRESOURCE m{};const HRESULT hr=context->Map(slot.staging.Get(),0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&m);
            if(hr==DXGI_ERROR_WAS_STILL_DRAWING)continue;
            slot.pending=false;if(FAILED(hr)){++refused;continue;}
            const auto counts=effect_scan::Scan(static_cast<const uint8_t*>(m.pData),m.RowPitch,slot.desc.Width,slot.desc.Height);context->Unmap(slot.staging.Get(),0);
            ++scanned;
            if(counts.Any()){++flagged;if(rows_left>0){--rows_left;
                const auto four=[](const std::array<uint32_t,4>& v){char c[64]{};sprintf_s(c,"[%u,%u,%u,%u]",v[0],v[1],v[2],v[3]);return std::string(c);};
                char row[600]{};sprintf_s(row,"{\"event\":\"reference_effect_scan\",\"read\":%llu,\"age_ms\":%.1f,\"source\":\"%llX\",\"pass\":%d,\"deferred\":%d,\"size\":[%u,%u],\"nan\":%s,\"inf\":%s,\"neg\":%s,\"bad_box\":%s,\"neg_box\":%s}\n",
                    static_cast<unsigned long long>(scanned),Ms(now-slot.at),static_cast<unsigned long long>(slot.source),slot.pass,int(slot.deferred),slot.desc.Width,slot.desc.Height,four(counts.nan).c_str(),four(counts.inf).c_str(),four(counts.neg).c_str(),
                    four(counts.bad_box).c_str(),four(counts.neg_box).c_str());Log(row);}}
            if(scanned%40==0){char row[200]{};sprintf_s(row,"{\"event\":\"reference_effect_scan_window\",\"taken\":%llu,\"scanned\":%llu,\"flagged\":%llu,\"refused\":%llu}\n",
                static_cast<unsigned long long>(taken),static_cast<unsigned long long>(scanned),static_cast<unsigned long long>(flagged),static_cast<unsigned long long>(refused));Log(row);}}}
    catch(...){}}
}
void STDMETHODCALLTYPE ObserveCopyResource(ID3D11DeviceContext* c,ID3D11Resource* d,ID3D11Resource* s) noexcept {DepthCopyRow("copy",c,d,s);SeriesCopyRow("copy",c,d,s);copy_resource_original(c,d,s);}
void STDMETHODCALLTYPE ObserveDeferredCopyResource(ID3D11DeviceContext* c,ID3D11Resource* d,ID3D11Resource* s) noexcept {DepthCopyRow("copy_deferred",c,d,s);SeriesCopyRow("copy_deferred",c,d,s);deferred_copy_resource_original(c,d,s);}
void STDMETHODCALLTYPE ObserveCopyRegion(ID3D11DeviceContext* c,ID3D11Resource* d,UINT ds,UINT x,UINT y,UINT z,ID3D11Resource* s,UINT ss,const D3D11_BOX* b) noexcept {
    DepthCopyRow("region",c,d,s);SeriesCopyRow("region",c,d,s,b,x,y);reference_scan::Take(c,d,s,b);copy_region_original(c,d,ds,x,y,z,s,ss,b);}
void STDMETHODCALLTYPE ObserveDeferredCopyRegion(ID3D11DeviceContext* c,ID3D11Resource* d,UINT ds,UINT x,UINT y,UINT z,ID3D11Resource* s,UINT ss,const D3D11_BOX* b) noexcept {
    DepthCopyRow("region_deferred",c,d,s);SeriesCopyRow("region_deferred",c,d,s,b,x,y);reference_scan::Take(c,d,s,b);deferred_copy_region_original(c,d,ds,x,y,z,s,ss,b);}
// The immediate and deferred contexts' CopySubresourceRegion (46) and CopyResource (47) of a scratch hardware device (a deferred one
// null where it equals the immediate one's), and the immediate context's ExecuteCommandList (58; I-0113 tranche 30's reads).
bool ReferenceCopyEntries(std::array<void*,5>& entries) noexcept {
    const auto module=LoadLibraryW(L"d3d11.dll");if(!module)return false;
    using Create=HRESULT(WINAPI*)(IDXGIAdapter*,D3D_DRIVER_TYPE,HMODULE,UINT,const D3D_FEATURE_LEVEL*,UINT,UINT,ID3D11Device**,D3D_FEATURE_LEVEL*,ID3D11DeviceContext**);
    const auto create=reinterpret_cast<Create>(GetProcAddress(module,"D3D11CreateDevice"));if(!create)return false;
    Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,deferred;
    if(FAILED(create(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)) || !context)return false;
    void** table=*reinterpret_cast<void***>(context.Get());entries={table[47],table[46],nullptr,nullptr,table[58]};
    if(SUCCEEDED(device->CreateDeferredContext(0,&deferred)) && deferred){void** d=*reinterpret_cast<void***>(deferred.Get());
        if(d[47]!=table[47])entries[2]=d[47];if(d[46]!=table[46])entries[3]=d[46];}
    return entries[0] && entries[1] && entries[4];}
// I-0113 tranche 16 (STUDY-NOT-YET-NATIVE row 16, the face's mark layer): 836E40's mark fields (+0x40 texture, +0x48 centre,
// +0x50 angle, +0x54 scale, +0x60 colour) when it composes with a mark, then the next 12 quad replays 142379C90(exec, A, B):
// A and B (pos, size), the drawer view *(exec+0x10)'s +0x110/+0x114 (the transform's width and height), +0x210 and its
// matrix +0xD0 (used when +0x210 is null), to place the mark in the 512x512 face image.
// 836E40 returns a byte in AL (its epilogue; Ghidra shows void): the detour returns the original's value.
using FaceCompose=bool(__fastcall*)(uintptr_t);FaceCompose face_compose_original{};
using QuadReplay=void(__fastcall*)(uintptr_t,const float*,const float*);QuadReplay quad_replay_original{};
std::atomic<int> quad_rows_left{},face_mark_rows_left{64};   // at most 64 marks witnessed a session (character creation recomposes)
bool __fastcall FaceComposeObserved(uintptr_t receiver) noexcept {
    uint32_t status{};uintptr_t mark{};std::array<float,8> layer{};
    if(face_mark_rows_left.load(std::memory_order_relaxed)>0 && Read(receiver+0x70,&status,4) && status==2 && Read(receiver+0x40,&mark,8) && mark && Read(receiver+0x48,layer.data(),16) && Read(receiver+0x60,layer.data()+4,16)){
        uintptr_t table{},inner{};Read(mark,&table,8);Read(mark+8,&inner,8);   // the mark texture object's class (its vtable, image offset) and first member
        char row[400]{};sprintf_s(row,"{\"event\":\"reference_face_mark\",\"receiver\":%llu,\"texture\":%llu,\"texture_vtable\":\"%llX\",\"texture_8\":%llu,\"mark\":[%g,%g,%g,%g],\"colour\":[%g,%g,%g,%g]}\n",
            static_cast<unsigned long long>(receiver),static_cast<unsigned long long>(mark),static_cast<unsigned long long>(table>image?table-image:table),static_cast<unsigned long long>(inner),
            layer[0],layer[1],layer[2],layer[3],layer[4],layer[5],layer[6],layer[7]);Log(row);face_mark_rows_left.fetch_sub(1);quad_rows_left.store(12);}
    return face_compose_original(receiver);}
void __fastcall QuadReplayObserved(uintptr_t exec,const float* a,const float* b) noexcept {
    if(quad_rows_left.load(std::memory_order_relaxed)>0 && quad_rows_left.fetch_sub(1)>0){
        float fa[8]{},fb[8]{},matrix[16]{};uintptr_t view{},flag{};uint32_t size[2]{};
        Read(reinterpret_cast<uintptr_t>(a),fa,32);Read(reinterpret_cast<uintptr_t>(b),fb,32);
        if(Read(exec+0x10,&view,8) && view){Read(view+0x110,size,8);Read(view+0x210,&flag,8);Read(view+0xD0,matrix,64);}
        std::string row;char cell[400]{};sprintf_s(cell,"{\"event\":\"reference_quad_replay\",\"a\":[%g,%g,%g,%g,%g,%g,%g,%g],\"b\":[%g,%g,%g,%g,%g,%g,%g,%g],\"view_size\":[%u,%u],\"view_210\":%llu,\"matrix\":[",
            fa[0],fa[1],fa[2],fa[3],fa[4],fa[5],fa[6],fa[7],fb[0],fb[1],fb[2],fb[3],fb[4],fb[5],fb[6],fb[7],size[0],size[1],static_cast<unsigned long long>(flag));row=cell;
        for(int i=0;i<16;++i){sprintf_s(cell,"%s%g",i?",":"",matrix[i]);row+=cell;}
        row+="]}\n";Log(row.c_str());}
    quad_replay_original(exec,a,b);}
// I-0113 tranche 17 (STUDY-PASS-31 §3): the half-size effect layer's composite 140102A80(pass context, block): the block's
// bilateral byte, the half target's view (format, size), the plain and bilateral pixel programmes' DXBC checksums (the
// ID3D11PixelShader under [[lib+0xB98]+0x10] and [[lib+0xBE0]+0x10], lib = [[4961B50]+0x4DD8]; named by reference_ps, which
// decal-observe.flag fills), the samplers at banks +0x2E0 and +0x2E8 and the blend at +0x1D0 (ID3D11 descriptions, each found
// by D3d11As), once per distinct row (at most 8), from the first 600 composites and every 600th after.
using LayerComposite=void(__fastcall*)(uintptr_t,uintptr_t);LayerComposite layer_composite_original{};
void __fastcall LayerCompositeObserved(uintptr_t context,uintptr_t block) noexcept {
    static std::atomic<int> rows_left{8};static std::atomic<uint64_t> composites{};   // final review L7: the first 600 composites read, then every 600th
    const auto call=composites.fetch_add(1,std::memory_order_relaxed);
    if(rows_left.load(std::memory_order_relaxed)>0 && (call<600 || call%600==0)){
        uint8_t bilateral{};uintptr_t srv_object{},root{},lib{},plain{},bilateral_ps{},display{},bank{};Read(block,&bilateral,1);Read(block+0x10,&srv_object,8);
        if(Read(image+0x4961B50,&root,8) && root && Read(root+0x4DD8,&lib,8) && lib){uintptr_t o{};if(Read(lib+0xB98,&o,8) && o)Read(o+0x10,&plain,8);if(Read(lib+0xBE0,&o,8) && o)Read(o+0x10,&bilateral_ps,8);}
        // Final review L5: the GX shader object's D3D11 programme found by QueryInterface under SEH (D3d11As), not read at +0.
        const auto checksum=[&](uintptr_t ps_object){const auto d3d=reinterpret_cast<ID3D11PixelShader*>(decal_observe::D3d11As(ps_object,__uuidof(ID3D11PixelShader)));if(!d3d)return uint64_t{};
            const std::lock_guard hold(reference_ps_lock);const auto f=reference_ps.find(d3d);return f==reference_ps.end()?uint64_t{1}:_byteswap_uint64(f->second.checksum);};
        D3D11_SHADER_RESOURCE_VIEW_DESC view{};UINT width{},height{};
        const uintptr_t srv_d3d=decal_observe::D3d11As(srv_object,__uuidof(ID3D11ShaderResourceView));   // a GX view holds it at +0x10 (1920DA0)
        if(auto* srv=reinterpret_cast<ID3D11ShaderResourceView*>(srv_d3d)){srv->GetDesc(&view);ID3D11Resource* r{};srv->GetResource(&r);
            if(r){D3D11_RESOURCE_DIMENSION k{};r->GetType(&k);if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC t{};static_cast<ID3D11Texture2D*>(r)->GetDesc(&t);width=t.Width;height=t.Height;}r->Release();}}
        D3D11_SAMPLER_DESC samplers[2]{};D3D11_BLEND_DESC blend{};
        if(Read(image+0x4961AB0,&display,8) && display && Read(display+0x2A0,&bank,8) && bank){
            for(int i=0;i<2;++i){uintptr_t o{};if(Read(bank+0x2E0+8*i,&o,8))if(auto* s=reinterpret_cast<ID3D11SamplerState*>(decal_observe::D3d11As(o,__uuidof(ID3D11SamplerState))))s->GetDesc(&samplers[i]);}
            uintptr_t o{};if(Read(bank+0x1D0,&o,8))if(auto* b=reinterpret_cast<ID3D11BlendState*>(decal_observe::D3d11As(o,__uuidof(ID3D11BlendState))))b->GetDesc(&blend);}
        const auto& rt=blend.RenderTarget[0];char row[480]{};
        sprintf_s(row,"{\"event\":\"reference_layer_composite\",\"bilateral\":%u,\"layer\":[%u,%u,%u],\"plain_ps\":\"%016llX\",\"bilateral_ps\":\"%016llX\",\"samplers\":[[%d,%d,%d],[%d,%d,%d]],"
            "\"blend\":[%d,%d,%d,%d,%d,%d,%d,%d]}\n",unsigned(bilateral),unsigned(view.Format),width,height,static_cast<unsigned long long>(checksum(plain)),static_cast<unsigned long long>(checksum(bilateral_ps)),
            int(samplers[0].Filter),int(samplers[0].AddressU),int(samplers[0].AddressV),int(samplers[1].Filter),int(samplers[1].AddressU),int(samplers[1].AddressV),
            int(rt.BlendEnable),int(rt.SrcBlend),int(rt.DestBlend),int(rt.BlendOp),int(rt.SrcBlendAlpha),int(rt.DestBlendAlpha),int(rt.BlendOpAlpha),int(rt.RenderTargetWriteMask));
        static std::mutex composite_lock;static std::set<std::string> seen;bool fresh{};{const std::lock_guard hold(composite_lock);fresh=seen.insert(row).second;}
        if(fresh && rows_left.fetch_sub(1)>0)Log(row);}
    layer_composite_original(context,block);}
void EffectSequenceRow(uintptr_t context) noexcept {
    effect_draws_seen.fetch_add(1,std::memory_order_relaxed);if(last_effect_pass==callback_pass)return;last_effect_pass=callback_pass;
    if(effect_draws_seen.load(std::memory_order_relaxed)<20000 || copy_rows_left.load(std::memory_order_relaxed)<=0)return;copy_rows_left.fetch_sub(1);
    ID3D11DeviceContext* d3d{};if(context)Read(context,&d3d,8);char row[200]{};
    sprintf_s(row,"{\"event\":\"reference_effect_pass_start\",\"pass\":%d,\"thread\":%lu,\"context\":%llu}\n",callback_pass,GetCurrentThreadId(),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(d3d)));Log(row);}
void EffectBlendWitness(uintptr_t context) noexcept;   // I-0113 tranche 17, below BankIndexOf
void __fastcall DrawVertices(uintptr_t context,uintptr_t a2,uint32_t count,uint32_t start) noexcept {
    if(inside_callback){{const std::lock_guard hold(lock);++draws[callback_pass];}PatchDraw(context,count,false);EffectDepthWitness(context);EffectSequenceRow(context);EffectBlendWitness(context);}
    count_original(context,a2,count,start);
}
thread_local uint64_t indexed_on_thread{};   // I-0120 H5: this thread's counted indexed draws (ModelDrawObserved's difference)
void __fastcall DrawIndices(uintptr_t context,uintptr_t a2,uint32_t count,uint32_t start,uint32_t base) noexcept {
    if(inside_callback){{const std::lock_guard hold(lock);++indexed[callback_pass];}++indexed_on_thread;TracerDraw(context,count,start,static_cast<int32_t>(base));PatchDraw(context,count,true);EffectDepthWitness(context);EffectSequenceRow(context);EffectBlendWitness(context);}
    indexed_original(context,a2,count,start,base);
}
// E03 witness (FFX-DRAW "spells"): the model particles' mesh draw 1400636F0(table, model, args, bones), reached from the adapter
// 1402C0130: per model data name, the calls, the engine pass of the executing callback, the LOD args+0x18, the bone pointer, the
// indexed draws it issued; one row every 3000th call (first at 30).
using ModelDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);ModelDraw model_draw_original{};
struct ModelDrawTally {uint64_t calls{},draws{};int32_t pass{};uint32_t lod{};bool bones{},materials{};};   // materials: 636F0's args+0x1A8 set once
std::map<std::string,ModelDrawTally> model_draws;uint64_t model_draw_calls{};
// I-0105 (Lightning Spear, s03000 refused natively for cbInstanceData +0x30): the instance block 59470 uploads last on this thread
// (the b4 image, its 1920300 call returning at 5A0F2; GxUpdateObserved), which 59470 leaves unwritten at +0x30 and +0xC0 when the
// draw has no animation object (29ADF0's), so the original's words there are read here.
thread_local std::array<float,0x44> e03_instance{};thread_local bool e03_instance_set{};
// I-0105 (Lightning Spear's E03 models drawn thinner natively): the bone rows 636F0 receives, in both modes (Reference here,
// natively from the submission): the model's +0x1C rows, its +0xB0 (compose) and +0x1E0 (gated) bytes, the LOD and the first eight
// rows, for the first 24 draws of each data name (I-0113 tranche 12: Flame Fan's s08280 opens by its bones 2 to 5).
void ModelRowsWitness(const char* mode,const std::string& name,uintptr_t model,uint32_t lod,const float* rows,size_t count) {
    static std::mutex rows_lock;static std::map<std::string,int> shown;
    {const std::lock_guard hold(rows_lock);if(shown.size()>=64 || shown[name]++>=24)return;}
    uint8_t compose{},gated{};Read(model+0xB0,&compose,1);Read(model+0x1E0,&gated,1);
    std::string row="{\"event\":\"ffx_model_rows\",\"mode\":\""+std::string(mode)+"\",\"model\":\""+name.substr(0,60)+"\",\"lod\":"+std::to_string(lod)+
        ",\"count\":"+std::to_string(count)+",\"compose\":"+std::to_string(compose)+",\"gated\":"+std::to_string(gated)+",\"rows\":[";
    for(size_t i=0;rows && i<count*12 && i<96;++i){char cell[32]{};if(std::isfinite(rows[i]))sprintf_s(cell,"%s%.5g",i?",":"",rows[i]);else sprintf_s(cell,"%snull",i?",":"");row+=cell;}
    // I-0113 gap 14: the first four mesh records 64B30 reads (ComposeNativeEffectModelPoses): [R+4 Dynamic, R+0x16 default bone,
    // R+0x18 list size, the list's first eight entries].
    row+="],\"meshes\":[";int32_t meshes{};uintptr_t table{};
    if(Read(model+0xA0,&meshes,4) && meshes>0 && Read(model+0xA8,&table,8) && table)for(int32_t i=0;i<meshes && i<4;++i){std::array<uint8_t,0x28> r{};
        if(!Read(table+0x48ull*uint64_t(i),r.data(),r.size()))break;uint16_t fallback{},listed{};uintptr_t list{};std::array<int32_t,8> ids{-1,-1,-1,-1,-1,-1,-1,-1};   // the reviews: unused slots -1, not bone 0
        std::memcpy(&fallback,r.data()+0x16,2);std::memcpy(&listed,r.data()+0x18,2);std::memcpy(&list,r.data()+0x20,8);
        const size_t head=std::min<size_t>(listed,ids.size());if(head && (!list || !Read(list,ids.data(),head*4)))ids.fill(-2);
        char cell[200]{};sprintf_s(cell,"%s[%u,%u,%u,[%d,%d,%d,%d,%d,%d,%d,%d]]",i?",":"",r[4],fallback,listed,ids[0],ids[1],ids[2],ids[3],ids[4],ids[5],ids[6],ids[7]);row+=cell;}
    row+="]}\n";Log(row.c_str());}
// I-0105 (an FFX model particle binds no blend of its own: 1A86C0 with args+0x2C4 = -1): the blend, depth and raster states last
// set on each GX context (the wrappers 1920ED0, 1920F00, 1920CA0), read at 636F0's entry as their offsets (BankIndexOf: the GX
// device table [[0x144961AB0]+0x2A0], the native's bank, as 0x10000 + its offset), so the original's E03 states compare with the
// native rows'. Read at the entry: a per-mesh bind inside 63CB0 is not seen (INFERRED none for blend and depth: the cached blend agrees).
// I-0113 gap 5: with each blend and depth the callback that set it (its engine pass, key and renderer class; pass -1: outside an
// FFX callback), as the native's native_effect_model_first names its own.
// Outside a callback, the setter's callers (i113-s9-flask-original: the flask's blend came from outside the FFX callbacks): the
// first four return addresses above the wrapper inside the executable, as RVAs.
struct StateSetter {int pass{-1};uint32_t key{};uintptr_t renderer{};std::array<uint32_t,4> callers{};};
// The stack is walked only once the flask has been drawn (e03_seen, step review L1: not on every set of the run).
StateSetter CurrentSetter() noexcept {if(inside_callback)return StateSetter{callback_pass,callback_key,callback_class,{}};
    StateSetter s{};if(!e03_seen.load(std::memory_order_relaxed))return s;
    static const uintptr_t image_end=[]{const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
        return image+reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew)->OptionalHeader.SizeOfImage;}();
    void* stack[12]{};const auto n=RtlCaptureStackBackTrace(1,12,stack,nullptr);size_t k{};
    for(USHORT i=0;i<n && k<s.callers.size();++i){const auto a=reinterpret_cast<uintptr_t>(stack[i]);if(a>image && a<image_end)s.callers[k++]=static_cast<uint32_t>(a-image);}
    return s;}
struct BoundStates {uintptr_t blend{},depth{},raster{};StateSetter blend_from{},depth_from{};};std::mutex bound_states_lock;std::unordered_map<uintptr_t,BoundStates> bound_states;   // under bound_states_lock (final review L4: not the shared `lock`)
using BlendSet=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);using DepthSet=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint8_t);
using RasterSet=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);BlendSet blend_set_original{};DepthSet depth_set_original{};RasterSet raster_set_original{};   // effect_draw's signatures
void __fastcall BlendSetObserved(uintptr_t c,uintptr_t s,uintptr_t p,uintptr_t f,uint32_t m) noexcept {
    try{const auto from=CurrentSetter();if(inside_callback && callback_count)++callback_ring[(callback_count-1)%callback_ring.size()].blends;
        const std::lock_guard hold(bound_states_lock);if(bound_states.size()<256 || bound_states.contains(c)){auto& b=bound_states[c];b.blend=p;b.blend_from=from;}}catch(...){}
    blend_set_original(c,s,p,f,m);}
void __fastcall DepthSetObserved(uintptr_t c,uintptr_t s,uintptr_t p,uint8_t r) noexcept {
    try{const auto from=CurrentSetter();const std::lock_guard hold(bound_states_lock);if(bound_states.size()<256 || bound_states.contains(c)){auto& b=bound_states[c];b.depth=p;b.depth_from=from;}}catch(...){}
    depth_set_original(c,s,p,r);}
void __fastcall RasterSetObserved(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {try{const std::lock_guard hold(bound_states_lock);if(bound_states.size()<256 || bound_states.contains(c))bound_states[c].raster=p;}catch(...){}raster_set_original(c,s,p);}
// The offset in [[G+0x4DD0]] (0x200 slots: the bank reaches +0x320 and beyond, BLOOD.md), else 0x10000 + the offset in the GX
// device's table [[0x144961AB0]+0x2A0] (0x100 slots: the FXR sprite renderer's blends and the erase pass's depth), else -1.
int BankIndexOf(uintptr_t object) noexcept {
    if(!object)return -1;std::array<uintptr_t,0x200> slots{};uintptr_t root{},bank{};
    if(Read(image+0x4961B50,&root,8) && root && Read(root+0x4DD0,&bank,8) && bank && Read(bank,slots.data(),sizeof(slots)))
        for(size_t i=0;i<slots.size();++i)if(slots[i]==object)return static_cast<int>(i*8);
    if(Read(image+0x4961AB0,&root,8) && root && Read(root+0x2A0,&bank,8) && bank && Read(bank,slots.data(),0x800))
        for(size_t i=0;i<0x100;++i)if(slots[i]==object)return 0x10000+static_cast<int>(i*8);
    return -1;}
// I-0113 tranche 31 (effect-census.flag, gap 16), both modes: each effect draw of pass 0x2D counted by its blend bank offset (bank
// 8 natively and 0x10008 in the original the default, the unblended quads), its pixel programme (the DXBC checksum's first eight
// bytes, printed as named_oracle's ps_dxbc) and its t0 (« copy »: an R16G16B16A16_FLOAT picture of the bound target's size, the frame copy;
// « tex » another, « none »), per window of one second of the tick clock: one row (effect_census) a window that holds a 0x2D draw
// with another bank, 400 rows at most, stamped with the window's UTC start to meet the runner's cast times. The census has no cap
// within a window: tranche 30's distortion witness kept the first 96 rows and only the quads sampling the copy.
std::string EffectCensusCount(const char* mode,int32_t pass,uint32_t bank,bool default_bank,uint64_t ps_checksum,const char* t0) noexcept {
    try{static std::mutex census_lock;static std::map<std::string,uint64_t> uses;static ULONGLONG window_from{};static SYSTEMTIME window_utc{};
        static bool flagged{};static int rows{};
        const ULONGLONG now=GetTickCount64();char key[80]{};
        sprintf_s(key,"%X/%X/%016llX/%s",unsigned(pass),bank,static_cast<unsigned long long>(_byteswap_uint64(ps_checksum)),t0);
        std::string row;
        {const std::lock_guard hold(census_lock);
            if(window_from && now>=window_from+1000){
                if(flagged && rows<400){++rows;char head[160]{};
                    sprintf_s(head,"{\"event\":\"effect_census\",\"mode\":\"%s\",\"row\":%d,\"utc\":\"%02u:%02u:%02u.%03u\",\"ms\":%llu,\"uses\":{",mode,rows,
                        window_utc.wHour,window_utc.wMinute,window_utc.wSecond,window_utc.wMilliseconds,static_cast<unsigned long long>(now-window_from));row=head;
                    bool first=true;for(const auto& [k,v]:uses){row+=(first?"\"":",\"")+k+"\":"+std::to_string(v);first=false;}row+="}}\n";}
                uses.clear();flagged=false;window_from=0;}
            if(!window_from){window_from=now;GetSystemTime(&window_utc);}
            if(uses.size()<512 || uses.contains(key))++uses[key];flagged|=pass==0x2D && !default_bank;}
        if(!row.empty())Log(row.c_str());
        return key;}catch(...){return {};}}
// I-0113 tranche 17: the blend each effect draw of passes 0x29, 0x2D and 0x31 really runs with (the D3D11 context's bound state,
// OMGetBlendState, its first target's factors) and the bank offset last set on that GX context (BankIndexOf), counted by pass:
// reference_effect_pass_blends, after 20000 effect draws and every 40000th after, eight rows; the native's native_effect_pass_blends.
void EffectBlendWitness(uintptr_t context) noexcept {
    if(callback_pass!=0x29 && callback_pass!=0x2D && callback_pass!=0x31)return;
    static std::atomic<int> rows_left{8};static std::atomic<uint64_t> seen{};
    static const bool census=ModuleFlag(L"effect-census.flag");const bool counted=census && callback_pass==0x2D;   // tranche 31
    if(rows_left.load(std::memory_order_relaxed)<=0 && !counted)return;
    try{ID3D11DeviceContext* d3d{};if(!context || !Read(context,&d3d,8) || !d3d)return;
        ID3D11BlendState* state{};FLOAT factor[4]{};UINT mask{};d3d->OMGetBlendState(&state,factor,&mask);D3D11_BLEND_DESC d{};if(state){state->GetDesc(&d);state->Release();}
        uintptr_t bound{};{const std::lock_guard hold(bound_states_lock);const auto f=bound_states.find(context);if(f!=bound_states.end())bound=f->second.blend;}
        static std::mutex blends_lock;static std::map<uintptr_t,int> offsets;static std::map<std::string,uint64_t> uses;
        int offset{-1};{const std::lock_guard hold(blends_lock);const auto f=offsets.find(bound);if(f!=offsets.end())offset=f->second;}
        if(offset<0 && bound){offset=BankIndexOf(bound);const std::lock_guard hold(blends_lock);offsets[bound]=offset;}
        if(counted){   // tranche 31: the bound PS's checksum (ObserveCreatePixelShader) and t0's picture
            uint64_t checksum{};ID3D11PixelShader* ps{};d3d->PSGetShader(&ps,nullptr,nullptr);
            if(ps){{const std::lock_guard hold(reference_ps_lock);if(const auto f=reference_ps.find(ps);f!=reference_ps.end())checksum=f->second.checksum;}ps->Release();}
            // The step review's L4: « copy » a R16G16B16A16_FLOAT t0 of the bound render target's size (no size held as a constant).
            uint32_t target_w{},target_h{};{ID3D11RenderTargetView* rtv{};d3d->OMGetRenderTargets(1,&rtv,nullptr);
                if(rtv){ID3D11Resource* r{};rtv->GetResource(&r);D3D11_RESOURCE_DIMENSION k{};if(r)r->GetType(&k);
                    if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC td{};static_cast<ID3D11Texture2D*>(r)->GetDesc(&td);target_w=td.Width;target_h=td.Height;}
                    if(r)r->Release();rtv->Release();}}
            const char* t0="none";ID3D11ShaderResourceView* srv{};d3d->PSGetShaderResources(0,1,&srv);
            if(srv){ID3D11Resource* r{};srv->GetResource(&r);D3D11_RESOURCE_DIMENSION k{};if(r)r->GetType(&k);t0="tex";
                if(k==D3D11_RESOURCE_DIMENSION_TEXTURE2D){D3D11_TEXTURE2D_DESC td{};static_cast<ID3D11Texture2D*>(r)->GetDesc(&td);
                    if(td.Format==DXGI_FORMAT_R16G16B16A16_FLOAT && target_w && td.Width==target_w && td.Height==target_h)t0="copy";}
                if(r)r->Release();srv->Release();}
            // The step review's L3a: an unresolved bank (-1) is counted under its own key and never marks the window.
            EffectCensusCount("reference",callback_pass,unsigned(offset),offset==0x10008 || offset<0,checksum,t0);
            if(rows_left.load(std::memory_order_relaxed)<=0)return;}
        const auto& rt=d.RenderTarget[0];char key[96]{};
        sprintf_s(key,"%X/%X/%d,%d,%d,%d,%d,%d,%d,%u",unsigned(callback_pass),unsigned(offset),int(rt.BlendEnable),int(rt.SrcBlend),int(rt.DestBlend),int(rt.BlendOp),
            int(rt.SrcBlendAlpha),int(rt.DestBlendAlpha),int(rt.BlendOpAlpha),unsigned(rt.RenderTargetWriteMask));
        const std::lock_guard hold(blends_lock);++uses[key];const auto n=seen.fetch_add(1,std::memory_order_relaxed);
        if(n<20000 || (n-20000)%40000)return;rows_left.fetch_sub(1);
        std::string row="{\"event\":\"reference_effect_pass_blends\",\"draws\":"+std::to_string(n)+",\"uses\":{";
        bool first=true;for(const auto& [k,v]:uses){row+=(first?"\"":",\"")+k+"\":"+std::to_string(v);first=false;}row+="}}\n";Log(row.c_str());}catch(...){}}
void __fastcall ModelDrawObserved(uintptr_t table,uintptr_t model,uintptr_t args,uintptr_t bones) noexcept {
    static const bool skip=ModuleFlag(L"ffx-skip-e03.flag");   // A/B witness (Reference): what the E03 draws show
    if(skip)return;
    std::string clean;for(const auto ch:NativeModelDataName(model))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
    if(clean=="s07002")e03_seen.store(true,std::memory_order_relaxed);   // the chunk witness starts at the Estus Flask's first draw (I-0113 gap 5)
    {   // The first four draws of each model with the instance block: +0x30, +0x70, +0xC0 and +0x100 (59470's last upload on this
        // thread; the decal path's 59470 upload, not followed by 636F0, would be read here under the next model: none seen).
        const auto& name=clean;
        {int32_t count{};uint32_t lod{};std::array<float,96> rows{};if(args)Read(args+0x18,&lod,4);   // up to eight rows (the witness prints at most 96 floats)
            if(Read(model+0x1C,&count,4) && count>0 && count<=4096 && bones && Read(bones,rows.data(),sizeof(float)*std::min<size_t>(96,size_t(count)*12)))
                ModelRowsWitness("reference",name,model,lod,rows.data(),size_t(count));}
        {static std::map<std::string,int> stated;uintptr_t a8{},l3{},context{},cached_blend{},cached_other{};BoundStates b{};bool show{},known{};size_t contexts{};
            if(args && Read(args+8,&a8,8) && a8 && Read(a8,&l3,8) && l3 && Read(l3+8,&context,8) && context){Read(l3+0x480,&cached_blend,8);Read(l3+0x4E8,&cached_other,8);
                const std::lock_guard hold(bound_states_lock);contexts=bound_states.size();
                // The first 4 draws of each data name and engine pass (I-0113: s07002 is drawn in 0x21 and 0x29), and since tranche 5
                // the first draw of each further key of s07002 (24 at most: its later particles).
                const auto key=name+"/"+std::to_string(inside_callback?callback_pass:-1);static std::set<uint32_t> flask_keys;
                const bool new_key=name=="s07002" && inside_callback && flask_keys.size()<24 && flask_keys.insert(callback_key).second;
                if(name=="s07002" && inside_callback && callback_pass==0x29)e03_key.store(callback_key,std::memory_order_relaxed);
                if(((stated.contains(key) || stated.size()<64) && stated[key]++<4) || new_key){show=true;if(const auto f=bound_states.find(context);f!=bound_states.end()){b=f->second;known=true;}}
                // Tranche 5: every draw of s07002 in 0x29 (600 at most) with the milliseconds since the first, its key and the bound blend
                // and depth through a whole drink (the native's: native_effect_model_states).
                static int traced{};static ULONGLONG first_ms{};
                if(name=="s07002" && inside_callback && callback_pass==0x29 && traced<600){++traced;const auto now=GetTickCount64();if(!first_ms)first_ms=now;
                    BoundStates t{};if(const auto f=bound_states.find(context);f!=bound_states.end())t=f->second;char row[160]{};
                    sprintf_s(row,"{\"event\":\"reference_flask_draw\",\"ms\":%llu,\"key\":%u,\"blend\":%d,\"depth\":%d}\n",now-first_ms,callback_key,BankIndexOf(t.blend),BankIndexOf(t.depth));Log(row);}}
            // The first run printed -1 everywhere: with the context's presence among the wrapper calls' contexts, the raw objects' tables
            // and the commit's cached blend (15220 keeps it at the holder's +0x480, +0x4E8). I-0113: the draw's own pass, key and class,
            // the context, and the callback that set the blend and the depth.
            if(show){const auto table_of=[](uintptr_t o){uintptr_t t{};if(o)Read(o,&t,8);return static_cast<unsigned long long>(t>image?t-image:t);};char row[900]{};
                const auto self=CurrentSetter();
                sprintf_s(row,"{\"event\":\"reference_e03_states\",\"model\":\"%.60s\",\"pass\":%d,\"key\":%u,\"renderer\":\"%llX\",\"context\":\"%llX\",\"blend\":%d,\"depth\":%d,\"raster\":%d,\"known\":%d,\"contexts\":%zu,"
                    "\"blend_from\":[%d,%u,\"%llX\",\"%X\",\"%X\",\"%X\",\"%X\"],\"depth_from\":[%d,%u,\"%llX\",\"%X\",\"%X\",\"%X\",\"%X\"],\"tables\":[\"%llX\",\"%llX\",\"%llX\"],\"cached\":[%d,%d],\"cached_tables\":[\"%llX\",\"%llX\"]}\n",
                    name.c_str(),self.pass,self.key,static_cast<unsigned long long>(self.renderer),static_cast<unsigned long long>(context),
                    BankIndexOf(b.blend),BankIndexOf(b.depth),BankIndexOf(b.raster),int(known),contexts,
                    b.blend_from.pass,b.blend_from.key,static_cast<unsigned long long>(b.blend_from.renderer),b.blend_from.callers[0],b.blend_from.callers[1],b.blend_from.callers[2],b.blend_from.callers[3],
                    b.depth_from.pass,b.depth_from.key,static_cast<unsigned long long>(b.depth_from.renderer),b.depth_from.callers[0],b.depth_from.callers[1],b.depth_from.callers[2],b.depth_from.callers[3],
                    table_of(b.blend),table_of(b.depth),table_of(b.raster),
                    BankIndexOf(cached_blend),BankIndexOf(cached_other),table_of(cached_blend),table_of(cached_other));
                // The callbacks this thread ran before, oldest first: [pass context, pass, key, class, blends set], and the draw's pass
                // context (args+8 = pass context + 0x460).
                std::string out(row);out.resize(out.size()-2);char cell[120]{};sprintf_s(cell,",\"pass_context\":\"%llX\",\"sequence\":[",static_cast<unsigned long long>(a8-0x460));out+=cell;
                const auto shown=static_cast<size_t>(std::min<uint64_t>(callback_count,callback_ring.size()));
                for(size_t i=0;i<shown;++i){const auto& e=callback_ring[(callback_count-shown+i)%callback_ring.size()];
                    sprintf_s(cell,"%s[\"%llX\",%d,%u,\"%llX\",%u]",i?",":"",static_cast<unsigned long long>(e.pass_context),e.pass,e.key,static_cast<unsigned long long>(e.renderer),e.blends);out+=cell;}
                out+="]}\n";Log(out.c_str());}}
        static std::map<std::string,int> shown;bool show{};{const std::lock_guard hold(lock);show=e03_instance_set && shown.size()<64 && shown[name]++<24;}
        e03_instance_set=false;   // one block per draw: a draw without its own upload prints nothing
        if(show){std::string row="{\"event\":\"reference_e03_instance\",\"model\":\""+name.substr(0,60)+"\"";
            for(const auto& [label,at,words]:{std::tuple{"w30",12,4},{"w70",28,4},{"wC0",48,4},{"w100",64,1}}){row+=",\"";row+=label;row+="\":[";
                for(int i=0;i<words;++i){char cell[32]{};const float f=e03_instance[size_t(at+i)];   // stack words: a non-finite one as null
                    if(std::isfinite(f))sprintf_s(cell,"%s%g",i?",":"",f);else sprintf_s(cell,"%snull",i?",":"");row+=cell;}row+="]";}
            // I-0113 tranche 16 (the crystals' hue): the whole block, as native_e03_instance's « all ».
            row+=",\"all\":[";for(int i=0;i<0x44;++i){char cell[24]{};const float f=e03_instance[size_t(i)];
                if(std::isfinite(f))sprintf_s(cell,"%s%g",i?",":"",f);else sprintf_s(cell,"%snull",i?",":"");row+=cell;}
            // The light and fog keys 5A120 builds the draw's cbLight and cbFog from (args +0x338/+0x340 with the blend +0x348, +0x350/+0x358
            // with +0x360), as native_e03_instance's keys (its group's +0xF8..+0x124).
            uint32_t keys[11]{};if(args)Read(args+0x338,keys,sizeof keys);float light_t{},fog_t{};std::memcpy(&light_t,&keys[4],4);std::memcpy(&fog_t,&keys[10],4);
            char cell[200]{};sprintf_s(cell,"],\"light\":[%u,%u,%u,%u],\"light_t\":%g,\"fog\":[%u,%u,%u,%u],\"fog_t\":%g}\n",keys[0],keys[1],keys[2],keys[3],light_t,keys[6],keys[7],keys[8],keys[9],fog_t);
            row+=cell;Log(row.c_str());}}
    // H5: this thread's draws across the call (the process-wide count took the effect chunks of other threads too).
    const uint64_t before=indexed_on_thread;
    e03_model=&clean;model_draw_original(table,model,args,bones);e03_model=nullptr;
    const uint64_t after=indexed_on_thread;
    uint32_t lod{};uintptr_t material_table{};if(args){Read(args+0x18,&lod,4);Read(args+0x1A8,&material_table,8);}
    std::string row;
    {const std::lock_guard hold(lock);auto& t=model_draws[clean];++t.calls;t.draws+=after-before;t.pass=inside_callback?callback_pass:-1;t.lod=lod;t.bones=bones!=0;t.materials|=material_table!=0;
        if(++model_draw_calls%3000==30){row="{\"event\":\"reference_ffx_model_draws\",\"calls\":"+std::to_string(model_draw_calls)+",\"models\":[";bool first=true;
            for(const auto& [name,v]:model_draws){char cell[300]{};sprintf_s(cell,"%s{\"model\":\"%.120s\",\"calls\":%llu,\"draws\":%llu,\"pass\":%d,\"lod\":%u,\"bones\":%d,\"materials\":%d}",first?"":",",name.c_str(),
                static_cast<unsigned long long>(v.calls),static_cast<unsigned long long>(v.draws),v.pass,v.lod,v.bones?1:0,v.materials?1:0);row+=cell;first=false;}
            row+="]}\n";}}
    if(!row.empty())Log(row.c_str());
}
}
// Rows GL1/GL2 (W5d GL-3, with shadow-observe.flag): FUN_1401BB810's glow record [*argument 3] (+0x10 BlurSampleScale,
// +0x14 Blur RgbScale, +0x18 Blur AlphaScale, +0x1C New Glow Enable, +0x20 NewGlow Final ColorScale, +0x24 Enable) and the
// factor [[GXRS+0x5368]+0x80], the inputs native's CollectNativeGlowInputs reproduces. First 12 calls, then every 300th.
using GlowFilterPass=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
GlowFilterPass glow_filter_original{};
void __fastcall ObserveGlowFilter(uintptr_t a1,uintptr_t pass,uintptr_t holder) noexcept {
    static volatile LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=12 || n%300==0){uintptr_t record{},root{},settings{};std::array<float,5> f{};uint8_t new_glow{},enable{};float factor{-1.f};
        if(Read(holder,&record,8) && record){Read(record+0x10,f.data(),12);Read(record+0x1C,&new_glow,1);Read(record+0x20,&f[3],4);Read(record+0x24,&enable,1);}
        if(Read(image+0x4961B50,&root,8) && root && Read(root+0x5368,&settings,8) && settings)Read(settings+0x80,&factor,4);
        char row[256]{};sprintf_s(row,"{\"event\":\"reference_glow_record\",\"call\":%lld,\"record\":%s,\"enable\":%u,\"new_glow\":%u,\"blur_scale\":%g,\"rgb\":%g,\"alpha\":%g,\"final\":%g,\"factor\":%g}\n",
            static_cast<long long>(n),record?"true":"false",enable,new_glow,f[0],f[1],f[2],f[3],factor);Log(row);}
    glow_filter_original(a1,pass,holder);
}
// Sun source (STATE: the gparam map transform): every FUN_140049D00 call of the original (manager, resource handle, area, bank,
// the 3x4 transform the area's records take their rotation from (140037340's argument 5), weight) with its caller, the input
// the native bank records must reproduce per area. Rare (map loads); all calls are logged, at most 400.
using GparamBind=void(__fastcall*)(uintptr_t,uint64_t,uint32_t,uint32_t,const float*,uint32_t);
GparamBind gparam_bind_original{};
void __fastcall ObserveGparamBind(uintptr_t manager,uint64_t handle,uint32_t area,uint32_t bank,const float* transform,uint32_t weight) noexcept {
    static volatile LONG64 calls{};
    if(InterlockedIncrement64(&calls)<=400){std::array<float,12> m{};const bool read=transform && Read(reinterpret_cast<uintptr_t>(transform),m.data(),48);
        char row[512]{};sprintf_s(row,"{\"event\":\"reference_gparam_bind\",\"caller\":%llu,\"handle\":%lld,\"area\":%u,\"bank\":%u,\"weight_bits\":%u,\"transform\":[%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g],\"read\":%d}\n",
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image),static_cast<long long>(handle),area,bank,weight,
            m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8],m[9],m[10],m[11],read?1:0);Log(row);}
    gparam_bind_original(manager,handle,area,bank,transform,weight);
}
// The same at FUN_140D275F0 (cap, bank, weight), the single entry of D272C0's admission bind, D26A60 and D26A70: the file's
// name, kind (+0x60), handle state (+0x70), bound byte (+0x78) and +0x80 transform before the call, with the caller, so the
// file the original binds per area is named (the native row is native_gparam_scene_binding_source).
std::wstring ReadCpuString(uintptr_t string);
using GparamFileBind=void(__fastcall*)(uintptr_t,int32_t,float);
GparamFileBind gparam_file_bind_original{};
void __fastcall ObserveGparamFileBind(uintptr_t cap,int32_t bank,float weight) noexcept {
    static volatile LONG64 calls{};
    if(InterlockedIncrement64(&calls)<=400){std::array<float,12> m{};uint32_t kind{};int64_t handle{};BYTE bound{};
        Read(cap+0x80,m.data(),48);Read(cap+0x60,&kind,4);Read(cap+0x70,&handle,8);Read(cap+0x78,&bound,1);
        std::string name;for(const auto ch:ReadCpuString(cap+0x10))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
        char row[640]{};sprintf_s(row,"{\"event\":\"reference_gparam_file_bind\",\"caller\":%llu,\"name\":\"%s\",\"kind\":%u,\"bank\":%d,\"weight\":%.7g,\"handle_valid\":%d,\"bound\":%u,\"transform\":[%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g]}\n",
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image),name.substr(0,96).c_str(),kind,bank,weight,handle!=-1?1:0,bound,
            m[0],m[1],m[2],m[3],m[4],m[5],m[6],m[7],m[8],m[9],m[10],m[11]);Log(row);}
    gparam_file_bind_original(cap,bank,weight);
}
}  // namespace
