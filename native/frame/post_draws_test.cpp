// W10 GL-1 T13 (W10-NATIVE-PASSES 5): our generic 2D post draw against the original's own draws. The Reference run
// i66-reference-1648 (m37, yebis-draws-capture.flag) dumped every library draw of frame 1200: its inputs t0..t5, its target after the
// draw, its vertices, its samplers (the "state" row) and, in the reference_yebis_draw row just before, its technique and `$Globals`.
// Each requested draw is replayed here with the programmes found in the executable by their pins (yebis_pins.h), the captured block,
// inputs and samplers, and its output is compared with the original's, texel for texel. This fixes the draw's mechanics (vertices,
// samplers, render state, viewport) independently of the constants, which GL-2 computes natively. Arguments: warp|hardware, the
// executable, the capture directory, then the draws (comma-separated; default the glare family 14-36 and 44). Skipped with a message
// without the executable or the capture. Also: no debug-layer message of severity WARNING or worse. LS-1: a draw with a partial
// write mask (the blend row, Reference1660 on) starts from the original's output with the written channels zeroed; the scene depth
// (format 21) is replayed as its R32_FLOAT word (Alpha_R, draw 10, at m37, m31, m50, m45).
// I-0138 tranche 2: with the profile warp-ours or hardware-ours each draw is replayed with our own programme for its technique
// (post_glare_programs.h) instead of the library's, the captured block, inputs and samplers the same; a draw whose technique is not
// one of our glare family's fails the run. Each draw is then replayed a second time with the pinned original's programmes on the
// same device, and the two outputs must be equal byte for byte, on any adapter (the step review's L2).
// I-0138 tranche 8b: our programmes read our block (yebis::glare_block_registers), the captured block gathered into it; the
// library's read the captured block as it is. Before the replay, whatever the arguments: every programme of our family's
// PostGlareBlock (D3DReflect) ends within yebis::glare_block_bytes and holds each field at the slot of the library register
// it stands for (the step review's L2).
// I-0138 tranche 8f: GlareShape 3's draws (i138-t8e-oracle-240, -900): a draw of four indices is the ghost's strip (PostDraws' own
// vertices, checked against the captured ones); the ghost's trim (slot 4, A8_UNORM 64x64), which the capture does not dump, is
// replayed from yebis::TrimCircle, so an exact ghost draw also checks the trim's texels. The samplers are read up to eight (the
// captures hold eight; six before, which left MadLumGammaT7's s6 point-sampled).
// I-0138 tranche 8i: the profile's « -lit » (with « -ours »): inputs uniformly zero in RGB given synthetic light (LitInput, below).
#include "glare_library_block.h"
#include "../renderer/post_draws.h"
#include "../renderer/post_glare_programs.h"
#include "../world/yebis_pins.h"
#include "../world/yebis_shafts.h"
#include "../world/yebis_glare.h"
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <algorithm>
#include <cmath>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
std::vector<uint8_t> FileBytes(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
ComPtr<ID3D12Resource> Texture(ID3D12Device* device,DXGI_FORMAT format,UINT w,UINT h,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,UINT16 mips=1){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=w;desc.Height=h;desc.DepthOrArraySize=1;desc.MipLevels=mips;desc.SampleDesc.Count=1;desc.Format=format;desc.Flags=flags;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control texture");return result;}
// Our cbuffer's fields and the library registers they stand for (shaders/post_glare.hlsl).
void BlockLayout(){
    static const std::map<std::string,uint32_t> library{{"depth_decode",0},{"screen_scale",5},{"shaft_grain",10},{"mb_planes",15},{"shaft_mask",15},
        {"dof_factor",17},{"dof_back",19},{"vertex_offsets",34},{"ghost_offsets",42},{"pixel_offsets",50},{"scalers",146},{"weights",154},{"mb_decode",186},{"offset0",186},
        {"mb_step",187},{"offset1",187},{"mb_range",188},{"offset2",188},{"mb_test",189},{"mb_rect",190},{"gamma",202},{"remap",209},{"mb_previous",224}};
    size_t blocks=0;
    for(const auto& [technique,blobs]:*PostGlarePrograms())for(const auto* code:{&blobs.vertex,&blobs.pixel}){
        ComPtr<ID3D12ShaderReflection> reflection;Check(D3DReflect(code->data(),code->size(),IID_PPV_ARGS(&reflection)),"glare programme reflection");
        D3D12_SHADER_DESC shader{};Check(reflection->GetDesc(&shader),"glare programme description");
        for(UINT b=0;b<shader.ConstantBuffers;++b){auto* block=reflection->GetConstantBufferByIndex(b);D3D12_SHADER_BUFFER_DESC desc{};Check(block->GetDesc(&desc),"glare cbuffer");
            const std::string where=technique+" "+(code==&blobs.vertex?"vertex":"pixel");
            Require(std::string(desc.Name)=="PostGlareBlock",("an unknown cbuffer in "+where).c_str());++blocks;
            Require(desc.Size<=yebis::glare_block_bytes,("PostGlareBlock past our block in "+where).c_str());
            for(UINT v=0;v<desc.Variables;++v){D3D12_SHADER_VARIABLE_DESC var{};Check(block->GetVariableByIndex(v)->GetDesc(&var),"glare cbuffer field");
                const auto found=library.find(var.Name);Require(found!=library.end(),("a field without a library register: "+std::string(var.Name)+" in "+where).c_str());
                Require(yebis::glare_block_slots[found->second]>=0 && var.StartOffset==16u*uint32_t(yebis::glare_block_slots[found->second]) &&
                    var.StartOffset+var.Size<=yebis::glare_block_bytes,("a field off its slot: "+std::string(var.Name)+" in "+where).c_str());
                // An array's every element at the slot of the register after its predecessor's (the second step review's L1).
                const size_t first=size_t(yebis::glare_block_slots[found->second]);
                for(size_t i=1;i<(var.Size+15)/16;++i)Require(first+i<yebis::glare_block_registers.size() && yebis::glare_block_registers[first+i]==found->second+i,
                    ("an array element off its register: "+std::string(var.Name)+"["+std::to_string(i)+"] in "+where).c_str());}}}
    Require(blocks>0,"no PostGlareBlock reflected");
    std::printf("PASS post draws: our family's %zu PostGlareBlock cbuffers within %u bytes, each field at its library register's slot\n",blocks,yebis::glare_block_bytes);}
// The view format of a captured resource format (the library's targets are typeless where it reinterprets them).
DXGI_FORMAT ViewFormat(uint32_t f){
    // 21 (R32_FLOAT_X8X24, the scene depth Alpha_R reads, captured 8 bytes per texel): its depth word, replayed as R32_FLOAT.
    switch(f){case 21:case 41:return DXGI_FORMAT_R32_FLOAT;case 9:case 10:return DXGI_FORMAT_R16G16B16A16_FLOAT;case 26:return DXGI_FORMAT_R11G11B10_FLOAT;case 27:case 28:return DXGI_FORMAT_R8G8B8A8_UNORM;case 65:return DXGI_FORMAT_A8_UNORM;
    default:throw std::runtime_error("capture format outside the replay table");}}
uint32_t TexelBytes(DXGI_FORMAT f){return f==DXGI_FORMAT_R16G16B16A16_FLOAT?8u:f==DXGI_FORMAT_A8_UNORM?1u:4u;}
// I-0138 tranche 8i (c7e0ac515's step review, L3): a captured output's content by its view format: a texel with a channel not zero
// is content, a zero's sign ignored (RGBA16F, R32F); its RGB alone apart from alpha (RGBA16F and RGBA8: the fourth channel left out;
// A8: alpha only, never RGB; R11G11B10 and R32F: every channel is colour). Before, every byte counted, so an RGBA8 or A8 alpha passed
// as RGB and an R32F -0.0 as content.
void Content(const std::vector<uint8_t>& bytes,DXGI_FORMAT f,bool& any,bool& rgb){
    any=false;rgb=false;const uint32_t texel=TexelBytes(f);
    for(size_t t=0;t+texel<=bytes.size() && !(any && rgb);t+=texel){const uint8_t* x=bytes.data()+t;
        switch(f){
        case DXGI_FORMAT_R16G16B16A16_FLOAT:for(uint32_t c=0;c<4;++c)if((x[2*c]|(x[2*c+1]&0x7F))!=0){any=true;if(c<3)rgb=true;}break;
        case DXGI_FORMAT_R32_FLOAT:if((x[0]|x[1]|x[2]|(x[3]&0x7F))!=0)any=rgb=true;break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:for(uint32_t c=0;c<4;++c)if(x[c]){any=true;if(c<3)rgb=true;}break;
        case DXGI_FORMAT_A8_UNORM:if(x[0])any=true;break;
        case DXGI_FORMAT_R11G11B10_FLOAT:if(x[0]|x[1]|x[2]|x[3])any=rgb=true;break;
        default:throw std::runtime_error("a content count outside the replay table's formats");}}}
// The integers of a row field: "name":[a,b,...] after position at.
std::vector<double> Numbers(const std::string& row,const std::string& name,size_t at=0){
    std::vector<double> out;const auto key=row.find("\""+name+"\":[",at);if(key==std::string::npos)return out;
    size_t p=key+name.size()+4;while(p<row.size() && row[p]!=']'){char* end{};const double v=std::strtod(row.c_str()+p,&end);if(end==row.c_str()+p){++p;continue;}
        out.push_back(v);p=size_t(end-row.c_str());}
    return out;}
std::string Text(const std::string& row,const std::string& name,size_t at=0){
    const auto key=row.find("\""+name+"\":\"",at);if(key==std::string::npos)return {};const auto from=key+name.size()+4;
    return row.substr(from,row.find('"',from)-from);}
struct CapturedDraw {std::string draw_row,state_row,blend_row,geometry_row;};
// The draw rows and state rows of the captured frame, by draw index.
std::map<uint32_t,CapturedDraw> CapturedDraws(const std::string& trace){
    std::ifstream file(trace);std::string line,last_draw;std::map<uint32_t,CapturedDraw> out;int current=-1;
    while(std::getline(file,line)){
        if(line.find("\"event\":\"reference_yebis_draw\"")!=std::string::npos){last_draw=line;continue;}
        if(line.find("\"event\":\"reference_yebis_draw_capture\"")!=std::string::npos){
            current=int(std::stoul(line.substr(line.find("\"draw\":")+7)));out[uint32_t(current)].draw_row=last_draw;continue;}
        if(current>=0 && line.find("\"what\":\"state\"")!=std::string::npos && out[uint32_t(current)].state_row.empty())out[uint32_t(current)].state_row=line;
        if(current>=0 && line.find("\"what\":\"blend\"")!=std::string::npos && out[uint32_t(current)].blend_row.empty())out[uint32_t(current)].blend_row=line;
        if(current>=0 && line.find("\"what\":\"geometry\"")!=std::string::npos && out[uint32_t(current)].geometry_row.empty())out[uint32_t(current)].geometry_row=line;}
    return out;}
// The programme containers of a technique, found in the executable file by their pins.
bool Programmes(const std::vector<uint8_t>& exe,const char* technique,std::vector<uint8_t>& vertex,std::vector<uint8_t>& pixel){
    const auto* pin=yebis::FindTechniquePin(technique);if(!pin)return false;vertex.clear();pixel.clear();
    for(size_t at=0;at+32<=exe.size() && (vertex.empty() || pixel.empty());at+=4){
        if(std::memcmp(exe.data()+at,"DXBC",4))continue;uint32_t size{};std::memcpy(&size,exe.data()+at+24,4);
        if(size<32 || size>65536 || at+size>exe.size())continue;char hex[65]{};if(!ds3content::Sha256(exe.data()+at,size,hex))continue;
        if(vertex.empty() && !std::strcmp(hex,pin->vs_sha256))vertex.assign(exe.begin()+at,exe.begin()+at+size);
        if(pixel.empty() && !std::strcmp(hex,pin->ps_sha256))pixel.assign(exe.begin()+at,exe.begin()+at+size);}
    return !vertex.empty() && !pixel.empty();}
// The distance of two texels of the format in units of the last place, channel by channel (positive floats order as integers).
uint32_t Distance(DXGI_FORMAT f,const uint8_t* a,const uint8_t* b){
    uint32_t largest{};
    if(f==DXGI_FORMAT_R16G16B16A16_FLOAT){for(int c=0;c<4;++c){uint16_t x{},y{};std::memcpy(&x,a+2*c,2);std::memcpy(&y,b+2*c,2);
            const auto key=[](uint16_t v){return (v&0x8000)?-int(v&0x7FFF):int(v);};largest=std::max(largest,uint32_t(std::abs(key(x)-key(y))));}}
    else if(f==DXGI_FORMAT_R11G11B10_FLOAT){uint32_t x{},y{};std::memcpy(&x,a,4);std::memcpy(&y,b,4);
        const int fields[3][2]{{0,11},{11,11},{22,10}};for(const auto& k:fields){const int mask=(1<<k[1])-1;
            largest=std::max(largest,uint32_t(std::abs(int((x>>k[0])&mask)-int((y>>k[0])&mask))));}}
    else for(int c=0;c<4;++c)largest=std::max(largest,uint32_t(std::abs(int(a[c])-int(b[c]))));
    return largest;}
// I-0138 tranche 8i (the step review's H1: the shape-3 shafts captures' surfaces are zero from the bright pass on, their scene under
// its threshold): the lit replay, the profile's « -lit » with « -ours ». An input whose RGB is uniformly zero (its alpha kept) takes a
// synthetic RGB: a floor, a hashed noise across the thresholds and a bright spot, each channel apart; RGBA16F and R11G11B10 inputs
// only (the others stay as captured). Our programme and the pinned original's then draw from the same synthetic inputs and the
// captured block and must agree byte for byte, and our shaft draws must give light. The captured outputs are not compared (they were
// drawn from zeros): it checks the programmes on light with the frame's words, not the original's frame.
uint16_t Half(float v){uint32_t b{};std::memcpy(&b,&v,4);const int e=int((b>>23)&0xFF)-127+15;if(!(v>0.0f) || e<=0)return 0;
    if(e>=31)return 0x7BFF;return uint16_t((uint32_t(e)<<10)|((b>>13)&0x3FFu));}
uint32_t SmallFloat(float v,int mantissa){uint32_t b{};std::memcpy(&b,&v,4);const int e=int((b>>23)&0xFF)-127+15;if(!(v>0.0f) || e<=0)return 0;
    if(e>=31)return (30u<<mantissa)|((1u<<mantissa)-1u);return (uint32_t(e)<<mantissa)|((b>>(23-mantissa))&((1u<<mantissa)-1u));}
float LitValue(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t c,uint32_t seed){
    uint32_t k=x*73856093u^y*19349663u^(c+1u)*83492791u^seed*2654435761u;k^=k>>13;k*=0x5BD1E995u;k^=k>>15;
    const float u=(float(x)+0.5f)/float(w),v=(float(y)+0.5f)/float(h),du=u-0.62f,dv=v-0.38f;
    const float spot=6.0f*std::exp(-(du*du+dv*dv)/(2.0f*0.06f*0.06f)),noise=float(k&1023u)/1023.0f;
    return (0.02f+0.9f*noise+spot)*(c==0?1.0f:c==1?0.8f:0.6f);}
// Its step review (H1 there): a texel is lit when a colour channel holds a normal value of its format (RGBA16F and R11G11B10: an
// exponent field neither 0 nor 31, so at least 2^-14 and finite), and an output is lit when at least 1 % of its texels are (a denormal
// step is not light, nor an infinity or a NaN).
uint64_t LitTexels(const std::vector<uint8_t>& bytes,DXGI_FORMAT f){
    uint64_t lit{};const uint32_t texel=TexelBytes(f);
    for(size_t t=0;t+texel<=bytes.size();t+=texel){const uint8_t* x=bytes.data()+t;bool normal=false;
        if(f==DXGI_FORMAT_R16G16B16A16_FLOAT)for(uint32_t c=0;c<3;++c){uint16_t h{};std::memcpy(&h,x+2*c,2);const uint32_t e=(h>>10)&0x1Fu;normal=normal || (e!=0 && e!=0x1Fu);}
        else if(f==DXGI_FORMAT_R11G11B10_FLOAT){uint32_t v{};std::memcpy(&v,x,4);for(const uint32_t at:{6u,17u,27u}){const uint32_t e=(v>>at)&0x1Fu;normal=normal || (e!=0 && e!=0x1Fu);}}
        else throw std::runtime_error("a lit count outside RGBA16F and R11G11B10");
        if(normal)++lit;}
    return lit;}
bool LitInput(std::vector<uint8_t>& bytes,DXGI_FORMAT f,uint32_t w,uint32_t h,uint32_t seed){
    if(f!=DXGI_FORMAT_R16G16B16A16_FLOAT && f!=DXGI_FORMAT_R11G11B10_FLOAT)return false;
    bool any{},rgb{};Content(bytes,f,any,rgb);if(rgb)return false;const uint32_t texel=TexelBytes(f);
    for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x){uint8_t* t=bytes.data()+(size_t(y)*w+x)*texel;
        if(f==DXGI_FORMAT_R16G16B16A16_FLOAT)for(uint32_t c=0;c<3;++c){const uint16_t half=Half(LitValue(x,y,w,h,c,seed));std::memcpy(t+2*c,&half,2);}
        else{const uint32_t packed=SmallFloat(LitValue(x,y,w,h,0,seed),6)|(SmallFloat(LitValue(x,y,w,h,1,seed),6)<<11)|(SmallFloat(LitValue(x,y,w,h,2,seed),5)<<22);std::memcpy(t,&packed,4);}}
    return true;}
}
int main(int argc,char** argv){try{
    Require(argc>=4 && argc<=5,"usage: post_draws_test warp|hardware[-ours[-lit]] <DarkSoulsIII.exe> <capture directory> [draws]");
    std::string profile=argv[1];const bool lit=profile.size()>4 && !profile.compare(profile.size()-4,4,"-lit");if(lit)profile.resize(profile.size()-4);
    const bool ours=profile.size()>5 && !profile.compare(profile.size()-5,5,"-ours");if(ours)profile.resize(profile.size()-5);
    const bool warp=profile=="warp";Require(warp || profile=="hardware","unknown profile");
    Require(!lit || ours,"a lit replay compares our programmes with the pinned original's: -lit needs -ours");
    BlockLayout();
    const std::string dir=argv[3];const auto exe=FileBytes(argv[2]);
    const auto captured=CapturedDraws(dir+"/startup.jsonl");
    if(exe.empty() || captured.empty()){std::printf("PASS post draws %s: skipped, no executable or capture\n",argv[1]);return 0;}
    std::vector<uint32_t> draws;
    if(argc==5){std::stringstream list(argv[4]);std::string item;while(std::getline(list,item,','))draws.push_back(uint32_t(std::stoul(item)));}
    else{for(uint32_t d=14;d<=36;++d)draws.push_back(d);draws.push_back(44);}
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"control WARP adapter");
    else{Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"control hardware adapter");
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);Require(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE),"no hardware adapter");}
    std::string this_adapter;{DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);LARGE_INTEGER driver{};(void)adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice),&driver);
        char text[96]{};std::snprintf(text,sizeof(text),"%04X %04X %016llX",desc.VendorId,desc.DeviceId,static_cast<unsigned long long>(driver.QuadPart));this_adapter=text;}
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"control device");
    const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)),"control queue");
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"control allocator");
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"control fence");const HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);UINT64 fence_value{};
    PostDraws post;std::string reason;Require(post.Open(device.Get(),uint32_t(draws.size()),reason),reason.c_str());
    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,2};ComPtr<ID3D12DescriptorHeap> rtv;Check(device->CreateDescriptorHeap(&rtv_desc,IID_PPV_ARGS(&rtv)),"control rtv heap");
    // The captured block at 0 (the library's layout), ours at library_block_bytes (I-0138 tranche 8b).
    using ds3native::post_test::library_block_bytes;
    const auto constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,library_block_bytes+(yebis::glare_block_bytes+255)/256*256,D3D12_RESOURCE_STATE_GENERIC_READ);
    uint32_t exact{},replayed{},worst{},equal_library{};bool same_everywhere=true;std::string report;
    // I-0138 tranche 8i (the step review's H1): a surface of zeros equal to zeros proves nothing. Every replayed draw's reference surface is
    // counted as with content (a texel with a channel that is not zero, by its format: Content, above) or uniformly zero, and
    // separately by its RGB alone. A zone named « -sun- » (i138-t8i-oracle-sun-*: a capture whose light shafts
    // carry light) must carry content in its shaft draws (the mask LightShaftMask and the radial blurs TexCoordMadT1_S*, not the composites, which
    // carry the glare whatever the shafts hold): all of those uniformly zero in RGB fails the run. A zone without that name
    // (i138-t8i-oracle-shafts-1250: all zero, known) proves its words, techniques, targets, sizes, vertices and draws, not its texels.
    const bool must_carry=!lit && dir.find("-sun-")!=std::string::npos;uint32_t with_content{},with_rgb{},shaft_replayed{},shaft_with_rgb{};
    // The lit replay: inputs given light, our outputs lit (LitTexels); the draws required lit: the radial blurs, the layers' composite
    // MadT5 and the draw after it (the 2x draw, which reads it), each kind at least once. Not the mask: tech_LightShaftMask_SM3 is shape
    // 1's programme (the same pins, 7F09C8F7D24E and D275F69939F1; exact at 1667 and 1669, lit by this rule at 1669: 25 % of its texels,
    // 1667's 0.5 %), and on 1250 the frame's own words leave it unlit (the sun at (-5.79, 2.43), the falloff about 3e-6 at most), so a
    // mask draw is required replayed and its lit share is reported, not required.
    uint32_t lit_inputs{},lit_outputs{},lit_required{},lit_required_lit{},lit_blurs{},lit_composites{},lit_after{},masks{};
    uint64_t mask_lit_texels{},mask_texels{};bool after_composite=false;
    const std::string zone_name=dir.substr(dir.find_last_of("/\\")+1);
    const auto is_blur=[](const std::string& p){return p.rfind("tech_TexCoordMadT1_S",0)==0;};
    const UINT rtv_stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for(size_t n=0;n<draws.size();++n){const uint32_t d=draws[n];const auto found=captured.find(d);
        Require(found!=captured.end() && !found->second.draw_row.empty() && !found->second.state_row.empty(),"a requested draw is not in the capture");
        const auto& row=found->second.draw_row;const auto pass=Text(row,"pass");
        // I-0138 tranche 2: with our programmes the draw is replayed a second time with the pinned original's, on the same device.
        std::vector<uint8_t> vertex,pixel,library_vertex,library_pixel;
        if(ours){GlareBlobs blobs;Require(PostGlareProgram(pass,blobs),"a draw's technique is not one of our glare programmes");vertex=std::move(blobs.vertex);pixel=std::move(blobs.pixel);
            Require(Programmes(exe,pass.c_str(),library_vertex,library_pixel),"a draw's pinned programmes are not in the executable");}
        else Require(Programmes(exe,pass.c_str(),vertex,pixel),"a draw's pinned programmes are not in the executable");
        const auto rt=Numbers(row,"rt0");Require(rt.size()>=4,"a draw's target row");const DXGI_FORMAT target_format=ViewFormat(uint32_t(rt[0]));const UINT W=UINT(rt[2]),H=UINT(rt[3]);
        // The block: the captured bytes, zero beyond (the observer writes the variables the effect set, rounded to 256 bytes).
        std::array<uint8_t,library_block_bytes> block{};{const auto hex=Text(row,"globals");Require(hex.size()%2==0 && hex.size()/2<=block.size(),"a draw's block");
            for(size_t i=0;i<hex.size()/2;++i)block[i]=uint8_t(std::stoul(hex.substr(2*i,2),nullptr,16));}
        {void* memory{};const D3D12_RANGE none{};Check(constants->Map(0,&none,&memory),"constants map");std::memcpy(memory,block.data(),block.size());
            ds3native::post_test::OurBlock(block.data(),static_cast<uint8_t*>(memory)+library_block_bytes);constants->Unmap(0,nullptr);}
        PostSamplers samplers{};{const auto& s=found->second.state_row;const auto at=s.find("\"samplers\":[");Require(at!=std::string::npos,"a draw's samplers");
            std::string flat=s.substr(at+12,s.find("]]",at)-at-12);for(auto& c:flat)if(c=='[' || c==']')c=',';
            std::vector<double> words;for(size_t p=0;p<flat.size();){char* end{};const double v=std::strtod(flat.c_str()+p,&end);if(end==flat.c_str()+p){++p;continue;}words.push_back(v);p=size_t(end-flat.c_str());}Require(words.size()>=24,"a draw's sampler words");
            for(size_t k=0;k<std::min<size_t>(PostDraws::Inputs,words.size()/4);++k){samplers[k].filter=D3D12_FILTER(int(words[4*k]));const int address=int(words[4*k+1]);
                samplers[k].address=address?D3D12_TEXTURE_ADDRESS_MODE(address):D3D12_TEXTURE_ADDRESS_MODE_CLAMP;   // an unbound slot reads 0: clamp
                samplers[k].max_lod=float(std::min(words[4*k+3],double(D3D12_FLOAT32_MAX)));}}
        Check(allocator->Reset(),"allocator reset");ComPtr<ID3D12GraphicsCommandList> list;
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");
        // The inputs, from the captured texture list of the draw row.
        std::vector<ComPtr<ID3D12Resource>> keep;
        const auto textures_at=row.find("\"textures\":[");const auto globals_at=row.find("\"globals\":");Require(textures_at!=std::string::npos && globals_at!=std::string::npos,"a draw's textures");
        const auto textures=row.substr(textures_at,globals_at-textures_at);
        for(size_t from=0;;){const auto slot_at=textures.find("{\"slot\":",from);if(slot_at==std::string::npos)break;
            const auto close=textures.find("]}",slot_at);Require(close!=std::string::npos,"a draw's texture entry");const auto entry=textures.substr(slot_at,close+2-slot_at);from=close+2;
            const uint32_t slot=uint32_t(std::stoul(entry.substr(8)));const uint32_t fmt=uint32_t(std::stoul(entry.substr(entry.find("\"format\":")+9)));
            const auto size=Numbers(entry,"size");Require(size.size()==2 && slot<PostDraws::Inputs,"a draw's texture entry");
            const DXGI_FORMAT view=ViewFormat(fmt);const UINT w=UINT(size[0]),h=UINT(size[1]);const uint32_t texel=TexelBytes(view);
            char name[64]{};std::snprintf(name,sizeof(name),"/reference-draw-%02u-t%u.raw",d,slot);auto bytes=FileBytes(dir+name);
            if(fmt==65 && bytes.empty()){Require(w==64 && h==64,"a trim input of another extent");const auto trim=yebis::TrimCircle();bytes.assign(trim.begin(),trim.end());}
            const size_t stored=fmt==21?8:texel;Require(bytes.size()==size_t(w)*h*stored,"a captured input's size");
            // LS-3: the film grain (its bytes are the executable's block) carries its CPU mip chain (yebis::FilmGrainLevels).
            std::vector<std::vector<uint8_t>> levels;{char hex[65]{};if(ds3content::Sha256(bytes.data(),bytes.size(),hex) && !std::strcmp(hex,yebis::film_grain_sha256))levels=yebis::FilmGrainLevels(bytes);}
            if(!levels.empty()){const auto texture=Texture(device.Get(),view,w,h,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST,UINT16(levels.size()));
                for(UINT16 m=0;m<levels.size();++m){const UINT lw=std::max(w>>m,1u),lh=std::max(h>>m,1u),lp=(lw*4+255)/256*256;
                    const auto up=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(lp)*lh,D3D12_RESOURCE_STATE_GENERIC_READ);
                    {uint8_t* memory{};const D3D12_RANGE none{};Check(up->Map(0,&none,reinterpret_cast<void**>(&memory)),"mip map");
                        for(UINT y=0;y<lh;++y)std::memcpy(memory+size_t(y)*lp,levels[m].data()+size_t(y)*lw*4,size_t(lw)*4);up->Unmap(0,nullptr);}
                    D3D12_TEXTURE_COPY_LOCATION to{texture.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},src{up.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
                    to.SubresourceIndex=m;src.PlacedFootprint.Footprint={view,lw,lh,1,lp};list->CopyTextureRegion(&to,0,0,0,&src,nullptr);keep.push_back(up);}
                const auto barrier=Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);
                post.SetInput(uint32_t(n),slot,texture.Get(),view);keep.push_back(texture);continue;}
            // MB-0: the scene depth (21) as the renderer holds it, a planar R32G8X24 depth-stencil resource whose plane 0 takes the captured
            // depth words, viewed as R32_FLOAT_X8X24 (a bilinear read, the motion blur's velocity draw, filters that view as the original's).
            if(fmt==21){const auto texture=Texture(device.Get(),DXGI_FORMAT_R32G8X24_TYPELESS,w,h,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,D3D12_RESOURCE_STATE_COPY_DEST);
                const UINT pitch=(w*4+255)/256*256;const auto upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*h,D3D12_RESOURCE_STATE_GENERIC_READ);
                {uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"depth map");
                    for(UINT y=0;y<h;++y)for(UINT x=0;x<w;++x)std::memcpy(memory+size_t(y)*pitch+size_t(x)*4,bytes.data()+(size_t(y)*w+x)*8,4);upload->Unmap(0,nullptr);}
                D3D12_TEXTURE_COPY_LOCATION to{texture.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},src{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
                src.PlacedFootprint.Footprint={DXGI_FORMAT_R32_TYPELESS,w,h,1,pitch};list->CopyTextureRegion(&to,0,0,0,&src,nullptr);
                const auto barrier=Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);
                post.SetInput(uint32_t(n),slot,texture.Get(),DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);keep.push_back(texture);keep.push_back(upload);continue;}
            if(lit && LitInput(bytes,view,w,h,d*8u+slot))++lit_inputs;
            const auto texture=Texture(device.Get(),view,w,h,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);const UINT pitch=(w*texel+255)/256*256;
            const auto upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*h,D3D12_RESOURCE_STATE_GENERIC_READ);
            {uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"input map");
                for(UINT y=0;y<h;++y)for(UINT x=0;x<w;++x)std::memcpy(memory+size_t(y)*pitch+size_t(x)*texel,bytes.data()+(size_t(y)*w+x)*stored,texel);upload->Unmap(0,nullptr);}
            D3D12_TEXTURE_COPY_LOCATION to{texture.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},src{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
            src.PlacedFootprint.Footprint={view,w,h,1,pitch};list->CopyTextureRegion(&to,0,0,0,&src,nullptr);
            const auto barrier=Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);
            post.SetInput(uint32_t(n),slot,texture.Get(),view);keep.push_back(texture);keep.push_back(upload);}
        // LS-1: the captured write mask (the blend row; 15 for captures before Reference1660). A partial mask keeps the target's other
        // channels: the target then starts as the original's output with the written channels zeroed (RGBA16F only), so the
        // comparison checks both what the draw writes and what it keeps.
        uint8_t write_mask=D3D12_COLOR_WRITE_ENABLE_ALL;{const auto& b=found->second.blend_row;const auto at=b.find("\"write_mask\":");
            if(at!=std::string::npos)write_mask=uint8_t(std::stoul(b.substr(at+13)));Require(b.empty() || b.find("\"enable\":0")!=std::string::npos,"a blended draw (outside the replay)");}
        const uint32_t texel=TexelBytes(target_format);const UINT pitch=(W*texel+255)/256*256;
        char output_name[64]{};std::snprintf(output_name,sizeof(output_name),"/reference-draw-%02u-output.raw",d);const auto original=FileBytes(dir+output_name);
        Require(original.size()==size_t(W)*H*texel,"a captured output's size");
        const bool shaft_draw=pass.rfind("tech_LightShaftMask",0)==0 || is_blur(pass);
        {bool any_content=false,rgb_content=false;Content(original,target_format,any_content,rgb_content);   // the step review of c7e0ac515 (H1): the mask and the radial blurs only (the composites carry the glare whatever the shafts hold)
            if(any_content)++with_content;if(rgb_content)++with_rgb;
            if(shaft_draw){++shaft_replayed;if(rgb_content)++shaft_with_rgb;}}
        const auto make_target=[&]{
        const auto target=Texture(device.Get(),target_format,W,H,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
            write_mask==D3D12_COLOR_WRITE_ENABLE_ALL?D3D12_RESOURCE_STATE_RENDER_TARGET:D3D12_RESOURCE_STATE_COPY_DEST);
        if(write_mask!=D3D12_COLOR_WRITE_ENABLE_ALL){Require(target_format==DXGI_FORMAT_R16G16B16A16_FLOAT,"a partial write mask outside RGBA16F");
            const auto upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*H,D3D12_RESOURCE_STATE_GENERIC_READ);
            {uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"target map");
                for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){uint8_t* t=memory+size_t(y)*pitch+size_t(x)*texel;std::memcpy(t,original.data()+(size_t(y)*W+x)*texel,texel);
                    for(int c=0;c<4;++c)if(write_mask&(1u<<c))std::memset(t+2*c,0,2);}
                upload->Unmap(0,nullptr);}
            D3D12_TEXTURE_COPY_LOCATION to{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},src{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
            src.PlacedFootprint.Footprint={target_format,W,H,1,pitch};list->CopyTextureRegion(&to,0,0,0,&src,nullptr);
            const auto to_target=Transition(target.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&to_target);keep.push_back(upload);}
        return target;};
        const auto target=make_target();
        device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        const auto readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(pitch)*H,D3D12_RESOURCE_STATE_COPY_DEST);
        // LS-3: a layout with more than one texture-coordinate pair (the geometry row's stride 8 + 8 n) draws the captured vertices.
        uint32_t stride=16;{const auto& g=found->second.geometry_row;const auto at=g.find("\"stride\":");if(at!=std::string::npos)stride=uint32_t(std::stoul(g.substr(at+9)));}
        Require(stride>=16 && stride<=40 && stride%8==0,"a draw's vertex stride outside the replay");const uint32_t texcoords=(stride-8)/8;
        // I-0138 tranche 8f: four indices, the ghost's strip: the captured vertices and indices must be PostDraws' strip.
        const bool strip=row.find("\"count\":4,")!=std::string::npos;
        if(strip){char vname[64]{};std::snprintf(vname,sizeof(vname),"/reference-draw-%02u-vertices.raw",d);const auto v=FileBytes(dir+vname);
            std::snprintf(vname,sizeof(vname),"/reference-draw-%02u-indices.raw",d);const auto i=FileBytes(dir+vname);
            constexpr float words[16]{-1,1,0,1, -1,-1,0,0, 1,1,1,1, 1,-1,1,0};constexpr uint16_t order[4]{0,1,2,3};
            Require(stride==16 && v.size()>=sizeof(words) && !std::memcmp(v.data(),words,sizeof(words)) && i.size()>=sizeof(order) && !std::memcmp(i.data(),order,sizeof(order)),
                "a four-index draw that is not the ghost's strip");}
        D3D12_VERTEX_BUFFER_VIEW captured_vertices{};
        if(texcoords>1){char vname[64]{};std::snprintf(vname,sizeof(vname),"/reference-draw-%02u-vertices.raw",d);const auto v=FileBytes(dir+vname);
            Require(v.size()>=3*size_t(stride),"a draw's captured vertices");const auto vb=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,3*stride,D3D12_RESOURCE_STATE_GENERIC_READ);
            {void* memory{};const D3D12_RANGE none{};Check(vb->Map(0,&none,&memory),"vertices map");std::memcpy(memory,v.data(),3*size_t(stride));vb->Unmap(0,nullptr);}
            captured_vertices={vb->GetGPUVirtualAddress(),3*stride,stride};keep.push_back(vb);}
        post.Begin(list.Get());post.Draw(list.Get(),post.Pipeline(vertex,pixel,target_format,texcoords,write_mask),uint32_t(n),post.SamplerTable(samplers),
            constants->GetGPUVirtualAddress()+(ours?library_block_bytes:0),
            rtv->GetCPUDescriptorHandleForHeapStart(),W,H,texcoords>1?&captured_vertices:nullptr,strip);
        auto barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION out{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        out.PlacedFootprint.Footprint={target_format,W,H,1,pitch};list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
        ComPtr<ID3D12Resource> library_readback;
        if(ours){const auto library_target=make_target();auto library_rtv=rtv->GetCPUDescriptorHandleForHeapStart();library_rtv.ptr+=rtv_stride;
            device->CreateRenderTargetView(library_target.Get(),nullptr,library_rtv);library_readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(pitch)*H,D3D12_RESOURCE_STATE_COPY_DEST);
            post.Begin(list.Get());post.Draw(list.Get(),post.Pipeline(library_vertex,library_pixel,target_format,texcoords,write_mask),uint32_t(n),post.SamplerTable(samplers),
                constants->GetGPUVirtualAddress(),library_rtv,W,H,texcoords>1?&captured_vertices:nullptr,strip);
            auto to_copy=Transition(library_target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&to_copy);
            D3D12_TEXTURE_COPY_LOCATION library_out{library_readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},library_image{library_target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
            library_out.PlacedFootprint.Footprint={target_format,W,H,1,pitch};list->CopyTextureRegion(&library_out,0,0,0,&library_image,nullptr);keep.push_back(library_target);}
        Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        Check(queue->Signal(fence.Get(),++fence_value),"control signal");Check(fence->SetEventOnCompletion(fence_value,event),"control wait");WaitForSingleObject(event,INFINITE);
        char name[64]{};
        uint64_t differing{};uint32_t largest{};
        {uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(pitch)*H};Check(readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control readback");
            for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){const uint8_t* a=memory+size_t(y)*pitch+size_t(x)*texel;const uint8_t* b=original.data()+(size_t(y)*W+x)*texel;
                if(std::memcmp(a,b,texel)){++differing;largest=std::max(largest,Distance(target_format,a,b));
                    if(GetEnvironmentVariableA("POST_DRAWS_WHERE",nullptr,0) && differing<=12){std::printf("  (%u,%u):",x,y);for(uint32_t c=0;c<texel;c+=2)std::printf(" %04X/%04X",unsigned(a[c]|a[c+1]<<8),unsigned(b[c]|b[c+1]<<8));std::printf("\n");}}}
            if(lit){std::vector<uint8_t> tight(size_t(W)*H*texel);for(UINT y=0;y<H;++y)std::memcpy(tight.data()+size_t(y)*W*texel,memory+size_t(y)*pitch,size_t(W)*texel);
                const bool mask=pass.rfind("tech_LightShaftMask",0)==0,required=is_blur(pass) || pass=="tech_MadT5" || after_composite;
                const uint64_t texels=LitTexels(tight,target_format);   // another format refused (its step review's L2)
                const bool lit_output=texels*100>=uint64_t(W)*H;if(lit_output)++lit_outputs;
                if(mask){++masks;mask_lit_texels+=texels;mask_texels+=uint64_t(W)*H;}
                if(is_blur(pass))++lit_blurs;else if(pass=="tech_MadT5")++lit_composites;else if(after_composite)++lit_after;
                if(required){++lit_required;if(lit_output)++lit_required_lit;else{char line[192]{};std::snprintf(line,sizeof(line),"draw %u %s: %llu of %u texels lit, under 1 %%\n",d,pass.c_str(),(unsigned long long)texels,W*H);report+=line;}}
                after_composite=pass=="tech_MadT5";}
            if(library_readback){uint8_t* theirs{};Check(library_readback->Map(0,&read,reinterpret_cast<void**>(&theirs)),"control readback");bool equal=true;
                for(UINT y=0;y<H && equal;++y)equal=!std::memcmp(memory+size_t(y)*pitch,theirs+size_t(y)*pitch,size_t(W)*texel);
                const D3D12_RANGE none{};library_readback->Unmap(0,&none);
                if(equal)++equal_library;else{char line[192]{};std::snprintf(line,sizeof(line),"draw %u %s: our programme's output differs from the pinned original's on this device\n",d,pass.c_str());report+=line;}}
            const D3D12_RANGE none{};readback->Unmap(0,&none);}
        std::string captured_adapter;{std::snprintf(name,sizeof(name),"/reference-draw-%02u-adapter.txt",d);std::ifstream file(dir+name);std::getline(file,captured_adapter);}
        same_everywhere=same_everywhere && !warp && captured_adapter==this_adapter;
        ++replayed;if(!differing)++exact;worst=std::max(worst,largest);
        char line[256]{};std::snprintf(line,sizeof(line),"draw %u %s %ux%u: %llu of %u texels differ, largest %u units%s\n",d,pass.c_str(),W,H,(unsigned long long)differing,W*H,largest,
            lit?" (lit: against the captured output drawn from zeros, a diagnostic)":"");
        report+=line;}
    std::fputs(report.c_str(),stdout);CloseHandle(event);
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t messages{};
    for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::fprintf(stderr,"%s\n",message->pDescription);}}
    Require(messages==0,"debug-layer messages");
    if(same_everywhere && !lit)Require(exact==replayed,"a replayed draw differs from the original on the capture's adapter");
    if(lit){Require(lit_inputs>0,"a lit replay without an input given light");
        std::printf("lit replay %s: %u inputs given light; our outputs %u of %u lit (1 %% of texels normal); the required draws %u of %u lit (%u radial blurs, %u MadT5, %u after it); the mask, %u draw(s): %llu of %llu texels lit (reported, not required)\n",
            zone_name.c_str(),lit_inputs,lit_outputs,replayed,lit_required_lit,lit_required,lit_blurs,lit_composites,lit_after,masks,(unsigned long long)mask_lit_texels,(unsigned long long)mask_texels);
        Require(lit_blurs>0 && lit_composites>0 && lit_after>0,("zone "+zone_name+": the lit replay lacks a radial blur, MadT5 or the draw after it").c_str());
        Require(masks>0,("zone "+zone_name+": the lit replay holds no shaft mask").c_str());
        Require(lit_required_lit==lit_required,("zone "+zone_name+": a required draw of the lit replay gives no light").c_str());}
    if(must_carry)Require(shaft_replayed>0,("zone "+zone_name+": no shaft draw among the replayed draws, and the zone is declared as carrying content in them").c_str());
    if(must_carry)Require(shaft_with_rgb>0,("zone "+zone_name+": the shaft draws' surfaces are all zero: the capture proves nothing on the shafts").c_str());
    if(ours){std::printf("our programmes against the pinned originals on this device: %u of %u draws byte for byte\n",equal_library,replayed);
        Require(equal_library==replayed,"our programme's output differs from the pinned original's on this device");}
    if(lit){std::printf("PASS post draws %s: T13 %s, lit: our programmes and the pinned originals agree byte for byte on %u of %u draws from synthetic light; %u of %u required draws lit (%u radial blurs, %u MadT5, %u after it); the mask %s (%llu of %llu texels, not required); the captured outputs, drawn from zeros, not compared; no debug-layer message\n",
        argv[1],zone_name.c_str(),equal_library,replayed,lit_required_lit,lit_required,lit_blurs,lit_composites,lit_after,mask_lit_texels*100>=mask_texels?"lit":"unlit",(unsigned long long)mask_lit_texels,(unsigned long long)mask_texels);return 0;}
    std::printf("PASS post draws %s: T13 %s: %u of %u captured draws replayed through our post draw equal the original's output (%u with content, %u with RGB content; %u shaft draws, %u with RGB content: a surface of zeros equal to zeros proves nothing)%s; largest difference %u units; no debug-layer message\n",
        argv[1],dir.substr(dir.find_last_of("/\\")+1).c_str(),exact,replayed,with_content,with_rgb,shaft_replayed,shaft_with_rgb,same_everywhere?" (the capture's adapter and driver: exactness required)":" (another adapter, driver or WARP: reported)",worst);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
