#include "../renderer/glow_math.h"
#include "light_producers.h"
#include "producers.h"
#include "../content/gparam.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <DirectXMath.h>
#include <emmintrin.h>
#include <fstream>
#include <mutex>
#include <set>

namespace ds3light {
namespace {
Reader g_reader=nullptr;   // I-0048: UseReader (in-memory inputs of the live host)
ListingReader g_listing_reader=nullptr;
RdataReader g_rdata_reader=nullptr;
SharedReader g_shared_reader=nullptr;   // I-0052: UseSharedReader
RecordBlend g_light_blend=nullptr, g_fog_blend=nullptr;   // I-0141 tranche 3: UseRecordBlends
std::vector<uint8_t> ReadFile(const std::filesystem::path& p) { if(g_reader) return g_reader(p); std::ifstream in(p,std::ios::binary); return {std::istreambuf_iterator<char>(in),{}}; }
std::vector<std::string> Lines(const std::filesystem::path& p) { std::vector<std::string> out;
    if(g_reader) { const auto b=g_reader(p); size_t a=0; for(size_t i=0;i<=b.size();++i) if(i==b.size() || b[i]==static_cast<uint8_t>(10)) { size_t e=i; if(e>a && b[e-1]==static_cast<uint8_t>(13)) --e; if(e>a) out.emplace_back(reinterpret_cast<const char*>(b.data())+a,e-a); a=i+1; } return out; }
    std::ifstream in(p); std::string l; while(std::getline(in,l)) if(!l.empty()) out.push_back(l); return out; }
uint64_t Num(const std::string& line,const char* key) { const std::string k=std::string("\"")+key+"\":"; const size_t p=line.find(k); return p==std::string::npos?0:std::strtoull(line.c_str()+p+k.size(),nullptr,10); }
void Parse(Snapshot& s) {
    const uint8_t* d=s.data; const size_t size=s.size; std::vector<Region>* into=&s.regions;
    for(size_t p=0;p+16<=size;) { Region r; uint32_t n{}; std::memcpy(&r.key,d+p,4); std::memcpy(&n,d+p+4,4); std::memcpy(&r.address,d+p+8,8);
        if(p+16+n>size) break;
        if((r.key&0xC0000000u)==0xC0000000u && !n) { s.children.push_back({r.key&0xFFFu,{}}); into=&s.children.back().regions; p+=16; continue; }   // child marker
        r.data=d+p+16; r.bytes=n; into->push_back(r); p+=(16+size_t{n}+15)&~size_t{15}; }
}
const Banks* g_banks=nullptr;   // I-0045: UseBanks
BankStats g_bank_stats;
// Record words the sun / fog builders read (byte ranges [begin, end)), for the bank check.
constexpr uint32_t sun_read[][2]={{0x00,0x0C},{0x10,0x30},{0x40,0x44},{0x50,0x54},{0x70,0x7C},{0x80,0xA0},{0xB0,0xB4},{0xC0,0xC4},{0xE0,0xEC},{0xF0,0x110},
    {0x120,0x124},{0x130,0x134},{0x150,0x15C},{0x160,0x180},{0x190,0x194},{0x1A0,0x1A4},{0x1E0,0x200},{0x220,0x248},{0x24C,0x24F}};
constexpr uint32_t fog_read[][2]={{0x00,0x58}};
constexpr uint32_t fog_variant_read[][2]={{0x00,0x18},{0x1C,0x1D},{0x20,0x28},{0x30,0x4C},{0x50,0x54}};   // I-0063: the fog record words that decide its GPARAM variant
template<size_t N> void CompareRecord(const uint32_t (&ranges)[N][2],const uint8_t* copied,const uint8_t* built,const char* what) {
    ++g_bank_stats.records;
    for(const auto& r:ranges) for(uint32_t o=r[0];o<r[1];++o) {
        if((o&3u) && r[1]-r[0]>=4) continue;   // word ranges: compare once per word (byte ranges: per byte)
        const uint32_t n=(r[1]-r[0]>=4)?4u:1u; ++g_bank_stats.words;
        static const bool show=[]{ char v[4]{}; size_t k=0; return !getenv_s(&k,v,sizeof v,"BANK_DIFF") && k; }();
        if(show && std::memcmp(copied+o,built+o,n)) { uint32_t a{},b{}; std::memcpy(&a,copied+o,n); std::memcpy(&b,built+o,n); std::printf("bankdiff %s +0x%X copied %08X built %08X\n",what,o,a,b); }
        if(std::memcmp(copied+o,built+o,n)) { if(!g_bank_stats.mismatched_words++) { char t[96]; uint32_t a{},b{}; std::memcpy(&a,copied+o,n); std::memcpy(&b,built+o,n);
            std::snprintf(t,sizeof t,"%s +0x%X copied %08X built %08X",what,o,a,b); g_bank_stats.first=t; } } }
}
// Adapter v19 writer_specs indices.
enum Writer : uint32_t { W13AA60=0, W137910=1, W1A1DA0=2, W1A0080=3, W1A5B50=4, W1A55B0=5, W19FB20=6, W19F640=7, W19F4D0=8, W764D0=9, W10ED10=10,
    W1D6D20=11, W1D7850=12, W1B26E0=13, W1B33D0=14, W1B38D0=15, W1CB9D0=16, W675B0=17, W10CC20=18, W1072C0=19, W1D8050=20, W1141B0=21, W1496D0=22, W14A2D0=23, W14E900=24, W1A6690=25, W1A60F0=26, W88A10=27, W67070=28, W1B5720=29,
    W10C7E0=30, W1B9790=31, W1BB810=32, W0E5110=33, W264230=34, W2A7E70=35, W102A80=36, W25E720=37, W25D700=38, W0CADD0=39, W16F7650=40, W01DB60=41, W1495F0=42 };   // I-0064 (adapter v46, v46b, v47)
bool FxaaParam(const Snapshot& s,uint8_t* out,uint32_t bytes);   // I-0064, below
bool GlowParam(const Snapshot& s,uint32_t caller,uint8_t* out,uint32_t bytes);
bool GlowTaps(const Snapshot& s,uint8_t* out,uint32_t bytes);
bool CustomDof(const Snapshot& s,const Snapshots* all,uint8_t* out,uint32_t bytes);
bool BoxWvp(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes);
bool Offsets102A80(const Snapshot& s,uint8_t* out,uint32_t bytes);
// Region indices inside the 1A5B50 / 1A55B0 specs: 7 = the light item vector (elements = item objects, 0x300 bytes).
constexpr uint32_t batch_items=7;

template<class T> T At(const Region& r,uint32_t offset) { T v{}; if(offset+sizeof(T)<=r.bytes) std::memcpy(&v,r.data+offset,sizeof(T)); return v; }
// 1A5B50 groups the items of its vector in batches of at most 12 (1A55B0, shadowed lights: 4), one batch per upload,
// in vector order.
unsigned BatchSize(const Snapshot& s) { return (s.writer==W1A55B0 || s.writer==W1A6690 || s.writer==W1A60F0)?4u:12u; }
// v23 (I-0045): 1A6690 / 1A60F0 run the 1A55B0 body; the batch depth reads item +0x20 in 1A6690, item +8 elsewhere.
bool Shadowed(uint32_t w) { return w==W1A55B0 || w==W1A6690 || w==W1A60F0; }
// cbLightAccLightParam (1A5B50 / 1A55B0): each item's vtable+0x10 (212DB0) appends to the batch buffer the float4
// rows item [+0x20, +0x50) and {item [+0x90, +0x98), 0, 0}; a shadowed item (item+0xA8 or +0xB0 set) also appends
// item+0x50, item+0x60 and six shadow-atlas rectangles that need the shadow map size ([item+0xB0]+0x50/+0x54), not
// in the v19 snapshot: such a batch is not produced.
// Shadowed items (item+0xA8 or +0xB0 set) append, after the four rows, item+0x50, item+0x60 and, with a shadow map
// ([item+0xB0], 1A55B0 v20 region 9 = its first 0x80 bytes; W, H = its +0x50/+0x54), six atlas rectangles from the int
// quadruples (x0, y0, x1, y1) at item+0xBC+16k: {(x1-x0)/W * 0.5, (y1-y0)/H * -0.5, x0/W + that x, y0/H + (y1-y0)/H * 0.5}.
// Each texture the item binds (212DB0 -> sink+0x10) appends {w, h, 1/w, 1/h} to cbLightAccShadowInfo (ShadowInfo).
constexpr uint32_t shadow_maps=9;
bool PointLights(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    const unsigned n=s.Elements(batch_items), size=BatchSize(s), first=ordinal*size; if(first>=n) return false;
    float half{},neg_half{}; if(!Rdata(0x3D7EDB4,&half,4) || !Rdata(0x3D7F0C0,&neg_half,4)) return false;
    uint32_t at=0;
    for(unsigned j=0;first+j<n && j<size;++j) {
        const Region* item=s.Find(batch_items|((first+j+1)<<16)); if(!item || item->bytes<0x120) return false;
        if(at+64>bytes) return false;
        std::memcpy(out+at,item->data+0x20,0x30); std::memcpy(out+at+0x30,item->data+0x90,8); at+=64;
        if(!At<uint64_t>(*item,0xA8) && !At<uint64_t>(*item,0xB0)) continue;
        if(at+32>bytes) return false;
        std::memcpy(out+at,item->data+0x50,16); std::memcpy(out+at+16,item->data+0x60,16); at+=32;
        if(!At<uint64_t>(*item,0xB0)) continue;
        const Region* map=s.Find(shadow_maps|((first+j+1)<<16)); if(!map || map->bytes<0x58 || at+96>bytes) return false;
        const float W=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*map,0x50))), H=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*map,0x54)));
        for(int k=0;k<6;++k) { const int32_t x0=At<int32_t>(*item,0xBC+16*k), y0=At<int32_t>(*item,0xC0+16*k), x1=At<int32_t>(*item,0xC4+16*k), y1=At<int32_t>(*item,0xC8+16*k);
            const float dy=static_cast<float>(y1-y0)/H, dx=(static_cast<float>(x1-x0)/W)*half;
            const float r[4]={dx,dy*neg_half,static_cast<float>(x0)/W+dx,static_cast<float>(y0)/H+dy*half}; std::memcpy(out+at,r,16); at+=16; } }
    return true;
}
// cbLightAccLightParam (1A6690, I-0045): each spot item's vtable+0x10 (2144C0) appends 8 rows: item [+0x30, +0x60),
// {item+0xE4, item+0xE8, 0, item+0x1C9 ? 0 : 1}, item [+0x60, +0xA0); with a shadow map ([item+0x1C0], v24 region 9) a row
// {uint item+0xE0, 0, 0, 0} and, when item+0x264 == 0, the atlas rectangle of ints item+0x268..+0x274 over the map size
// ([map+0x50] / [map+0x54]): {dx, dy * -0.5, x0/W + dx, y0/H + dy * 0.5}, dx = (x1-x0)/W * 0.5, dy = (y1-y0)/H. A
// projected texture ([item+0x278]) goes through sink slots 3/4 (no-ops for this sink).
bool SpotLights(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    const unsigned n=s.Elements(batch_items), size=BatchSize(s), first=ordinal*size; if(first>=n) return false;
    float one{},half{},neg_half{}; if(!Rdata(0x3D7EE10,&one,4) || !Rdata(0x3D7EDB4,&half,4) || !Rdata(0x3D7F0C0,&neg_half,4)) return false;
    uint32_t at=0;
    for(unsigned j=0;first+j<n && j<size;++j) {
        const Region* item=s.Find(batch_items|((first+j+1)<<16)); if(!item || item->bytes<0x280 || at+128>bytes) return false;
        std::memcpy(out+at,item->data+0x30,0x30); std::memcpy(out+at+0x30,item->data+0xE4,8);
        const float zero=0.0f, w=item->data[0x1C9]?0.0f:one; std::memcpy(out+at+0x38,&zero,4); std::memcpy(out+at+0x3C,&w,4);
        std::memcpy(out+at+0x40,item->data+0x60,0x40); at+=128;
        if(!At<uint64_t>(*item,0x1C0)) continue;
        if(at+16>bytes) return false;
        std::memset(out+at,0,16); std::memcpy(out+at,item->data+0xE0,4); at+=16;
        if(At<int32_t>(*item,0x264)) continue;
        const Region* map=s.Find(shadow_maps|((first+j+1)<<16)); if(!map || map->bytes<0x58 || at+16>bytes) return false;
        const float W=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*map,0x50))), H=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*map,0x54)));
        const int32_t x0=At<int32_t>(*item,0x268), y0=At<int32_t>(*item,0x26C), x1=At<int32_t>(*item,0x270), y1=At<int32_t>(*item,0x274);
        const float dy=static_cast<float>(y1-y0)/H, dx=(static_cast<float>(x1-x0)/W)*half;
        const float r[4]={dx,dy*neg_half,static_cast<float>(x0)/W+dx,static_cast<float>(y0)/H+dy*half}; std::memcpy(out+at,r,16); at+=16; }
    return true;
}
// cbLightAccShadowInfo (1A6690): {w, h, 1/w, 1/h} of each spot item's shadow map ([item+0x1C0], v24 region 9).
bool SpotShadowInfo(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    const unsigned n=s.Elements(batch_items), size=BatchSize(s), first=ordinal*size; if(first>=n) return false;
    uint32_t at=0;
    for(unsigned j=0;first+j<n && j<size;++j) {
        const Region* item=s.Find(batch_items|((first+j+1)<<16)); if(!item) return false;
        if(!At<uint64_t>(*item,0x1C0)) continue;
        const Region* map=s.Find(shadow_maps|((first+j+1)<<16)); if(!map || map->bytes<0x58 || at+16>bytes) return false;
        const float w=static_cast<float>(At<uint32_t>(*map,0x50)), h=static_cast<float>(At<uint32_t>(*map,0x54));
        const float v[4]={w,h,1.0f/w,1.0f/h}; std::memcpy(out+at,v,16); at+=16; }
    return true;
}
// cbLightAccShadowInfo (1A55B0): one {w, h, 1/w, 1/h} per item binding a shadow map, in batch order. The size is taken from
// the map object ([item+0xB0]+0x50/+0x54), on the assumption (checked against the oracle) that it equals the D3D11
// description 1A70F0 reads.
bool ShadowInfo(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    const unsigned n=s.Elements(batch_items), size=BatchSize(s), first=ordinal*size; if(first>=n) return false;
    uint32_t at=0;
    for(unsigned j=0;first+j<n && j<size;++j) {
        const Region* item=s.Find(batch_items|((first+j+1)<<16)); if(!item) return false;
        if(!At<uint64_t>(*item,0xA8) && !At<uint64_t>(*item,0xB0)) continue;
        if(At<uint32_t>(*item,0xB8)) return false;   // texture through [item+0xA8]'s virtual: not reproduced
        const Region* map=s.Find(shadow_maps|((first+j+1)<<16)); if(!map || map->bytes<0x58 || at+16>bytes) return false;
        const float w=static_cast<float>(At<uint32_t>(*map,0x50)), h=static_cast<float>(At<uint32_t>(*map,0x54));
        const float v[4]={w,h,1.0f/w,1.0f/h}; std::memcpy(out+at,v,16); at+=16; }
    return true;
}
// cbLightAccVtxParam (1A5B50 / 1A55B0): {r, 0, 0, 0} with z the smallest item+8 of the batch and (x, y, z, w) =
// (0, 0, z, 1) x the row-major matrix at p2+0x50 (SSE order (w*r3 + z*r2) + (y*r1 + x*r0)); r = min(z'/w', 1) when
// z' > 0 and w' > 0, else 0.
void Transpose(const Region& r,uint32_t at,unsigned columns,float* out) {   // columns of the row-major 4x4 at r+at
    for(unsigned c=0;c<columns;++c) for(unsigned k=0;k<4;++k) out[4*c+k]=At<float>(r,at+16*k+4*c);
}
// cbLightAccParam as 1A1DA0 assembles it (point-light pass); `pass` = its this, `view` = [pass+0x418],
// cam = [G+0x4DE8], cam8 = [cam+8], flag = the byte at 0x144962316. Float offsets: 0 transpose(4BCA0 inverse of
// pass+0x50), 16 view columns x/y/z (pass+0x10), 28 {W, H, 1/W, 1/H} with W = [view+0x7C] + 2[view+0x78] and
// H = [view+0x80] + 2[view+0x78], 32 transpose(pass+0xD0), 48 {[cam+0x9CC], 0, 0, 0}, 52 transpose(pass+0x50),
// 68 0, 69 flag, 70-71 0, 72 [view+0x54] (0.1 without view), 73 [view+0x58] (5000), 74 [cam8+0x4B8], 75 [cam8+0x4BC].
// The same assembly in 19F640 (sun, pass = arg0), 10ED10 (AO, pass = arg1, float 69 = 0) and 1A0080 (organize: light counts
// and tile size, see OrganizeParam). `view` and `flag` absent from a spec leave their floats zero (19F640 reads neither).
bool LightAccParamCommon(const Snapshot& s,uint32_t pass_region,int view_region,int flag_region,uint8_t* out,uint32_t bytes) {
    const Region *pass=s.Find(pass_region), *view=view_region<0?nullptr:s.Find(static_cast<uint32_t>(view_region)), *cam=s.Find(2), *cam8=s.Find(3), *flag=flag_region<0?nullptr:s.Find(static_cast<uint32_t>(flag_region));
    if(!pass || !cam || !cam8 || bytes<320 || pass->bytes<0x110 || cam->bytes<0x9D0 || cam8->bytes<0x4C0) return false;
    float f[80]{}; float m[16],inv[16];
    for(unsigned i=0;i<16;++i) m[i]=At<float>(*pass,0x50+4*i);
    ds3prod::InverseMatrix(m,inv);
    for(unsigned c=0;c<4;++c) for(unsigned k=0;k<4;++k) f[4*c+k]=inv[4*k+c];
    Transpose(*pass,0x10,3,f+16);
    if(view) { const int pad=At<int32_t>(*view,0x78); const uint32_t w=static_cast<uint32_t>(At<int32_t>(*view,0x7C)+pad*2), h=static_cast<uint32_t>(At<int32_t>(*view,0x80)+pad*2);
        const float W=static_cast<float>(static_cast<int64_t>(w)), H=static_cast<float>(static_cast<int64_t>(h)); f[28]=W; f[29]=H; f[30]=1.0f/W; f[31]=1.0f/H; }
    Transpose(*pass,0xD0,4,f+32);
    f[48]=At<float>(*cam,0x9CC);
    Transpose(*pass,0x50,4,f+52);
    std::memcpy(out,f,320);
    if(flag) { const uint32_t visual=At<uint8_t>(*flag,6); std::memcpy(out+4*69,&visual,4); }
    return true;
}
// 1A1DA0 adds floats 72-75: near/far from the view (0.1 / 5000 without one) and the slice scale/bias of cam8.
bool LightAccParam1A1DA0(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    if(!LightAccParamCommon(s,4,5,7,out,bytes) || !s.Find(5) || !s.Find(7)) return false;
    const Region *view=s.Find(5), *cam8=s.Find(3);
    const float nearz=At<float>(*view,0x54), farz=At<float>(*view,0x58); std::memcpy(out+4*72,&nearz,4); std::memcpy(out+4*73,&farz,4);
    std::memcpy(out+4*74,cam8->data+0x4B8,8);
    return true;
}
// GXLightOrganize light buffers (1A0080, branch [0x144962315] == 0): over the n items of its list (region 6), by
// type [item+0x1C] (jump table 0x1A0AA0): point 101/102/108/109 -> 64-byte records {item [+0x20, +0x50),
// +0x90, +0x94, 0, +0x98}; spot 103-106/110-113 -> 128-byte records from 214710 (transcribed, I-0045); line 107 ->
// 80-byte records {item [+0x20, +0x60), +0xC0, +0xC4, [0x143D7EE7C], +0xA0}. Each buffer is uploaded with n
// records' bytes, filled from its start; the unfilled tail is allocator residue the programmes do not read (the
// counts bound them): produced as zeros. Upload order: line [pc+0x460], spot [pc+0x458], point [pc+0x450].
bool OrganizeBuffer(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    constexpr uint32_t items=6; const unsigned n=s.Elements(items); if(ordinal>2) return false;
    uint32_t at=0;
    for(unsigned i=0;i<n;++i) { const Region* item=s.Find(items|((i+1)<<16)); if(!item || item->bytes<0xD0) return false;
        const int type=At<int32_t>(*item,0x1C);
        const bool point=type==101 || type==102 || type==108 || type==109, line=type==107, spot=(type>=103 && type<=106) || (type>=110 && type<=113);
        if(ordinal==2 && point) { if(at+64>bytes) return false; std::memcpy(out+at,item->data+0x20,0x30); std::memcpy(out+at+0x30,item->data+0x90,8);
            std::memset(out+at+0x38,0,4); std::memcpy(out+at+0x3C,item->data+0x98,4); at+=64; }
        else if(ordinal==1 && spot) { if(at+128>bytes) return false;   // 214710(item, dst, 0) (I-0045): {item [+0x30, +0x60), +0xE4, +0xE8, 1, +0x1C9 ? 0 : 1, item [+0x60, +0xA0)}
            float one{}; if(!Rdata(0x3D7EE10,&one,4) || item->bytes<0x1CA) return false;
            std::memcpy(out+at,item->data+0x30,0x30); std::memcpy(out+at+0x30,item->data+0xE4,8); std::memcpy(out+at+0x38,&one,4);
            const float w=item->data[0x1C9]?0.0f:one; std::memcpy(out+at+0x3C,&w,4); std::memcpy(out+at+0x40,item->data+0x60,0x40); at+=128; }
        else if(ordinal==0 && line) { if(at+80>bytes) return false; std::memcpy(out+at,item->data+0x20,0x40); std::memcpy(out+at+0x40,item->data+0xC0,8);
            const uint32_t k=0; std::memcpy(out+at+0x48,&k,4); std::memcpy(out+at+0x4C,item->data+0xA0,4); return false; }   // constant 0x143D7EE7C unread: not produced yet
    }
    return true;
}
// cbCompositeShadowMask (10CC20): FC_Mult and FC_Add are executable constants, one pair per upload site.
bool CompositeMask(uint32_t caller,uint8_t* out,uint32_t bytes) {
    if(bytes<32) return false;
    if(caller==0x10CDD9) return Rdata(0x3D7F690,out,16) && Rdata(0x3D7FC00,out+16,16);
    if(caller==0x10D01E) return Rdata(0x3D7F370,out,16) && Rdata(0x3D7FE70,out+16,16);
    return false;
}
// cbSceneParam (13AA60): 13A580 copies the 960-byte block image [p2+0xCB0, +0x1070) into this+0xCB0, which is
// uploaded at 13AFA1. Without a camera (UseCamera) the image is copied (class B). I-0045 cycle 6, with the CED670
// camera output of the frame: the camera fields from ProduceSceneCamera (K-0414) and the shadow sub-block
// (+0x1C0, 0x1B0 bytes, copied with 6F920) from the cascaded-shadow state object (1D6D20 argument 2, written by
// 1B5720 from the ShadowParam bank record of 88A10 and by the cascade fit): +0x1C0 = state+0xA90, +0x1D0 = +0xAB0,
// +0x1E0 = +0xA60, +0x230..+0x25F = +0xAC0..+0xAEF. Only the fields the lighting programmes read are produced.
const Region* ShadowState(const Snapshots* all) {
    if(!all) return nullptr;
    for(const Snapshot* s:all->OfWriter(W1D6D20)) { const Region* st=s->Find(7); if(st && st->bytes>=0xAF0) return st; }
    return nullptr;
}
const float* g_camera=nullptr;   // I-0045: UseCamera
uint32_t g_shadow_map=0, g_random_texture=0;   // I-0045: UseShadowSetup (engine/backend-facing sizes)
// I-0045 cycle 7: the shadow sub-block from game data (with UseBanks and UseShadowSetup): the ShadowParam record of the
// scene's key {map area (from the 137910 light keys), ID 0 (set at map load, D64C30)} through 1B5720 (cascade clamps,
// +[0x143D7ED70]) and 1B6B80 case 3 / 23A0C0 (state+0xA90, +0xAB0, +0xAC0, +0xAD0, +0xAE0). Compared word for word with
// the state object (bank check).
bool ShadowFieldsFromBanks(const Snapshots* all,uint8_t* block) {
    if(!g_banks || !g_shadow_map || !g_random_texture || !all) return false;
    uint32_t area=0;
    for(const Snapshot* sp:all->OfWriter(W137910)) { const Region* e=sp->Find(3); if(!e || e->bytes<0x30) continue;
        const uint32_t a=At<uint32_t>(*e,8); if(a>1) { area=a; break; } }
    uint8_t R[0x94]; if(!area || !g_banks->Shadow(area,0,R,sizeof R)) return false;
    auto f=[&](uint32_t at){ float v; std::memcpy(&v,R+at,4); return v; };
    float eps{},tiny{},half{},one{},k[4],c255{}; if(!Rdata(0x3D7ED70,&eps,4) || !Rdata(0x3D7ECF0,&tiny,4) || !Rdata(0x3D7EDB4,&half,4) || !Rdata(0x3D7EE10,&one,4)
        || !Rdata(0x3D7FEF0,k,16) || !Rdata(0x3D7EFF4,&c255,4)) return false;
    const float near_z=f(0x34), c0=f(0x40), c1=f(0x44), c2=f(0x48);
    float t=near_z+eps; const float a7c=t<c0?c0:t; t=c0+eps; const float a80=t<c1?c1:t; t=c1+eps; const float a84=t<c2?c2:t;
    t=c2+eps; const float a74=t<f(0x38)?f(0x38):t; const float a78=f(0x3C), fade_end=a74+a78;
    const float size=static_cast<float>(g_shadow_map);
    const float a90[4]={fade_end,one/(a78>tiny?a78:tiny),half/size,half/size};
    const uint8_t* col=R+0x8C; const float cr=static_cast<float>(col[2])/c255, cg=static_cast<float>(col[1])/c255, cb=static_cast<float>(col[0])/c255, ca=static_cast<float>(col[3])/c255;
    const float ab0[4]={ca*(k[0]-cr),ca*(k[1]-cg),ca*(k[2]-cb),f(0x68)};
    const float ac0[4]={f(0x74),f(0x78),f(0x7C),one/static_cast<float>(g_random_texture)};
    const float ad0[4]={one/size,one/size,f(0x80),0.0f};
    const float d0=a7c-near_z; float den[3]={a80-a7c,a84-a80,fade_end-a84}; for(float& d:den) if(!(d>0.0f)) d=d0+one;
    const float ae0[4]={one,d0/den[0],d0/den[1],d0/den[2]};
    std::memcpy(block+0x1C0,a90,16); std::memcpy(block+0x1D0,ab0,16); std::memcpy(block+0x230,ac0,16); std::memcpy(block+0x240,ad0,16); std::memcpy(block+0x250,ae0,16);
    if(const Region* st=ShadowState(all)) { uint8_t copied[0x260]{}; std::memcpy(copied+0x1C0,st->data+0xA90,16); std::memcpy(copied+0x1D0,st->data+0xAB0,16);
        std::memcpy(copied+0x230,st->data+0xAC0,0x30); static constexpr uint32_t words[][2]={{0x1C0,0x1E0},{0x230,0x260}}; CompareRecord(words,copied,block,"shadow state"); }
    return true;
}
bool ShadowFields(const Snapshots* all,uint8_t* block,uint32_t bytes) {
    if(bytes<0x260) return false; uint8_t from_banks[0x260]{}; if(ShadowFieldsFromBanks(all,from_banks)) { for(const uint32_t o:{0x1C0u,0x1D0u,0x230u,0x240u,0x250u}) std::memcpy(block+o,from_banks+o,16); return true; }
    const Region* st=ShadowState(all); if(!st) return false;
    std::memcpy(block+0x1C0,st->data+0xA90,16); std::memcpy(block+0x1D0,st->data+0xAB0,16); std::memcpy(block+0x1E0,st->data+0xA60,16);
    std::memcpy(block+0x230,st->data+0xAC0,0x30); return true;
}
// I-0064: the fields of the image only the forward programmes read: FC_ProjSpaceToShadowMatrix[4] (+0x260) as cbPerScene's (1D6D20:
// transpose(1CE40(pass+0xD0, state+0xAF0+0x40 n))), FC_CascadeSelectDist (+0x360) = state+0xAA0, FC_FrustumSliceScale/Bias
// (+0x374) = [cam8+0x4B8/+0x4BC], FC_LightTile_Width (+0x380) = [cam8+0x4CC] and FC_LightTile_NumCellX (+0x384) = the view's
// width with its border in tiles, rounded up (read in 1400764D0 by I-0107: (X + T - 1) / T, X = [V+0x7C] + 2 [V+0x78]).
void Multiply(const float* a,const float* b,float* out);   // 14001CE40, below
void ForwardFields(const Snapshot& s,const Snapshots* all,uint8_t* block) {
    if(const Region* cam8=s.Find(3); cam8 && cam8->bytes>=0x4D0) { std::memcpy(block+0x374,cam8->data+0x4B8,8); const uint32_t tile=At<uint32_t>(*cam8,0x4CC), width=At<uint32_t>(*cam8,0x4B0);
        std::memcpy(block+0x380,&tile,4); if(tile) { const uint32_t cells=(width+tile-1)/tile; std::memcpy(block+0x384,&cells,4); } }
    if(!all) return;
    // the cascaded-shadow state of the same camera frame as this call (a capture spans several frames; i63-live-13: three)
    const Snapshot* best=nullptr; for(const Snapshot* xp:all->OfWriter(W1D6D20)) { const Snapshot& x=*xp; const Region *st=x.Find(7); if(!st || st->bytes<0xBF0 || !At<uint64_t>(*st,0x110)) continue;
        if(!best || (x.frame==s.frame && best->frame!=s.frame)) best=&x; }
    if(best) { const Snapshot& x=*best; const Region *pass=x.Find(6), *state=x.Find(7); if(!pass || pass->bytes<0x110) return;
        float cam[16]; for(int i=0;i<16;++i) cam[i]=At<float>(*pass,0xD0+4*i);
        if(const Region* own=s.Find(6); own && own->bytes>=0x110) for(int i=0;i<16;++i) cam[i]=At<float>(*own,0xD0+4*i);   // the view of this call (13AA60 argument 1), not the window's
        for(int n=0;n<4;++n) { float b[16],p[16],f[16]; for(int i=0;i<16;++i) b[i]=At<float>(*state,0xAF0+0x40*n+4*i); Multiply(cam,b,p); for(int c=0;c<4;++c) for(int k=0;k<4;++k) f[4*c+k]=p[4*k+c]; std::memcpy(block+0x260+0x40*n,f,64); }
        std::memcpy(block+0x360,state->data+0xAA0,16); }
}
bool SceneImage(const Snapshot& s,const Snapshots* all,uint8_t* out,uint32_t bytes) {
    // The pose is the one latched in the scene context (argument 1 +0xD0, 4x4 row-major, CED670 layout): it can be one
    // camera update older than the frame record's (camera turns); projection, near/far and viewport from the record.
    if(g_camera) { const Region* p2=s.Find(6); if(bytes>960 || !p2 || p2->bytes<0x110) return false;
        float camera[24]; std::memcpy(camera,g_camera,sizeof camera); std::memcpy(camera,p2->data+0xD0,64);
        uint8_t block[960]{}; ds3prod::ProduceSceneCamera(ds3prod::ReadCamera(camera),block);
        if(!ShadowFields(all,block,sizeof block)) return false; ForwardFields(s,all,block); std::memcpy(out,block,bytes); return true; }
    // Spec region 6 = argument 1 object; v19 copies its first 0x1000 bytes, so floats 212.. of the image (beyond any
    // lighting read) are left zero.
    const Region* p2=s.Find(6); if(!p2 || p2->bytes<=0xCB0 || bytes>960) return false;
    std::memcpy(out,p2->data+0xCB0,(std::min)(bytes,p2->bytes-0xCB0u)); return true;
}
// I-0064: cbMatrixWVP (13AA60 uploads it at 13AFEF after cbSceneParam, the 0xC0 bytes of the view image [p2+0x1070, +0x1130)):
// for every view 13AA60 renders (the main camera, the sun's cascades), from the view object (argument 1, spec region 6): its
// pose +0xD0 and its projection +0x50; VC_ViewFrustumPlanes (+0x80) is read by no programme.
bool ViewWvp(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region* v=s.Find(6); if(!v || v->bytes<0x110 || bytes<128) return false;
    float pose[16],proj[16]; std::memcpy(pose,v->data+0xD0,64); std::memcpy(proj,v->data+0x50,64);
    ds3prod::ProduceViewMatrices(pose,proj,out); return true;
}
// cbSceneParam variant of 764D0 (one draw): the shadow-parameter range [+0x1C0, +0x370) is 6F920's copy of
// arg0+0x5E0; the lighting draw reads only FC_ShadowColor (+0x1D0) of the block. Other fields are not produced.
// With a camera (UseCamera), the same sub-block fields from the cascaded-shadow state object instead.
bool SceneShadow764D0(const Snapshot& s,const Snapshots* all,uint8_t* out,uint32_t bytes) {
    if(g_camera) return ShadowFields(all,out,bytes);
    const Region* a0=s.Find(5); if(!a0 || a0->bytes<0x790 || bytes<0x370) return false;
    std::memcpy(out+0x1C0,a0->data+0x5E0,0x1B0); return true;
}
// 140112B40: ARGB colour -> squared (R, G, B; alpha linear) / 255, clamped to [c150, c120], scaled by [0x143D46690],
// rounded to nearest (CVTPS2DQ, values below 2^23), repacked as ARGB. 1072C0 then uploads cbColor = {R, G, B, A} / 255 of the result.
bool ColourLinear(uint32_t argb,float out[4]) {
    float lo[4],hi[4],k[4]; if(!Rdata(0x3D1C150,lo,16) || !Rdata(0x3D1C120,hi,16) || !Rdata(0x3D46690,k,16)) return false;
    float d=0; if(!Rdata(0x3D7EFF4,&d,4)) return false;
    const float r=static_cast<float>((argb>>16)&0xFF)/d, g=static_cast<float>((argb>>8)&0xFF)/d, b=static_cast<float>(argb&0xFF)/d, a=static_cast<float>(argb>>24)/d;
    float v[4]={r*r,g*g,b*b,a}; uint32_t q[4];
    for(int i=0;i<4;++i) { v[i]=v[i]>lo[i]?v[i]:lo[i]; v[i]=v[i]<hi[i]?v[i]:hi[i]; float f=v[i]*k[i];
        uint32_t bits; std::memcpy(&bits,&f,4); if((bits&0x7FFFFFFFu)<0x4B000000u) f=static_cast<float>(_mm_cvtsi128_si32(_mm_cvtps_epi32(_mm_set_ss(f)))); q[i]=static_cast<uint32_t>(static_cast<int64_t>(f)); }   /* I-0063: CVTPS2DQ rounds to nearest (was truncated: 1/255 for 2/255, m45 turned view) */
    const uint32_t packed=(q[3]<<24)|((q[0]&0xFF)<<16)|((q[1]&0xFF)<<8)|(q[2]&0xFF);
    out[0]=static_cast<float>((packed>>16)&0xFF)/d; out[1]=static_cast<float>((packed>>8)&0xFF)/d; out[2]=static_cast<float>(packed&0xFF)/d; out[3]=static_cast<float>(packed>>24)/d;
    return true;
}
bool Colour1072C0(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region* a2=s.Find(7); if(!a2 || a2->bytes<0x70 || bytes<16) return false;   // spec region 7 = argument 2 object
    float c[4]; if(!ColourLinear(At<uint32_t>(*a2,0x6C),c)) return false; std::memcpy(out,c,16); return true;
}
// cbSSAOParam (1B33D0 / 1B38D0): this+0x110 = {p22, p32 / f, f, 0} (p = the projection rows at arg1+0x70/+0x80, f = the
// float argument 5 / 6, on the stack), this+0x120 = {this+0x168, this+0x16C, 0, 0}, this+0x130 kept from before.
bool SsaoParam(const Snapshot& s,unsigned float_arg,uint8_t* out,uint32_t bytes) {
    const Region *args=s.Find(0), *pass=s.Find(5), *a1=s.Find(6); if(!args || !pass || !a1 || a1->bytes<0x90 || pass->bytes<0x170 || bytes<48) return false;
    const float f=At<float>(*args,8*float_arg), p22=At<float>(*a1,0x78), p32=At<float>(*a1,0x88);
    const float v[8]={p22,p32/f,f,0,At<float>(*pass,0x168),At<float>(*pass,0x16C),0,0};
    std::memcpy(out,v,32); std::memcpy(out+32,pass->data+0x130,16); return true;
}
// cbSampleOffset (1B33D0 at 1B35CC): kernel k = the 8 float4 at argument 6 (region 8); d[i] = k[i] * C (C = [0x143D7FEF0]);
// S = [0x1439F2490] + sum k[i] (in order); out[i] = {d.x, d.y, d.z * (1 / S).z, d.w}. Entries 8-15 of the array are not
// written by the engine (stack residue): zero.
bool SsaoKernel(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region* k=s.Find(8); if(!k || k->bytes<128 || bytes<128) return false;
    float c[4],sum[4],one[4]; if(!Rdata(0x3D7FEF0,c,16) || !Rdata(0x39F2490,sum,16) || !Rdata(0x3D1C120,one,16)) return false;
    float d[8][4];
    for(int i=0;i<8;++i) for(int j=0;j<4;++j) { const float v=At<float>(*k,16*i+4*j); sum[j]=sum[j]+v; d[i][j]=v*c[j]; }
    float inv[4]; for(int j=0;j<4;++j) inv[j]=one[j]/sum[j];
    for(int i=0;i<8;++i) { const float o[4]={d[i][0],d[i][1],inv[2]*d[i][2],d[i][3]}; std::memcpy(out+16*i,o,16); }
    return true;
}
// cbLightAccParam of 1A0080 (GXLightOrganize; v20 regions: 7 = [pass+0x418], 8 = the bytes at 0x144962310): the common
// floats 0-67, then 68 = n if [0x144962315] else the point-light count, 69 = [0x144962316], 70 = spot count, 71 = line
// count (types as OrganizeBuffer; the counting branch runs when the light list is not empty, as in the captures),
// 72-73 near/far of the view, 74-75 [cam8+0x4B8/+0x4BC], 76 [cam8+0x4CC], 77 = that value >> 5 when above 32 else 1.
bool OrganizeParam(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    if(!LightAccParamCommon(s,4,7,-1,out,bytes)) return false;
    const Region *view=s.Find(7), *flags=s.Find(8), *cam8=s.Find(3); if(!view || !flags || !cam8 || cam8->bytes<0x4D0) return false;
    constexpr uint32_t items=6; const unsigned n=s.Elements(items); uint32_t point=0,spot=0,line=0;
    for(unsigned i=0;i<n;++i) { const Region* item=s.Find(items|((i+1)<<16)); if(!item) return false; const int type=At<int32_t>(*item,0x1C);
        if(type==101 || type==102 || type==108 || type==109) ++point; else if(type==107) ++line; else if((type>=103 && type<=106) || (type>=110 && type<=113)) ++spot; }
    const uint32_t c68=At<uint8_t>(*flags,5)?n:point, c69=At<uint8_t>(*flags,6), tile=At<uint32_t>(*cam8,0x4CC), tile_div=tile>32?tile>>5:1u;
    std::memcpy(out+4*68,&c68,4); std::memcpy(out+4*69,&c69,4); std::memcpy(out+4*70,&spot,4); std::memcpy(out+4*71,&line,4);
    const float nearz=At<float>(*view,0x54), farz=At<float>(*view,0x58); std::memcpy(out+4*72,&nearz,4); std::memcpy(out+4*73,&farz,4);
    std::memcpy(out+4*74,cam8->data+0x4B8,8); std::memcpy(out+4*76,&tile,4); std::memcpy(out+4*77,&tile_div,4);
    return true;
}
// cbSampleOffset of 1CB9D0 (depth downsample): W, H = the source texture's D3D11 size (vtable+0x40/+0x48 of argument 4
// call GetDesc on [[arg4+0x40]+0x18]; v20 region 9 = [arg4+0x40], its +0x18 the texture); the four 2x2 taps
// {-.5/W, -.5/H}, {.5/W, -.5/H}, {-.5/W, .5/H}, {.5/W, .5/H} (z, w zero); entries 4-15 are stack residue: zero.
bool DownsampleOffsets(const Snapshot& s,const Snapshots& all,uint8_t* out,uint32_t bytes) {
    const Region* tex=s.Find(9); if(!tex || tex->bytes<0x20 || bytes<64) return false;
    uint32_t w{},h{}; if(!all.TextureSize(At<uint64_t>(*tex,0x18),w,h) || !w || !h) return false;
    float lo{},hi{}; if(!Rdata(0x3D7F0C0,&lo,4) || !Rdata(0x3D7EDB4,&hi,4)) return false;   // -0.5, 0.5
    const float W=static_cast<float>(static_cast<int64_t>(w)), H=static_cast<float>(static_cast<int64_t>(h));
    const float x0=lo/W, x1=hi/W, y0=lo/H, y1=hi/H;
    const float v[16]={x0,y0,0,0, x1,y0,0,0, x0,y1,0,0, x1,y1,0,0}; std::memcpy(out,v,64); return true;
}
// 14001CE40: row-major A x B, each output row (a.z B2 + a.x B0) + (a.w B3 + a.y B1) (SSE order).
void Multiply(const float* a,const float* b,float* out) {
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) { const float* r=a+4*i;
        out[4*i+j]=(r[2]*b[8+j]+r[0]*b[j])+(r[3]*b[12+j]+r[1]*b[4+j]); }
}
// cbPerScene (1D6D20, cascaded sun shadow; argument 1 = pass, argument 2 = shadow state, v20 region 7 up to +0xC00):
// transpose(4BCA0 inverse of pass+0x50), then for the 4 cascades transpose(1CE40(pass+0xD0, state+0xAF0+0x40 i)), then
// g_CascadeSelectDist = state+0xAA0. Built only when [state+0x110] is set.
bool PerScene(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region *pass=s.Find(6), *state=s.Find(7); if(!pass || !state || pass->bytes<0x110 || state->bytes<0xBF0 || bytes<336) return false;
    if(!At<uint64_t>(*state,0x110)) return false;
    float m[16],inv[16],f[84]{};
    for(int i=0;i<16;++i) m[i]=At<float>(*pass,0x50+4*i);
    ds3prod::InverseMatrix(m,inv);
    for(int c=0;c<4;++c) for(int k=0;k<4;++k) f[4*c+k]=inv[4*k+c];
    float cam[16]; for(int i=0;i<16;++i) cam[i]=At<float>(*pass,0xD0+4*i);
    for(int n=0;n<4;++n) { float b[16],p[16]; for(int i=0;i<16;++i) b[i]=At<float>(*state,0xAF0+0x40*n+4*i);
        Multiply(cam,b,p); for(int c=0;c<4;++c) for(int k=0;k<4;++k) f[16+16*n+4*c+k]=p[4*k+c]; }
    for(int i=0;i<4;++i) f[80+i]=At<float>(*state,0xAA0+4*i);
    std::memcpy(out,f,336); return true;
}
// cbSSAOParam2 (1B26E0; p1 = this, p2 = pass, prev = *argument 2 = the previous-frame object, v20 region 8): 0
// transpose(4BCA0 inverse of p2+0x50), 16 {W, H, 1/W, 1/H} (W, H = uint p1+0xA0/+0xA4), 20 transpose(p2+0x10),
// 36 {f [p1+0x168], ([p1+0x16C] - 1) f + 1, 0, 0} with f = [prev+0x64], 40 {([p1+0xC] / [0x143D7F044]) [p1+0x170],
// [p1+0x174], [p1+0x178], 0}, 44 transpose(prev+0x20), 60 {p1+0x17C (3 floats), 0}, 64 {[p1+0x188], 0, 0, 0},
// 68 {([p2+0x78] d + [p2+0x88]) / d, 1 / [p1+0x198], -([p1+0x194] / [p1+0x198]), 0} with d = [p1+0x194] + [p1+0x198].
bool SsaoParam2(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region *p1=s.Find(5), *p2=s.Find(6), *prev=s.Find(8);
    if(!p1 || !p2 || !prev || p1->bytes<0x19C || p2->bytes<0x90 || prev->bytes<0x68 || bytes<288) return false;
    float k{}; uint32_t sign{}; if(!Rdata(0x3D7F044,&k,4) || !Rdata(0x3D801C0,&sign,4)) return false;
    float f[72]{}, m[16], inv[16];
    for(int i=0;i<16;++i) m[i]=At<float>(*p2,0x50+4*i);
    ds3prod::InverseMatrix(m,inv);
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) f[4*c+r]=inv[4*r+c];
    const float W=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*p1,0xA0))), H=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*p1,0xA4)));
    f[16]=W; f[17]=H; f[18]=1.0f/W; f[19]=1.0f/H;
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) f[20+4*c+r]=At<float>(*p2,0x10+16*r+4*c);
    const float g=At<float>(*prev,0x64);
    f[36]=g*At<float>(*p1,0x168); f[37]=(At<float>(*p1,0x16C)-1.0f)*g+1.0f;
    const float range=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*p1,0xC)))/k;
    f[40]=range*At<float>(*p1,0x170); f[41]=At<float>(*p1,0x174); f[42]=At<float>(*p1,0x178);
    for(int c=0;c<4;++c) for(int r=0;r<4;++r) f[44+4*c+r]=At<float>(*prev,0x20+16*r+4*c);
    f[60]=At<float>(*p1,0x17C); f[61]=At<float>(*p1,0x180); f[62]=At<float>(*p1,0x184);
    std::memcpy(out,f,288); std::memcpy(out+4*64,p1->data+0x188,4);
    const float d=At<float>(*p1,0x194)+At<float>(*p1,0x198);
    const float r0=(At<float>(*p2,0x78)*d+At<float>(*p2,0x88))/d, r1=1.0f/At<float>(*p1,0x198);
    float r2=At<float>(*p1,0x194)/At<float>(*p1,0x198); uint32_t bits; std::memcpy(&bits,&r2,4); bits^=sign; std::memcpy(&r2,&bits,4);
    std::memcpy(out+4*68,&r0,4); std::memcpy(out+4*69,&r1,4); std::memcpy(out+4*70,&r2,4);
    return true;
}
// cbBlurParam (1D7850, screen-space shadow blur; p2 = pass, view = [p2+0x418] (v20 region 8), q = argument 2, global =
// [G+0x5368] (region 9)): s = ([global+0x128], else 1) * [q+0x24]; built when s > 0 and [q+0x28] != 0.
// {[p2+0x78], [p2+0x88] / [view+0x58], 0, 0}, then for the 12 kernel taps (x, y) of the executable table below
// {x / W * s, y / H * s, exp((y y + x x) * -0.5) / sqrt([0x143D7EEFC]), 0} with W, H = uint [view+0x5C] / [view+0x60]
// (141FDC9A0 = expf, 141FD1F60 = sqrtf; expf is taken from this build's CRT and checked against the oracle).
bool BlurParam(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region *p2=s.Find(6), *q=s.Find(7), *view=s.Find(8), *global=s.Find(9);
    if(!p2 || !q || !view || p2->bytes<0x90 || view->bytes<0x64 || bytes<208) return false;
    float scale=1.0f; if(global && global->bytes>=0x12C) scale=At<float>(*global,0x128);
    scale=scale*At<float>(*q,0x24);
    if(!(0.0f<scale) || !At<int32_t>(*q,0x28)) return false;
    static const uint32_t taps[12]={0x3D7F4D0,0x3D7F4C0,0x3D7F4A0,0x3D7F4B0,0x3D7F260,0x3D7F240,0x3D7F1A0,0x3D7F200,0x3D7F390,0x3D7F370,0x3D7F380,0x3D7F3A0};
    float k{},neg_half{}; if(!Rdata(0x3D7EEFC,&k,4) || !Rdata(0x3D7F0C0,&neg_half,4)) return false;
    const float norm=std::sqrt(k);
    const float W=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*view,0x5C))), H=static_cast<float>(static_cast<int64_t>(At<uint32_t>(*view,0x60)));
    const float head[4]={At<float>(*p2,0x78),At<float>(*p2,0x88)/At<float>(*view,0x58),0,0}; std::memcpy(out,head,16);
    for(int i=0;i<12;++i) { float t[2]; if(!Rdata(taps[i],t,8)) return false; const float x=t[0], y=t[1];
        const float e=std::exp((y*y+x*x)*neg_half);
        const float v[4]={(x/W)*scale,(y/H)*scale,e/norm,0}; std::memcpy(out+16+16*i,v,16); }
    return true;
}
// 1401496D0: one 0x120-byte cbLight record from a parameter-bank light set `r` (0x260 bytes) and the view rows `v`
// (16 floats), transcribed in Ghidra's operation order. Directions are rotated by pi (sin/cos of 0x143D7EEC0 = pi)
// when the flags r+0x24C/D/E are set; masks [0x1439F2720] clear w; colours are c^2 * intensity.
// View transform of a rotated direction r: (r3 V3 + r2 V2) + (r1 V1 + r0 V0), the SSE order of 1496D0 (I-0045 fix).
void SunRecord(const uint8_t* r,const float* v,uint8_t* o) {
    auto F=[&](uint32_t off){ float x; std::memcpy(&x,r+off,4); return x; };
    auto M=[&](uint32_t off,int lane){ uint32_t u; std::memcpy(&u,r+off,4); if(lane==3) u=0; float x; std::memcpy(&x,&u,4); return x; };   // & 0xFFFFFFFF,..,0
    auto P=[&](uint32_t off,float x){ std::memcpy(o+off,&x,4); };
    auto U=[&](uint32_t off,uint32_t x){ std::memcpy(o+off,&x,4); };
    float c080[4],c480[4],cfa0[4],c500[4],c600[4],pi{}; Rdata(0x3D1C080,c080,16); Rdata(0x3D1C480,c480,16); Rdata(0x3D1BFA0,cfa0,16); Rdata(0x39F2500,c500,16); Rdata(0x39F2600,c600,16); Rdata(0x3D7EEC0,&pi,4);
    const float sa=std::sin(pi), ca=std::cos(pi);   // 141FD1A40 (sinf), 141FD1740 (cosf)
    std::memcpy(o,r,16);
    if(!r[0x24C]) std::memcpy(o+0x10,r+0x70,16);
    else { const float f17=M(0x70,0),f18=M(0x74,1),f19=M(0x78,2),f20=M(0x7C,3);
        const float f8=f19*sa+f20*c080[0]+f17*ca*c480[0]+f18*cfa0[0], f10=f19*0.0f+f20*c080[1]+f17*c480[1]*0.0f+f18*cfa0[1];
        const float f12=f19*ca+f20*c080[2]+f17*sa*c480[2]+f18*cfa0[2], f14=f19*0.0f+f20*c080[3]+f17*c480[3]*0.0f+f18*cfa0[3];
        for(int k=0;k<4;++k) P(0x10+4*k,(f14*v[12+k]+f12*v[8+k])+(f10*v[4+k]+f8*v[k])); }
    std::memcpy(o+0x0C,r+0x40,4); std::memcpy(o+0x1C,r+0xB0,4);
    auto Colour=[&](uint32_t src,uint32_t dst){ const float k=F(src+12); P(dst,F(src)*F(src)*k); P(dst+4,F(src+4)*F(src+4)*k); P(dst+8,F(src+8)*F(src+8)*k); P(dst+12,k*k); };
    Colour(0x10,0x40); Colour(0x80,0x50); Colour(0x20,0x80); Colour(0x90,0x90);
    if(!r[0x24D]) U(0x100,0);
    else { const float f17=M(0xE0,0),f18=M(0xE4,1),f19=M(0xE8,2),f20=M(0xEC,3);
        const float f9=f18*cfa0[0]+f17*ca*c480[0]+f20*c080[0]+f19*sa, f11=f18*cfa0[1]+f17*c480[1]*0.0f+f20*c080[1]+f19*0.0f;
        const float f13=f18*cfa0[2]+f17*sa*c480[2]+f20*c080[2]+f19*ca, f15=f18*cfa0[3]+f17*c480[3]*0.0f+f20*c080[3]+f19*0.0f;
        for(int k=0;k<3;++k) P(0x20+4*k,(f15*v[12+k]+f13*v[8+k])+(f11*v[4+k]+f9*v[k]));
        std::memcpy(o+0x2C,r+0x120,4); Colour(0xF0,0x60);
        U(0x100,1); { const float k=F(0x10C); P(0xA0,F(0x100)*F(0x100)*k); P(0xA4,F(0x104)*F(0x104)*k); P(0xA8,F(0x108)*F(0x108)*k); P(0xAC,k*k); } }
    if(!r[0x24E]) U(0x104,0);
    else { const float f17=M(0x150,0),f18=M(0x154,1),f19=M(0x158,2),f20=M(0x15C,3);
        const float f9=f20*c080[0]+f19*sa+f18*cfa0[0]+f17*ca*c480[0], f11=f20*c080[1]+f19*0.0f+f18*cfa0[1]+f17*c480[1]*0.0f;
        const float f13=f20*c080[2]+f19*ca+f18*cfa0[2]+f17*sa*c480[2], f15=f20*c080[3]+f19*0.0f+f18*cfa0[3]+f17*c480[3]*0.0f;
        for(int k=0;k<3;++k) P(0x30+4*k,(f15*v[12+k]+f13*v[8+k])+(f11*v[4+k]+f9*v[k]));
        std::memcpy(o+0x3C,r+0x190,4); Colour(0x160,0x70);
        U(0x104,1); { const float k=F(0x17C); P(0xB0,F(0x170)*F(0x170)*k); P(0xB4,F(0x174)*F(0x174)*k); P(0xB8,F(0x178)*F(0x178)*k); P(0xBC,k*k); } }
    const float k1=F(0x1EC), k2=F(0x1FC);
    const float a0=M(0x220,0)+c500[0], a1=M(0x224,1)+c500[1], a2=M(0x228,2)+c500[2], a3=M(0x22C,3)+c500[3];
    const float f11=F(0x1E0)*F(0x1E0)*k1, f15=F(0x1E4)*F(0x1E4)*k1, f5=F(0x1E8)*F(0x1E8)*k1;
    const float f17=F(0x1F0)*F(0x1F0)*k2, f18=F(0x1F4)*F(0x1F4)*k2, f6=F(0x1F8)*F(0x1F8)*k2;
    const float d0=(f11-f17)*c600[0], d1=(f15-f18)*c600[1], d2=(f5-f6)*c600[2];
    const float s0=(f17+f11)*c600[0], s1=(f18+f15)*c600[1], s2=(f6+f5)*c600[2];
    P(0xC0,d0*a0); P(0xC4,d0*a1); P(0xC8,d0*a2); P(0xCC,s0*a3);
    P(0xD0,d1*a0); P(0xD4,d1*a1); P(0xD8,d1*a2); P(0xDC,s1*a3);
    P(0xE0,d2*a0); P(0xE4,d2*a1); P(0xE8,d2*a2); P(0xEC,s2*a3);
    std::memcpy(o+0x110,r+0x50,4); std::memcpy(o+0x114,r+0xC0,4); std::memcpy(o+0x118,r+0x130,4); std::memcpy(o+0x11C,r+0x1A0,4);
    { const float k=F(0x23C); P(0xF0,F(0x230)*F(0x230)*k); P(0xF4,F(0x234)*F(0x234)*k); P(0xF8,F(0x238)*F(0x238)*k); std::memcpy(o+0xFC,r+0x240,4); }
}
// I-0063 (i63-live-26): 14014A2D0, the cbLight record of a blended entry (the engine moving between two light sets, e.g. a
// doorway in m45's Corvian Settlement): every field of 1496D0 from the two sets a, b lerped as (b - a) * t + a, in Ghidra's
// operation order. Its arguments point 0x40 bytes before the sets SunRecord reads (p + X = r + X - 0x40); the view transform
// keeps SunRecord's pairwise SSE order. Section flags: the direction rotation by the nearer set's flag (a below t = 1, else
// b), the second and third lights when either set has them.
void BlendSunRecord(const uint8_t* a,const uint8_t* b,float t,const float* v,uint8_t* o) {
    auto L=[&](uint32_t off){ float x,y; std::memcpy(&x,a+off,4); std::memcpy(&y,b+off,4); return (y-x)*t+x; };
    auto LM=[&](uint32_t off,int lane){ float x=L(off); if(lane==3) { uint32_t u=0; std::memcpy(&x,&u,4); } return x; };
    auto P=[&](uint32_t off,float x){ std::memcpy(o+off,&x,4); };
    auto U=[&](uint32_t off,uint32_t x){ std::memcpy(o+off,&x,4); };
    float c080[4],c480[4],cfa0[4],c500[4],c600[4],pi{},one{}; Rdata(0x3D1C080,c080,16); Rdata(0x3D1C480,c480,16); Rdata(0x3D1BFA0,cfa0,16); Rdata(0x39F2500,c500,16); Rdata(0x39F2600,c600,16); Rdata(0x3D7EEC0,&pi,4); Rdata(0x3D7EE10,&one,4);
    const float sa=std::sin(pi), ca=std::cos(pi);
    auto View=[&](uint32_t dst,float f6,float f7,float f8,float f9,int n){ for(int k=0;k<n;++k) P(dst+4*k,(f9*v[12+k]+f8*v[8+k])+(f7*v[4+k]+f6*v[k])); };
    P(0x00,L(0x00)); P(0x04,L(0x04)); P(0x08,L(0x08));
    if((a[0x24C] && t<one) || (b[0x24C] && one<=t)) { const float x=LM(0x70,0),y=LM(0x74,1),z=LM(0x78,2),w=LM(0x7C,3);
        const float f6=x*ca*c480[0]+y*cfa0[0]+w*c080[0]+z*sa, f7=x*c480[1]*0.0f+y*cfa0[1]+w*c080[1]+z*0.0f;
        const float f8=x*sa*c480[2]+y*cfa0[2]+w*c080[2]+z*ca, f9=x*c480[3]*0.0f+y*cfa0[3]+w*c080[3]+z*0.0f;
        View(0x10,f6,f7,f8,f9,4); }
    else { P(0x10,L(0x70)); P(0x14,L(0x74)); P(0x18,L(0x78)); }
    P(0x0C,L(0x40)); P(0x1C,L(0xB0));
    auto Colour=[&](uint32_t src,uint32_t dst){ const float r=L(src),g=L(src+4),bl=L(src+8),k=L(src+12); P(dst,r*r*k); P(dst+4,g*g*k); P(dst+8,bl*bl*k); P(dst+12,k*k); };
    Colour(0x10,0x40); Colour(0x80,0x50); Colour(0x20,0x80); Colour(0x90,0x90);
    if(!a[0x24D] && !b[0x24D]) U(0x100,0);
    else { const float x=LM(0xE0,0),y=LM(0xE4,1),z=LM(0xE8,2),w=LM(0xEC,3);
        const float f6=w*c080[0]+z*sa+x*ca*c480[0]+y*cfa0[0], f7=w*c080[1]+z*0.0f+x*c480[1]*0.0f+y*cfa0[1];
        const float f8=w*c080[2]+z*ca+x*sa*c480[2]+y*cfa0[2], f9=w*c080[3]+z*0.0f+x*c480[3]*0.0f+y*cfa0[3];
        View(0x20,f6,f7,f8,f9,3); P(0x2C,L(0x120)); Colour(0xF0,0x60); Colour(0x100,0xA0); U(0x100,1); }
    if(!a[0x24E] && !b[0x24E]) U(0x104,0);
    else { const float x=LM(0x150,0),y=LM(0x154,1),z=LM(0x158,2),w=LM(0x15C,3);
        const float f6=x*ca*c480[0]+y*cfa0[0]+z*sa+w*c080[0], f7=x*c480[1]*0.0f+y*cfa0[1]+z*0.0f+w*c080[1];
        const float f8=x*sa*c480[2]+y*cfa0[2]+z*ca+w*c080[2], f9=x*c480[3]*0.0f+y*cfa0[3]+z*0.0f+w*c080[3];
        View(0x30,f6,f7,f8,f9,3); P(0x3C,L(0x190)); Colour(0x160,0x70); Colour(0x170,0xB0); U(0x104,1); }
    { const float k1=L(0x1EC), k2=L(0x1FC);
      const float e0=L(0x1E0)*L(0x1E0)*k1, e1=L(0x1E4)*L(0x1E4)*k1, e2=L(0x1E8)*L(0x1E8)*k1;
      const float g0=L(0x1F0)*L(0x1F0)*k2, g1=L(0x1F4)*L(0x1F4)*k2, g2=L(0x1F8)*L(0x1F8)*k2;
      const float d0=(e0-g0)*c600[0], d1=(e1-g1)*c600[1], d2=(e2-g2)*c600[2], s0=(e0+g0)*c600[0], s1=(e1+g1)*c600[1], s2=(e2+g2)*c600[2];
      const float a0=LM(0x220,0)+c500[0], a1=LM(0x224,1)+c500[1], a2=LM(0x228,2)+c500[2], a3=LM(0x22C,3)+c500[3];
      P(0xC0,d0*a0); P(0xC4,d0*a1); P(0xC8,d0*a2); P(0xCC,s0*a3); P(0xD0,d1*a0); P(0xD4,d1*a1); P(0xD8,d1*a2); P(0xDC,s1*a3);
      P(0xE0,d2*a0); P(0xE4,d2*a1); P(0xE8,d2*a2); P(0xEC,s2*a3); }
    P(0x110,L(0x50)); P(0x114,L(0xC0)); P(0x118,L(0x130)); P(0x11C,L(0x1A0));
    { const float k=L(0x23C), r=L(0x230), g=L(0x234), bl=L(0x238); P(0xF0,r*r*k); P(0xF4,g*g*k); P(0xF8,bl*bl*k); P(0xFC,L(0x240)); }
}
// 14014E900: one 0x60-byte cbFog record from a fog set p (floats) and the view (+0x34 height h). 141FD3430(2, x) = powf.
// I-0063: when the global byte [0x144961ABD] is set (by D573B0, for the whole session and every zone; natively since the wave-1
// closure, FOG-BYTE-4961ABD) the distance factor is p[6] (record +0x18; m45: 0.01, the native banks give the same): `flag` (adapter v45d: region 3 of the 14E900 snapshot;
// earlier captures: clear) and `p6` (the engine's copied record word) carry them.
void FogRecord(const float* p,const uint8_t* view,float* o,bool flag=false,float p6=0.0f) {
    float one{},tenth{},two{}; uint32_t sign{}; Rdata(0x3D7EE10,&one,4); Rdata(0x3D7ED70,&tenth,4); Rdata(0x3D7EE7C,&two,4); Rdata(0x3D801C0,&sign,4);
    float f7=tenth<p[5]?p[5]:0.1f;
    if(!reinterpret_cast<const uint8_t*>(p)[0x1C]) { uint32_t u; std::memcpy(&u,&f7,4); u^=sign; std::memcpy(&f7,&u,4); }
    f7=one/f7;
    if(flag) f7=p6;
    const float f9=p[4]<=0.0f?0.0f:one/p[4];
    float h; std::memcpy(&h,view+0x34,4);
    const float f6=std::pow(two,(0.0f-h)*f7);
    o[0]=p[3]*p[8]; o[1]=f9; o[2]=f6*f9; o[3]=f7; o[4]=p[0]*p[0]; o[5]=p[1]*p[1]; o[6]=p[2]*p[2]; o[7]=p[9];
    const float f8=p[16]<=0.0f?0.0f:one/p[16];
    o[8]=p[3]*p[8]; o[9]=p[17]; o[10]=f9; o[11]=f7; o[12]=p[15]*p[8]; o[13]=p[20]+p[18]; o[14]=f8*f7; o[15]=h;
    o[16]=p[12]*p[12]; o[17]=p[13]*p[13]; o[18]=p[14]*p[14]; o[19]=p[9];
    const uint32_t one_bits=1; std::memcpy(o+20,&one_bits,4);
}
// cbLight / cbFog (137910): the builders called for each entry of the call (persistent record children or in-window
// records with this call as parent), in call order: record i of the block = the i-th 1496D0 (sun) / 14E900 (fog) call.
// A 14A2D0 blend among them is not transcribed (no such call in the captures): not produced.
bool SunFog(const Snapshot& s,const Snapshots* all,bool fog,uint8_t* out,uint32_t bytes) {
    static const uint32_t sun_snap=[]{ char v[16]{}; size_t n=0; return (!getenv_s(&n,v,sizeof v,"LIGHT_SUN_SNAP") && n)?static_cast<uint32_t>(std::strtoul(v,nullptr,0)):0u; }();
    if(!fog && all && sun_snap) {   // diagnostic: take another call's inputs (LIGHT_SUN_SNAP)
        const auto it=all->All().find(sun_snap); if(it!=all->All().end() && it->second.id!=s.id) return SunFog(it->second,all,fog,out,bytes); }
    std::vector<const std::vector<Region>*> calls; std::vector<uint32_t> writers;
    for(const auto& c:s.children) { calls.push_back(&c.regions); writers.push_back(c.writer); }
    if(all) for(const Snapshot* x:all->ChildrenOf(s.id)) { calls.push_back(&x->regions); writers.push_back(x->writer); }
    uint32_t at=0; const uint32_t stride=fog?0x60u:0x120u;
    // I-0045 (UseBanks): entry k of the call (137910 argument 0: count +0x90, entries [+0x88] stride 0x30, keys {area, ID}
    // at +8 / +0x10 (sun) and +0x18 / +0x20 (fog)) names the record of the k-th builder call of its kind.
    const Region *self=s.Find(2), *entries=s.Find(3); unsigned k=0;
    for(size_t i=0;i<calls.size() && at+stride<=bytes;++i) {
        if(!fog && writers[i]==W14A2D0) {   // I-0063: a blended entry (two light sets, the factor its 5th argument)
            const Region *args=nullptr,*v=nullptr; for(const auto& r:*calls[i]) { if(r.key==0) args=&r; if(r.key==1) v=&r; }
            if(!g_banks || !args || args->bytes<40 || !v || v->bytes<0x40 || !self || !entries || k>=static_cast<unsigned>(At<int32_t>(*self,0x90))) return false;
            const uint32_t base=0x30u*k+8u; const uint32_t area_a=At<uint32_t>(*entries,base), id_a=At<uint32_t>(*entries,base+4), area_b=At<uint32_t>(*entries,base+8), id_b=At<uint32_t>(*entries,base+12);
            {   // the variant from the copied sets themselves (p + 0x40 = the record's first 0x220 bytes): a later unblended entry
                // must not decide it after this one is built (i63-live-26 frame 600: the previous variant's set 100)
                // (review) both sets: b's area may differ from a's
                for(const auto& [key,ar,ir]:{std::make_tuple(2u,area_a,id_a),std::make_tuple(3u,area_b,id_b)}) { if(key==3u && ar==area_a) continue;
                    const Region *sa=nullptr; for(const auto& r:*calls[i]) if(r.key==key) sa=&r;
                    if(sa && sa->bytes>=0x260) { alignas(16) uint8_t copied[0x260]{}, probe[0x260]; std::memcpy(copied,sa->data+0x40,0x220); bool same=g_banks->Sun(ar,ir,probe,sizeof probe);
                        for(const auto& r:sun_read) for(uint32_t o=r[0];o<r[1] && r[1]<=0x220 && same;o+=(r[1]-r[0]>=4)?4u:1u) same=!std::memcmp(copied+o,probe+o,(r[1]-r[0]>=4)?4u:1u);
                        if(!same) g_banks->SelectVariant(ar,ir,copied,false,0x220); } } }
            alignas(16) uint8_t ra[0x260]{}, rb[0x260]{}; if(!g_banks->Sun(area_a,id_a,ra,sizeof ra) || !g_banks->Sun(area_b,id_b,rb,sizeof rb)) return false;
            float t; std::memcpy(&t,args->data+32,4); float vv[16]; std::memcpy(vv,v->data,64);
            { static const bool why=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_LIGHTWHY") && m; }();   /* I-0063 diagnostic */
              if(why) { uint64_t aa[8]; std::memcpy(aa,args->data,64); std::fprintf(stderr,"{\"blend_entry\":%u,\"a\":[%u,%u],\"b\":[%u,%u],\"t\":%g,\"args\":[\"%llX\",\"%llX\",\"%llX\",\"%llX\",\"%llX\",\"%llX\"]}\n",k,area_a,id_a,area_b,id_b,t,
                  static_cast<unsigned long long>(aa[0]),static_cast<unsigned long long>(aa[1]),static_cast<unsigned long long>(aa[2]),static_cast<unsigned long long>(aa[3]),static_cast<unsigned long long>(aa[4]),static_cast<unsigned long long>(aa[5]));
                for(const auto& r:*calls[i]) if(r.key==2 || r.key==3) { const uint8_t* bank=r.key==2?ra:rb; int best=-1; for(int sh=-0x60;sh<=0x60 && best<0;sh+=4) { int eq=0; for(uint32_t o=0x60;o<0x200;o+=4) if(o+sh<r.bytes && o<0x260 && !std::memcmp(r.data+o+sh,bank+o,4)) ++eq; if(eq>80) best=sh; }
                    std::fprintf(stderr,"{\"blend_set\":%u,\"bytes\":%u,\"shift\":%d,\"p40\":[%g,%g,%g,%g],\"bank0\":[%g,%g,%g,%g]}\n",r.key,r.bytes,best,At<float>(r,0x40),At<float>(r,0x44),At<float>(r,0x48),At<float>(r,0x4C),[&]{ float x; std::memcpy(&x,bank,4); return x; }(),[&]{ float x; std::memcpy(&x,bank+4,4); return x; }(),[&]{ float x; std::memcpy(&x,bank+8,4); return x; }(),[&]{ float x; std::memcpy(&x,bank+12,4); return x; }()); } } }
            BlendSunRecord(ra,rb,t,vv,out+at); ++k; at+=stride; continue; }
        if(writers[i]!=(fog?W14E900:W1496D0)) continue;
        const Region *rec=nullptr,*v=nullptr,*gflag=nullptr; for(const auto& r:*calls[i]) { if(r.key==1) rec=&r; if(r.key==2) v=&r; if(r.key==3) gflag=&r; }
        if(!rec || !v) return false;
        const uint8_t* record=rec->data; alignas(16) uint8_t built[0x260]{};
        if(g_banks) {
            if(!self || !entries || k>=static_cast<unsigned>(At<int32_t>(*self,0x90))) return false;
            const uint32_t base=0x30u*k+(fog?0x18u:8u);
            const uint32_t area=At<uint32_t>(*entries,base), id=At<uint32_t>(*entries,base+4);
            { static const bool why=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_LIGHTWHY") && m; }();   /* I-0063 diagnostic: the entry's two keys */
              if(why) std::fprintf(stderr,"{\"light_entry\":%u,\"fog\":%d,\"a\":[%u,%u],\"b\":[%u,%u],\"blend\":[%g,%g,%g,%g]}\n",k,fog?1:0,area,id,At<uint32_t>(*entries,base+8),At<uint32_t>(*entries,base+12),
                  At<float>(*entries,0x30u*k+0x28),At<float>(*entries,0x30u*k+0x2C),At<float>(*entries,0x30u*k),At<float>(*entries,0x30u*k+4)); }
            if(area!=At<uint32_t>(*entries,base+8) || id!=At<uint32_t>(*entries,base+12)) return false;   // blended entry: not transcribed
            if(!fog && rec->bytes>=0x250) {   /* I-0063: the GPARAM variant the engine loaded for this area (the record's values decide among the map's files) */
                alignas(16) uint8_t probe[0x260]; bool same=g_banks->Sun(area,id,probe,sizeof probe);
                for(const auto& r:sun_read) for(uint32_t o=r[0];o<r[1] && same;o+=(r[1]-r[0]>=4)?4u:1u) same=!std::memcmp(rec->data+o,probe+o,(r[1]-r[0]>=4)?4u:1u);
                if(!same) g_banks->SelectVariant(area,id,rec->data); }
            if(fog && rec->bytes>=0x58) {   /* I-0063: the same for the fog record (m45 Snowfield: m45_00_0100's fog over m45_00_0000's light set) */
                alignas(16) uint8_t probe[0x260]; bool same=g_banks->Fog(area,id,probe,sizeof probe);
                for(const auto& r:fog_variant_read) for(uint32_t o=r[0];o<r[1] && same;o+=(r[1]-r[0]>=4)?4u:1u) same=!std::memcmp(rec->data+o,probe+o,(r[1]-r[0]>=4)?4u:1u);
                if(!same) g_banks->SelectVariant(area,id,rec->data,true); }
            if(!(fog?g_banks->Fog(area,id,built,sizeof built):g_banks->Sun(area,id,built,sizeof built))) return false;
            if(fog) { if(rec->bytes>=0x58) CompareRecord(fog_read,rec->data,built,"fog"); }
            else if(rec->bytes>=0x250) CompareRecord(sun_read,rec->data,built,"sun");
            record=built; }
        ++k;
        if(fog) { static const bool why=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_FOGWHY") && m; }();   /* I-0064 diagnostic: the copied fog record against the built one */
            if(why && rec->bytes>=0x58) { std::string d; for(uint32_t o=0;o<0x58;o+=4) { uint32_t x,y; std::memcpy(&x,rec->data+o,4); std::memcpy(&y,record+o,4); if(x!=y) { char c[64]; float fx,fy; std::memcpy(&fx,&x,4); std::memcpy(&fy,&y,4); std::snprintf(c,sizeof c," %X:%g/%g",o,fx,fy); d+=c; } }
                std::fprintf(stderr,"{\"fogwhy\":%u,\"copied_vs_built\":\"%s\",\"flag\":%d,\"b1c\":[%u,%u]}\n",k,d.c_str(),gflag && gflag->bytes?gflag->data[0]:-1,rec->data[0x1C],record[0x1C]); } }
        if(fog) { if(rec->bytes<0x58 || v->bytes<0x38) return false; float p[22]; std::memcpy(p,record,sizeof p); { static const bool force=[]{ size_t n=0; return !getenv_s(&n,nullptr,0,"LIGHT_FOG_FLAG") && n; }();   /* diagnostic: the flag set on captures before v45d */
            const bool flag=force || (gflag && gflag->bytes && gflag->data[0]); float p6=0; std::memcpy(&p6,rec->data+0x18,4); FogRecord(p,v->data,reinterpret_cast<float*>(out+at),flag,p6); } }
        else { if(rec->bytes<0x244 || v->bytes<0x40) return false; float vv[16]; std::memcpy(vv,v->data,64); SunRecord(record,vv,out+at); }
        at+=stride; }
    return at>0;
}
// 140112D80: per channel c, expf(-(r / (f_c + [0x143D7ED10]))^2 / (2 v)) / (v [0x143D7EEF4]), f = the falloff colour
// at 0x144745FF0 (.data, initialised). 140112E60: the five-Gaussian profile, channel sums (((g5 + g4) + g3) + g2) + g1.
bool SssProfile(float r,float out[3]) {
    float falloff[3],eps{},norm{},two{}; if(!Rdata(0x4745FF0,falloff,12) || !Rdata(0x3D7ED10,&eps,4) || !Rdata(0x3D7EEF4,&norm,4) || !Rdata(0x3D7EE7C,&two,4)) return false;
    static const uint32_t var_rva[5]={0x3D7EF1C,0x3D7EE78,0x3D7EDC8,0x3D7ED8C,0x3D7ED54}, w_rva[5]={0x3D7ED68,0x3D7EDA8,0x3D7ED74,0x3D7ED78,0x3D7ED70};
    float g[5][3];
    for(int j=0;j<5;++j) { float v{},w{}; if(!Rdata(var_rva[j],&v,4) || !Rdata(w_rva[j],&w,4)) return false;
        const float twov=v*two, vn=v*norm;
        for(int c=0;c<3;++c) { float t=r/(falloff[c]+eps); t=t*t; t=-t; t=t/twov; g[j][c]=(std::exp(t)/vn)*w; } }
    for(int c=0;c<3;++c) out[c]=(((g[4][c]+g[3][c])+g[2][c])+g[1][c])+g[0][c];
    return true;
}
// 1401130F0: the separable SSS kernel of a profile with strength s (p+0x20/+0x24/+0x28), 17 float4 {r, g, b, offset}:
// offset_i = |powf(x, 2)| * (sign 2) / powf(2, 2), x = i [0x143D7ED9C] - 2, sign = [0x143D7F0C8] for x < 0 else 1;
// rgb_i = profile(offset_i) * (|o_i - o_(i-1)| + |o_i - o_(i+1)|) * 0.5; entry 8 rotated to the front; rgb normalised by
// the sequential channel sums; entry 0 = (1 - s) + s e, the others s e.
bool SssKernel(const float strength[3],float k[17][4]) {
    float two{},step{},neg{},one{},half{}; if(!Rdata(0x3D7EE7C,&two,4) || !Rdata(0x3D7ED9C,&step,4) || !Rdata(0x3D7F0C8,&neg,4) || !Rdata(0x3D7EE10,&one,4) || !Rdata(0x3D7EDB4,&half,4)) return false;
    const float denom=std::pow(two,two);
    for(int i=0;i<17;++i) { const float x=static_cast<float>(i)*step-two; const float sign=0.0f>x?neg:one; const float t=std::pow(x,two);
        k[i][3]=(static_cast<float>(std::fabs(static_cast<double>(t)))*(sign*two))/denom; }
    for(int i=0;i<17;++i) { const float left=i>0?static_cast<float>(std::fabs(static_cast<double>(k[i][3]-k[i-1][3]))):0.0f;
        const float right=i<16?static_cast<float>(std::fabs(static_cast<double>(k[i][3]-k[i+1][3]))):0.0f;
        const float area=(right+left)*half; float p[3]; if(!SssProfile(k[i][3],p)) return false;
        for(int c=0;c<3;++c) k[i][c]=p[c]*area; }
    float e8[4]; std::memcpy(e8,k[8],16); for(int i=8;i>0;--i) std::memcpy(k[i],k[i-1],16); std::memcpy(k[0],e8,16);
    float sum[3]={0,0,0}; for(int i=0;i<17;++i) for(int c=0;c<3;++c) sum[c]=k[i][c]+sum[c];
    for(int i=0;i<17;++i) for(int c=0;c<3;++c) k[i][c]=k[i][c]/sum[c];
    for(int c=0;c<3;++c) k[0][c]=(one-strength[c])+strength[c]*k[0][c];
    for(int i=1;i<17;++i) for(int c=0;c<3;++c) k[i][c]=strength[c]*k[i][c];
    return true;
}
// cbSSS (1141B0; the branch flag [0x14473D39D] is not captured, the used-profile branch is checked against the oracle):
// s = 1 / tanf(q+0x50 * 0.5) (q = argument 2; 141FD59E0 = tanf); profiles of the vector [0x1449622F8] (v21 regions 6 =
// profile objects, 8 = [p+0x38]); slot j (0x120 bytes) = {radius * s, 0, 0, 0, kernel (0x110 bytes)},
// radius = p+0x2C, or the +0x2C of the last profile along the p+0x38 links (112C60); kernel = SssKernel(p+0x20 strength)
// (1130F0 rebuilds it in the call).
// I-0045 (UseBanks): the profile values from game data. An unlinked profile at vector position k takes Banks::SssBase(k)
// (113D80 defaults, then one profile per MTD with g_SSSWidth in material-bank order); a linked one (22BF70) takes the
// GX15 strength of the FLVER materials of its base's MTD, the radius of that base and the 113D80 default at +0x2C.
// Which profiles exist, are linked to which base and are marked used this frame stay engine inputs.
bool SssProfiles(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region *a0=s.Find(2), *q=s.Find(4);
    {   static const bool why=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_SSSWHY") && m; }();   /* I-0063 diagnostic */
        if(why) std::fprintf(stderr,"{\"sss_entry\":1,\"a0\":%u,\"q\":%u,\"f44\":%d,\"f45\":%d,\"profiles\":%u}\n",a0?a0->bytes:0u,q?q->bytes:0u,a0&&a0->bytes>=0x48?At<uint8_t>(*a0,0x44):-1,a0&&a0->bytes>=0x48?At<uint8_t>(*a0,0x45):-1,s.Elements(6)); }
    if(!a0 || !q || a0->bytes<0x48 || q->bytes<0x54) return false;
    if(!At<uint8_t>(*a0,0x44) || !At<uint8_t>(*a0,0x45)) return false;
    float half{},one{}; if(!Rdata(0x3D7EDB4,&half,4) || !Rdata(0x3D7EE10,&one,4)) return false;
    const float scale=one/std::tan(At<float>(*q,0x50)*half);
    const unsigned n=s.Elements(6);
    auto radius_of=[&](unsigned k,float& r)->bool { const Region* p=s.Find(6|((k+1)<<16)); r=At<float>(*p,0x2C);
        if(At<uint64_t>(*p,0x38)) { const Region* next=s.Find(8|((k+1)<<16)); if(!next || At<uint64_t>(*next,0x38)) return false; r=At<float>(*next,0x2C); } return true; };
    // Profile k: words +0x20..+0x2C (strength, raw +0x2C) and the radius 1141B0 uses (112C60).
    struct Profile { uint32_t w[4]; float radius; };
    auto profile_of=[&](unsigned k,Profile& o)->bool { const Region* p=s.Find(6|((k+1)<<16)); if(!p || p->bytes<0x40) return false;
        if(!g_banks) { std::memcpy(o.w,p->data+0x20,16); return radius_of(k,o.radius); }
        const uint64_t link=At<uint64_t>(*p,0x38);
        if(!link) { if(!g_banks->SssBase(k,o.w)) return false; std::memcpy(&o.radius,&o.w[3],4); }
        else { unsigned j=n; for(unsigned t=0;t<n;++t) { const Region* b=s.Find(6|((t+1)<<16)); if(b && b->address==link) { j=t; break; } }
            if(j==n || At<uint64_t>(*s.Find(6|((j+1)<<16)),0x38)) return false;
            uint32_t base[4],def[4]; if(!g_banks->SssBase(j,base) || !g_banks->SssBase(0,def)) return false;
            // I-0063: when the base's MTD carries several GX15 strengths in game data (m39: C[ARSN]_SSS 0.3 and 1.0), which
            // material linked this profile is not known: its strength is taken from the profile object itself (engine input).
            if(!g_banks->SssInstance(j,o.w)) std::memcpy(o.w,p->data+0x20,12);
            o.w[3]=def[3]; std::memcpy(&o.radius,&base[3],4); }
        uint8_t built[0x40]; std::memcpy(built,p->data,0x40); std::memcpy(built+0x20,o.w,16);
        static constexpr uint32_t words[][2]={{0x20,0x30}}; CompareRecord(words,p->data,built,"sss profile");
        float copied_radius; if(!radius_of(k,copied_radius)) return false;
        static constexpr uint32_t radius_word[][2]={{0,4}}; CompareRecord(radius_word,reinterpret_cast<const uint8_t*>(&copied_radius),reinterpret_cast<const uint8_t*>(&o.radius),"sss radius");
        return true; };
    // Branch [0x14473D39D] != 0 (the one the captures follow): profiles marked used this frame (+0x36) are deduplicated
    // with 112C90 (equal radius and equal p+0x20 vector, 115BC0), unique ones fill slots 0, 1, ... in vector order.
    std::vector<Profile> unique;
    for(unsigned k=0;k<n;++k) { const Region* p=s.Find(6|((k+1)<<16)); if(!p || p->bytes<0x40 || !At<uint8_t>(*p,0x36)) continue;
        Profile pk; if(!profile_of(k,pk)) {
            static const bool why=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_SSSWHY") && m; }();   /* I-0063 diagnostic */
            if(why) { uint32_t b[4]{}; const uint64_t link=At<uint64_t>(*p,0x38); std::fprintf(stderr,"{\"sss_profile_failed\":%u,\"profiles\":%u,\"linked\":%d,\"base_known\":%d}\n",k,n,link?1:0,g_banks?(g_banks->SssBase(k,b)?1:0):-1); }
            return false; } bool dup=false;
        for(size_t j=unique.size();j-- >0;) if(unique[j].radius==pk.radius && !std::memcmp(unique[j].w,pk.w,16)) { dup=true; break; }
        if(!dup) unique.push_back(pk); }
    for(size_t j=0;j<unique.size() && 0x120u*(j+1)<=bytes;++j) {
        float strength[3]; std::memcpy(strength,unique[j].w,12); float kernel[17][4];
        if(!SssKernel(strength,kernel)) { static const bool why=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_SSSWHY") && m; }();   /* I-0063 diagnostic */
            if(why) std::fprintf(stderr,"{\"sss_kernel_failed\":%zu,\"strength\":[%g,%g,%g],\"radius\":%g}\n",j,static_cast<double>(strength[0]),static_cast<double>(strength[1]),static_cast<double>(strength[2]),static_cast<double>(unique[j].radius));
            return false; }
        const float v=unique[j].radius*scale;
        std::memcpy(out+0x120*j,&v,4); std::memcpy(out+0x120*j+0x10,kernel,0x110); }
    { static const bool why=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_SSSWHY") && m; }(); if(why) std::fprintf(stderr,"{\"sss_unique\":%zu,\"bytes\":%u}\n",unique.size(),bytes); }   /* I-0063 diagnostic */
    return !unique.empty();
}
// CB_ENVMAP_REGION (675B0; v21 regions 9 = IBL region records rec (0x200), 11 = [arg1+0x418] view, 12 = 1400416A0 lookups,
// 5 = this, 8 = the .data at 0x144929B00): block k for record k. W = rows {s.x R0, s.y R1, s.z R2, c R} (s = rec+0x10,
// R0..R2 = rec+0x20/0x30/0x40, T = rec+0x50, c = [0x144929B20]) each ((w T + y R1) + z R2) + x R0; world = transpose(W),
// inverse = transpose(4BCA0(W)), extents = rec+0x10, colour scale = lookup+0xC ([this+0x2B8] == 0) else rec+0xDC, fade
// distance = rec+0xCC, fade rate = 1 - clamp01((|clamp(L, -e, e) - L| - (rec+0xC8 - rec+0xD8)) / rec+0xD8) with
// L = (R0.d, R1.d, R2.d), d = view+0x30 - T (dots (x + y) + z), e = rec+0x10 (this+0x2C0/0x2C4 replace rec+0xC8/0xD8
// when this+0x2BD). Fields the lighting programmes do not read are left zero.
bool EnvRegion(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    const Region *self=s.Find(5), *rec=s.Find(9|((ordinal+1)<<16)), *view=s.Find(11), *look=s.Find(12|((ordinal+1)<<16)), *data=s.Find(8);
    if(!self || !rec || !view || !data || bytes<0x150 || rec->bytes<0xE0 || view->bytes<0x40 || self->bytes<0x2C8) return false;
    // I-0045 (UseBanks): the record and the colour scale from the MSB box and GPARAM of the record's {area, id} (the list
    // of {area, id} per call stays an engine input).
    alignas(16) uint8_t built[0xE0]; Region own{}; float scale=0; bool banked=false;
    if(g_banks) { const uint32_t area=At<uint32_t>(*rec,0), id=At<uint32_t>(*rec,4);
        if(!g_banks->Env(area,id,built,sizeof built)) return false;
        if(!At<uint8_t>(*self,0x2B8) && !g_banks->EnvColorScale(area,id,scale)) return false;
        own=*rec; own.data=built; own.bytes=sizeof built; rec=&own; banked=true; }
    auto R=[&](uint32_t o){ return At<float>(*rec,o); };
    const float sx=R(0x10), sy=R(0x14), sz=R(0x18);
    const float c[4]={At<float>(*data,0x20),At<float>(*data,0x24),At<float>(*data,0x28),At<float>(*data,0x2C)};
    const float a[4][4]={{sx,0,0,0},{0,sy,0,0},{0,0,sz,0},{c[0],c[1],c[2],c[3]}};
    float w[16];
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) w[4*i+j]=((a[i][3]*R(0x50+4*j)+a[i][1]*R(0x30+4*j))+a[i][2]*R(0x40+4*j))+a[i][0]*R(0x20+4*j);
    float inv[16]; ds3prod::InverseMatrix(w,inv);
    float f[100]{};
    for(int col=0;col<4;++col) for(int row=0;row<4;++row) { f[4*col+row]=w[4*row+col]; f[16+4*col+row]=inv[4*row+col]; }
    for(int k=0;k<4;++k) f[32+k]=R(0x10+4*k);
    if(!At<uint8_t>(*self,0x2B8)) { if(banked) f[36]=scale; else { if(!look || look->bytes<0x10) return false; f[36]=At<float>(*look,0xC); } } else f[36]=R(0xDC);
    f[37]=R(0xCC);
    float d[3],l[3]; for(int k=0;k<3;++k) d[k]=At<float>(*view,0x30+4*k)-R(0x50+4*k);
    for(int r=0;r<3;++r) { const float x=d[0]*R(0x20+16*r), y=d[1]*R(0x24+16*r), z=d[2]*R(0x28+16*r); l[r]=(x+y)+z; }
    float q[3]; for(int k=0;k<3;++k) { const float e=R(0x10+4*k); float v=0.0f-e; v=v>l[k]?v:l[k]; v=v<e?v:e; const float t=v-l[k]; q[k]=t*t; }
    const float dist=std::sqrt((q[0]+q[1])+q[2]);
    float base=R(0xC8), span=R(0xD8); if(At<uint8_t>(*self,0x2BD)) { base=At<float>(*self,0x2C0); span=At<float>(*self,0x2C4); }
    const float t=(dist-(base-span))/span; const float clamped=0.0f>t?0.0f:(t<=1.0f?t:1.0f);
    f[80]=1.0f-clamped;
    std::memcpy(out,f,(std::min)(bytes,400u)); return true;
}
bool BatchDepth(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    const unsigned n=s.Elements(batch_items), size=BatchSize(s), first=ordinal*size; if(first>=n || bytes<16) return false;
    const Region* p2=s.Find(6); if(!p2 || p2->bytes<0x90) return false;   // spec region 6 = argument 1 object
    float z=0;
    for(unsigned j=0;first+j<n && j<size;++j) { const Region* item=s.Find(batch_items|((first+j+1)<<16)); if(!item) return false;
        const float v=At<float>(*item,s.writer==W1A6690?0x20u:8u); if(!j || v<=z) z=v; }
    const float x=0, y=0, w=1;
    auto row=[&](unsigned r,unsigned c){ return At<float>(*p2,0x50+16*r+4*c); };
    float col[4];
    for(unsigned c=0;c<4;++c) { const float t3=w*row(3,c)+z*row(2,c); const float t1=y*row(1,c)+x*row(0,c); col[c]=t3+t1; }
    float r=0;
    if(col[2]>0 && col[3]>0) { const float q=col[2]/col[3]; r=q<=1.0f?q:1.0f; }
    std::memcpy(out,&r,4); return true;
}
}
SnapshotRows Snapshots::ReadRows(const std::filesystem::path& capture) {
    SnapshotRows rows;
    for(const auto& l:Lines(capture/"resources.jsonl")) if(Num(l,"dim")==3) rows.textures.push_back({Num(l,"object"),static_cast<uint32_t>(Num(l,"w")),static_cast<uint32_t>(Num(l,"h"))});
    for(const auto& l:Lines(capture/"origins.jsonl")) { OriginRow o; o.ref=static_cast<uint32_t>(Num(l,"ref")); o.snap=static_cast<uint32_t>(Num(l,"snap")); o.caller=static_cast<uint32_t>(Num(l,"caller")); o.gen=static_cast<uint32_t>(Num(l,"gen")); o.handle=Num(l,"handle");
        o.stack1=o.caller;   // no second return address: the caller stays
        const size_t p=l.find("\"stack\":["); if(p!=std::string::npos) { const char* q=l.c_str()+p+9; std::strtoul(q,const_cast<char**>(&q),10); if(*q==',') o.stack1=static_cast<uint32_t>(std::strtoul(q+1,nullptr,10)); }
        rows.origins.push_back(o); }
    for(const auto& l:Lines(capture/"snaps.jsonl")) { SnapRow s; s.id=static_cast<uint32_t>(Num(l,"snap")); s.writer=static_cast<uint32_t>(Num(l,"writer")); s.offset=Num(l,"offset"); s.bytes=Num(l,"bytes");
        s.parent=static_cast<uint32_t>(Num(l,"parent")); s.gen=static_cast<uint32_t>(Num(l,"gen")); s.has_slot=l.find("\"slot\":")!=std::string::npos; s.slot=static_cast<uint32_t>(Num(l,"slot"));
        s.seq=static_cast<uint32_t>(Num(l,"seq")); s.frame=static_cast<int64_t>(Num(l,"frame")); rows.snaps.push_back(s); }
    return rows;
}
void Snapshots::Load(const std::filesystem::path& capture,const std::vector<uint8_t>& pool) { Load(capture,pool,ReadRows(capture)); }
void Snapshots::Load(const std::filesystem::path& capture,const std::vector<uint8_t>& pool,const SnapshotRows& rows,std::shared_ptr<const void> pool_owner) {
    texture_size_.reserve(rows.textures.size()); snap_of_.reserve(rows.origins.size()); caller_of_.reserve(rows.origins.size());
    for(const auto& t:rows.textures) texture_size_[t.object]={t.w,t.h};
    std::unordered_map<uint32_t,uint32_t> gen_of; gen_of.reserve(rows.origins.size());   // transport 31: upload generation per pool reference
    handle_of_.clear(); for(const auto& o:rows.origins) if(o.handle) handle_of_[o.ref]=o.handle;
    for(const auto& o:rows.origins) { snap_of_[o.ref]=o.snap; caller_of_[o.ref]=o.caller; gen_of[o.ref]=o.gen;
        // Uploads through the generic helper 21397C0 report its return address 21397F5: take the next frame.
        if(o.caller==0x21397F5) caller_of_[o.ref]=o.stack1; }
    for(const auto& r:rows.snaps) { Snapshot s; s.id=r.id; s.writer=r.writer; s.parent=r.parent; s.gen=r.gen; s.seq=r.seq; s.frame=r.frame;
        if(s.id&0x80000000u) { const auto file=capture/"persist"/(r.has_slot ? std::to_string(s.writer)+"-"+std::to_string(r.slot)+".bin" : std::to_string(s.writer)+".bin");
            if(g_shared_reader) { if(auto b=g_shared_reader(file)) { s.data=b->data(); s.size=b->size(); s.keep=std::move(b); } } else s.raw=ReadFile(file); }   // I-0052: viewed, not copied
        else if(r.offset+r.bytes<=pool.size()) { if(pool_owner) { s.data=pool.data()+r.offset; s.size=static_cast<size_t>(r.bytes); s.keep=pool_owner; } else s.raw.assign(pool.begin()+static_cast<ptrdiff_t>(r.offset),pool.begin()+static_cast<ptrdiff_t>(r.offset+r.bytes)); }
        const uint32_t id=s.id; auto& slot=snaps_[id]; slot=std::move(s); if(!slot.keep) { slot.data=slot.raw.data(); slot.size=slot.raw.size(); } Parse(slot); }
    // Transport 31: a block pairs with a persistent record only when the generation it was uploaded under is the record's
    // (a mismatch means the slot was rewritten after the upload: unattributed, never silently paired).
    for(auto& [ref,id]:snap_of_) { if(!(id&0x80000000u)) continue; const uint32_t g=gen_of[ref]; const auto it=snaps_.find(id);
        if(!g || it==snaps_.end() || !it->second.gen) continue;
        ++gen_checked_; if(it->second.gen!=g) { id=0; ++gen_mismatch_; } }
    children_.clear(); by_writer_.clear(); for(const auto& [id,s]:snaps_) { by_writer_[s.writer].push_back(&s); if(s.parent && !(id&0x80000000u)) children_[s.parent].push_back(&s); }   // I-0064 (review)
    refs_of_.clear(); for(const auto& [ref,id]:snap_of_) if(id) refs_of_[id].push_back(ref); for(auto& [id,v]:refs_of_) std::sort(v.begin(),v.end());   // I-0064
}
int Snapshots::UploadOrdinal(uint32_t ref,bool same_caller) const { const uint32_t id=SnapOf(ref); if(!id) return -1; const auto it=refs_of_.find(id); if(it==refs_of_.end()) return -1;
    const auto at=std::lower_bound(it->second.begin(),it->second.end(),ref); if(at==it->second.end() || *at!=ref) return -1;
    if(!same_caller) return static_cast<int>(at-it->second.begin());
    const uint32_t caller=CallerOf(ref); return static_cast<int>(std::count_if(it->second.begin(),at,[&](uint32_t x){ return CallerOf(x)==caller; })); }
const Snapshot* Snapshots::OfRef(uint32_t ref) const { const uint32_t id=SnapOf(ref); if(!id) return nullptr; const auto it=snaps_.find(id); return it==snaps_.end()?nullptr:&it->second; }
uint32_t WriterRva(uint32_t w) {
    static const uint32_t r[]={0x13AA60,0x137910,0x1A1DA0,0x1A0080,0x1A5B50,0x1A55B0,0x19FB20,0x19F640,0x19F4D0,0x764D0,0x10ED10,0x1D6D20,0x1D7850,0x1B26E0,0x1B33D0,0x1B38D0,0x1CB9D0,0x675B0,0x10CC20,0x1072C0,0x1D8050,0x1141B0,0x1496D0,0x14A2D0,0x14E900,0x1A6690,0x1A60F0,0x88A10,0x67070,0x1B5720,0x10C7E0,0x1B9790,0x1BB810,0xE5110,0x264230,0x2A7E70,0x102A80,0x25E720,0x25D700,0xCADD0,0x16F7650,0x1DB60,0x1495F0,0x263C70};
    return w<sizeof(r)/sizeof(r[0])?r[w]:0;
}
// I-0064: cbLight / cbFog of a forward material draw (5A120): 149CB0 (= 1496D0 on the lookup result, its record 0x40 further) or
// 14AB20 (two sets and a factor) for the light keys of the draw state (+0x338, +0x340, factor +0x348), 14E900 on the fog record
// of +0x350 (14E5D0 blends two: not transcribed) for the fog; the view matrix and fog view as the frame's sun and fog builders
// (137910's 1496D0 / 14E900 calls) received them. Records from the parameter banks (UseBanks).
bool ProduceKeyed(bool fog,const uint32_t a[2],const uint32_t b[2],float t,const Snapshots* all,uint8_t* out,uint32_t bytes,int64_t frame,uint32_t before_seq) {
    if(!g_banks || !all) return false; std::memset(out,0,bytes); const uint32_t stride=fog?0x60u:0x120u; if(bytes<stride) return false;
    // the frame's 137910 call whose entry k carries the same keys and factor: record k of its block, as the window produces it
    // (the same lookup objects: their run-time words, e.g. the fog's +0x18, are the ones the window's builders copied). Preferred:
    // the latest call of the draw's camera frame before the draw (another view's lighting, e.g. m37's water reflection, comes earlier
    // with its own camera), then any call of that frame, then the others (a capture spans several frames).
    // First of all, the call whose fog builders saw the forward pass's camera ([pass+0x418], 102A80's region 5, the view 5A120 hands its
    // own builders): m37 runs 137910 a second time for its water reflection, with another camera.
    uint8_t pass_pos[12]{}; bool have_pass=false;
    for(const Snapshot* s:all->OfWriter(W102A80)) if(s->frame==frame) if(const Region* cam=s->Find(5); cam && cam->bytes>=0x3C) { std::memcpy(pass_pos,cam->data+0x30,12); have_pass=true; }
    auto sees_pass=[&](uint32_t call) { if(!have_pass) return false; for(const Snapshot* c:all->ChildrenOf(call)) if(c->writer==W14E900) if(const Region* v=c->Find(2); v && v->bytes>=0x3C) return !std::memcmp(v->data+0x30,pass_pos,12); return false; };
    const Snapshot* best=nullptr; int best_k=-1, best_rank=-1; uint32_t best_seq=0;
    for(const Snapshot* sp:all->OfWriter(W137910)) { const Snapshot& s=*sp; const uint32_t id=s.id; if(id&0x80000000u && s.children.empty()) continue; const Region *self=s.Find(2), *entries=s.Find(3); if(!self || !entries) continue;
        const int rank=s.frame==frame?(sees_pass(id)?3:before_seq && s.seq<before_seq?2:1):0; if(rank<best_rank || (rank==best_rank && (rank<2 || s.seq<=best_seq))) continue;
        const int n=At<int32_t>(*self,0x90);
        for(int k=0;k<n && 0x30u*k+0x30u<=entries->bytes;++k) { const uint32_t base=0x30u*k+(fog?0x18u:8u);
            if(At<uint32_t>(*entries,base)!=a[0] || At<uint32_t>(*entries,base+4)!=a[1] || At<uint32_t>(*entries,base+8)!=b[0] || At<uint32_t>(*entries,base+12)!=b[1]) continue;
            if((a[0]!=b[0] || a[1]!=b[1]) && At<float>(*entries,0x30u*k+(fog?0x2Cu:0x28u))!=t) continue;
            best=&s; best_k=k; best_rank=rank; best_seq=s.seq; break; } }
    if(!best) return false;
    const int n=(std::min)(At<int32_t>(*best->Find(2),0x90),static_cast<int32_t>(best->Find(3)->bytes/0x30)); if(n<=best_k) return false; std::vector<uint8_t> whole(stride*static_cast<size_t>(n),0); if(!SunFog(*best,all,fog,whole.data(),static_cast<uint32_t>(whole.size()))) return false;
    std::memcpy(out,whole.data()+stride*static_cast<size_t>(best_k),stride); return true;
}
bool Produce(const std::string& family,const Snapshot& snap,unsigned ordinal,uint32_t caller,uint8_t* out,uint32_t bytes,const Snapshots* all) {
    if(family=="cbSampleOffset" && snap.writer==W1CB9D0) return all && DownsampleOffsets(snap,*all,out,bytes);
    if(family=="cbPerScene" && snap.writer==W1D6D20) return PerScene(snap,out,bytes);
    if(family=="cbSSAOParam2" && snap.writer==W1B26E0) return SsaoParam2(snap,out,bytes);
    if(family=="cbBlurParam" && snap.writer==W1D7850) return BlurParam(snap,out,bytes);
    if(family=="cbLight" && snap.writer==W137910) return SunFog(snap,all,false,out,bytes);
    if(family=="cbFog" && snap.writer==W137910) return SunFog(snap,all,true,out,bytes);
    if(family=="cbSSS" && snap.writer==W1141B0) return SssProfiles(snap,out,bytes);
    if(family=="CB_ENVMAP_REGION" && snap.writer==W675B0) return EnvRegion(snap,ordinal,out,bytes);
    if(family=="cbLightAccShadowInfo" && snap.writer==W1A55B0) return ShadowInfo(snap,ordinal,out,bytes);
    if(family=="cbLightAccShadowInfo" && snap.writer==W1A6690) return SpotShadowInfo(snap,ordinal,out,bytes);
    if(family=="cbLightAccLightParam" && snap.writer==W1A6690) return SpotLights(snap,ordinal,out,bytes);
    std::memset(out,0,bytes);
    if(family=="cbCompositeShadowMask" && snap.writer==W10CC20) return CompositeMask(caller,out,bytes);
    if(family=="cbColor" && snap.writer==W1072C0 && caller==0x1075EF) return Colour1072C0(snap,out,bytes);
    if(family=="cbSSAOParam" && snap.writer==W1B33D0) return SsaoParam(snap,4,out,bytes);
    if(family=="cbSSAOParam" && snap.writer==W1B38D0) return SsaoParam(snap,5,out,bytes);
    if(family=="cbSampleOffset" && snap.writer==W1B33D0 && caller==0x1B35CC) return SsaoKernel(snap,out,bytes);
    if(family=="cbColor" && snap.writer==W1D8050) return bytes>=16 && Rdata(0x3D7FE40,out,16);   // 1D8050: executable constant
    if(family=="cbVtxParam" && snap.writer==W1072C0 && caller==0x1076A5) { const float one=1.0f; std::memcpy(out,&one,4); return bytes>=16; }   // {1, 0, 0, 0}
    if(family=="cbLightAccVtxParam" && ((snap.writer==W19F4D0 && caller==0x19F5C8) || (snap.writer==W19FB20 && caller==0x19FEA8))) return bytes>=16;   // 16 zero bytes
    if(family=="cbSceneParam" && snap.writer==W13AA60) return SceneImage(snap,all,out,bytes);
    if(family=="cbMatrixWVP" && snap.writer==W13AA60) return ViewWvp(snap,out,bytes);   // I-0064
    if(family=="CBFxaaParam" && snap.writer==W1B9790) return FxaaParam(snap,out,bytes);
    if(family=="cbMatrixWVP" && snap.writer==W10C7E0) {   // I-0064 (class B): face `ordinal` of a point light's cube: its pose (+0x40 i) and its view-projection
        // image (+0x180 + 0x40 i) as the light's shadow setup left them in 10C7E0's argument 2 (the projection is not traced upstream)
        // The call's uploads: the face table to [+0x330] first, then face i to [+0x340 + 8 i] for each set buffer (adapter v46b region 3:
        // the buffer handles; without it the face of an upload is not known and nothing is produced)
        const Region *f=snap.Find(1), *buffers=snap.Find(3); if(!f || f->bytes<0x340 || !buffers || buffers->bytes<0x40 || ordinal<1 || bytes<128) return false;
        unsigned face=6; if(snap.Find(4)) face=ordinal-1; else for(unsigned i=0,k=1;i<6;++i) if(At<uint64_t>(*buffers,0x10+8*i)) { if(k==ordinal) { face=i; break; } ++k; }   // v47: the host names the face by the block's buffer (region 4)
        if(face>5) return false; std::memset(out,0,bytes); std::memcpy(out,f->data+0x40*face,64); std::memcpy(out+64,f->data+0x180+0x40*face,64); return true; }
    if(family=="cbGlowParam" && snap.writer==W1BB810) return GlowParam(snap,caller,out,bytes);
    if(family=="cbSampleOffset" && snap.writer==W1BB810 && caller==0x7FB8A) return GlowTaps(snap,out,bytes);
    // I-0064 (class B): the GPU particles' blocks (25D700): ccbSceneParam = the data argument as uploaded; ccbForceFiledParam = the
    // two structures 25DDF0 and 25D8D0 copy from data +0x450 (0x910 bytes) and +0xD60 (0x490 bytes): data [+0x450, +0x11F0) (region 2 = data:
    // 0x1200 bytes from adapter v46c, 0x1100 before: too short)
    // the k-th upload is the k-th item whose type ([[item+0x20]+0x20], adapter v46c region 7) is 1; without the types, only a batch whose
    // every item uploads (i64-live-02's upload 57 of 83, 45 of 58, 59 of 83: none)
    if(family=="cbwvpmatrix" && snap.writer==W0CADD0) { const Region *b=snap.Find(1), *types=snap.Find(7); if(!all || !b || b->bytes<0x94) return false;
        const uint32_t n=At<uint32_t>(*b,0x8C)+At<uint32_t>(*b,0x90); unsigned item=ordinal;
        static const bool raw=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_WVPRAW") && m; }();   // diagnostic: `ordinal` names the item itself
        if(raw) { if(ordinal==0 && types) { std::string t; for(unsigned i=0;i<n && 4*i<types->bytes;++i) t+=" "+std::to_string(At<int32_t>(*types,4*i)); std::fprintf(stderr,"{\"wvptypes\":%u,\"n1\":%u,\"types\":\"%s\"}\n",snap.id,At<uint32_t>(*b,0x8C),t.c_str()); } return BoxWvp(snap,ordinal,out,bytes); }
        if(types && types->bytes>=4*n) { unsigned k=0; item=n; for(unsigned i=0;i<n;++i) if(At<int32_t>(*types,4*i)==1) { if(k==ordinal) { item=i; break; } ++k; } if(item>=n) return false; }
        else if(all->UploadCount(snap.id)!=n) return false;
        return BoxWvp(snap,item,out,bytes); }
    if(family=="cbSampleOffset" && snap.writer==W102A80) return Offsets102A80(snap,out,bytes);
    if(family=="cbComputeCustomDof" && snap.writer==W0E5110) return CustomDof(snap,all,out,bytes);
    if(family=="cbCubeEnvSH1" && snap.writer==W2A7E70) {   // I-0064 (class B, adapter v46e): the three SH rows obj+0x2A20 times {[0x1439F24B0].xyz, [[obj+0x2B80]+8]+0x110+4k} (MULPS at 0x1402A836D..)
        const Region *rows=snap.Find(5), *w=snap.Find(6); float one[4]{}; if(!rows || rows->bytes<0x30 || !w || w->bytes<12 || bytes<0x30 || !Rdata(0x39F24B0,one,16)) return false;
        for(uint32_t k=0;k<3;++k) for(uint32_t j=0;j<4;++j) { const float s=j<3?one[j]:At<float>(*w,4*k), r=At<float>(*rows,16*k+4*j)*s; std::memcpy(out+16*k+4*j,&r,4); } return true; }
    if(family=="ccbEmitionSeed" && snap.writer==W25E720) { const Region* d=snap.Find(2); if(!d || d->bytes<bytes) return false; std::memcpy(out,d->data,bytes); return true; }   // the seed table argument as uploaded (25E720)
    if((family=="ccbSceneParam" || family=="ccbForceFiledParam") && snap.writer==W25D700) { const Region* d=snap.Find(2); const uint32_t at=family=="ccbSceneParam"?0u:0x450u;
        if(!d || d->bytes<at+bytes) return false; std::memcpy(out,d->data+at,bytes); return true; }
    if(family=="cbSceneParam" && snap.writer==W764D0) return SceneShadow764D0(snap,all,out,bytes);
    if(family=="cbLightAccLightParam" && (snap.writer==W1A5B50 || snap.writer==W1A55B0)) return PointLights(snap,ordinal,out,bytes);   // 1A6690 / 1A60F0 items: not yet transcribed
    if(family=="cbLightAccParam" && snap.writer==W1A1DA0) return LightAccParam1A1DA0(snap,out,bytes);
    if(family=="cbLightAccParam" && snap.writer==W19F640) return LightAccParamCommon(snap,4,-1,-1,out,bytes);
    if(family=="cbLightAccParam" && snap.writer==W10ED10) return LightAccParamCommon(snap,6,7,-1,out,bytes);   // AO pass: pass = arg1, float 69 = 0
    if(family=="cbLightAccParam" && snap.writer==W1A0080) return OrganizeParam(snap,out,bytes);
    if(family=="cbLightAccVtxParam" && (snap.writer==W1A5B50 || Shadowed(snap.writer))) return BatchDepth(snap,ordinal,out,bytes);
    return false;
}
}
namespace ds3light {
namespace {   // with the declarations above (the file's unnamed namespace)
// I-0064 (adapter v46): CBFxaaParam (1B9790; regions 2 = desc {mode, ...}, 3 = its settings p[0..3], 4 = the source texture
// (+0x50 / +0x54 size), 5 = [[G]+0x5360] (+8 the enabling byte), 6 = [[G]+0x5368] (+0x98 the fade)): {W, H, 1 / W, 1 / H, then
// for the DLAA mode (3) (1 - p0) s + p0, p1, p2, 1, else p1, p2, p3, (0 - p0) s + p0, then 2 / W, 2 / H, 0.5 / W, 0.5 / H} with
// s = the clamp to [0, 1] of 1 - fade (fade 1 when the configuration or its byte is absent).
float FadeSelect(float one,float f) { return 0.0f<=f?(f<=one?f:one):0.0f; }
bool FxaaParam(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region *desc=s.Find(2), *set=s.Find(3), *tex=s.Find(4), *cfg=s.Find(5), *fadeobj=s.Find(6);
    if(!desc || !set || !tex || desc->bytes<4 || set->bytes<16 || tex->bytes<0x58 || bytes<48) return false;
    float one{},two{},half{}; if(!Rdata(0x3D7EE10,&one,4) || !Rdata(0x3D7EE7C,&two,4) || !Rdata(0x3D7EDB4,&half,4)) return false;
    float fade=one; if(cfg && cfg->bytes>8 && cfg->data[8] && fadeobj && fadeobj->bytes>=0x9C) fade=At<float>(*fadeobj,0x98);
    const float sel=FadeSelect(one,one-fade); const uint32_t mode=At<uint32_t>(*desc,0);
    const float W=static_cast<float>(At<uint32_t>(*tex,0x50)), H=static_cast<float>(At<uint32_t>(*tex,0x54)); float p[4]; std::memcpy(p,set->data,16);
    float f[12]={W,H,one/W,one/H,0,0,0,0,two/W,two/H,half/W,half/H};
    if(mode==3) { f[4]=(one-p[0])*sel+p[0]; f[5]=p[1]; f[6]=p[2]; f[7]=one; } else { f[4]=p[1]; f[5]=p[2]; f[6]=p[3]; f[7]=(0.0f-p[0])*sel+p[0]; }
    std::memcpy(out,f,48); return true;
}
// I-0064 (adapter v46): cbGlowParam (1BB810; region 1 = the glow settings [*arg2], 2 = [[G]+0x5368] (+0x80 the fade)):
// {(0 - g) s + g, the int at +0x18, 0, 0} with g = settings +0x14 (upload 1BB996) or +0x20 (1BC333 and the third site),
// s = the clamp of 1 - fade.
bool GlowParam(const Snapshot& s,uint32_t caller,uint8_t* out,uint32_t bytes) {
    const Region *set=s.Find(1), *fadeobj=s.Find(2); if(!set || set->bytes<0x24 || bytes<16) return false;
    float one{}; if(!Rdata(0x3D7EE10,&one,4)) return false; const float fade=fadeobj && fadeobj->bytes>=0x84?At<float>(*fadeobj,0x80):one;
    if(one!=1.0f) return false;   // W5d GL-1: the core's one; the .rdata word is 1.0
    ds3native::glow::ParamImage(At<float>(*set,caller==0x1BB996?0x14u:0x20u),At<uint32_t>(*set,0x18),fade,out); return true;
}
}
namespace {
// I-0064: cbSampleOffset of the glow's filter passes (7F950 inside 1BB810, which passes the glow settings +0x10 as the spread and
// selects 14007FED0 at both call sites): 13 taps {x s, y s, w / sum, 0}, (x, y) from the stack table 7FED0 builds (.rdata
// 0x143D7FB10, 7D0, 80250, 7FA10, 7FA20, 80210 pairs, then {-0.5, [0x143D7F0BC]}), w = expf((y y + x x) [0x143D7F0C0]) /
// sqrtf([0x143D7EEFC]); the rest of the 256-byte block is stack residue (written as zero).
// I-0064: cbComputeCustomDof (E5110 upload at 0x1400E61BE: arg0 [+0xC0, +0x20D0)): g_nTexInfo = the width and height of the LUT texture
// [[arg0+0x20D0]+0x18] (141924730: the D3D11 description; a frame input, the capture's resource table), +0xC8 = 0, +0xCC as at entry;
// g_Coc[2048]: for t = clamp(i * [0x143D7ED04], 0, 1) (1 for i > 0x7FD), c = 1416C9C30(YEBIS, 0, 3, t) (the lens mapping a t + b of
// 1416F4AF0 between spaces 0 and 3: 1/far-near scale [Y+0xC48], [Y+0xC38..C3C], [Y+0xC40..C44] times 1416F52A0's [[Y+0x110]+0x1B0]),
// d = 1416C9C30(YEBIS, 0, 7, t) (the view depth n f / -((f - n) t - f)), then the near / far blur ramps of the DOF settings
// [0x144962244..0x14496226C] (adapter regions: 2 = the settings, 8 = YEBIS +0x400.., 9 = [[YEBIS+0x110]+0x1B0] from v46e).
bool CustomDof(const Snapshot& s,const Snapshots* all,uint8_t* out,uint32_t bytes) {
    const Region *arg=s.Find(1), *g=s.Find(2), *tex=s.Find(5), *y=s.Find(8), *ev=s.Find(9);
    // v47: the camera as the YEBIS update E5110 runs first left it (the last 16F7650 call inside this one), else as at entry
    if(all) { uint32_t best=0; for(const Snapshot* x:all->ChildrenOf(s.id)) if(x->writer==W16F7650 && x->id>best && x->Find(1) && x->Find(2)) { best=x->id; y=x->Find(1); ev=x->Find(2); } } if(!all || !arg || arg->bytes<0xD0 || !g || g->bytes<0x70 || !tex || tex->bytes<0x20 || !y || y->bytes<0x850 || bytes<0x2010) return false;
    const bool exposure=At<uint8_t>(*y,0xC34-0x400)!=0; static const bool forced_e=[]{ size_t m=0; return !getenv_s(&m,nullptr,0,"LH_DOFE") && m; }(); if(exposure && !forced_e && (!ev || ev->bytes<4)) return false;   // 1416F52A0: 0 unless +0xC34
    if(At<uint8_t>(*y,0x6BD-0x400)) return false;   // 1416E9050 would recompute the lens values first (not transcribed)
    uint32_t w{},h{}; const uint64_t texture=At<uint64_t>(*tex,0x18); if(!texture || !all->TextureSize(texture,w,h)) return false;
    ds3light::CustomDofInput in; if(!ds3light::CustomDofConstants(in)) return false;
    auto GU=[&](uint32_t a){ return At<uint32_t>(*g,a-0x200); }; auto Y=[&](uint32_t a){ return At<float>(*y,a-0x400); };
    in.width=w; in.height=h; std::memcpy(&in.word_cc,arg->data+0xCC,4);
    in.near_plane=Y(0x564); in.far_plane=Y(0x568); in.lens={Y(0xC48),Y(0xC38),Y(0xC3C),Y(0xC40),Y(0xC44)};
    in.exposure=[&]{ static const float forced=[]{ char v[32]{}; size_t m=0; return (!getenv_s(&m,v,sizeof v,"LH_DOFE") && m)?std::strtof(v,nullptr):-1.0f; }(); return forced>=0.0f?forced:exposure?At<float>(*ev,0):0.0f; }();   // LH_DOFE: diagnostic override
    for(uint32_t k=0;k<11;++k) in.dof[k]=GU(0x244+4*k);
    std::memset(out,0,bytes); ds3light::CustomDofTable(in,out);
    return true;
}
}
bool CustomDofConstants(CustomDofInput& in) {
    float fallback[4]{};
    if(!Rdata(0x3D7EE10,&in.one,4) || !Rdata(0x3D7ED04,&in.step,4) || !Rdata(0x3D7ECE4,&in.tiny,4) || !Rdata(0x39F2500,fallback,16)) return false;
    in.fallback=fallback[2]; return true;
}
void CustomDofTable(const CustomDofInput& in,uint8_t* out) {
    const float one=in.one, step=in.step, tiny=in.tiny;
    auto G=[&](uint32_t a){ float v; std::memcpy(&v,&in.dof[(a-0x244)/4],4); return v; }; auto GU=[&](uint32_t a){ return in.dof[(a-0x244)/4]; };
    const float n=in.near_plane, f=in.far_plane, c48=in.lens[0], c38=in.lens[1], c3c=in.lens[2], c40=in.lens[3], c44=in.lens[4], e=in.exposure;
    // 1416F4AF0 (space 0 to 3): a, b in its SSE order
    float a=f, nf=f*n; a=a-n; a=a*c48; const float k=c48/n, b0=one-k; a=a/nf; float b=a*0.0f; b=b+b0;
    a=a*c38; b=c38*b; b=b+c3c;
    a=a*c40; b=c40*b; b=b+c44; b=b*e; a=a*e; b=b+0.0f;
    auto pick=[&](float v){ return v<=0.0f?in.fallback:v; };   // a setting <= 0 takes the .rdata row 0x1439F2500's lane 2
    const float f24c=pick(G(0x24C)), f250=pick(G(0x250)), f25c=pick(G(0x25C)), f260=pick(G(0x260)), f26c=pick(G(0x26C));
    const uint32_t eps=0x33D6BF95u; float epsf; std::memcpy(&epsf,&eps,4);
    auto clamp01=[&](float x){ return 0.0f<=x?(x<=one?x:1.0f):0.0f; };
    std::memset(out,0,0x2010); std::memcpy(out,&in.width,4); std::memcpy(out+4,&in.height,4); std::memcpy(out+0xC,&in.word_cc,4);
    for(int i=0;i<0x800;++i) {
        float t=static_cast<float>(i)*step; t=0.0f<=t?(t<=one?t:1.0f):0.0f; if(i>0x7FD) t=one;
        float c=a*t; c=c+b;                                               // 1416C9C30 mode 3
        float d=n*f, q=f-n; q=q*t; q=q-f; d=d/q; { uint32_t u; std::memcpy(&u,&d,4); u^=0x80000000u; std::memcpy(&d,&u,4); }   // mode 7
        bool scale_near=true;
        if(GU(0x244)) { c=0.0f; scale_near=false;
            if(d<f250) { float den=f250-f24c; if(den<=tiny) den=epsf; const float u=clamp01((d-f24c)/den); c=(one-u)*G(0x254); uint32_t x; std::memcpy(&x,&c,4); x^=0x80000000u; std::memcpy(&c,&x,4); scale_near=true; } }
        if(scale_near && c<0.0f) c=c*G(0x258);
        if(GU(0x248) && f25c<d) { float den=f260-f25c; if(den<=tiny) den=epsf; c=G(0x264)*clamp01((d-f25c)/den); }
        if(0.0f<c) c=c*G(0x268);
        if(f26c<d) c=0.0f;
        std::memcpy(out+0x10+4*i,&c,4); }
}
namespace {
bool GlowTaps(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region* set=s.Find(1); if(!set || set->bytes<0x14 || bytes<13*16) return false;
    float t[26]; static const uint32_t rows[6]={0x3D7FB10,0x3D7F7D0,0x3D80250,0x3D7FA10,0x3D7FA20,0x3D80210};
    for(int i=0;i<6;++i) if(!Rdata(rows[i],t+4*i,16)) return false;
    t[24]=-0.5f; if(!Rdata(0x3D7F0BC,t+25,4)) return false;
    uint32_t k{},n{}; if(!Rdata(0x3D7F0C0,&k,4) || !Rdata(0x3D7EEFC,&n,4)) return false;
    // W5d GL-1: the core of the native glow stage on the words read here.
    uint32_t words[26]; std::memcpy(words,t,sizeof words); uint8_t image[256];
    ds3native::glow::TapImage(At<float>(*set,0x10),image,words,k,n);
    std::memset(out,0,bytes); std::memcpy(out,image,bytes<256?bytes:256);
    return true;
}
}
namespace {
// I-0064 (adapter v46b, writer CADD0, region 1 = its batch argument): cbwvpmatrix of the box of item `ordinal` (B2010, for the items
// whose object type is 1): the batch's camera (pose +0x00, fovy +0x50, near +0x54, far +0x58,
// aspect +0x5C or the uint size +0x60 / +0x64 when 0, mode +0x68) gives VP = 1CE40(4BCA0(pose), 4BA40 projection); items: two
// lists after the header (size [0x14473F6DC], a pointer list of [batch+0x88] entries aligned to 16), counts +0x8C and +0x90,
// 0x30 bytes each (the box min, max as float4), scale and padding +0x94 / +0x98 (second list +0x9C / +0xA0). B2010: centre
// c = (max + min) h, half size e = ((max - min) h + pad) s (h = .rdata 0x1439F2600), rows e & 0x143D4A6E0, e & 0x143D1F6D0,
// e & 0x143D1F6E0, c; uploaded = 1CE40(rows, VP).
// 14004B070 on its camera structure (fovy +0x50, near +0x54, far +0x58, size +0x5C / +0x60 (uint), aspect +0x64 (the size's ratio
// when 0), tiles +0x70, tile +0x74, offset +0x78 over +0x7C x +0x80 (uints)): 4BA40 without tiles, else 4BB40 off centre.
struct Camera4B070 { float fovy,nearz,farz,aspect; uint32_t w,h,tiles,tile,off,ow,oh; };
bool Projection4B070(Camera4B070 k,float proj[16]) {
    float half{},one{},two{},c08c[4]{}; uint32_t sign{}; if(!Rdata(0x3D7EDB4,&half,4) || !Rdata(0x3D7EE10,&one,4) || !Rdata(0x3D7EE7C,&two,4) || !Rdata(0x3D1C080,c08c,16) || !Rdata(0x3D801C0,&sign,4)) return false;
    if(k.aspect==0.0f) k.aspect=static_cast<float>(k.w)/static_cast<float>(k.h);
    auto neg=[&](float x) { uint32_t v; std::memcpy(&v,&x,4); v^=sign; std::memcpy(&x,&v,4); return x; };
    if(!k.tiles) {   // 4BA40: rows (cot / aspect), (0, cot, 0, 0), (0, 0, q, [0x143D1C08C]), (0, 0, -q n, [0x143D1C088])
        const float fov=k.fovy*half, q=k.farz/(k.farz-k.nearz), cot=std::cos(fov)/std::sin(fov); const float r[16]={cot/k.aspect,0,0,0, 0,cot,0,0, 0,0,q,c08c[3], 0,0,neg(q*k.nearz),c08c[2]}; std::memcpy(proj,r,64); return true; }
    const float f1=(std::tan(k.fovy*half)*k.nearz)*two, f4=f1/static_cast<float>(k.tiles), f6=(f1*k.aspect)/static_cast<float>(k.tiles);
    const float f5=(static_cast<float>(k.off)/static_cast<float>(k.ow))*f6, f3=(static_cast<float>(k.off)/static_cast<float>(k.oh))*f4;
    const float f8=f1*half-static_cast<float>(k.tile/k.tiles)*f4, f10=static_cast<float>(k.tile%k.tiles)*f6-(f1*k.aspect)*half;
    const float l=f10-f5, r=(f10+f6)+f5, bo=(f8-f4)-f3, to=f8+f3;
    const float ix=one/(r-l), iy=one/(to-bo), q=k.farz/(k.farz-k.nearz), n2=k.nearz+k.nearz;
    const float m[16]={ix*n2,0,0,0, 0,iy*n2,0,0, neg((l+r)*ix),neg((bo+to)*iy),q,one, 0,0,neg(q*k.nearz),0}; std::memcpy(proj,m,64); return true;
}
bool BoxWvp(const Snapshot& s,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    const Region* b=s.Find(1); if(!b || b->bytes<0xA4 || bytes<64) return false;
    uint32_t header{}; float h[4]{},m0[4]{},m1[4]{},m2[4]{}; if(!Rdata(0x473F6DC,&header,4) || !Rdata(0x39F2600,h,16) || !Rdata(0x3D4A6E0,m0,16) || !Rdata(0x3D1F6D0,m1,16) || !Rdata(0x3D1F6E0,m2,16)) return false;
    float pose[16],view[16]; for(int i=0;i<16;++i) pose[i]=At<float>(*b,4*i); ds3prod::InverseMatrix(pose,view);
    // CADD0 copies the batch's camera into 4B070's structure with its own order: size +0x60 / +0x64, aspect +0x5C, tiles +0x68 ..
    // (ushorts) +0x6A, +0x6C, +0x6E, +0x70
    const Camera4B070 k{At<float>(*b,0x50),At<float>(*b,0x54),At<float>(*b,0x58),At<float>(*b,0x5C),At<uint32_t>(*b,0x60),At<uint32_t>(*b,0x64),
        At<uint16_t>(*b,0x68),At<uint16_t>(*b,0x6A),At<uint16_t>(*b,0x6C),At<uint16_t>(*b,0x6E),At<uint16_t>(*b,0x70)};
    float proj[16]; if(!Projection4B070(k,proj)) return false;    float vp[16]; Multiply(view,proj,vp);
    const uint32_t n1=At<uint32_t>(*b,0x8C), n2=At<uint32_t>(*b,0x90); const uint32_t list=header+((At<uint32_t>(*b,0x88)*8u+15u)&~15u);
    uint32_t at,sc_off; if(ordinal<n1) { at=list+0x30u*ordinal; sc_off=0x94; } else if(ordinal<n1+n2) { at=list+0x30u*n1+0x30u*(ordinal-n1); sc_off=0x9C; } else return false;
    if(at+0x20>b->bytes) return false; const float scale=At<float>(*b,sc_off), pad=At<float>(*b,sc_off+4);
    float c[4],e[4]; for(int i=0;i<4;++i) { const float mn=At<float>(*b,at+4*i), mx=At<float>(*b,at+16+4*i); c[i]=(mx+mn)*h[i]; e[i]=((mx-mn)*h[i]+pad)*scale; }
    auto masked=[](const float* v,const float* m,float* r) { for(int i=0;i<4;++i) { uint32_t a,k; std::memcpy(&a,v+i,4); std::memcpy(&k,m+i,4); a&=k; std::memcpy(r+i,&a,4); } };
    float rows[16]; masked(e,m0,rows); masked(e,m1,rows+4); masked(e,m2,rows+8); std::memcpy(rows+12,c,16);
    float r[16]; Multiply(rows,vp,r); std::memcpy(out,r,64); return true;
}
}
namespace {
// I-0064 (adapter v46c, writer 102A80: region 3 = its second argument (int sizes +0x2C..+0x38), region 5 = the camera [pass+0x418]):
// the forward cbSampleOffset at 10322F: {-1 / a, -1 / c, 1, 1}, {1 / a, -1 / c, 1, 1}, {-1 / a, 1 / c, 1, 1}, {1 / a, 1 / c, 1, 1}
// (a = +0x34, c = +0x38, -1 = [0x143D7F0C8]), {+0x2C, +0x30, 0, 1}, {P[2][2], P[3][2] / (far - near), far - near, 1} (P = 4B070 on
// the camera), then seven rows of .rdata 0x143D7FB40; the rest of the 256-byte block is stack residue (zero).
bool Offsets102A80(const Snapshot& s,uint8_t* out,uint32_t bytes) {
    const Region *p2=s.Find(3), *cam=s.Find(5); if(!p2 || p2->bytes<0x3C || !cam || cam->bytes<0x84 || bytes<0xD0) return false;
    float one{},m1{},row[4]{}; if(!Rdata(0x3D7EE10,&one,4) || !Rdata(0x3D7F0C8,&m1,4) || !Rdata(0x3D7FB40,row,16)) return false;
    const Camera4B070 k{At<float>(*cam,0x50),At<float>(*cam,0x54),At<float>(*cam,0x58),At<float>(*cam,0x64),At<uint32_t>(*cam,0x5C),At<uint32_t>(*cam,0x60),
        At<uint32_t>(*cam,0x70),At<uint32_t>(*cam,0x74),At<uint32_t>(*cam,0x78),At<uint32_t>(*cam,0x7C),At<uint32_t>(*cam,0x80)};
    if(At<uint8_t>(*cam,0x68) && !k.tiles) return false;   // 4B070's orthographic branch (flag +0x68): not transcribed
    float P[16]; if(!Projection4B070(k,P)) return false;
    const float a=static_cast<float>(At<int32_t>(*p2,0x34)), c=static_cast<float>(At<int32_t>(*p2,0x38)), xa=m1/a, ya=m1/c, xb=one/a, yb=one/c, depth=k.farz-k.nearz;
    const float f[24]={xa,ya,one,one, xb,ya,one,one, xa,yb,one,one, xb,yb,one,one,
        static_cast<float>(At<int32_t>(*p2,0x2C)),static_cast<float>(At<int32_t>(*p2,0x30)),0.0f,one, P[10],P[14]/depth,depth,one};
    std::memset(out,0,bytes); std::memcpy(out,f,sizeof f); for(int i=0;i<7;++i) std::memcpy(out+0x60+16*i,row,16); return true;
}
}
// W10 LS-2b: 14004B070 on a camera record laid out as E69A0's ([engine+0x210]: pose +0x00, fovy +0x50, near +0x54, far +0x58,
// size +0x5C / +0x60 (uint), aspect +0x64 (the size's ratio when 0), the orthographic flag +0x68 (refused), tiles +0x70, tile +0x74,
// offset +0x78 over +0x7C x +0x80); `bytes` short of +0x84 reads the last height as 1 (the tiled branch only divides the offset by it).
bool CameraProjection4B070(const uint8_t* camera,size_t bytes,float proj[16]) {
    if(!camera || bytes<0x80) return false;
    const auto f=[&](size_t o){float v{};std::memcpy(&v,camera+o,4);return v;};const auto u=[&](size_t o){uint32_t v{};std::memcpy(&v,camera+o,4);return v;};
    if(camera[0x68]) return false;
    const Camera4B070 k{f(0x50),f(0x54),f(0x58),f(0x64),u(0x5C),u(0x60),u(0x70),u(0x74),u(0x78),u(0x7C),bytes>=0x84?u(0x80):1u};
    return Projection4B070(k,proj);
}
bool ProduceSized(const std::string& family,uint32_t width,uint32_t height,uint8_t* out,uint32_t bytes) {
    if(!width || !height) return false; std::memset(out,0,bytes);
    const float W=static_cast<float>(width), H=static_cast<float>(height); float one{}; if(!Rdata(0x3D7EE10,&one,4)) return false;
    if(family=="CBDlaaParam") {   // afStack_398: x taps from 1 / W, y taps from 1 / H, in the order 1B9790 stores them
        if(bytes<160) return false; const float iw=one/W, ih=one/H; static const uint32_t k[10]={0x3D7F0CC,0x3D7EE38,0x3D7F104,0x3D7F0FC,0x3D7F0F0,0x3D7EEC8,0x3D7EEE4,0x3D7EF28,0x3D7F0C8,0};
        float f[40]{}; for(int i=0;i<10;++i) { float c=one; if(k[i] && !Rdata(k[i],&c,4)) return false; f[4*i]=iw*c; f[4*i+3]=ih*c; }
        std::memcpy(out,f,160); return true; }
    if(family=="cbFilterParam") { if(bytes<32) return false; float k{}; if(!Rdata(0x3D7ED38,&k,4)) return false;
        const float f[8]={W*k,H*k,0,0,one/W,one/H,0,0}; std::memcpy(out,f,32); return true; }
    return false;
}
bool ProduceBuffer(const Snapshot& snap,unsigned ordinal,uint8_t* out,uint32_t bytes) {
    std::memset(out,0,bytes);
    if(snap.writer==W1A0080) return OrganizeBuffer(snap,ordinal,out,bytes);
    return false;
}
}
namespace ds3light {
bool SssProfileKernel(const float strength[3],float kernel[17][4]) { return SssKernel(strength,kernel); }
bool Rdata(uint32_t rva,void* out,size_t bytes) {
    if(g_rdata_reader) return g_rdata_reader(rva,out,bytes);
    struct Image { std::vector<uint8_t> file; struct Section { uint32_t va, size, raw; }; std::vector<Section> sections; bool loaded{}; };
    static Image image;
    if(!image.loaded) { image.loaded=true; char path[512]{}; size_t n=0;
        if(getenv_s(&n,path,sizeof path,"DS3_EXE") || !n) std::snprintf(path,sizeof path,"%s","game/DarkSoulsIII.exe");
        image.file=ReadFile(path); const auto& b=image.file;
        if(b.size()>0x40) { uint32_t pe{}; std::memcpy(&pe,b.data()+0x3C,4); uint16_t count{},opt{};
            if(pe+24<=b.size()) { std::memcpy(&count,b.data()+pe+6,2); std::memcpy(&opt,b.data()+pe+20,2);
                for(uint32_t i=0;i<count;++i) { const size_t o=pe+24+opt+40ull*i; if(o+40>b.size()) break; Image::Section s{};
                    std::memcpy(&s.size,b.data()+o+8,4); std::memcpy(&s.va,b.data()+o+12,4); std::memcpy(&s.raw,b.data()+o+20,4); image.sections.push_back(s); } } } }
    for(const auto& s:image.sections) if(rva>=s.va && rva+bytes<=s.va+s.size) { const size_t f=size_t{rva}-s.va+s.raw; if(f+bytes>image.file.size()) return false; std::memcpy(out,image.file.data()+f,bytes); return true; }
    return false;
}
char Kind(const std::string& family,uint32_t writer) {
    if(family=="cbSceneParam" && writer==W13AA60) return g_camera?'A':'B';
    if(family=="cbSceneParam" && writer==W764D0) return g_camera?'A':'B';
    if((family=="cbLightAccLightParam" || family=="cbLightAccVtxParam") && (writer==W1A5B50 || writer==W1A55B0)) return 'A';
    if(family=="cbLightAccParam" && (writer==W1A1DA0 || writer==W19F640 || writer==W10ED10 || writer==W1A0080)) return 'A';
    if(family=="cbCompositeShadowMask" && writer==W10CC20) return 'A';
    if((family=="cbColor" || family=="cbVtxParam") && writer==W1072C0) return 'A';
    if(family=="cbColor" && writer==W1D8050) return 'A';
    if((family=="cbSSAOParam" && (writer==W1B33D0 || writer==W1B38D0)) || (family=="cbSampleOffset" && writer==W1B33D0)) return 'A';
    if(family=="cbLightAccVtxParam" && (writer==W19F4D0 || writer==W19FB20)) return 'A';
    if(family.rfind("buffer#",0)==0 && writer==W1A0080) return 'A';
    return 0;
}
}
namespace ds3light {
// I-0138 tranche 4 part 4b: the bank file of an area, the inverse of GetGparamArea_140d27790 (GhidraMCP, 2026-11-13): areas 0
// and 1 (the lookup fallback key {1, 0}) default.gparam; a kind-11 cutscene file sAA_BB_VVVV has the area
// ((AA + 200) x 100 + BB) x 10000 + VVVV, a kind-7 map file mAA_BB_CCCC ((AA + 100) x 100 + BB) x 10000 (its CCCC 0 here; the
// bound and live layers name the others). The engine's lookup (GparamBankLookup_140032770) finds a key by that area, then its
// id; before this the cutscene areas were looked for as mAA_BB_VVVV and fell to {1, 0}.
// I-0141 (STUDY-UNREAD-SETTINGS item 5): the resident kinds' areas are 27790's table 14294EFB0 (READ, GhidraMCP 2026-11-15: kind 12
// `eclipse` area 7; the templates and `empty` 2, 3, 5, 6; MapBase, MapOverride and MapOverride_forRemo 100000000, RemoBase 200000000).
// Not named here: Banks reads a file's groups by position (0 LightSet, 1 Fog) and those files hold partial group tables (eclipse.gparam
// a Fog group alone), so a key of their areas would read the wrong group (the final review's L1); left for a reading by group name.
std::string AreaFileName(uint32_t area) {
    if(area<=1) return "default";
    char n[32]; std::snprintf(n,sizeof n,"%c%02u_%02u_%04u",area/100000000u==2u?'s':'m',(area/1000000u)%100u,(area/10000u)%100u,area%10000u);
    return n;
}
// ---- I-0045: sun / fog bank records from game data ----
struct Banks::Impl {
    std::map<std::string,std::shared_ptr<const ds3content::Gparam>> files;   // "m40_00_0000", "default", ...; I-0079: shared with the next Banks when its bytes are the same
    std::map<std::string,std::shared_ptr<const std::vector<uint8_t>>> sources;   // I-0079: the bytes each file was parsed from (shared reader)
    std::map<std::string,std::array<float,4>> offsets; // "m40_00_00_00" -> MapOffset {x, y, z, degree}
    // "m40_00_00_00" -> Env_Box number -> {position xyz, rotation xyz, width, height, depth, UnkT00, UnkT04, UnkT08}
    std::map<std::string,std::map<uint32_t,std::array<uint32_t,14>>> boxes;
    std::map<unsigned,std::pair<std::string,std::array<uint32_t,4>>> sss_base;          // k -> {MTD, strength, width}
    std::map<std::string,std::vector<std::array<uint32_t,3>>> gx15;                    // MTD -> distinct GX15 strengths
    // I-0063: the GPARAM variant the engine loaded for an area key (m31_00_0000 keyed records carry m31_00_0010's values in
    // the Shebang save's Undead Settlement): set by SelectVariant from the engine's own sun record, else the key's name.
    mutable std::map<uint64_t,std::string> variant; mutable std::mutex variant_m; mutable std::atomic<uint64_t> binding_generation{};   /* I-0141 tranche 6's step review L4: read without the lock */   /* key area * 16 + group */
    // The engine's own binding of the area's file (Banks::BindArea): file key and 3x4 transform; guarded by variant_m.
    struct Bound { std::string file; std::array<float,12> rows{}; };
    mutable std::map<uint32_t,Bound> bound;
    // Every live binding of an area, latest first (SetAreaLayers); a lookup takes the latest layer that holds the value ID.
    mutable std::map<uint32_t,std::vector<Bound>> layers;
    mutable RecordOverrides overrides;   // I-0141 tranche 3 (SetRecordOverrides); guarded by variant_m
    mutable std::atomic<bool> any_override{};   // the optimisation pass: an empty table costs a lookup no lock (Sun and Fog are hot)
    // I-0141 tranche 7 (SetAreaFades): per area, the layers before the change, the start and the duration; guarded by variant_m.
    // The weight is the one AdvanceFades last set (the fade's answers change only there, with the generation: the step review's M1).
    struct Fade { std::shared_ptr<const std::vector<Bound>> from; double start{}; float duration{}; float weight{}; };
    mutable std::map<uint32_t,Fade> fades;
    mutable std::atomic<bool> any_fade{};
    mutable std::atomic<uint64_t> fade_generation{};   // bumped under variant_m by SetAreaFades and AdvanceFades (the step review's M1)
    // The earlier layers of a fading area, seen by Layer, File, BoundFile and OwnEntries on this thread while a lookup builds the fade's first
    // record (FadeView); the files themselves are immutable after Load.
    struct View { const Impl* impl{}; uint32_t area{}; const std::vector<Bound>* layers{}; };
    static thread_local View view;
    const std::vector<Bound>* Viewed(uint32_t area) const { return view.impl==this && view.area==area?view.layers:nullptr; }
    // The weight AdvanceFades or SetAreaFades last gave a fade of `area` (and its earlier layers, shared); false when none.
    bool FadeOf(uint32_t area,float& w,std::shared_ptr<const std::vector<Bound>>* from) const {
        if(!any_fade.load(std::memory_order_acquire)) return false; std::lock_guard<std::mutex> l(variant_m);
        const auto it=fades.find(area); if(it==fades.end()) return false;
        w=it->second.weight; if(from) *from=it->second.from; return true; }
    static float Weight(double now,double start,float duration) { const double x=(now-start)/duration; return x>0.0?static_cast<float>(x):0.0f; }
    // The override of a raw key (before Resolve: the engine's node at that key answers before any fallback), copied under the lock.
    bool Override(uint32_t group,uint32_t area,uint32_t id,RecordOverride& out) const {
        if(!any_override.load(std::memory_order_acquire)) return false; std::lock_guard<std::mutex> l(variant_m); if(overrides.empty()) return false;
        const auto it=overrides.find({group,area,id}); if(it==overrides.end()) return false; out=it->second; return true; }
    const ds3content::Gparam* Layer(uint32_t area,uint32_t id,size_t group,std::array<float,12>& rows) const {
        if(const auto* earlier=Viewed(area)) { for(const auto& b:*earlier) { const auto f=files.find(b.file); if(f==files.end()) continue; const auto& g=*f->second;
                if(group<g.groups.size() && !g.groups[group].params.empty() && g.groups[group].params[0].IndexOf(static_cast<int32_t>(id))>=0) { rows=b.rows; return &g; } }
            return nullptr; }
        std::lock_guard<std::mutex> l(variant_m); const auto it=layers.find(area); if(it==layers.end()) return nullptr;
        for(const auto& b:it->second) { const auto f=files.find(b.file); if(f==files.end()) continue; const auto& g=*f->second;
            if(group<g.groups.size() && !g.groups[group].params.empty() && g.groups[group].params[0].IndexOf(static_cast<int32_t>(id))>=0) { rows=b.rows; return &g; } }
        return nullptr; }
    // A record's source label for the file File(area) gave: default (areas 0, 1), a cutscene's own file by its name (sAA_BB_VVVV,
    // part 4b: the grading resolver builds a map's LUT name from "area" and refuses any other non-map source), else "area".
    std::string Label(uint32_t area,const ds3content::Gparam* g) const {
        if(area<=1) return "default";
        if(area/100000000u==2u && g) { const auto n=AreaFileName(area); const auto it=files.find(n); if(it!=files.end() && it->second.get()==g) return n; }
        return "area"; }
    const ds3content::Gparam* File(uint32_t area,int group=-1) const {   /* group: the record kind (0 sun, 1 fog) whose variant decides; other lookups take the sun's */
        const std::string n=AreaFileName(area);
        const auto* earlier=Viewed(area);   // I-0141 tranche 7: the earlier layers' bound file (the first), none when they were empty
        if(earlier && !earlier->empty()) { const auto it=files.find(earlier->front().file); if(it!=files.end()) return it->second.get(); }
        { std::lock_guard<std::mutex> l(variant_m);
            if(const auto b=bound.find(area); !earlier && b!=bound.end()) { const auto it=files.find(b->second.file); if(it!=files.end()) return it->second.get(); }
            for(const int k:{group,0}) if(k>=0) if(const auto v=variant.find(area*16ull+static_cast<unsigned>(k)); v!=variant.end()) { const auto it=files.find(v->second); if(it!=files.end()) return it->second.get(); } }
        const auto it=files.find(n); return it==files.end()?nullptr:it->second.get(); }
    // The bound file of an area and its rows, read under one lock (a binding change never pairs one binding's file with
    // another's rows); null when the area is unbound or its file was not loaded.
    const ds3content::Gparam* BoundFile(uint32_t area,std::array<float,12>& rows) const {
        if(const auto* earlier=Viewed(area)) { if(earlier->empty()) return nullptr; const auto it=files.find(earlier->front().file);
            if(it==files.end()) return nullptr; rows=earlier->front().rows; return it->second.get(); }
        std::lock_guard<std::mutex> l(variant_m); const auto b=bound.find(area); if(b==bound.end()) return nullptr;
        const auto it=files.find(b->second.file); if(it==files.end()) return nullptr; rows=b->second.rows; return it->second.get(); }
    const std::array<float,4>* Offset(uint32_t area) const {
        if(!area) return nullptr; char n[32]; std::snprintf(n,sizeof n,"m%02u_%02u_00_00",(area/1000000u)%100u,(area/10000u)%100u);
        const auto it=offsets.find(n); return it==offsets.end()?nullptr:&it->second; }
    // 140032770 / 140038E70: key {-1, -1} reads as {1, 0}; a key absent from the bank falls back to {1, 0}.
    void Resolve(uint32_t& area,uint32_t& id,size_t group) const {
        if(area==0xFFFFFFFFu && id==0xFFFFFFFFu) { area=1; id=0; return; }
        const ds3content::Gparam* g=File(area,static_cast<int>(group)); bool found=g && group<g->groups.size() && !g->groups[group].params.empty() && g->groups[group].params[0].IndexOf(static_cast<int32_t>(id))>=0;
        std::array<float,12> rows{}; if(!found) found=Layer(area,id,group,rows)!=nullptr;
        if(!found) { area=1; id=0; } }
};
thread_local Banks::Impl::View Banks::Impl::view{};
// I-0141 tranche 16: the keys a lookup answered from an unbound area's file (Banks::UnboundAnswers), the first 32, process-wide.
namespace { constexpr uint64_t unbound_cap=32; std::mutex unbound_m; Banks::UnboundAnswer unbound_seen[unbound_cap]; std::atomic<uint64_t> unbound_generation{};
    // Entries [0, count) are written before the count's release store and never rewritten: readers scan them without the lock.
    bool UnboundHeld(uint64_t from,uint64_t to,uint32_t area,uint32_t id,uint32_t group,bool fade_view) {
        for(uint64_t k=from;k<to;++k) { const auto& u=unbound_seen[k]; if(u.area==area && u.id==id && u.group==group && u.fade_view==fade_view) return true; }
        return false; }
    void NoteUnbound(uint32_t area,uint32_t id,uint32_t group,bool offset,bool fade_view) {
        const uint64_t seen=unbound_generation.load(std::memory_order_acquire);
        if(seen>=unbound_cap || UnboundHeld(0,seen,area,id,group,fade_view)) return;   // full, or a key already listed: no lock
        std::lock_guard<std::mutex> l(unbound_m); const uint64_t n=unbound_generation.load(std::memory_order_relaxed);
        if(n>=unbound_cap || UnboundHeld(seen,n,area,id,group,fade_view)) return;
        unbound_seen[n]={area,id,group,offset,fade_view}; unbound_generation.store(n+1,std::memory_order_release); } }
uint64_t Banks::UnboundAnswers(std::vector<UnboundAnswer>* out) {   // the copy and the count from one acquire load
    const uint64_t n=unbound_generation.load(std::memory_order_acquire);
    if(out) out->assign(unbound_seen,unbound_seen+n);
    return n; }
// I-0141 tranche 7: the earlier layers seen while the fade's first record is built (restored on every exit).
struct FadeView { Banks::Impl::View before; FadeView(const Banks::Impl* impl,uint32_t area,const std::vector<Banks::Impl::Bound>* layers):before(Banks::Impl::view) { Banks::Impl::view={impl,area,layers}; }
    ~FadeView() { Banks::Impl::view=before; } FadeView(const FadeView&)=delete; FadeView& operator=(const FadeView&)=delete; };
// I-0141 tranche 24: an area's fade as OwnSun and OwnFog take it, the earlier layers and the weight.
struct Banks::LayerFade { const std::vector<Impl::Bound>* from{}; float w{}; };
Banks::Banks():impl_(new Impl) {}
Banks::~Banks() { delete impl_; }
size_t Banks::Files() const { return impl_->files.size(); }
bool Banks::Load(const std::filesystem::path& dir,const Banks* previous) {
    std::vector<std::filesystem::path> disk_files;
    if(!g_listing_reader) for(const auto& f:std::filesystem::directory_iterator(dir)) disk_files.push_back(f.path());
    const auto& files=g_listing_reader?g_listing_reader(dir):disk_files;
    for(const auto& f:files) { const std::string n=f.filename().string();
        const size_t g=n.find(".gparam"); if(g==std::string::npos) continue;
        const size_t p=n.find("drawparam_"); const size_t from=p==std::string::npos?0:p+10;
        const std::string key=n.substr(from,g-from);
        try { std::shared_ptr<const std::vector<uint8_t>> bytes=g_shared_reader?g_shared_reader(f):nullptr;
            if(bytes) {
                if(previous) if(const auto known=previous->impl_->sources.find(key); known!=previous->impl_->sources.end() && known->second==bytes) {
                    if(const auto parsed=previous->impl_->files.find(key); parsed!=previous->impl_->files.end()) { impl_->files[key]=parsed->second; impl_->sources[key]=std::move(bytes); continue; } }
                impl_->files[key]=std::make_shared<const ds3content::Gparam>(ds3content::ReadGparam(*bytes)); impl_->sources[key]=std::move(bytes); }
            else impl_->files[key]=std::make_shared<const ds3content::Gparam>(ds3content::ReadGparam(ReadFile(f))); } catch(...) {} }
    for(const auto& l:Lines(dir/"mapoffset.txt")) { char map[32]{}; unsigned b[4]{};
        if(sscanf_s(l.c_str(),"%31s %x %x %x %x",map,static_cast<unsigned>(sizeof map),&b[0],&b[1],&b[2],&b[3])!=5) continue;
        std::array<float,4> v{}; std::memcpy(v.data(),b,16); impl_->offsets[map]=v; }
    for(const auto& l:Lines(dir/"envregions.txt")) { char map[32]{}; unsigned n=0; std::array<uint32_t,14> b{}; int t08=0;
        if(sscanf_s(l.c_str(),"%31s %u %x %x %x %x %x %x %x %x %x %x %x %d",map,static_cast<unsigned>(sizeof map),&n,&b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9],&b[10],&t08)!=14) continue;
        b[11]=static_cast<uint32_t>(t08); impl_->boxes[map][n]=b; }
    for(const auto& l:Lines(dir/"sssprofiles.txt")) { char mtd[128]{}; unsigned k=0; std::array<uint32_t,4> v{};
        if(sscanf_s(l.c_str(),"base %u %127s %x %x %x %x",&k,mtd,static_cast<unsigned>(sizeof mtd),&v[0],&v[1],&v[2],&v[3])==6) impl_->sss_base[k]={mtd,v};
        else if(sscanf_s(l.c_str(),"gx15 %127s %x %x %x",mtd,static_cast<unsigned>(sizeof mtd),&v[0],&v[1],&v[2])==4) impl_->gx15[mtd].push_back({v[0],v[1],v[2]}); }
    return impl_->files.count("default")!=0;
}
namespace {
// Value `component` of `param` (group `group`) at `id`; false when the ID is absent.
bool Value(const ds3content::Gparam& g,size_t group,size_t param,int32_t id,unsigned component,uint32_t& out) {
    if(group>=g.groups.size() || param>=g.groups[group].params.size()) return false;
    const auto& p=g.groups[group].params[param]; const int i=p.IndexOf(id); if(i<0 || component>=p.Components()) return false;
    out=p.values[static_cast<size_t>(i)][component]; return true;
}
bool CopyValue(const ds3content::Gparam& g,size_t group,size_t param,int32_t id,unsigned n,uint8_t* to) {
    for(unsigned c=0;c<n;++c) { uint32_t u; if(!Value(g,group,param,id,c,u)) return false; std::memcpy(to+4*c,&u,4); } return true;
}
void FadeValue(const std::string& group,const std::string& param,uint8_t type,unsigned n,const uint32_t* a,const uint32_t* v,float w,uint32_t* out);
// I-0141 tranche 24 (« Suggested next step » 5): the earlier end of a value fade, the file and value id the earlier layers' lookup
// found (null: no earlier end, the current values alone).
struct ValueEnd { const ds3content::Gparam* g{}; int32_t v{}; float w{}; };
// CopyValue, then, with an earlier end, the value between the earlier layers' and the current's at the fade's weight by its class
// (FadeValue: the whole value, so a vector's equality test sees every lane). The parameters are read by position, as the records
// are filled; a position whose type differs between the two files keeps the current's (as Shadow does; GroupWord matches by name). Two files whose layouts differ inside a group would pair other parameters (the final review's L1: only m21_00_0000's and m22_00_0000's LightSet differ among the maps, and OwnSun refuses them as the current end; as the earlier end, unreachable: one file per test area). `group_name`
// and `param_name` name the definition for FadeValue's two tests (the values declared without the fade, the angle pairs): the
// LightSet and Fog groups declare none of the first (fadeflags.tsv), LightSet's Angle the second.
bool FadedCopy(const ds3content::Gparam& g,size_t group,size_t param,int32_t id,unsigned n,uint8_t* to,const ValueEnd* earlier,const char* group_name,const char* param_name) {
    if(!CopyValue(g,group,param,id,n,to)) return false;
    if(!earlier || !earlier->g || group>=earlier->g->groups.size() || param>=earlier->g->groups[group].params.size()) return true;
    const auto& p=g.groups[group].params[param]; const auto& q=earlier->g->groups[group].params[param];
    const int i=p.IndexOf(id), j=q.IndexOf(earlier->v); const unsigned m=std::min(4u,p.Components());
    if(q.type!=p.type || i<0 || j<0 || q.Components()<m || m<n) return true;
    uint32_t a[4]{},v[4]{},o[4]{}; for(unsigned c=0;c<m;++c) { a[c]=q.values[static_cast<size_t>(j)][c]; v[c]=p.values[static_cast<size_t>(i)][c]; }
    FadeValue(group_name,param_name,p.type,m,a,v,earlier->w,o); std::memcpy(to,o,4u*n); return true;
}
// 140148E30: the direction of an Angle pair (theta, phi): rows of .rdata 0x143D1C480 / C4A0 / BFA0 / C080 combined with
// sinf / cosf of phi (141FD1A40 / 141FD1740) then theta, in the decompiled order.
void AngleDirection(float theta,float phi,float out[4]) {
    float c480[4],c4a0[4],bfa0[4],c080[4]; Rdata(0x3D1C480,c480,16); Rdata(0x3D1C4A0,c4a0,16); Rdata(0x3D1BFA0,bfa0,16); Rdata(0x3D1C080,c080,16);
    const float s1=std::sin(phi), c1=std::cos(phi);
    const float f9=c1*c480[0], f10=c480[1]*0.0f, f11=s1*c480[2], f12=c480[3]*0.0f;
    const float c0=std::cos(theta), s0=std::sin(theta);
    const float f13=c4a0[0]*0.0f, f5=s0*c4a0[1], f4=c0*c4a0[2], f16=c4a0[3]*0.0f;
    out[0]=((f4*s1+f13*f9)+f16*c080[0])+f5*bfa0[0];
    out[1]=((f4*0.0f+f13*f10)+f16*c080[1])+f5*bfa0[1];
    out[2]=((f4*c1+f13*f11)+f16*c080[2])+f5*bfa0[2];
    out[3]=((f4*0.0f+f13*f12)+f16*c080[3])+f5*bfa0[3];
}
// 140149040(out, v, q), instruction by instruction (SSE): v masked to xyz and normalised (sqrtps / divps, inf and zero
// guards), then q v conj(q) with the sign rows at .rdata 0x143D4EE80 / E90 / EA0 / EB0; out[0..3] and {x, z, y} at +0x44.
void Rotate(const float v[4],const float q[4],uint8_t* out) {
    alignas(16) float c[8][4]; const uint32_t rva[8]={0x39F2720,0x3D1C160,0x3D1C170,0x3D1C0E0,0x3D4EE80,0x3D4EE90,0x3D4EEA0,0x3D4EEB0};
    for(int i=0;i<8;++i) Rdata(rva[i],c[i],16);
    const __m128 mask=_mm_load_ps(c[0]), inf=_mm_load_ps(c[1]), nan=_mm_load_ps(c[2]), keep=_mm_load_ps(c[3]), e80=_mm_load_ps(c[4]), e90=_mm_load_ps(c[5]), ea0=_mm_load_ps(c[6]), eb0=_mm_load_ps(c[7]);
    __m128 x3=_mm_and_ps(_mm_loadu_ps(v),mask), x8=_mm_loadu_ps(q);
    __m128 x2=_mm_mul_ps(x3,x3), x0=_mm_shuffle_ps(x2,x2,0xEE); x2=_mm_add_ps(x2,x0); x2=_mm_shuffle_ps(x2,x2,0x40); x0=_mm_shuffle_ps(x0,x2,0xF0); x2=_mm_add_ps(x2,x0);
    x0=_mm_setzero_ps(); x2=_mm_shuffle_ps(x2,x2,0xAA); __m128 x1=_mm_sqrt_ps(x2); x2=_mm_cmpneq_ps(x2,inf); x3=_mm_div_ps(x3,x1); x1=_mm_cmpneq_ps(x1,x0);
    x3=_mm_and_ps(x3,x1); x3=_mm_and_ps(x3,x2); x2=_mm_andnot_ps(x2,nan); x3=_mm_or_ps(x3,x2);
    __m128 x6=_mm_and_ps(x3,keep); x3=_mm_mul_ps(x8,e80); __m128 x7=x3;
    x2=_mm_shuffle_ps(x6,x6,0x00); x1=_mm_shuffle_ps(x6,x6,0x55); x0=_mm_shuffle_ps(x6,x6,0xAA); x6=_mm_shuffle_ps(x6,x6,0xFF);
    x7=_mm_shuffle_ps(x7,x3,0x1B); x2=_mm_mul_ps(x2,x7); x6=_mm_mul_ps(x6,x3); x7=_mm_shuffle_ps(x7,x7,0xB1); x2=_mm_mul_ps(x2,e90); x1=_mm_mul_ps(x1,x7);
    x7=_mm_shuffle_ps(x7,x7,0x1B); x6=_mm_add_ps(x6,x2); x2=x8; x1=_mm_mul_ps(x1,ea0); x7=_mm_mul_ps(x7,x0); x2=_mm_shuffle_ps(x2,x8,0x00); x0=x8;
    x7=_mm_mul_ps(x7,eb0); x0=_mm_shuffle_ps(x0,x8,0xAA); x7=_mm_add_ps(x7,x1); x1=_mm_shuffle_ps(x8,x8,0x55); x8=_mm_shuffle_ps(x8,x8,0xFF);
    x7=_mm_add_ps(x7,x6); x8=_mm_mul_ps(x8,x7); x6=_mm_shuffle_ps(x7,x7,0x1B); x2=_mm_mul_ps(x2,x6); x6=_mm_shuffle_ps(x6,x6,0xB1);
    x2=_mm_mul_ps(x2,e90); x1=_mm_mul_ps(x1,x6); x6=_mm_shuffle_ps(x6,x6,0x1B); x8=_mm_add_ps(x8,x2); x1=_mm_mul_ps(x1,ea0); x6=_mm_mul_ps(x6,x0);
    x6=_mm_mul_ps(x6,eb0); x6=_mm_add_ps(x6,x1); x6=_mm_add_ps(x6,x8);
    float o[4]; _mm_storeu_ps(o,x6);
    std::memcpy(out,o,16); std::memcpy(out+0x44,&o[0],4); std::memcpy(out+0x48,&o[2],4); std::memcpy(out+0x4C,&o[1],4);
}
// 035E60, the game's older matrix-to-quaternion routine. It forms reciprocal
// square-root candidates and selects/permutates them; normalising a modern
// quaternion tensor instead changes rounding near a half turn.
void MapQuaternion(__m128 r0,__m128 r1,__m128 r2,float q[4]) {
    auto C=[](uint32_t rva) { __m128 x{}; Rdata(rva,&x,16); return x; };
    auto choose=[](__m128 mask,__m128 yes,__m128 no) {return _mm_or_ps(_mm_and_ps(mask,yes),_mm_andnot_ps(mask,no));};
    auto permute=[](__m128 a,__m128 b,__m128 control) {
        uint8_t joined[32],indices[16],out[16];
        std::memcpy(joined,&a,16); std::memcpy(joined+16,&b,16); std::memcpy(indices,&control,16);
        for(unsigned i=0;i<16;++i) out[i]=joined[(indices[i]^3u)&31u];
        __m128 x; std::memcpy(&x,out,16); return x;
    };
    __m128 x4=C(0x3D1C120), x13=C(0x3D1C190);
    __m128 x0=_mm_mul_ps(_mm_shuffle_ps(r0,r0,0),C(0x3D1DBD0));
    __m128 x5=_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(r1,r1,0x55),C(0x3D1DC50)),x0);
    x5=_mm_add_ps(x5,_mm_mul_ps(_mm_shuffle_ps(r2,r2,0xAA),C(0x3D1DBE0)));
    __m128 x3=_mm_add_ps(x4,x5); x4=_mm_div_ps(x4,_mm_sqrt_ps(x3));
    const __m128 integer_mask=_mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(_mm_and_ps(x4,C(0x3D1C180))),_mm_castps_si128(C(0x3D1C150))));
    const __m128 exponent_mask=_mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(_mm_and_ps(x4,C(0x3D1C160))),_mm_castps_si128(C(0x3D1C160))));
    __m128 x2=_mm_andnot_ps(integer_mask,exponent_mask);
    const __m128 x9=_mm_mul_ps(choose(x2,x3,_mm_mul_ps(x4,x3)),x13);
    x13=_mm_mul_ps(x13,x4);
    const __m128 comparisons=_mm_cmple_ps(permute(x5,C(0x3D1C0C0),C(0x3D1DC20)),permute(x5,x5,C(0x3D1DB90)));
    x4=_mm_shuffle_ps(comparisons,comparisons,0xAA);
    const __m128 x7=_mm_shuffle_ps(comparisons,comparisons,0), x6=_mm_shuffle_ps(comparisons,comparisons,0x55), x8=_mm_shuffle_ps(comparisons,comparisons,0xFF);
    __m128 x1=choose(x4,C(0x3D1DC10),C(0x3D1DC70)); x5=choose(x7,C(0x3D1DC70),x1);
    x1=choose(x4,C(0x3D1DBA0),C(0x3D1DB50)); x3=choose(x7,C(0x3D1DB50),x1);
    x1=choose(x4,C(0x3D1DB60),C(0x3D1DBB0)); x4=choose(x7,C(0x3D1DBB0),x1);
    x1=choose(x6,C(0x3D1DC30),x3); x1=choose(x7,x1,x3); x2=choose(x8,C(0x3D1C130),x1);
    x1=choose(x6,C(0x3D1DC60),x4); x1=choose(x7,x1,x4); x3=choose(x8,C(0x3D1DBF0),x1);
    x0=choose(x6,C(0x3D1DC40),x5); x0=choose(x7,x0,x5); x1=choose(x8,C(0x3D1DB80),x0);
    const __m128 scale=permute(x13,x13,x1), pair=permute(r1,r2,C(0x3D1DC00));
    x0=_mm_mul_ps(_mm_add_ps(_mm_mul_ps(permute(r0,pair,C(0x3D1DBC0)),x2),permute(r0,pair,C(0x3D1DB70))),scale);
    _mm_storeu_ps(q,permute(x9,x0,x3));
}
// Record rotation (record +0x290): identity, or for a map record the rotation part of the map transform 30D80 receives,
// decomposed by 1400362B0: even pure-rotation rows are normalised before the quaternion is extracted.
// Float sin/cos rows need not have exactly unit length. The transform is the yaw of the map's MSB MapOffset (Degree,
// radians = Degree * (pi / 180) in float, rows {c, 0, -s}, {0, 1, 0}, {s, 0, c}); checked bit-exact on m40 (Degree 28).
// The rotation of a bound 3x4 transform (stored {r0, tx}, {r1, ty}, {r2, tz}), decomposed as 1400362B0 does. The records
// take the transposed rotation (Native1543, m37: the stored rows turned the sun 1.6 degrees past the original's direction,
// twice the rows' 0.8-degree offset from a half turn); the yaw rows MapYaw builds from a MapOffset Degree are that transpose.
void MapRows(const std::array<float,12>& t,float q[4]) {
    const std::array<float,12> m{t[0],t[4],t[8],0.0f, t[1],t[5],t[9],0.0f, t[2],t[6],t[10],0.0f};
    auto normalise=[](__m128 row) {
        // 036431..036470: squared x+y+z, sqrtps, divps (rotation rows are finite and nonzero).
        __m128 sum=_mm_mul_ps(row,row), y=_mm_shuffle_ps(sum,sum,0x99);
        sum=_mm_add_ss(sum,y); y=_mm_shuffle_ps(y,y,0x55); sum=_mm_add_ss(sum,y);
        sum=_mm_shuffle_ps(sum,sum,0); return _mm_div_ps(row,_mm_sqrt_ps(sum));
    };
    const __m128 r0=normalise(_mm_setr_ps(m[0],m[1],m[2],0.0f)), r1=normalise(_mm_setr_ps(m[4],m[5],m[6],0.0f)), r2=normalise(_mm_setr_ps(m[8],m[9],m[10],0.0f));
    MapQuaternion(r0,r1,r2,q);
}
void MapYaw(const std::array<float,4>* offset,float q[4]) {
    q[0]=q[1]=q[2]=0.0f; q[3]=1.0f; if(!offset) return;
    const float rad=(*offset)[3]*(3.141592654f/180.0f), s=std::sin(rad), c=std::cos(rad);
    MapRows({c,0.0f,s,0.0f, 0.0f,1.0f,0.0f,0.0f, -s,0.0f,c,0.0f},q);   // stored form: decomposed as {c, 0, -s}, {0, 1, 0}, {s, 0, c}
}
}
bool Banks::Sun(uint32_t area,uint32_t id,uint8_t* r,uint32_t bytes) const {
    if(bytes<0x260) return false;
    if(RecordOverride o; impl_->Override(0,area,id,o)) {   // I-0141 tranche 3: FUN_140031A20's record (A's, or A and B blended at t)
        alignas(16) uint8_t ra[0x2A0], rb[0x2A0], out[0x2A0];
        // 147ED0 reads B only when t > 0 (the final review's L9): at t <= 0 A stands for it, so a B without a record loses nothing.
        if(!g_light_blend || !OwnSun(o.a[0],o.a[1],ra,sizeof ra) || (0.0f<o.t && !OwnSun(o.b[0],o.b[1],rb,sizeof rb))) { std::memset(r,0,bytes); return false; }
        g_light_blend(out,ra,0.0f<o.t?rb:ra,o.t); std::memset(r,0,bytes); std::memcpy(r,out,bytes<sizeof out?bytes:sizeof out); return true; }
    // I-0141 tranche 7, on the per-value path since tranche 24: a fading area's record from its values faded one by one between the
    // earlier layers' and the current layers' at the fade's weight (OwnSun with the fade).
    { float w{}; std::shared_ptr<const std::vector<Impl::Bound>> from; if(impl_->FadeOf(area,w,&from) && from) { const LayerFade fade{from.get(),w}; return OwnSun(area,id,r,bytes,&fade); } }
    return OwnSun(area,id,r,bytes);
}
// A key's own record, by the bank's rule, without an override (an override's sources: one level, the bank's own records).
// I-0141 tranche 24 (« Suggested next step » 5; STUDY-LAYER-FADE §2, the value classes READ in tranche 23): with `fade`, each value
// the record is derived from is the one FadeValue gives between the earlier layers' (their lookup by the same rule, through
// FadeView) and the current layers', as FUN_140095B00 rebuilds the live fields before the records are made from them; the record
// is then derived from those values as from a file's: an Angle pair through AngleLerp, then its direction turned by the current
// layers' rotation (the map transform of the current binding: INFERRED, the derivation's transform not read during a fade), the
// colours and floats lerped, the three bools at once, the 14B3A0 constants as ever. Without an earlier record (its lookup finding
// no file), the current values alone.
bool Banks::OwnSun(uint32_t area,uint32_t id,uint8_t* r,uint32_t bytes,const LayerFade* fade) const {
    std::memset(r,0,bytes);
    {   static const bool show=[]{ char v[4]{}; size_t k=0; return !getenv_s(&k,v,sizeof v,"BANK_DIFF") && k; }();   /* I-0063 diagnostic: the key and its resolution */
        if(show) { uint32_t a=area,i=id; impl_->Resolve(a,i,0); std::printf("banksun key %u %u -> %u %u file %d\n",area,id,a,i,impl_->File(a)?1:0); } }
    // The file, value id and rotation a lookup finds for {a, i} (Resolve, then the latest layer holding the id, the bound file, else
    // the area's file turned by its MapOffset yaw); `unbound` and `turned` for NoteUnbound.
    const auto source=[&](uint32_t& a,uint32_t& i,float q[4],bool& unbound,bool& turned)->const ds3content::Gparam* {
        impl_->Resolve(a,i,0);
        std::array<float,12> rows; const ds3content::Gparam* g=impl_->Layer(a,i,0,rows); if(!g) g=impl_->BoundFile(a,rows);
        unbound=turned=false;
        if(g) MapRows(rows,q); else { g=impl_->File(a,0); const auto* o=impl_->Offset(a); MapYaw(o,q); unbound=g && a>1; turned=o!=nullptr; }
        return g; };
    const uint32_t asked=area, asked_id=id;
    float q[4]; bool unbound{}, turned{};   // noted once the record is whole (after the last refusal below)
    const ds3content::Gparam* g=source(area,id,q,unbound,turned);
    if(!g) return false; const int32_t v=static_cast<int32_t>(id);
    ValueEnd before{}; uint32_t before_area=asked, before_id=asked_id; bool before_unbound{}, before_turned{};
    if(fade && fade->from) { const FadeView view(impl_,asked,fade->from); float unused[4];
        before.g=source(before_area,before_id,unused,before_unbound,before_turned); before.v=static_cast<int32_t>(before_id); before.w=fade->w; }
    const ValueEnd* e=before.g?&before:nullptr;
    for(uint32_t k=0;k<4;++k) { const uint32_t b=0x70u*k; const size_t p=5u*k;
        if(!FadedCopy(*g,0,p+1,v,4,r+b+0x10,e,"LightSet","") || !FadedCopy(*g,0,p+2,v,4,r+b+0x20,e,"LightSet","") || !FadedCopy(*g,0,p,v,2,r+b+0x30,e,"LightSet","Angle")
            || !FadedCopy(*g,0,p+3,v,1,r+b+0x40,e,"LightSet","") || !FadedCopy(*g,0,p+4,v,1,r+b+0x50,e,"LightSet","")) return false;
        std::memcpy(r+b+0x38,r+b+0x30,8);   // the pair twice, as before
        float a[2]; std::memcpy(a,r+b+0x30,8); float d[4]; AngleDirection(a[0],a[1],d); Rotate(d,q,r+b); }
    if(!FadedCopy(*g,0,20,v,4,r+0x1E0,e,"LightSet","") || !FadedCopy(*g,0,21,v,4,r+0x1F0,e,"LightSet","") || !FadedCopy(*g,0,22,v,1,r+0x248,e,"LightSet","")) return false;
    if(!Rdata(0x39F24E0,r+0x220,16) || !Rdata(0x3D7FED0,r+0x230,16)) return false;
    const uint32_t tenth=0x3DCCCCCDu, fifty=0x42480000u; std::memcpy(r+0x240,&tenth,4); std::memcpy(r+0x244,&fifty,4);   // 14B3A0 defaults
    for(uint32_t f=0;f<3;++f) { uint8_t u[4]; if(!FadedCopy(*g,0,23+f,v,1,u,e,"LightSet","")) return false; uint32_t x; std::memcpy(&x,u,4); r[0x24C+f]=static_cast<uint8_t>(x!=0); }
    if(bytes>=0x2A0) std::memcpy(r+0x290,q,16);   // I-0140 tranche 7: the record's rotation (engine +0x2D0), for F90F0's blend
    // I-0141 tranche 6's witness (i141-t6a, effect-light-observe): the engine record's +0x2C0 (here +0x280), the strength the effect draws
    // scale their colours by (264230 at 140265919), is the LightSet group's « LightMask Influence » (its parameter 26): the original's
    // answers hold 0.96 for Archdragon Peak's keys, m32_00_0000's value, and 0 for default's {1, 0}, its value there (OBSERVED, both
    // runs' effect_light_key rows); 14B3A0's default 1 where a file lacks the parameter (some map files do).
    if(bytes>=0x284) { const float one=1.0f; std::memcpy(r+0x280,&one,4); FadedCopy(*g,0,26,v,1,r+0x280,e,"LightSet",""); }
    if(unbound) NoteUnbound(area,id,0,turned,impl_->Viewed(area)!=nullptr);
    if(e && before_unbound) NoteUnbound(before_area,before_id,0,before_turned,true);   // the earlier end's, as its own lookup noted it before
    return true;
}
void Banks::BindArea(uint32_t area,const std::string& file,const std::array<float,12>& transform) const {
    std::lock_guard<std::mutex> l(impl_->variant_m); impl_->bound[area]=Impl::Bound{file,transform}; impl_->layers.erase(area); ++impl_->binding_generation;
}
void Banks::ClearAreaBindings() const { std::lock_guard<std::mutex> l(impl_->variant_m); impl_->bound.clear(); impl_->layers.clear(); ++impl_->binding_generation; }
uint64_t Banks::BindingGeneration() const { return impl_->binding_generation.load(std::memory_order_acquire); }   // bumped under variant_m, read without it
void Banks::SetRecordOverrides(const RecordOverrides& overrides) const {
    std::lock_guard<std::mutex> l(impl_->variant_m);
    const bool same=overrides.size()==impl_->overrides.size() && std::equal(overrides.begin(),overrides.end(),impl_->overrides.begin(),[](const auto& x,const auto& y){
        return x.first==y.first && !std::memcmp(x.second.a,y.second.a,8) && !std::memcmp(x.second.b,y.second.b,8) && x.second.t==y.second.t && x.second.layer==y.second.layer; });
    if(same) return; impl_->overrides=overrides; impl_->any_override.store(!overrides.empty(),std::memory_order_release); ++impl_->binding_generation;
}
bool Banks::AuthoredLightKey(uint32_t area,uint32_t id) const {
    if(CutsceneArea(area)) return true;
    RecordOverride o; if(!impl_->Override(0,area,id,o)) return false;
    return CutsceneArea(o.a[0]) && (!(0.0f<o.t) || CutsceneArea(o.b[0]));
}
void Banks::SetAreaFades(const std::map<uint32_t,AreaFade>& fades) const {
    const double now=FadeClock();
    std::map<uint32_t,Impl::Fade> next;
    for(const auto& [area,f]:fades) { if(!(f.duration>0.0f) || f.from.empty()) continue; const float w=Impl::Weight(now,f.start,f.duration); if(!(w<1.0f)) continue;
        auto& n=next[area]; n.start=f.start; n.duration=f.duration; n.weight=w;
        auto from=std::make_shared<std::vector<Impl::Bound>>(); for(const auto& b:f.from) from->push_back(Impl::Bound{b.first,b.second}); n.from=std::move(from); }
    std::lock_guard<std::mutex> l(impl_->variant_m);
    const bool same=next.size()==impl_->fades.size() && std::equal(next.begin(),next.end(),impl_->fades.begin(),[](const auto& x,const auto& y){
        return x.first==y.first && x.second.start==y.second.start && x.second.duration==y.second.duration && x.second.from->size()==y.second.from->size()
            && std::equal(x.second.from->begin(),x.second.from->end(),y.second.from->begin(),[](const auto& p,const auto& q){ return p.file==q.file && p.rows==q.rows; }); });
    if(same) return; impl_->fades.swap(next); impl_->any_fade.store(!impl_->fades.empty(),std::memory_order_release); ++impl_->fade_generation;
}
void Banks::AdvanceFades() const {
    if(!impl_->any_fade.load(std::memory_order_acquire)) return;
    const double now=FadeClock(); std::lock_guard<std::mutex> l(impl_->variant_m); if(impl_->fades.empty()) return;
    for(auto it=impl_->fades.begin();it!=impl_->fades.end();) { const float w=Impl::Weight(now,it->second.start,it->second.duration);
        if(!(w<1.0f)) it=impl_->fades.erase(it); else { it->second.weight=w; ++it; } }
    impl_->any_fade.store(!impl_->fades.empty(),std::memory_order_release); ++impl_->fade_generation;   // Sun's and Fog's answers changed (RecordGeneration)
}
uint64_t Banks::RecordGeneration() const { return impl_->binding_generation.load(std::memory_order_acquire)+impl_->fade_generation.load(std::memory_order_acquire); }
bool Banks::Fading(uint32_t area) const { float w{}; return impl_->FadeOf(area,w,nullptr); }
bool Banks::Overridden(uint32_t group,uint32_t area,uint32_t id) const { if(!impl_->any_override.load(std::memory_order_acquire)) return false; std::lock_guard<std::mutex> l(impl_->variant_m); return impl_->overrides.count({group,area,id})!=0; }
void Banks::SetAreaLayers(const std::map<uint32_t,std::vector<std::pair<std::string,std::array<float,12>>>>& layers) const {
    std::map<uint32_t,Impl::Bound> next; std::map<uint32_t,std::vector<Impl::Bound>> all;
    for(const auto& [area,list]:layers) { if(list.empty()) continue; next[area]=Impl::Bound{list.front().first,list.front().second};
        for(const auto& b:list) all[area].push_back(Impl::Bound{b.first,b.second}); }
    std::lock_guard<std::mutex> l(impl_->variant_m); impl_->bound.swap(next); impl_->layers.swap(all); ++impl_->binding_generation;
}
void Banks::SetAreaBindings(const std::map<uint32_t,std::pair<std::string,std::array<float,12>>>& bindings) const {
    std::map<uint32_t,Impl::Bound> next; for(const auto& [area,b]:bindings) next[area]=Impl::Bound{b.first,b.second};
    std::lock_guard<std::mutex> l(impl_->variant_m); impl_->bound.swap(next); impl_->layers.clear(); ++impl_->binding_generation;   // review: no stale layer overrides it
}
bool Banks::SelectVariant(uint32_t area,uint32_t id,const uint8_t* copied,bool fog,uint32_t limit) const {
    if(area<=1) return false;
    // words the builders read: sun (1496D0) sun_read; fog (14E900) floats 0-5, 8, 9, 12-18, 20 and the byte at 0x1C
    const uint64_t key=area*16ull+(fog?1u:0u);
    char prefix[16]; std::snprintf(prefix,sizeof prefix,"m%02u_%02u_",(area/1000000u)%100u,(area/10000u)%100u);
    std::string before; { std::lock_guard<std::mutex> l(impl_->variant_m); if(const auto v=impl_->variant.find(key); v!=impl_->variant.end()) before=v->second; }
    auto same_words=[&](const uint8_t* built,const auto& ranges) { for(const auto& r:ranges) { if(r[1]>limit) continue; const uint32_t n=(r[1]-r[0]>=4)?4u:1u; for(uint32_t o=r[0];o<r[1];o+=n) if(std::memcmp(copied+o,built+o,n)) return false; } return true; };
    for(const auto& [name,g]:impl_->files) { if(name.rfind(prefix,0)!=0) continue;
        { std::lock_guard<std::mutex> l(impl_->variant_m); impl_->variant[key]=name; }
        alignas(16) uint8_t built[0x260];
        if(fog ? (Fog(area,id,built,sizeof built) && same_words(built,fog_variant_read)) : (Sun(area,id,built,sizeof built) && same_words(built,sun_read))) { if(before!=name) { std::lock_guard<std::mutex> l(impl_->variant_m); ++impl_->binding_generation; } return true; } }   // pre-merge review: the caches renew only when the chosen variant changes
    // I-0063 (CONTENT-AXES): no map file holds the engine's record: the global states (eclipse, mapoverride_000N, mapbase,
    // default) next, which a story state can load in place of the map's (offline gap, not yet witnessed live).
    // (review) in an explicit order, the first match staying the area's: eclipse, the overrides, mapbase, default
    for(const char* name:{"eclipse","mapoverride_0000","mapoverride_0001","mapoverride_0002","mapoverride_0003","mapbase","default"}) { if(!impl_->files.count(name)) continue;
        { std::lock_guard<std::mutex> l(impl_->variant_m); impl_->variant[key]=name; }
        alignas(16) uint8_t built[0x260];
        if(fog ? (Fog(area,id,built,sizeof built) && same_words(built,fog_variant_read)) : (Sun(area,id,built,sizeof built) && same_words(built,sun_read))) { if(before!=name) { std::lock_guard<std::mutex> l(impl_->variant_m); ++impl_->binding_generation; } return true; } }   // pre-merge review: the caches renew only when the chosen variant changes
    std::lock_guard<std::mutex> l(impl_->variant_m); if(before.empty()) impl_->variant.erase(key); else impl_->variant[key]=before; return false;
}
bool Banks::Fog(uint32_t area,uint32_t id,uint8_t* r,uint32_t bytes) const {
    if(bytes<0x80) return false;
    // I-0141 tranche 3: FUN_1400380A0's record: its +0x10..+0x6F (here +0x00..+0x5F) A's, or 14014E5D0's blend of A and B when t > 0
    // (its blend branch writes +0x00..+0x57, t >= 1 copies B's 0x60 bytes: the disassembly); the rest is the overridden node's own:
    // 380A0 writes the key's id at the node's +0x70 and its area at +0x74 (here +0x60, left 0 as this bank's own records leave it,
    // and +0x64, the key's area).
    if(RecordOverride o; impl_->Override(1,area,id,o)) {
        alignas(16) uint8_t fa[0x80], fb[0x80];
        const bool blend=0.0f<o.t;
        std::memset(r,0,bytes);
        if(!OwnFog(o.a[0],o.a[1],fa,sizeof fa) || (blend && (!g_fog_blend || !OwnFog(o.b[0],o.b[1],fb,sizeof fb)))) return false;
        if(blend) { alignas(16) uint8_t out[0x80]{}; g_fog_blend(out,fa,fb,o.t); std::memcpy(r,out,0x60); } else std::memcpy(r,fa,0x60);
        std::memcpy(r+0x64,&area,4); return true; }
    // I-0141 tranche 7, on the per-value path since tranche 24 (as Sun's): a fading area's record from its values faded one by one.
    { float w{}; std::shared_ptr<const std::vector<Impl::Bound>> from; if(impl_->FadeOf(area,w,&from) && from) { const LayerFade fade{from.get(),w}; return OwnFog(area,id,r,bytes,&fade); } }
    return OwnFog(area,id,r,bytes);
}
// I-0141 tranche 24: with `fade`, each Fog value faded from the earlier layers' (their lookup by the same rule, through FadeView) as
// OwnSun's (the floats and colours lerped, ToUpper, a bool, at once); the map position +0x4C and the area +0x64 the current layers'
// (the current binding's: INFERRED, as Sun's rotation).
bool Banks::OwnFog(uint32_t area,uint32_t id,uint8_t* r,uint32_t bytes,const LayerFade* fade) const {
    std::memset(r,0,bytes);
    ValueEnd before{}; uint32_t before_area{}, before_id{}; bool before_unbound{}, before_turned{};
    if(fade && fade->from) { const FadeView view(impl_,area,fade->from); uint32_t a=area,i=id; impl_->Resolve(a,i,1);
        std::array<float,12> unused{}; const ds3content::Gparam* layer=impl_->Layer(a,i,1,unused);
        before.g=layer?layer:impl_->File(a,1); before.v=static_cast<int32_t>(i); before.w=fade->w;
        before_area=a; before_id=i; before_unbound=before.g && !layer && !impl_->BoundFile(a,unused) && a>1; before_turned=impl_->Offset(a)!=nullptr; }
    const ValueEnd* e=before.g?&before:nullptr;
    impl_->Resolve(area,id,1);
    std::array<float,12> rows{}; const ds3content::Gparam* layer=impl_->Layer(area,id,1,rows);
    const ds3content::Gparam* g=layer?layer:impl_->File(area,1); if(!g) return false; const int32_t v=static_cast<int32_t>(id);
    static const struct { size_t param; uint32_t at; unsigned n; } fields[]={{0,0x00,4},{2,0x10,1},{6,0x14,1},{7,0x18,1},{8,0x1C,1},{9,0x20,1},{10,0x24,1},{1,0x30,4},{3,0x40,1},{4,0x44,1},{5,0x48,1}};
    for(const auto& m:fields) if(!FadedCopy(*g,1,m.param,v,m.n,r+m.at,e,"Fog","")) return false;
    // The map position (+0x4C): a bound area's binding translation (rows {r0, tx}, {r1, ty}, {r2, tz}), else the MapOffset listing.
    // INFERRED rule from one zone: Reference1743 m31, the original's fog records hold (50, -155, -580), m31_00_0010's translation.
    if(layer || impl_->BoundFile(area,rows)){const float p[3]{rows[3],rows[7],rows[11]};std::memcpy(r+0x4C,p,12);}
    else { const auto* o=impl_->Offset(area); if(o) std::memcpy(r+0x4C,o->data(),12); if(area>1) NoteUnbound(area,id,1,o!=nullptr,impl_->Viewed(area)!=nullptr); }
    std::memcpy(r+0x64,&area,4);
    if(e && before_unbound) NoteUnbound(before_area,before_id,1,before_turned,true);   // the earlier end's, as its own lookup noted it before
    return true;
}
// IBL region record of Env_Box<id> (observed on m40 boxes, i45-cap-01..03): area, id, half size {W, H, D} * 0.5, the rows
// of the yaw Rotation.Y (degrees * (pi / 180) in float, rows {c, 0, -s}, {0, 1, 0}, {s, 0, c}), translation Position +
// (0, H/2, 0); +0xC8 fade distance = UnkT00 when > 0, else (EnvMapRegionManager 140065D60 -> 140066FA0, p = manager
// +0x2B0 = 0.1) sqrtf((hy^2 + hx^2) + hz^2) / ((tanf(.rdata 0x143D7EDB0 = 27.5 deg) * 2) * p); +0xD8 = C8 - C8 * 0.9
// (manager +0x2B4); +0xCC = UnkT04, +0xD0 = UnkT08 >> 8, +0xDC = 1 (1400650F0 default). Builder chain located by Fable
// (140D0B8A0 per MSB box); 140065D60 listing and the constants checked by Opus.
bool Banks::Env(uint32_t area,uint32_t id,uint8_t* r,uint32_t bytes) const {
    if(bytes<0xE0) return false; std::memset(r,0,bytes);
    if(!area) return false; char n[32]; std::snprintf(n,sizeof n,"m%02u_%02u_00_00",(area/1000000u)%100u,(area/10000u)%100u);
    const auto m=impl_->boxes.find(n); if(m==impl_->boxes.end()) return false; const auto it=m->second.find(id); if(it==m->second.end()) return false;
    const auto& b=it->second; auto F=[&](size_t i){ float f; std::memcpy(&f,&b[i],4); return f; };
    const float h[4]={F(6)*0.5f,F(7)*0.5f,F(8)*0.5f,0.0f};
    const float rad=F(4)*(3.141592654f/180.0f), s=std::sin(rad), c=std::cos(rad);
    const float rows[16]={c,0.0f,-s,0.0f, 0.0f,1.0f,0.0f,0.0f, s,0.0f,c,0.0f, F(0),F(1)+h[1],F(2),1.0f};
    std::memcpy(r,&area,4); std::memcpy(r+4,&id,4); std::memcpy(r+0x10,h,16); std::memcpy(r+0x20,rows,64);
    float fov; if(!Rdata(0x3D7EDB0,&fov,4)) return false;
    const float len=std::sqrt((h[1]*h[1]+h[0]*h[0])+h[2]*h[2]), k=(std::tan(fov)*2.0f)*0.1f;
    const float fade=F(9)>0.0f?F(9):len/k, span=fade-fade*0.9f, one=1.0f; const uint32_t t08=(b[11]>>8)&0xFFu;
    std::memcpy(r+0xC8,&fade,4); std::memcpy(r+0xCC,&b[10],4); std::memcpy(r+0xD0,&t08,4); std::memcpy(r+0xD8,&span,4); std::memcpy(r+0xDC,&one,4);
    return true;
}
namespace { bool MapBaseAnswers(uint32_t area) noexcept; }   // below, with FindGroupParam: the one mapbase rule
namespace {
float AngleLerp(float a,float b,float t,float lo,float hi);   // below, 1400F8FD0
std::string Trimmed(const std::string& s) { const auto a=s.find_first_not_of(' '),b=s.find_last_not_of(' '); return a==std::string::npos?std::string{}:s.substr(a,b-a+1); }
// I-0141 tranche 23 (RESULT « Tranche 23 »; READ in the executable, llvm-objdump and GhidraMCP, 2026-11-23): the original fades every
// draw-parameter value, not records. FUN_140095B00 rebuilds each group's live fields layer by layer through the value slot's
// vt+0x80(key, A the field's value before the layer, B the field, w): w the layer's weight (FUN_140092B50) where the slot's
// definition marks the value « fades » (byte +0x44 of its 0x50-byte definition, AddParam 1401F72A0's last argument), else 1. The
// slots (GXValueAccessorSlot<T>, vt+0x80 1401EEC30 float, 1401F2AD0 vectors, 1401E8FA0 s32, 1401ECCF0 u32, 1401E8070 s16, 1401EAE50
// u8, 1401E7120 s8, 1401DECD0 color, 1401DECA0 bool, 1401DEDF0 angle pair): w <= 0 gives A, w >= 1 the layer's value V; between,
// a float (1 - w) A + V w (V when they are equal), a vector each used lane so (V only when every used lane is equal: cmpeqps and
// movmskps; V2 1401F0BB0, V3 1401F1B70, V4 1401F2AD0), an integer the same in float, truncated (cvttss2si),
// a colour each byte so (no equality test), a bool V at once, an angle pair AngleLerp (1401DEAB0, byte for byte 1400F8FD0) per component on the periods
// ShadowSunDirection reads (no equality test). Every definition of the 26 groups was read (410 AddParam calls; the 14 enumerations,
// added by 14015F520, 140164320, 14016F930 and 14018CF50, never fade): the definitions that do not fade, by group and short name.
// The class is the file's type here (the engine's slot class is the definition's; for a file whose type differs from it, INFERRED).
bool FadesAtOnce(const std::string& group,const std::string& param) {
    static const std::set<std::pair<std::string,std::string>> at_once{
        {"AOFields","Enable"},{"AOFields","Enable UpScale"},{"Anti-Alias","Enable"},{"Anti-Alias","AntiAliasType"},
        {"Bloom","Enable"},{"Bloom","GhostSharpness"},{"Bloom","StarForceDispersion"},{"Bloom","StarRotation"},{"Bloom","GlareShape"},
        {"Cloud Shadow","Enable"},{"ColorGrading","Enable"},{"ColorGrading","LutSourceId"},{"ColorParaffin Filter","Enable"},{"ColorParaffin Filter","Mode"},
        {"Dof","DonotPassOverFocus"},{"Dof","Enable-Far-Custom"},{"Dof","Enable-Near-Custom"},{"Dof","Enable"},{"Dof","CCD-Size"},{"Dof","DiaphragmRotate"},{"Dof","DofEdgeQuarity"},
        {"Env Map","EnvMap:ID"},{"FeedBackBlur","Enable"},{"Glow Filter","Enable"},{"Glow Filter","New Glow Enable"},
        {"Lens Simulation","Enable"},{"Lens Simulation","Solve Single Step"},{"Lens Simulation","ApertureShape"},{"Lens Simulation","CoCMode"},
        {"LightShaft","Enable"},{"MotionBlur","Bidirectional PermissionEnable"},{"MotionBlur","Blur ReccurenceCount"},{"MotionBlur","Enable"},
        {"SeaWave","Bubble:ID"},{"SeaWave","BubbleMask:ID"},{"SeaWave","Enable Average Inner Simulation Time"},{"SeaWave","Enable Choppy"},
        {"SeaWave","Enable Draw Wave"},{"SeaWave","Enable Fixed Inner Simulation Time"},{"SeaWave","Light:ID"},{"SeaWave","Normal:ID"},{"SeaWave","Use Inner Simulation Time"},
        {"Setting Info","Auto Compute Enable"},{"Setting Info","BG Color"},{"Setting Info","Create MotionBlurInternalVelocityTex"},{"Setting Info","DepthOfFieldQuality"},
        {"Setting Info","Enable AnamophicLensAspectratio"},{"Setting Info","Enable AnamorphicLensAperture"},{"Setting Info","Enable Anti-Alias"},
        {"Setting Info","Enable ApertureairyDisc"},{"Setting Info","Enable ApertureairyUnlimitedRadius"},{"Setting Info","Enable Choromaticaberration"},
        {"Setting Info","Enable ColorGrading"},{"Setting Info","Enable FeedBackEffect"},{"Setting Info","Enable Gaussian"},{"Setting Info","Enable LensDistortion"},
        {"Setting Info","Enable LightShaft"},{"Setting Info","Enable MotionBlur-Dof"},{"Setting Info","Enable MotionBlurUnlimitedrecursion"},{"Setting Info","GlareQuality"},
        {"Setting Info","ReflectScene Assign DynamicLight"},{"Setting Info","Use DepthTexture"},{"Setting Info","BlurShape"},
        {"ShadowParam","Shadow  Light  Id"},{"ShadowParam","Shadow Model Cull Flip"},{"ShadowParam","Shadow Map FilterMode"},{"ShadowParam","Shadow Map Resolution"},
        {"Snow","Texture:ID"},{"Tone Map","AutoExposure"},{"Tone Map","Enable AutoExposure"},{"Tone Map","Enable"},{"Tone Map","UseTable"},
        {"Tone Map","ToneType"},{"Tone Map","WaveSample"},{"Tone Map","ToneMapMode"},
        {"WaterInteraction Filter","Enable"},{"WaterInteraction Filter","MaxResolution"},{"WaterInteraction Filter","Screen Space Enable"},
        {"WaterInteraction Filter","ScreenSpace Max Resolution"},{"WaterInteraction Filter","ScreenSpace TexelSize"}};
    return at_once.count({group,param})!=0; }
// The angle pairs (the slot class of subtype 0x16): LightSet's four Angle, Snow's and ShadowParam's.
bool FadesAsAngle(const std::string& group,const std::string& param) { return param=="Angle" && (group=="LightSet" || group=="Snow" || group=="ShadowParam"); }
int32_t Truncate32(float f) { return _mm_cvtt_ss2si(_mm_set_ss(f)); }   // cvttss2si eax
int64_t Truncate64(float f) { return _mm_cvttss_si64(_mm_set_ss(f)); }  // cvttss2si rax
// One word between A and V at 0 < w < 1 by the file's type (gparam.cpp's words: bytes zero-extended, shorts sign-extended).
uint32_t FadeWord(uint8_t type,uint32_t a,uint32_t v,float w) {
    const float u=1.0f-w; const auto lerp=[&](float x,float y){ return u*x+y*w; };
    switch(type) {
    case 9: { const float x=std::bit_cast<float>(a),y=std::bit_cast<float>(v); return x==y?v:std::bit_cast<uint32_t>(lerp(x,y)); }
    case 0xC: case 0xD: case 0xE: return std::bit_cast<uint32_t>(lerp(std::bit_cast<float>(a),std::bit_cast<float>(v)));   // a lane: FadeValue tests the whole vector
    case 1: if((a&0xFFu)==(v&0xFFu)) return v; return static_cast<uint8_t>(Truncate32(lerp(static_cast<float>(static_cast<int8_t>(a)),static_cast<float>(static_cast<int8_t>(v)))));
    case 5: if((a&0xFFu)==(v&0xFFu)) return v; return static_cast<uint8_t>(Truncate64(lerp(static_cast<float>(a&0xFFu),static_cast<float>(v&0xFFu))));
    case 2: if((a&0xFFFFu)==(v&0xFFFFu)) return v; return static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(Truncate32(lerp(static_cast<float>(static_cast<int16_t>(a)),static_cast<float>(static_cast<int16_t>(v)))))));
    case 3: if(a==v) return v; return static_cast<uint32_t>(Truncate32(lerp(static_cast<float>(static_cast<int32_t>(a)),static_cast<float>(static_cast<int32_t>(v)))));
    case 7: if(a==v) return v; return static_cast<uint32_t>(Truncate64(lerp(static_cast<float>(static_cast<uint64_t>(a)),static_cast<float>(static_cast<uint64_t>(v)))));
    case 0xF: { uint32_t out{}; for(unsigned k=0;k<4;++k) { const float x=static_cast<float>((a>>(8*k))&0xFFu),y=static_cast<float>((v>>(8*k))&0xFFu);
            out|=static_cast<uint32_t>(static_cast<uint8_t>(Truncate64(lerp(x,y))))<<(8*k); } return out; }
    default: return v; }   // 0xB, a bool: the layer's value at once; no other type is declared
}
// A value of `group`'s `param` during an area fade at weight w: `a` the earlier layers' words, `v` the current layers', n words.
void FadeValue(const std::string& group,const std::string& param,uint8_t type,unsigned n,const uint32_t* a,const uint32_t* v,float w,uint32_t* out) {
    if(FadesAtOnce(group,param) || type==0xB || w>=1.0f) { std::memcpy(out,v,4u*n); return; }
    if(w<=0.0f) { std::memcpy(out,a,4u*n); return; }   // a NaN weight passes both tests and lerps, as COMISS/JB does
    if(FadesAsAngle(group,param) && n>=2 && type==0xC) {
        float half_lo{},half_hi{},pi_lo{},pi_hi{};
        if(!Rdata(0x3D7F0D4,&half_lo,4) || !Rdata(0x3D7EE40,&half_hi,4) || !Rdata(0x3D7F0EC,&pi_lo,4) || !Rdata(0x3D7EEC0,&pi_hi,4)) { std::memcpy(out,v,4u*n); return; }
        const float x0=AngleLerp(std::bit_cast<float>(a[0]),std::bit_cast<float>(v[0]),w,half_lo,half_hi),x1=AngleLerp(std::bit_cast<float>(a[1]),std::bit_cast<float>(v[1]),w,pi_lo,pi_hi);
        out[0]=std::bit_cast<uint32_t>(x0); out[1]=std::bit_cast<uint32_t>(x1); return; }
    if(type==0xC || type==0xD || type==0xE) { bool same=true; for(unsigned c=0;c<n;++c) same=same && std::bit_cast<float>(a[c])==std::bit_cast<float>(v[c]);
        if(same) { std::memcpy(out,v,4u*n); return; } }
    for(unsigned c=0;c<n;++c) out[c]=FadeWord(type,a[c],v[c],w);
}
}
// The test seam (glow_bank_test, the final review's M2): FadeValue on synthetic words.
void FadeValueForTest(const char* group,const char* param,uint8_t type,unsigned n,const uint32_t* a,const uint32_t* v,float w,uint32_t* out) { FadeValue(group,param,type,n,a,v,w,out); }
void ShadowDefaults(uint8_t* r) {
    // 1400839F0 defaults (listing 140083A1D..140083AA0 and Fable's transcription of the rest).
    std::memset(r,0,0x94);
    auto F=[&](uint32_t at,float v){ std::memcpy(r+at,&v,4); }; auto U=[&](uint32_t at,uint32_t v){ std::memcpy(r+at,&v,4); };
    F(0x10,1.0f); U(0x20,0xFFFFFFFFu); U(0x24,0xFFFFFFFFu); F(0x34,0.1f); F(0x38,1000.0f); F(0x3C,10.0f); F(0x40,10.0f); F(0x44,30.0f); F(0x48,70.0f);
    U(0x4C,1); U(0x50,1); F(0x54,-0.001f); F(0x5C,0.7f); F(0x60,1.0f); F(0x64,100.0f); U(0x68,0x3BA3D70Au); F(0x6C,3.0f); r[0x71]=1;
    F(0x74,1.0f); F(0x78,5.0f); F(0x7C,1.5f); U(0x80,0xBDAE147Bu); F(0x88,2.0f); U(0x8C,0xFF000000u);
}
bool Banks::Shadow(uint32_t area,uint32_t id,uint8_t* r,uint32_t bytes) const {
    if(bytes<0x94) return false; std::memset(r,0,bytes);
    static const struct { uint32_t at; unsigned n; bool byte; } map[22]={{0x00,2,false},{0x24,1,false},{0x34,1,false},{0x40,1,false},{0x44,1,false},{0x48,1,false},
        {0x38,1,false},{0x3C,1,false},{0x50,1,false},{0x54,1,false},{0x64,1,false},{0x6C,1,false},{0x70,1,true},{0x71,1,true},{0x74,1,false},{0x78,1,false},
        {0x7C,1,false},{0x80,1,false},{0x84,1,false},{0x88,1,false},{0x8C,1,false},{0x90,1,false}};
    auto empty=[](const ds3content::GparamGroup* x){ if(!x) return true; for(const auto& p:x->params) if(!p.values.empty()) return false; return true; };
    // One key: the area's GPARAM ShadowParam group, or mapbase.gparam's when a map area leaves it empty (observed on m40,
    // i45-cap-05; MapBaseAnswers, R's reading of GparamAddExtraId_140D26AB0: a cutscene area without the group reads default's
    // {1, 0}); the record is written only when every parameter holds the id.
    struct Used { const ds3content::GparamGroup* grp{}; };   // the group the record came from
    const auto fill=[&](uint8_t* out,uint32_t a,uint32_t i,Used& used)->bool {
        if(!a) return false;   // area 0 is no bank's (AreaFileName names it default)
        const ds3content::Gparam* g=impl_->File(a); const ds3content::GparamGroup* grp=g?g->Group("ShadowParam"):nullptr;
        if(empty(grp) && MapBaseAnswers(a)) { const auto it=impl_->files.find("mapbase"); grp=it==impl_->files.end()?nullptr:it->second->Group("ShadowParam"); }
        if(empty(grp) || grp->params.size()<22) return false;
        for(size_t k=0;k<22;++k) if(grp->params[k].IndexOf(static_cast<int32_t>(i))<0) return false;
        ShadowDefaults(out);
        for(size_t k=0;k<22;++k) { const auto& p=grp->params[k]; const int at=p.IndexOf(static_cast<int32_t>(i));
            for(unsigned c=0;c<map[k].n;++c) { const uint32_t v=p.values[static_cast<size_t>(at)][c]; if(map[k].byte) out[map[k].at]=static_cast<uint8_t>(v); else std::memcpy(out+map[k].at+4*c,&v,4); } }
        std::memcpy(out+0x20,&a,4);   // bank tag written by 18C590 on a hit
        used=Used{grp}; return true; };
    const uint32_t asked=area;
    if(area==0xFFFFFFFFu && id==0xFFFFFFFFu) { area=1; id=0; }   // 18C590: {-1, -1} reads {1, 0}
    const auto own=[&](uint8_t* out,Used& used) { return fill(out,area,id,used) || fill(out,1,0,used); };   // 18C590: a key the bank lacks reads {1, 0}
    Used now{}; if(!own(r,now)) return false;
    // I-0141 tranche 23: a fading area's record, value by value (FadeValue) from the earlier layers' record to the current one's, the
    // parameters by record field as the engine's slots are (fill reads the files by position): Shadow  Light  Id (k 1), Shadow Map
    // FilterMode (k 8) and Shadow Map Resolution (k 21) declared without the fade and the two bool bytes the current's at once, the
    // angle pair (k 0) through AngleLerp, the rest by the file's type; the bank tag the current's. Without the earlier record, or
    // where a field's type differs between the two groups, the current's. (A file whose ShadowParam order differs from default's,
    // the eight with Shadow Map Mode and the (forPSM) rows, fills the record by position in both ends alike: INFERRED harmless.)
    float w{}; std::shared_ptr<const std::vector<Impl::Bound>> from;
    if(!impl_->FadeOf(asked,w,&from) || !from) return true;
    alignas(16) uint8_t e[0x94]; Used before{};
    { const FadeView v(impl_,asked,from.get()); if(!own(e,before)) return true; }
    for(size_t k=0;k<22;++k) { const auto& p=now.grp->params[k]; const auto& q=before.grp->params[k];
        if(p.type!=q.type || map[k].byte || k==1 || k==8 || k==21) continue;
        uint32_t a[4]{},v[4]{},o[4]{}; std::memcpy(a,e+map[k].at,4u*map[k].n); std::memcpy(v,r+map[k].at,4u*map[k].n);
        FadeValue("ShadowParam",k==0?"Angle":"",p.type,map[k].n,a,v,w,o); std::memcpy(r+map[k].at,o,4u*map[k].n); }
    return true;
}
// I-0140 tranche 7: 1401DAF40 from its listing (the decompiler's C drops the +0x8C kept on the whole-record branches).
void BlendShadowRecord(uint8_t* out,const uint8_t* a,const uint8_t* b,float t,float eps) {
    uint32_t colour; std::memcpy(&colour,out+0x8C,4);
    const auto keys=[&](const uint8_t* s) { std::memcpy(out+0x24,s+0x24,4); std::memcpy(out+0x20,s+0x20,4); std::memcpy(out+0x2C,s+0x24,4); std::memcpy(out+0x28,s+0x20,4); };
    if(eps>=std::fabs(t)) { std::memcpy(out,a,0x94); keys(a); }   // COMISS/JC: a NaN weight lerps, as the listing does
    else if(eps>=std::fabs(t-1.0f)) { std::memcpy(out,b,0x94); keys(b); }
    else {
        const auto lerp=[&](uint32_t at) { float x,y; std::memcpy(&x,a+at,4); std::memcpy(&y,b+at,4); const float v=(y-x)*t+x; std::memcpy(out+at,&v,4); };
        const auto copy=[&](uint32_t at,size_t n) { std::memcpy(out+at,b+at,n); };
        lerp(0x00); lerp(0x04);
        std::memcpy(out+0x24,a+0x24,4); std::memcpy(out+0x20,a+0x20,4); std::memcpy(out+0x2C,b+0x24,4); std::memcpy(out+0x28,b+0x20,4);
        for(const uint32_t at:{0x34u,0x38u,0x3Cu,0x40u,0x44u,0x48u}) lerp(at);
        copy(0x50,4); copy(0x70,1); lerp(0x54); lerp(0x64); lerp(0x6C); copy(0x71,1);
        for(const uint32_t at:{0x74u,0x78u,0x7Cu,0x80u}) lerp(at);
        copy(0x84,4); lerp(0x88); copy(0x90,4); }
    std::memcpy(out+0x30,&t,4); std::memcpy(out+0x8C,&colour,4);
    float angle[2]; std::memcpy(angle,out,8); float d[4]; AngleDirection(angle[0],angle[1],d); std::memcpy(out+0x10,d,16);
}
namespace {
// 1400F8FD0(a, b, t, lo, hi): an angle's interpolation on the period [lo, hi] (the listing's comparisons, float as it computes).
float AngleLerp(float a,float b,float t,float lo,float hi) {
    const float p=static_cast<float>(std::fabs(static_cast<double>(lo-hi)));
    if(lo>a) a+=p;
    if(lo>b) b+=p;
    const float d=static_cast<float>(std::fabs(static_cast<double>(a-b)));
    if(d<p) return (b-a)*t+a;
    if(b>a) return ((b-p)-a)*t+a;
    return ((p+b)-a)*t+a;
}
}
ShadowSunSource ShadowSunDirection(const Banks& banks,const uint8_t* R,const uint32_t view_keys[4],float out[4]) {
    uint32_t a[2],b[2]; std::memcpy(a,R+0x20,8); std::memcpy(b,R+0x28,8);
    // I-0141: a cutscene's record without a light set takes the cut's own where the bank holds it with its light 0 on (the
    // header's note); `ra` and `rb` are then built already.
    alignas(16) uint8_t ra[0x2A0]{},rb[0x2A0]{};
    // The final review's L1: each view key is tried in turn, and the first whose set the bank holds with its light 0 on answers
    // (a fade between two cuts whose first set is off takes the second's); a second key that is the first again is not tried
    // twice (the fixes' step review: the same set built again each frame where its light 0 is off).
    const auto cut=[&](uint32_t key[2],const uint32_t* first,const uint32_t* second,uint8_t* built) {
        for(const uint32_t* view:{first,second}) { uint32_t set[2];
            if(view==second && second[0]==first[0] && second[1]==first[1]) break;
            if(CutsceneShadowLightSet(key,view,set) && banks.Sun(set[0],set[1],built,sizeof ra) && SunLightOn(built)) { key[0]=set[0]; key[1]=set[1]; return true; } }
        return false; };
    const bool cut_a=cut(a,view_keys,view_keys+2,ra),cut_b=cut(b,view_keys+2,view_keys,rb);
    if(a[0]==0xFFFFFFFFu || a[1]==0xFFFFFFFFu) { std::memcpy(out,R+0x10,16); return ShadowSunSource::Angle; }
    if(!cut_a && !banks.Sun(a[0],a[1],ra,sizeof ra)) return ShadowSunSource::Missing;
    if(b[0]==0xFFFFFFFFu || b[1]==0xFFFFFFFFu) { std::memcpy(out,ra,16); return cut_a?ShadowSunSource::CutLightSet:ShadowSunSource::LightSet; }
    if(!cut_b) {
        if(b[0]==a[0] && b[1]==a[1]) std::memcpy(rb,ra,sizeof rb);   // the same light set (play): one build
        else if(!banks.Sun(b[0],b[1],rb,sizeof rb)) return ShadowSunSource::Missing; }
    const auto f=[](const uint8_t* p,uint32_t at) { float v; std::memcpy(&v,p+at,4); return v; };
    float t,pi_lo,pi_hi,half_lo,half_hi; std::memcpy(&t,R+0x30,4);
    if(!Rdata(0x3D7F0D4,&half_lo,4) || !Rdata(0x3D7EE40,&half_hi,4) || !Rdata(0x3D7F0EC,&pi_lo,4) || !Rdata(0x3D7EEC0,&pi_hi,4)) return ShadowSunSource::Missing;
    const float theta=AngleLerp(f(ra,0x60)+f(ra,0x30),f(rb,0x60)+f(rb,0x30),t,half_lo,half_hi);
    const float phi=AngleLerp(f(ra,0x64)+f(ra,0x34),f(rb,0x64)+f(rb,0x34),t,pi_lo,pi_hi);
    const DirectX::XMVECTOR q=DirectX::XMQuaternionSlerpV(DirectX::XMLoadFloat4A(reinterpret_cast<const DirectX::XMFLOAT4A*>(ra+0x290)),
        DirectX::XMLoadFloat4A(reinterpret_cast<const DirectX::XMFLOAT4A*>(rb+0x290)),DirectX::XMVectorReplicate(t));
    alignas(16) float qq[4]; DirectX::XMStoreFloat4A(reinterpret_cast<DirectX::XMFLOAT4A*>(qq),q);
    float d[4]; AngleDirection(theta,phi,d); alignas(16) uint8_t turned[0x50]{}; Rotate(d,qq,turned); std::memcpy(out,turned,16);
    return cut_a || cut_b?ShadowSunSource::CutBlend:ShadowSunSource::Blend;
}
namespace {
// The parameter `param` of the group `group` for {area, id} (1186, W5d GL-2): the area's GPARAM, else mapbase.gparam's group when
// a map area leaves it empty; a key the group lacks falls back to {1, 0} (default.gparam), as the engine's bank lookups do. Group
// and parameter short names are compared without their padding spaces (the files pad some, e.g. "Glow Filter").
// I-0138 tranche 6 (STUDY-UNREAD-SETTINGS item 2, read in the executable): mapbase answers map areas only. The engine binds
// MapBase's records under the area of each loaded map (GparamAddExtraId_140D26AB0: MapIdToAreaCode of the map id, at most 199,990,000),
// never under a cutscene file's area (200,000,000 and above, GetGparamArea_140D27790's kind 0xB), and its lookup
// (GparamBankLookup_140032770) knows the exact key, then {1, 0}, then the bank's built-in record: a cutscene key whose own file
// lacks the group reads default's {1, 0}, where this fell to mapbase for any area before.
bool MapBaseAnswers(uint32_t area) noexcept {return area>=100000000u && area<200000000u;}
const ds3content::GparamParam* FindGroupParam(const Banks::Impl& impl,uint32_t area,uint32_t id,const char* group,const char* param,size_t& index,std::string* source);
}
bool Banks::GroupFloat(uint32_t area,uint32_t id,const char* group,const char* param,float& out,std::string* source) const {
    uint32_t word{}; if(!GroupWord(area,id,group,param,word,source)) return false; out=std::bit_cast<float>(word); return true;   // p->Float(i)'s word
}
bool Banks::GroupWord(uint32_t area,uint32_t id,const char* group,const char* param,uint32_t& out,std::string* source) const {
    size_t i{}; const auto* p=FindGroupParam(*impl_,area,id,group,param,i,source); if(!p) return false; out=p->values[i][0];
    // I-0141 tranche 23: during a fade of the area, the value between the earlier layers' and the current's (FadeValue; the current's
    // alone where the earlier layers give none or one of another type).
    float w{}; std::shared_ptr<const std::vector<Impl::Bound>> from;
    if(impl_->FadeOf(area,w,&from) && from) { size_t j{}; const ds3content::GparamParam* q{};
        { const FadeView v(impl_,area,from.get()); q=FindGroupParam(*impl_,area,id,group,param,j,nullptr); }
        if(q && q->type==p->type) { const unsigned n=std::min(4u,p->Components()); uint32_t a[4]{},cur[4]{},o[4]{};   // a vector's lanes all (its equality test)
            for(unsigned c=0;c<n;++c) { a[c]=q->values[j][c]; cur[c]=p->values[i][c]; }
            FadeValue(group,Trimmed(p->name2),p->type,n,a,cur,w,o); out=o[0]; } }
    return true;
}
namespace {
const ds3content::GparamParam* FindGroupParam(const Banks::Impl& impl,uint32_t area,uint32_t id,const char* group,const char* param,size_t& index,std::string* source) {
    if(area==0xFFFFFFFFu && id==0xFFFFFFFFu) { area=1; id=0; }
    const auto trimmed=[](const std::string& s){ const auto a=s.find_first_not_of(' '),b=s.find_last_not_of(' '); return a==std::string::npos?std::string{}:s.substr(a,b-a+1); };
    const auto find=[&](const ds3content::Gparam* g)->const ds3content::GparamGroup* {
        if(!g) return nullptr; for(const auto& x:g->groups) if(trimmed(x.name2)==group) return &x; return nullptr; };
    const auto empty=[](const ds3content::GparamGroup* x){ if(!x) return true; for(const auto& p:x->params) if(!p.values.empty()) return false; return true; };
    const auto lookup=[&](const ds3content::Gparam* g,const char* name,uint32_t key_area,uint32_t key)->const ds3content::GparamParam* {
        const ds3content::GparamGroup* grp=find(g); std::string file=name;
        if(empty(grp) && MapBaseAnswers(key_area)) { const auto it=impl.files.find("mapbase"); grp=it==impl.files.end()?nullptr:find(it->second.get()); file="mapbase"; }
        if(empty(grp)) return nullptr;
        for(const auto& p:grp->params) if(trimmed(p.name2)==param) { const int i=p.IndexOf(static_cast<int32_t>(key)); if(i<0) return nullptr;
            index=static_cast<size_t>(i); if(source) *source=file; return &p; }
        return nullptr; };
    const auto* own=impl.File(area);
    if(const auto* p=lookup(own,impl.Label(area,own).c_str(),area,id)) return p;
    return lookup(impl.File(1),"default",1,0);   // the key {1, 0}: default alone (the step review's L3: mapbase answered here too for a map area)
}
}
Banks::GlowRecord Banks::GlowAt(uint32_t area,uint32_t id,std::string* source) const {
    static const char* const names[6]={"Enable","BlurSampleScale","Blur RgbScale","Blur AlphaScale","New Glow Enable","NewGlow Final ColorScale"};
    uint32_t w[6]{}; std::string file;
    for(int i=0;i<6;++i) if(!GroupWord(area,id,"Glow Filter",names[i],w[i],&file)) { if(source) *source="builtin"; return GlowRecord{}; }
    GlowRecord r; r.enable=w[0]; std::memcpy(&r.blur_scale,&w[1],4); std::memcpy(&r.rgb,&w[2],4); std::memcpy(&r.alpha,&w[3],4); r.new_glow=w[4];
    std::memcpy(&r.final_scale,&w[5],4); if(source) *source=file; return r;
}
Banks::GlowRecord Banks::Glow(uint32_t a_area,uint32_t a_id,uint32_t b_area,uint32_t b_id,float t,float eps,std::string* sources) const {
    std::string sa,sb; const GlowRecord a=GlowAt(a_area,a_id,&sa), b=GlowAt(b_area,b_id,&sb);
    if(sources) *sources=sa+","+sb;
    return BlendGlow(a,b,t,eps);
}
Banks::GlowRecord Banks::BlendGlow(const GlowRecord& a,const GlowRecord& b,float t,float eps) {
    if(std::fabs(t)<=eps) return a;
    if(std::fabs(t-1.0f)<=eps) return b;
    GlowRecord r=b; const auto lerp=[&](float x,float y){ return (y-x)*t+x; };
    r.blur_scale=lerp(a.blur_scale,b.blur_scale); r.rgb=lerp(a.rgb,b.rgb); r.alpha=lerp(a.alpha,b.alpha); r.final_scale=lerp(a.final_scale,b.final_scale);
    return r;
}
bool Banks::GroupEntries(uint32_t area,uint32_t id,const char* group,std::vector<BankEntry>& out,std::string* source) const {
    if(!OwnEntries(area,id,group,out,source)) return false;
    // I-0141 tranche 23: during a fade of the area, each entry between the earlier layers' entry and the current's (FadeValue), the
    // current entries' list and source; an entry is matched at its place when the name agrees there, else by the same occurrence of
    // its name, and stays the current's where the earlier layers hold none, one of another type or size, or either row lacks the id.
    float w{}; std::shared_ptr<const std::vector<Impl::Bound>> from;
    if(!impl_->FadeOf(area,w,&from) || !from) return true;
    std::vector<BankEntry> earlier; bool held{};
    { const FadeView v(impl_,area,from.get()); held=OwnEntries(area,id,group,earlier,nullptr); }
    if(!held) return true;
    for(size_t k=0;k<out.size();++k) { auto& e=out[k];
        const BankEntry* f=k<earlier.size() && earlier[k].name==e.name?&earlier[k]:nullptr;
        if(!f) { size_t nth{}; for(size_t j=0;j<k;++j) if(out[j].name==e.name) ++nth;   // a repeated name: its same occurrence
            for(const auto& x:earlier) if(x.name==e.name && nth--==0) { f=&x; break; } }
        if(!f || !f->present || !e.present || f->type!=e.type || f->words!=e.words) continue;   // a row without the id: the current's
        std::array<uint32_t,4> o{}; FadeValue(group,e.name,e.type,e.words,f->raw.data(),e.raw.data(),w,o.data()); e.raw=o; }
    return true;
}
bool Banks::OwnEntries(uint32_t area,uint32_t id,const char* group,std::vector<BankEntry>& out,std::string* source) const {
    if(area==0xFFFFFFFFu && id==0xFFFFFFFFu) { area=1; id=0; }
    const auto trimmed=[](const std::string& s){ const auto a=s.find_first_not_of(' '),b=s.find_last_not_of(' '); return a==std::string::npos?std::string{}:s.substr(a,b-a+1); };
    const auto find=[&](const ds3content::Gparam* g)->const ds3content::GparamGroup* {
        if(!g) return nullptr; for(const auto& x:g->groups) if(trimmed(x.name2)==group) return &x; return nullptr; };
    const auto empty=[](const ds3content::GparamGroup* x){ if(!x) return true; for(const auto& p:x->params) if(!p.values.empty()) return false; return true; };
    const auto collect=[&](const ds3content::Gparam* g,const char* name,uint32_t key_area,uint32_t key)->bool {
        const ds3content::GparamGroup* grp=find(g); std::string file=name;
        // W10 YB-3 (Native1595 against Reference1594 at m50): the bound file of the area may be a variant layer without the group
        // (m50_00_0200 has no Tone Map, Bloom or Setting Info; the original's records are m50_00_0000's): the area's own file
        // m<aa>_<bb>_<cccc> then (INFERRED rule for several kind-7 layers, witnessed at m50 only; AreaFileName's name).
        if(empty(grp) && key_area>1) { const std::string n=AreaFileName(key_area);
            const auto it=impl_->files.find(n); if(it!=impl_->files.end() && it->second.get()!=g) { grp=find(it->second.get()); file=n; } }
        if(empty(grp) && MapBaseAnswers(key_area)) { const auto it=impl_->files.find("mapbase"); grp=it==impl_->files.end()?nullptr:find(it->second.get()); file="mapbase"; }   // map areas only (FindGroupParam's note)
        if(empty(grp) || grp->params.empty() || grp->params[0].IndexOf(static_cast<int32_t>(key))<0) return false;
        std::vector<BankEntry> entries;
        for(const auto& p:grp->params) { BankEntry e; e.name=trimmed(p.name2); e.type=p.type; e.words=static_cast<uint8_t>(p.Components());
            const int i=p.IndexOf(static_cast<int32_t>(key)); if(i>=0) { e.raw=p.values[static_cast<size_t>(i)]; e.present=true; } entries.push_back(std::move(e)); }
        out=std::move(entries); if(source) *source=file; return true; };
    // Fable review (m33, Native2174 against Reference2173: the native tone map exposed at default.gparam's 1.52 and never graded
    // while the original took m33_00_0010's 2.3 and its grading LUT): the area's live kind-7 layers come first, latest first,
    // the first layer whose group holds the value ID (the rule the light, fog and sun lookups already apply through Layer();
    // the bound file of the area is MapOverride_0000 at m33 and the area's own m33_00_0000 is not loaded there, so the
    // collect below fell through to default). Then the bound file, the area's own file, mapbase, default as before.
    const auto layered=[&]()->bool {
        // I-0141 tranche 23: inside a fade's view, the earlier layers (Layer's rule).
        std::vector<std::string> names; if(const auto* earlier=impl_->Viewed(area)) { for(const auto& b:*earlier) names.push_back(b.file); }
        else { std::lock_guard<std::mutex> l(impl_->variant_m); const auto it=impl_->layers.find(area); if(it!=impl_->layers.end()) for(const auto& b:it->second) names.push_back(b.file); }
        for(const auto& name:names) { const auto f=impl_->files.find(name); if(f==impl_->files.end()) continue;
            const ds3content::GparamGroup* grp=find(f->second.get());
            if(empty(grp) || grp->params.empty() || grp->params[0].IndexOf(static_cast<int32_t>(id))<0) continue;
            std::vector<BankEntry> entries;
            for(const auto& p:grp->params) { BankEntry e; e.name=trimmed(p.name2); e.type=p.type; e.words=static_cast<uint8_t>(p.Components());
                const int i=p.IndexOf(static_cast<int32_t>(id)); if(i>=0) { e.raw=p.values[static_cast<size_t>(i)]; e.present=true; } entries.push_back(std::move(e)); }
            // The source names the layer file when it is map-named (mAA_BB_NNNN: the grading resolver builds the LUT name from it);
            // any other layer name keeps "area", the former label (pre-merge review).
            const bool map_named=name.size()==11 && name[0]=='m' && name[3]=='_' && name[6]=='_' &&
                std::all_of(name.begin()+1,name.begin()+3,[](char c){return c>='0' && c<='9';}) && std::all_of(name.begin()+4,name.begin()+6,[](char c){return c>='0' && c<='9';}) &&
                std::all_of(name.begin()+7,name.end(),[](char c){return c>='0' && c<='9';});
            out=std::move(entries); if(source) *source=map_named?name:std::string("area"); return true; }
        return false; };
    const auto* own=impl_->File(area);
    return (area>1 && layered()) || collect(own,impl_->Label(area,own).c_str(),area,id) || collect(impl_->File(1),"default",1,0);   // {1, 0}: default alone
}
bool Banks::DominantAt(uint32_t area,uint32_t id,std::array<float,11>& out,std::string* source) const {
    static const char* const names[11]={"Dominant Sharpness Min Value","Dominant Sharpness Offset","Dominant Sharpness Exponant",
        "Dominant Color Scale","Dominant R Scale","Dominant G Scale","Dominant B Scale","SH Color Scale","SH R Scale","SH G Scale","SH B Scale"};
    std::vector<BankEntry> entries; std::string file;
    if(!GroupEntries(area,id,"Dominant Light",entries,&file)) return false;
    std::array<float,11> row=out;   // the final review's M2: a parameter the group lacks keeps the caller's row (the built-in record's)
    bool any{};
    for(size_t i=0;i<11;++i) {
        // I-0141 (STUDY-UNREAD-SETTINGS item 10; the reviews): a parameter under a name of none of the eleven (m36_00_0002's fourth of
        // four, « Dominant RGB Scale », where every other file says « Dominant Color Scale ») is taken at its own position: the engine
        // reads a group's parameters through its registry indices, not by name (INFERRED: the files share one order); a parameter the
        // group lacks keeps the caller's row (the built-in record's).
        auto found=std::find_if(entries.begin(),entries.end(),[&](const BankEntry& e){ return e.name==names[i]; });
        if(found==entries.end() && i<entries.size() && std::none_of(std::begin(names),std::end(names),[&](const char* n){ return entries[i].name==n; }))
            found=entries.begin()+static_cast<std::ptrdiff_t>(i);   // a name of none of the eleven, at this parameter's place
        if(found==entries.end()) continue;
        if(found->type!=9 || found->words!=1) { if(found->name!=names[i]) continue; return false; }   // a positional pick of another kind: skipped
        std::memcpy(&row[i],&found->raw[0],4); any=true; }
    if(!any) return false;
    out=row; if(source) *source=file; return true;
}
bool Banks::SnowAt(uint32_t area,uint32_t id,std::array<float,16>& out,std::string* source,int32_t* texture_id) const {
    static const struct { const char* name; uint8_t at, words; } fields[]={{"Angle",0,2},{"Snow Tiling Scale",2,2},{"Scene Snow Rate",4,1},
        {"Sparkle Tiling Scale",5,2},{"Ground Plane Height",7,1},{"Snow Plane Height",8,1},{"Normal Scale",9,1},{"Albedo Scale",10,4},
        {"Reflectance",14,1},{"Normal Blur Radius",15,1}};
    std::vector<BankEntry> entries; std::string file;
    if(!GroupEntries(area,id,"Snow",entries,&file)) return false;
    std::array<float,16> row{};
    for(const auto& f:fields) {
        const auto found=std::find_if(entries.begin(),entries.end(),[&](const BankEntry& e){ return e.name==f.name; });
        if(found==entries.end() || found->words!=f.words) return false;
        std::memcpy(&row[f.at],found->raw.data(),4u*f.words); }
    out=row; if(source) *source=file;
    if(texture_id){ const auto found=std::find_if(entries.begin(),entries.end(),[](const BankEntry& e){ return e.name=="Texture:ID"; });
        *texture_id=0; if(found!=entries.end() && found->words==1) *texture_id=found->type==9?static_cast<int32_t>(std::bit_cast<float>(found->raw[0])):static_cast<int32_t>(found->raw[0]); } return true;   // type 9: a float (step review L3)
}
bool Banks::SnowModelAt(uint32_t area,uint32_t id,float& rate,std::string* source) const {
    std::vector<BankEntry> entries; std::string file;
    if(!GroupEntries(area,id,"Snow Model",entries,&file)) return false;
    const auto found=std::find_if(entries.begin(),entries.end(),[](const BankEntry& e){ return e.name=="Snow Rate"; });
    if(found==entries.end() || found->words!=1) return false;
    std::memcpy(&rate,found->raw.data(),4); if(source) *source=file; return true;
}
std::vector<uint32_t> Banks::EnvIds(uint32_t area) const {
    std::vector<uint32_t> ids; char n[32]; std::snprintf(n,sizeof n,"m%02u_%02u_00_00",(area/1000000u)%100u,(area/10000u)%100u);
    const auto m=impl_->boxes.find(n); if(m!=impl_->boxes.end()) for(const auto& [id,b]:m->second) ids.push_back(id); return ids;
}
// I-0141 tranche 23: Setting Info's BG Color is declared without the fade (FadesAtOnce): a fade answers the current layers at once.
bool Banks::BackgroundColor(uint32_t area,uint32_t id,float out[4]) const {
    if(area==0xFFFFFFFFu && id==0xFFFFFFFFu) { area=1; id=0; }
    const ds3content::Gparam* g=impl_->File(area); if(!g) return false;
    const auto trimmed=[](const std::string& s){ const auto a=s.find_first_not_of(' '),b=s.find_last_not_of(' '); return a==std::string::npos?std::string{}:s.substr(a,b-a+1); };
    for(const auto& grp:g->groups) { if(trimmed(grp.name2)!="Setting Info") continue;
        for(const auto& p:grp.params) { if(trimmed(p.name2)!="BG Color" || p.Components()<4) continue;
            const int i=p.IndexOf(static_cast<int32_t>(id)); if(i<0) return false;
            std::memcpy(out,p.values[static_cast<size_t>(i)].data(),16); return true; } }
    return false;
}
bool Banks::EnvColorScale(uint32_t area,uint32_t id,float& out) const {
    const auto own=[&](uint32_t& word,uint8_t& type)->bool {
        const ds3content::Gparam* g=impl_->File(area); if(!g) return false;
        for(const auto& grp:g->groups) for(const auto& p:grp.params) if(p.name2=="EnvMap:ColorScale") {
            const int i=p.IndexOf(static_cast<int32_t>(id)); if(i<0) return false; word=p.values[static_cast<size_t>(i)][0]; type=p.type; return true; }
        return false; };
    uint32_t v{}; uint8_t type{}; if(!own(v,type)) return false; std::memcpy(&out,&v,4);
    // I-0141 tranche 23: during a fade of the area, between the earlier layers' scale and the current's (Env Map's ColorScale fades).
    float w{}; std::shared_ptr<const std::vector<Impl::Bound>> from;
    if(impl_->FadeOf(area,w,&from) && from) { uint32_t a{}; uint8_t t{}; bool held{};
        { const FadeView view(impl_,area,from.get()); held=own(a,t); }
        if(held && t==type) { uint32_t o{}; FadeValue("Env Map","EnvMap:ColorScale",type,1,&a,&v,w,&o); std::memcpy(&out,&o,4); } }
    return true;
}
// 140113D80 stores the defaults as immediates: +0x2C (qword, low half) at 0x140113DC4, +0x20/+0x24/+0x28 at
// 0x140113DD5 / DDC / DE3.
bool Banks::SssBase(unsigned position,uint32_t out[4]) const {
    if(!position) return Rdata(0x113DD5,&out[0],4) && Rdata(0x113DDC,&out[1],4) && Rdata(0x113DE3,&out[2],4) && Rdata(0x113DC4,&out[3],4);
    const auto it=impl_->sss_base.find(position); if(it==impl_->sss_base.end()) return false;
    std::memcpy(out,it->second.second.data(),16); return true;
}
bool Banks::SssInstance(unsigned base_position,uint32_t strength[3]) const {
    const auto it=impl_->sss_base.find(base_position); if(it==impl_->sss_base.end()) return false;
    const auto g=impl_->gx15.find(it->second.first); if(g==impl_->gx15.end() || g->second.size()!=1) return false;
    std::memcpy(strength,g->second[0].data(),12); return true;
}
void UseBanks(const Banks* banks) { g_banks=banks; }
void UseCamera(const float* camera) { g_camera=camera; }
void UseShadowSetup(uint32_t map_size,uint32_t random_texture) { g_shadow_map=map_size; g_random_texture=random_texture; }
const BankStats& BankCheck() { return g_bank_stats; }
}

namespace ds3light { void UseReader(Reader reader) { g_reader=reader; } void UseSharedReader(SharedReader reader) { g_shared_reader=reader; } }
namespace ds3light { void UseListingReader(ListingReader reader) { g_listing_reader=reader; } void UseRdataReader(RdataReader reader) { g_rdata_reader=reader; } }
namespace ds3light { void UseRecordBlends(RecordBlend light,RecordBlend fog) { g_light_blend=light; g_fog_blend=fog; } }
namespace ds3light {
namespace { std::atomic<double(*)()> g_fade_clock{nullptr}; }
double FadeClock() {
    if(const auto clock=g_fade_clock.load(std::memory_order_acquire)) return clock();
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void UseFadeClock(double(*clock)()) { g_fade_clock.store(clock,std::memory_order_release); }
}
