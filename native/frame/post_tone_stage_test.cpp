// I-0138 tranche 1 (ROADMAP workstream 11): our tone-map pass (PostToneStage, shaders/post_tone.hlsl) against the original's own
// draws, and against the two stages it replaces.
//  1. The original's captured draws (the captures of T12 and T17: W10-NATIVE-PASSES 3.2 and 12). From a capture's tone-map draw
//     inputs (t0 the scene HDR R16G16B16A16 1280x720, t1 the glare R11G11B10, the Tone Map record's words, the vignette's
//     vertices) the pass must give the original's tone-map output; where the capture holds the frame's grading draw too
//     (tech_MadColorGradingLutT1_S1, found by its row; the tone-map draw is the one before it and its output is the grading's
//     input, checked), the pass with the LUT must give the original's grading output: from the HDR through its two draws, and
//     its LUT draw alone from the original's 8-bit tone-map output (T17's seam). The 16x256 strip is rebuilt from the captured
//     volume t16 (as T17) and the stage's volume must equal t16 bit for bit. Byte for byte on the adapter and driver that drew
//     the capture (its adapter file); elsewhere each draw alone within 1/255 on fewer than 5 % of the bytes (T12's and T17's
//     rule, unchanged), and the two draws chained reported without a bound (the tone map's 1/255 goes through the LUT: measured
//     on WARP, 2/255 on 4.8 % of the bytes).
//  2. The stages it replaces (TonemapStage and GradingStage, the original's programmes from the executable by their pins), on the
//     same device: the capture's inputs and a synthetic scene (ramps to 64, saturated colours, values around the curve's floor,
//     negatives, NaN, infinities, large values, alphas out of range, noise), each without and with the glare, the vignette, the
//     automatic exposure's factor, a hue, value and contrast matrix and the LUT. Byte for byte on any adapter.
//  3. I-0138 tranche 5: the HDR entry (ps_hdr, one draw into a float target, written display-encoded) against the 8-bit draws
//     from the same inputs. Where the curve's largest channel is under 0.97 of the knee (0.75 in linear light; the knee is on the
//     curve's output, before the colour matrix: read from the entry's own output through the matrix's inverse, no channel of that
//     output at a clamp, exactly 0 or 1 and above; the texels set aside are counted by reason), each of its
//     values times 255 is within 1.1 of the 8-bit draw's byte (the 8-bit draw's dither and rounding, the half float's last place); above the knee it is
//     never under the 8-bit byte by more than that, and every value is finite. With a LUT the two-draw form reads the LUT at an
//     8-bit value that its dither and its rounding move by up to one level, the entry at the value itself: under the knee
//     (read from the entry's draw of the same inputs without the LUT) the entry is within, and elsewhere never under the
//     two-draw form by more than, what one level of input moves that LUT (the slope: the largest sum, over the three axes, of
//     the differences between neighbouring nodes, times 15 / 255), plus what the two reads' filter weights can (a linear
//     filter's weights have 8 fractional bits: 1/256 of a node step of 17 levels, each read: the slope times 34 / 256; the
//     card's reads differ from WARP's by 0.3 level, INFERRED from this), plus 0.62 (the two-draw form's rounding, 0.5, and the
//     half float's last place, 0.12).
//     The alpha: within 0.65 of the 8-bit draw's byte (its rounding, the half float's last place). Without glare, vignette, factor or LUT, and where the
//     scene is finite, the entry also equals a CPU model of the whole pass (the block's exposure, curve and matrix, the gain
//     hdr::Highlight over the curve's largest channel) within 0.2 % and 1e-4, but where that channel is within 1e-4 of 1 (the
//     programme's single-precision value then decides the logarithm). The capture's scene and the synthetic one; the
//     sensitometric and the Reinhard curve.
//  4. I-0138 tranche 6: the candidate looks' entries (AgX, ACES, after Elden Ring; ps_agx, ps_aces, ps_er and their HDR entries) against the CPU models
//     of tone_looks.h through the whole pass (the block's exposure and look scale, the candidate's curve, the colour matrix, the
//     gamma, and for the HDR entry the gain along the candidate's tangent at the knee, look::HighlightGain), on the capture's scene and the synthetic one,
//     without glare, vignette, factor or LUT, where the scene is finite: the HDR entry's floats within 0.5 % of the model's
//     value and 5e-4 (the half float's last place, 4.9e-4 of the value; AgX's polynomial in single precision); the 8-bit
//     entry's bytes within 1.1 level (its dither and rounding, as section 3) plus that bound. The candidate after Elden Ring is
//     modelled without its dispersion (the block's words set to 0: each channel then reads the texel itself; no vignette:
//     vignette_power 0, I-0141 tranche 11); and with its dispersion: the 8-bit entry's red and alpha bytes equal to the draw without it, its green and its
//     blue bytes each against a CPU model of the channel's read (the four texels around its place, each through the exposure
//     and the curve, encoded, blended by the filter's weights; a texel that is not finite as the programme
//     takes it) within 1.2 level plus the same bound, and green and blue bytes that differ from the draw without it. Each
//     candidate's own count of modelled texels and of texels above its tangent point is required (the step review's L2). Not
//     drawn here: a candidate with a glare surface (the candidate after Elden Ring's bloom before the curve is read, not tested).
//     The area's own vignette in that candidate's body (I-0141 tranche 11): its own check after the vivid variant's, 8-bit, no glare.
// Arguments: warp|hardware, the executable, the programme cache directory (GX's LutCopy2DTo3DCs), the capture directory (skipped
// with a message without them), then optionally a prefix for the pass's outputs on the capture (<prefix>-tone.raw,
// <prefix>-lut.raw and <prefix>-lut-alone.raw, RGBA8 rows, for inspection). Also: no debug-layer message of severity WARNING or worse.
#include "../renderer/post_tone_stage.h"
#include "../renderer/tonemap_stage.h"
#include "../renderer/grading_stage.h"
#include "../renderer/yebis_stage.h"
#include "../renderer/hdr_output.h"
#include "../world/post_tone.h"
#include "../world/yebis_pins.h"
#include "../world/light_producers.h"
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
constexpr UINT W=1280,H=720,N=PostToneStage::LutEdge;
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
std::vector<uint8_t> FileBytes(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
// The device and one list, executed and waited for at each Run.
struct Gpu {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;HANDLE event{};uint64_t signalled{};
    void Run(){Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        Check(queue->Signal(fence.Get(),++signalled),"control signal");Check(fence->SetEventOnCompletion(signalled,event),"control wait");WaitForSingleObject(event,INFINITE);
        Check(allocator->Reset(),"control allocator reset");Check(list->Reset(allocator.Get(),nullptr),"control list reset");}
    ComPtr<ID3D12Resource> Buffer(D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
        desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
        ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
    ComPtr<ID3D12Resource> Texture(DXGI_FORMAT format,UINT w,UINT h,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear=nullptr){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=w;desc.Height=h;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=format;desc.Flags=flags;
        ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,clear,IID_PPV_ARGS(&result)),"control texture");return result;}
    // A texture holding `data` (rows of w texels of `texel` bytes), left in `state`.
    ComPtr<ID3D12Resource> Filled(DXGI_FORMAT format,UINT w,UINT h,UINT texel,const uint8_t* data,D3D12_RESOURCE_STATES state){
        const auto texture=Texture(format,w,h,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);const UINT pitch=(w*texel+255)/256*256;
        const auto upload=Buffer(D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*h,D3D12_RESOURCE_STATE_GENERIC_READ);
        uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"control upload map");
        for(UINT y=0;y<h;++y)std::memcpy(memory+size_t(y)*pitch,data+size_t(y)*w*texel,size_t(w)*texel);upload->Unmap(0,nullptr);
        D3D12_TEXTURE_COPY_LOCATION to{texture.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={format,w,h,1,pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        const auto barrier=Transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,state);list->ResourceBarrier(1,&barrier);Run();return texture;}
};
// The pinned containers of the two original techniques' programmes, found in the executable file.
bool Programmes(const std::vector<uint8_t>& exe,TonemapBlobs& tone,GradingBlobs& grading){
    const auto* pin=yebis::FindTechniquePin("tech_TonemapHDR_Dither_Exposure_Matrix_Gamma");const auto* vignette=yebis::FindTechniquePin("tech_TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma");
    const auto* lut=yebis::FindTechniquePin("tech_MadColorGradingLutT1_S1");if(!pin || !vignette || !lut)return false;
    for(size_t at=0;at+32<=exe.size();at+=4){
        if(std::memcmp(exe.data()+at,"DXBC",4))continue;uint32_t size{};std::memcpy(&size,exe.data()+at+24,4);
        if((size!=2980 && size!=4164 && size!=4356 && size!=2624 && size!=2876) || at+size>exe.size())continue;char hex[65]{};if(!ds3content::Sha256(exe.data()+at,size,hex))continue;
        const auto take=[&](std::vector<uint8_t>& blob,const char* sha){if(blob.empty() && !std::strcmp(hex,sha))blob.assign(exe.begin()+at,exe.begin()+at+size);};
        take(tone.vertex,pin->vs_sha256);take(tone.pixel,pin->ps_sha256);take(tone.pixel_vignette,vignette->ps_sha256);take(grading.vertex,lut->vs_sha256);take(grading.pixel,lut->ps_sha256);}
    return !tone.vertex.empty() && !tone.pixel.empty() && !tone.pixel_vignette.empty() && !grading.vertex.empty() && !grading.pixel.empty();}
// I-0138 part 4c: the Reinhard curve's two programmes (tone type 3), by their pins, any container size.
bool ReinhardProgrammes(const std::vector<uint8_t>& exe,TonemapBlobs& tone){
    const auto* pin=yebis::FindTechniquePin("tech_TonemapHDR_Dither_Exposure_Matrix_Gamma_REINHARD");
    const auto* vignette=yebis::FindTechniquePin("tech_TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma_REINHARD");if(!pin || !vignette)return false;
    for(size_t at=0;at+32<=exe.size();at+=4){
        if(std::memcmp(exe.data()+at,"DXBC",4))continue;uint32_t size{};std::memcpy(&size,exe.data()+at+24,4);
        if(size<32 || size>65536 || at+size>exe.size())continue;char hex[65]{};if(!ds3content::Sha256(exe.data()+at,size,hex))continue;
        const auto take=[&](std::vector<uint8_t>& blob,const char* sha){if(blob.empty() && !std::strcmp(hex,sha))blob.assign(exe.begin()+at,exe.begin()+at+size);};
        take(tone.vertex,pin->vs_sha256);take(tone.pixel,pin->ps_sha256);take(tone.pixel_vignette,vignette->ps_sha256);}
    return !tone.vertex.empty() && !tone.pixel.empty() && !tone.pixel_vignette.empty();}
// The captured frame's grading draw (as T17 finds it): the number of the first reference_yebis_draw row of
// tech_MadColorGradingLutT1_S1, at the frame the capture dumped, whose volume the capture holds; -1 without one.
int GradingDraw(const std::string& dir){
    std::string frame;
    {std::ifstream trace(dir+"/startup.jsonl");std::string line,last;
        while(std::getline(trace,line)){
            if(line.find("\"event\":\"reference_yebis_draw\",\"frame\":")!=std::string::npos){const auto at=line.find("\"frame\":")+8;last=line.substr(at,line.find(',',at)-at);}
            else if(line.find("\"event\":\"reference_yebis_draw_capture\"")!=std::string::npos){frame=last;break;}}}
    if(frame.empty())return -1;
    std::ifstream trace(dir+"/startup.jsonl");std::string line;
    while(std::getline(trace,line)){
        if(line.find("\"event\":\"reference_yebis_draw\",\"frame\":"+frame+",")==std::string::npos || line.find("\"pass\":\"tech_MadColorGradingLutT1_S1\"")==std::string::npos)continue;
        const auto d=line.find("\"draw\":");if(d==std::string::npos)continue;
        const int draw=int(std::stoul(line.substr(d+7)));char name[48]{};std::snprintf(name,sizeof(name),"/reference-draw-%02d-t16.raw",draw);
        if(std::ifstream(dir+name))return draw;}
    return -1;}
uint16_t Half(float f){   // round to nearest even
    uint32_t x;std::memcpy(&x,&f,4);const uint32_t sign=(x>>16)&0x8000u;x&=0x7FFFFFFFu;
    if(x>0x7F800000u)return uint16_t(sign|0x7E00u);if(x>=0x477FF000u)return uint16_t(sign|0x7C00u);
    if(x<0x38800000u){if(x<0x33000000u)return uint16_t(sign);
        const uint32_t m=(x&0x7FFFFFu)|0x800000u,shift=126u-(x>>23);uint32_t r=m>>shift;const uint32_t rest=m&((1u<<shift)-1u),half=1u<<(shift-1u);
        if(rest>half || (rest==half && (r&1u)))++r;return uint16_t(sign|r);}
    uint32_t h=(((x>>23)-112u)<<10)|((x>>13)&0x3FFu);const uint32_t rest=x&0x1FFFu;if(rest>0x1000u || (rest==0x1000u && (h&1u)))++h;return uint16_t(sign|h);}
// The synthetic scene: twelve bands of 60 rows.
std::vector<uint8_t> SyntheticScene(){
    std::vector<uint8_t> out(size_t(W)*H*8);uint32_t seed=0x1380138u;const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    const auto noise=[&]{seed=seed*1664525u+1013904223u;return float(seed>>8)/16777216.0f;};
    for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){const float u=float(x)/float(W-1),v=64.0f*u*u*u;float c[4]{v,v,v,1.0f};
        switch(y/60){
            case 0:break;case 1:c[1]=c[2]=0.0f;break;case 2:c[0]=c[2]=0.0f;break;case 3:c[0]=c[1]=0.0f;break;
            case 4:c[1]=v*0.5f;c[2]=0.0f;break;
            case 5:c[0]=c[1]=c[2]=1e-6f+u*2e-4f;break;                              // around the floor 2^-14 under the exposed scene
            case 6:c[0]=-v;c[2]=0.5f;break;
            case 7:if(x%64==0)c[0]=nan;else if(x%64==16)c[1]=inf;else if(x%64==32)c[2]=-inf;else if(x%64==48)c[0]=c[1]=c[2]=nan;break;
            case 8:c[0]=100.0f+u*60000.0f;c[1]=c[0]*0.5f;c[2]=c[0]*0.25f;break;
            case 9:c[3]=x<W/4?-1.0f:x<W/2?0.25f:x<3*W/4?2.0f:nan;break;
            default:c[0]=noise()*8.0f;c[1]=noise()*8.0f;c[2]=noise()*8.0f;c[3]=noise();}
        for(int k=0;k<4;++k){const uint16_t h=Half(c[k]);std::memcpy(out.data()+(size_t(y)*W+x)*8+size_t(k)*2,&h,2);}}
    return out;}
// A strip that is not the identity (the captures of m37 hold no LUT): each channel bent and tinted by the two others.
std::vector<uint8_t> SyntheticStrip(){
    std::vector<uint8_t> strip(size_t(N)*N*N*4);
    for(UINT z=0;z<N;++z)for(UINT y=0;y<N;++y)for(UINT x=0;x<N;++x){uint8_t* t=strip.data()+((size_t(z)*N+y)*N+x)*4;
        const float r=float(x)/15.0f,g=float(y)/15.0f,b=float(z)/15.0f;
        t[0]=uint8_t(std::lround(255.0f*std::pow(r,0.8f)*(0.9f+0.1f*g)));t[1]=uint8_t(std::lround(255.0f*(0.85f*g+0.15f*b*b)));
        t[2]=uint8_t(std::lround(255.0f*std::sqrt(b)*(1.0f-0.2f*r)));t[3]=255;}
    return strip;}
// One draw's inputs.
struct Inputs {
    ID3D12Resource* scene{};uint64_t scene_generation{};ID3D12Resource* glare{};float factor{};   // factor 0: the manual case (no t2)
    yebis::TonemapInputs record;float vignette_power{};ID3D12Resource* strip{};uint64_t strip_generation{};
    ID3D12Resource* toned{};   // our pass only, with a strip: an 8-bit image (COPY_SOURCE) the LUT's draw reads in place of the tone map's output
    look::ToneLook look{};     // our pass only (I-0138 tranche 6): a candidate's curve in the record's place
    bool er_plain{};           // the candidate after Elden Ring without its lateral dispersion (the model's form)
    bool er_vivid{};           // and its vivid variant (I-0138 tranche 7b, look::ErVivid)
    uint32_t grain{},grain_seed{};   // our pass only (I-0138 tranche 7): the film grain's row (percent, 0 none) and the frame's seed
    bool no_vignette_bit{};    // I-0141 tranche 11: the vignette's coordinates kept but flag bit 0 cleared (no vignette drawn)
};
// What one 8-bit level of input can move a strip's output, in levels (I-0138 tranche 5: the HDR entry's bound against the two-draw
// form): the largest, over the nodes and the channels, of the sum over the three axes of the difference with the next node,
// times (N - 1) / 255 (a level in node steps).
double StripSlope(const std::vector<uint8_t>& strip){
    const auto at=[&](UINT x,UINT y,UINT z,UINT k){return int(strip[((size_t(z)*N+y)*N+x)*4+k]);};int largest{};
    for(UINT z=0;z<N;++z)for(UINT y=0;y<N;++y)for(UINT x=0;x<N;++x)for(UINT k=0;k<3;++k){int sum{};
        if(x+1<N)sum+=std::abs(at(x+1,y,z,k)-at(x,y,z,k));if(y+1<N)sum+=std::abs(at(x,y+1,z,k)-at(x,y,z,k));if(z+1<N)sum+=std::abs(at(x,y,z+1,k)-at(x,y,z,k));
        largest=std::max(largest,sum);}
    return double(largest)*double(N-1)/255.0;}
struct Difference {uint64_t bytes{};int largest{};};
Difference Compare(const std::vector<uint8_t>& a,const std::vector<uint8_t>& b){Require(a.size()==b.size(),"compared sizes");Difference d;
    for(size_t i=0;i<a.size();++i){const int diff=std::abs(int(a[i])-int(b[i]));if(diff){++d.bytes;d.largest=std::max(d.largest,diff);}}return d;}
}
int main(int argc,char** argv){try{
    Require(argc==5 || argc==6,"usage: post_tone_stage_test warp|hardware <DarkSoulsIII.exe> <programme cache> <capture directory> [output prefix]");const bool warp=!std::strcmp(argv[1],"warp");
    Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
    const std::string dir=argv[4];const auto exe=FileBytes(argv[2]);const auto copy=FileBytes(std::string(argv[3])+"/gxposteffect__LutCopy2DTo3DCs.cpo");
    // The capture: the frame's grading draw and the tone-map draw before it, or the tone-map capture alone.
    const int grading_draw=GradingDraw(dir);
    Require(!std::ifstream(dir+"/startup.jsonl") || grading_draw>0 || std::ifstream(dir+"/reference-tonemap-t0.raw"),"the capture has a trace but neither a grading draw nor a tone-map capture");
    char number[48]{};std::snprintf(number,sizeof(number),"/reference-draw-%02d-",grading_draw-1);
    const std::string tone_prefix=dir+(grading_draw>0?number:"/reference-tonemap-");
    std::snprintf(number,sizeof(number),"/reference-draw-%02d-",grading_draw);const std::string grading_prefix=dir+number;
    const auto scene_bytes=FileBytes(tone_prefix+"t0.raw"),glare_bytes=FileBytes(tone_prefix+"t1.raw"),tone_output=FileBytes(tone_prefix+"output.raw");
    if(exe.empty() || copy.empty() || scene_bytes.empty()){std::printf("PASS post tone stage %s: skipped, no executable, programme cache or capture\n",argv[1]);return 0;}
    const bool large_glare=glare_bytes.size()==size_t(640)*540*4;UINT GW=large_glare?640:420,GH=large_glare?540:354;   // m50, m40: 640x540
    // I-0138 part 4c: a capture with a draw row gives the glare input's extent itself (the opening scene's 524x441): the last
    // reference_yebis_draw row of the tone-map draw's number, its slot 1's size.
    // The row of the captured frame (the frame of the last draw row before the first capture row, as GradingDraw finds it).
    if(grading_draw>0){std::ifstream trace(dir+"/startup.jsonl");std::string line,found,frame,last;
        while(std::getline(trace,line)){
            if(line.find("\"event\":\"reference_yebis_draw\",\"frame\":")!=std::string::npos){const auto at=line.find("\"frame\":")+8;last=line.substr(at,line.find(',',at)-at);}
            else if(line.find("\"event\":\"reference_yebis_draw_capture\"")!=std::string::npos){frame=last;break;}}
        const std::string key="\"frame\":"+frame+",\"draw\":"+std::to_string(grading_draw-1)+",";trace.clear();trace.seekg(0);
        while(!frame.empty() && std::getline(trace,line))if(line.find("\"event\":\"reference_yebis_draw\",")!=std::string::npos && line.find(key)!=std::string::npos &&
            line.find("\"pass\":\"tech_TonemapHDR")!=std::string::npos){found=line;break;}
        if(const auto at=found.find("{\"slot\":1,");at!=std::string::npos){const auto size=found.find("\"size\":[",at);
            if(size!=std::string::npos){GW=UINT(std::stoul(found.substr(size+8)));GH=UINT(std::stoul(found.substr(found.find(',',size+8)+1)));}}}
    Require(scene_bytes.size()==size_t(W)*H*8 && glare_bytes.size()==size_t(GW)*GH*4 && tone_output.size()==size_t(W)*H*4,"capture sizes");
    std::vector<uint8_t> t16,grading_output;
    if(grading_draw>0){t16=FileBytes(grading_prefix+"t16.raw");grading_output=FileBytes(grading_prefix+"output.raw");
        Require(t16.size()==size_t(N)*N*N*16 && grading_output.size()==tone_output.size(),"grading capture sizes");
        Require(FileBytes(grading_prefix+"t0.raw")==tone_output,"the captured grading draw's input is not the tone-map draw's output");}
    TonemapBlobs tone_blobs;GradingBlobs grading_blobs;Require(Programmes(exe,tone_blobs,grading_blobs),"the pinned programmes are not in the executable");grading_blobs.lut_copy=copy;
    TonemapBlobs reinhard_blobs;Require(ReinhardProgrammes(exe,reinhard_blobs),"the Reinhard programmes are not in the executable");
    {const auto* pin=&yebis_gx_pins[1];char hex[65]{};Require(!std::wcscmp(pin->name,L"LutCopy2DTo3DCs"),"pin table");
        Require(copy.size()==pin->bytes && ds3content::Sha256(copy.data(),copy.size(),hex) && !std::strcmp(hex,pin->sha256),"LutCopy2DTo3DCs differs from its pin");}
    _putenv_s("DS3_EXE",argv[2]);
    // The executable's data words and the capture's Tone Map record (m37's without a tone file, as T12).
    yebis::TonemapData d;std::array<float,16> rows{};float scale{},bias{},to_byte{};
    Require(ds3light::Rdata(0x2B60C40,d.curve.data(),84) && ds3light::Rdata(0x2B60CD0,&d.tx_scale,4) && ds3light::Rdata(0x3D7EDA4,&d.third,4) &&
        ds3light::Rdata(0x3D7EE10,&d.one,4) && ds3light::Rdata(0x3D7EF70,&d.steps,4) && ds3light::Rdata(0x2B60D60,rows.data(),16) &&
        ds3light::Rdata(0x2B60D40,rows.data()+4,16) && ds3light::Rdata(0x2B60D50,rows.data()+8,16) && ds3light::Rdata(0x2B60D30,rows.data()+12,16) &&
        ds3light::Rdata(0x3D7ED5C,&scale,4) && ds3light::Rdata(0x2A5F3CC,&bias,4) && ds3light::Rdata(0x2B60CE4,&to_byte,4),"executable data words");
    Require(yebis::LoadColorData(d.color,[](uint32_t rva,void* out,size_t bytes){return ds3light::Rdata(rva,out,bytes);}),"colour data words");
    yebis::TonemapInputs record;std::memcpy(&record.exposure,"\x5C\x8F\xC2\x3F",4);std::memcpy(&record.gamma,"\xCD\xCC\x0C\x40",4);
    std::memcpy(&record.filmic_contrast,"\x9A\x99\x99\x3E",4);record.color_h=0.0f;std::memcpy(&record.color_s,"\x66\x66\x66\x3F",4);record.color_v=1.0f;
    float capture_vignette{};
    if(const auto tone=FileBytes(tone_prefix+"tone.raw");tone.size()==28 || tone.size()==36){if(tone.size()==36)std::memcpy(&capture_vignette,tone.data()+28,4);
        std::memcpy(&record.exposure,tone.data(),4);std::memcpy(&record.gamma,tone.data()+4,4);std::memcpy(&record.filmic_contrast,tone.data()+12,4);
        std::memcpy(&record.color_h,tone.data()+16,4);std::memcpy(&record.color_s,tone.data()+20,4);std::memcpy(&record.color_v,tone.data()+24,4);
        // I-0138 part 4c: the tone type (144962010, the capture's word 2); the ToneMappingFactor (144962014, read by the Reinhard
        // curve only) and the Contrust (1449621B8) from the globals block the capture holds (reference-glare-globals.raw, 0x370 bytes from
        // 144961F70: +0xA4, +0x248), as the frame reads them (i138-4b-opening2-orig-1500: the opening scene's Reinhard record).
        std::memcpy(&record.tone_type,tone.data()+8,4);
        if(const auto globals=FileBytes(dir+"/reference-glare-globals.raw");globals.size()>=0x370){std::memcpy(&record.mapping_factor,globals.data()+0xA4,4);
            std::memcpy(&record.contrast,globals.data()+0x248,4);}
        else Require(record.tone_type==2u,"a tone type other than 2 without the capture's globals block");}
    const auto dither=yebis::TonemapDither(rows,scale,bias,to_byte);
    yebis::VignetteData vd;{float eighth_pi{};uint32_t bits{};Require(ds3light::Rdata(0x2A66FD0,&eighth_pi,4) && (std::memcpy(&bits,&eighth_pi,4),bits==0x3EC90FDBu),"pi/8 data word");
        const uint32_t tan_bits=0x3ED413CDu,k_bits=0x3FAAAAA8u,m_bits=0x3F2AAAE3u;std::memcpy(&vd.tan_eighth_pi,&tan_bits,4);std::memcpy(&vd.k,&k_bits,4);std::memcpy(&vd.m,&m_bits,4);}
    const auto vignette_of=[&](float power){return yebis::VignetteCoordinates(yebis::VignetteRect(vd,power,float(W)/float(H)));};
    if(0.0f<capture_vignette){   // the coordinates the pass interpolates equal the captured vertices' TEXCOORD2 bit for bit (as T12)
        const auto vertices=FileBytes(tone_prefix+"vertices.raw"),indices=FileBytes(tone_prefix+"indices.raw");const auto uv=vignette_of(capture_vignette);
        Require(vertices.size()>=3*48 && indices.size()>=6 && indices[0]==0 && indices[1]==0 && indices[2]==1 && indices[3]==0 && indices[4]==2 && indices[5]==0,"the capture's vertices and indices");
        for(size_t i=0;i<3;++i)Require(!std::memcmp(vertices.data()+i*48+24,uv[i].data(),8),"the vignette coordinates differ from the captured vertices");}
    // The strip from the captured volume (as T17: every word exactly k/255), or the synthetic one.
    std::vector<uint8_t> strip_bytes=SyntheticStrip();
    if(!t16.empty())for(size_t i=0;i<strip_bytes.size();++i){float v{};std::memcpy(&v,t16.data()+i*4,4);const long k=std::lround(v*255.0f);
        Require(k>=0 && k<=255 && float(double(k)/255.0)==v,"a LUT word is not an 8-bit UNORM value");strip_bytes[i]=uint8_t(k);}
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"control WARP adapter");
    else{Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"control hardware adapter");
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);Require(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE),"no hardware adapter");}
    Gpu gpu;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&gpu.device)),"control device");
    const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};Check(gpu.device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&gpu.queue)),"control queue");
    Check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&gpu.allocator)),"control allocator");
    Check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,gpu.allocator.Get(),nullptr,IID_PPV_ARGS(&gpu.list)),"control list");
    Check(gpu.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gpu.fence)),"control fence");gpu.event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    constexpr auto psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    const auto capture_scene=gpu.Filled(DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,8,scene_bytes.data(),psr);
    const auto synthetic_bytes=SyntheticScene();const auto synthetic_scene=gpu.Filled(DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,8,synthetic_bytes.data(),psr);
    const auto glare=gpu.Filled(DXGI_FORMAT_R11G11B10_FLOAT,GW,GH,4,glare_bytes.data(),psr);
    const auto strip=gpu.Filled(DXGI_FORMAT_R8G8B8A8_UNORM,N,N*N,4,strip_bytes.data(),static_cast<D3D12_RESOURCE_STATES>(psr|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
    const auto synthetic_strip_bytes=SyntheticStrip();
    const auto strip2=gpu.Filled(DXGI_FORMAT_R8G8B8A8_UNORM,N,N*N,4,synthetic_strip_bytes.data(),static_cast<D3D12_RESOURCE_STATES>(psr|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
    // The target is cleared to a sentinel before every draw (the integrity review): a path that drew nothing would not be compared
    // with the bytes the path before it left.
    const D3D12_CLEAR_VALUE sentinel{DXGI_FORMAT_R8G8B8A8_UNORM,{0.25f,0.75f,0.5f,0.125f}};
    const auto target=gpu.Texture(DXGI_FORMAT_R8G8B8A8_UNORM,W,H,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,&sentinel);
    D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};ComPtr<ID3D12DescriptorHeap> rtv;Check(gpu.device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv)),"control rtv heap");
    gpu.device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());const auto target_rtv=rtv->GetCPUDescriptorHandleForHeapStart();
    const UINT pitch=W*4;const auto readback=gpu.Buffer(D3D12_HEAP_TYPE_READBACK,UINT64(pitch)*H,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto constants=gpu.Buffer(D3D12_HEAP_TYPE_UPLOAD,65536,D3D12_RESOURCE_STATE_GENERIC_READ);
    uint8_t* constant_memory{};{const D3D12_RANGE none{};Check(constants->Map(0,&none,reinterpret_cast<void**>(&constant_memory)),"constants map");}
    uint64_t constant_at{};const auto allocate=[&](uint64_t bytes){const uint64_t at=constant_at;constant_at+=(bytes+255)/256*256;Require(constant_at<=65536,"constant space");
        return std::pair<void*,D3D12_GPU_VIRTUAL_ADDRESS>{constant_memory+at,constants->GetGPUVirtualAddress()+at};};
    const auto read_target=[&]{   // executes the list, returns the target's bytes
        auto barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION out{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        out.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,W,H,1,pitch};gpu.list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
        barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.list->ResourceBarrier(1,&barrier);gpu.Run();constant_at=0;
        std::vector<uint8_t> pixels(size_t(W)*H*4);uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(pitch)*H};Check(readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control readback");
        std::memcpy(pixels.data(),memory,pixels.size());const D3D12_RANGE none{};readback->Unmap(0,&none);return pixels;};
    const auto factor_texture=[&](float f){const float texel[4]{f,f,f,1.0f};   // the automatic exposure's texel (shaders/auto_exposure.hlsl)
        return gpu.Filled(DXGI_FORMAT_R32G32B32A32_FLOAT,1,1,16,reinterpret_cast<const uint8_t*>(texel),psr);};
    std::string reason;
    PostToneStage stage;Require(stage.Open(gpu.device.Get(),dither,DXGI_FORMAT_R8G8B8A8_UNORM,W,H,reason),reason.c_str());Require(stage.OpenLut(reason),reason.c_str());
    TonemapStage old_tone;Require(old_tone.Open(gpu.device.Get(),tone_blobs,dither,DXGI_FORMAT_R8G8B8A8_UNORM,W,H,reason),reason.c_str());
    TonemapStage old_reinhard;Require(old_reinhard.Open(gpu.device.Get(),reinhard_blobs,dither,DXGI_FORMAT_R8G8B8A8_UNORM,W,H,reason),reason.c_str());
    GradingStage old_grading;Require(old_grading.Open(gpu.device.Get(),grading_blobs,DXGI_FORMAT_R8G8B8A8_UNORM,W,H,reason),reason.c_str());
    std::vector<ComPtr<ID3D12Resource>> kept;   // the factor textures, until the end
    const auto draw_new=[&](const Inputs& in){
        gpu.list->ClearRenderTargetView(target_rtv,sentinel.Color,0,nullptr);
        const auto t=yebis::BuildTonemap(d,in.record);Require(t.matrix_supported && t.guarded,"the record is served");
        const auto uv=vignette_of(in.vignette_power);auto params=BuildPostTone(t,0.0f<in.vignette_power?&uv:nullptr,N);ApplyToneLook(params,in.look,false,true,in.er_vivid);
        if(in.er_plain)for(float& word:params.er_dispersion)word=0.0f;
        if(in.grain)SetFilmGrain(params,in.grain,H,in.grain_seed);
        if(in.no_vignette_bit)params.flags&=~uint32_t(kPostToneVignette);
        ID3D12Resource* adapted=nullptr;if(in.factor!=0.0f){kept.push_back(factor_texture(in.factor));adapted=kept.back().Get();}
        const PostToneLut lut{in.strip,DXGI_FORMAT_R8G8B8A8_UNORM,in.strip_generation};
        stage.Record(gpu.list.Get(),in.scene,in.scene_generation,target_rtv,params,[&](uint64_t bytes){const auto m=allocate(bytes);return PostToneConstantMemory{m.first,m.second};},
            in.glare,in.glare?1u:0u,adapted,in.strip?&lut:nullptr);
        if(in.strip){
            if(in.toned){   // the LUT's draw alone: the given 8-bit image in place of the tone map's
                constexpr auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET;auto barrier=Transition(stage.Intermediate(),rt,D3D12_RESOURCE_STATE_COPY_DEST);gpu.list->ResourceBarrier(1,&barrier);
                gpu.list->CopyResource(stage.Intermediate(),in.toned);barrier=Transition(stage.Intermediate(),D3D12_RESOURCE_STATE_COPY_DEST,rt);gpu.list->ResourceBarrier(1,&barrier);}
            stage.RecordLut(gpu.list.Get(),target_rtv);}
        return read_target();};
    const auto draw_old=[&](const Inputs& in){
        gpu.list->ClearRenderTargetView(target_rtv,sentinel.Color,0,nullptr);
        const auto t=yebis::BuildTonemap(d,in.record);std::array<uint8_t,TonemapStage::GlobalsBytes> globals{};yebis::TonemapGlobals(t,globals.data());
        const auto uv=vignette_of(in.vignette_power);
        ID3D12Resource* adapted=nullptr;if(in.factor!=0.0f){kept.push_back(factor_texture(in.factor));adapted=kept.back().Get();}
        (in.record.tone_type==3u?old_reinhard:old_tone).Record(gpu.list.Get(),in.scene,in.scene_generation,in.strip?old_grading.Source():target_rtv,globals.data(),
            [&](uint64_t bytes){const auto m=allocate(bytes);return TonemapConstantMemory{m.first,m.second};},in.glare,0.0f<in.vignette_power?&uv:nullptr,in.glare?1u:0u,adapted);
        if(in.strip)old_grading.Record(gpu.list.Get(),in.strip,DXGI_FORMAT_R8G8B8A8_UNORM,in.strip_generation,target_rtv,
            [&](uint64_t bytes){const auto m=allocate(bytes);return GradingConstantMemory{m.first,m.second};});
        return read_target();};
    // 1. The original's captured draws.
    std::string captured_adapter;{std::ifstream file(tone_prefix+"adapter.txt");std::getline(file,captured_adapter);}
    std::string this_adapter;{DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);LARGE_INTEGER driver{};(void)adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice),&driver);
        char text[96]{};std::snprintf(text,sizeof(text),"%04X %04X %016llX",desc.VendorId,desc.DeviceId,static_cast<unsigned long long>(driver.QuadPart));this_adapter=text;}
    const bool same=!warp && !captured_adapter.empty() && captured_adapter==this_adapter;
    // bounded: off the capture's adapter, within 1/255 on fewer than 5 % of the bytes (T12's and T17's rule for one draw); false for
    // the two draws chained, where that rule has no ground (the first draw's 1/255 goes through the LUT): reported only there.
    const auto against_original=[&](const char* what,const char* file,const std::vector<uint8_t>& ours,const std::vector<uint8_t>& original,bool bounded=true){
        if(argc==6){std::ofstream out(std::string(argv[5])+file,std::ios::binary);out.write(reinterpret_cast<const char*>(ours.data()),std::streamsize(ours.size()));}   // RGBA8 rows, for inspection
        const auto diff=Compare(ours,original);
        std::printf("%s against the original: %llu differing bytes of %u, largest %d/255%s\n",what,static_cast<unsigned long long>(diff.bytes),W*H*4,diff.largest,
            same || bounded?"":" (the two draws chained off the capture's adapter: reported, each draw is bounded alone)");
        if(same)Require(diff.largest==0,"the pass differs from the original on the capture's adapter");
        else if(bounded)Require(diff.largest<=1 && diff.bytes*20<uint64_t(W)*H*4,"the pass differs from the original beyond its bound");
        return diff;};
    Inputs capture{capture_scene.Get(),1,glare.Get(),0.0f,record,capture_vignette,nullptr,0};
    const auto tone_diff=against_original("tone map","-tone.raw",draw_new(capture),tone_output);
    Difference lut_diff,chain_diff;
    if(!t16.empty()){Inputs graded=capture;graded.strip=strip.Get();graded.strip_generation=1;
        chain_diff=against_original("tone map and LUT","-lut.raw",draw_new(graded),grading_output,false);
        // The LUT's draw alone from the original's 8-bit tone-map output (T17's seam).
        const auto original_toned=gpu.Filled(DXGI_FORMAT_R8G8B8A8_UNORM,W,H,4,tone_output.data(),D3D12_RESOURCE_STATE_COPY_SOURCE);graded.toned=original_toned.Get();
        lut_diff=against_original("LUT from the original's tone-map output","-lut-alone.raw",draw_new(graded),grading_output);
        // The stage's volume against the captured t16.
        const UINT volume_pitch=256;const auto volume_readback=gpu.Buffer(D3D12_HEAP_TYPE_READBACK,UINT64(volume_pitch)*N*N,D3D12_RESOURCE_STATE_COPY_DEST);
        auto barrier=Transition(stage.Volume(),psr,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION out{volume_readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{stage.Volume(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        out.PlacedFootprint.Footprint={DXGI_FORMAT_R32G32B32A32_FLOAT,N,N,N,volume_pitch};gpu.list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
        barrier=Transition(stage.Volume(),D3D12_RESOURCE_STATE_COPY_SOURCE,psr);gpu.list->ResourceBarrier(1,&barrier);gpu.Run();
        std::vector<uint8_t> volume(t16.size());uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(volume_pitch)*N*N};Check(volume_readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control volume readback");
        for(UINT row=0;row<N*N;++row)std::memcpy(volume.data()+size_t(row)*N*16,memory+size_t(row)*volume_pitch,N*16);const D3D12_RANGE none{};volume_readback->Unmap(0,&none);
        Require(volume==t16,"the volume differs from the original's LUT");}
    // 2. The stages it replaces, on this device.
    struct Case {const char* name;bool glare;float vignette,factor;bool matrix,lut;};
    static constexpr Case cases[]{{"plain",false,0.0f,0.0f,false,false},{"glare",true,0.0f,0.0f,false,false},{"vignette",true,0.3f,0.0f,false,false},
        {"factor 0.37",true,0.0f,0.37f,false,false},{"factor 2.75 and vignette",true,0.2f,2.75f,false,false},{"hue, value, contrast",true,0.0f,0.0f,true,false},
        {"LUT",true,0.0f,0.0f,false,true},{"LUT, vignette, factor 0.61",true,0.35f,0.61f,false,true},{"LUT, hue, value, contrast, factor 1.9",false,0.0f,1.9f,true,true}};
    uint64_t compared{},unequal{},reinhard_compared{};int largest{};uint64_t generation=2,strip_generation=2;
    for(const bool synthetic:{false,true})for(const auto& c:cases){
        Inputs in{synthetic?synthetic_scene.Get():capture_scene.Get(),++generation,c.glare?glare.Get():nullptr,c.factor,record,c.vignette,nullptr,0};
        if(c.matrix){in.record.color_h=1.0f;in.record.color_v=0.9f;in.record.contrast=1.1f;}
        if(c.lut){in.strip=synthetic?strip2.Get():strip.Get();in.strip_generation=++strip_generation;}
        // I-0138 part 4c: each case also with the Reinhard curve (tone type 3) at three ToneMappingFactors (32, the two cutscene
        // records' 32 and 0.36, and 4), against the library's REINHARD programmes.
        for(const float mapping:{0.0f,32.0f,0.36f,4.0f}){Inputs case_in=in;if(mapping!=0.0f){case_in.record.tone_type=3u;case_in.record.mapping_factor=mapping;case_in.scene_generation=++generation;
                if(c.lut)case_in.strip_generation=++strip_generation;}
            const auto ours=draw_new(case_in),theirs=draw_old(case_in);const auto diff=Compare(ours,theirs);++compared;if(mapping!=0.0f)++reinhard_compared;
            if(diff.bytes){++unequal;largest=std::max(largest,diff.largest);
                std::printf("FAIL the pass against the stages it replaces, %s scene, %s, %s: %llu differing bytes, largest %d/255\n",synthetic?"synthetic":"captured",c.name,
                    case_in.record.tone_type==3u?"Reinhard":"sensitometric",static_cast<unsigned long long>(diff.bytes),diff.largest);}}}
    std::printf("the pass against the stages it replaces: %llu draws compared, %llu unequal, largest %d/255\n",static_cast<unsigned long long>(compared),static_cast<unsigned long long>(unequal),largest);
    // I-0138 tranche 8: the strip's copy is our own compute (cs_lut_copy) in place of GX's LutCopy2DTo3DCs: the last case drew both
    // stages with the synthetic strip, so their volumes hold the same strip copied by each programme; they must be equal bit for bit.
    {const auto read_volume=[&](ID3D12Resource* volume_resource){
            const UINT volume_pitch=256;const auto volume_readback=gpu.Buffer(D3D12_HEAP_TYPE_READBACK,UINT64(volume_pitch)*N*N,D3D12_RESOURCE_STATE_COPY_DEST);
            auto barrier=Transition(volume_resource,psr,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.list->ResourceBarrier(1,&barrier);
            D3D12_TEXTURE_COPY_LOCATION out{volume_readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{volume_resource,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
            out.PlacedFootprint.Footprint={DXGI_FORMAT_R32G32B32A32_FLOAT,N,N,N,volume_pitch};gpu.list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
            barrier=Transition(volume_resource,D3D12_RESOURCE_STATE_COPY_SOURCE,psr);gpu.list->ResourceBarrier(1,&barrier);gpu.Run();constant_at=0;
            std::vector<uint8_t> bytes(size_t(N)*N*N*16);uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(volume_pitch)*N*N};
            Check(volume_readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control volume readback");
            for(UINT row=0;row<N*N;++row)std::memcpy(bytes.data()+size_t(row)*N*16,memory+size_t(row)*volume_pitch,N*16);const D3D12_RANGE none{};volume_readback->Unmap(0,&none);
            return bytes;};
        const auto ours=read_volume(stage.Volume()),theirs=read_volume(old_grading.Volume());
        Require(ours==theirs,"our LUT copy differs from GX's LutCopy2DTo3DCs");
        // And against the strip itself, word for word (3D[x][y][z] = 2D[x][16z + y]: the strip's rows are z-major, each byte k read as k / 255).
        for(UINT z=0;z<N;++z)for(UINT y=0;y<N;++y)for(UINT x=0;x<N;++x)for(UINT k=0;k<4;++k){float v{};std::memcpy(&v,ours.data()+(((size_t(z)*N+y)*N+x)*4+k)*4,4);
            Require(v==float(double(synthetic_strip_bytes[((size_t(z)*N+y)*N+x)*4+k])/255.0),"our LUT copy is not the strip's texel");}
        std::printf("the LUT copy (ours) against GX's LutCopy2DTo3DCs and the strip: equal\n");}
    // 3. The HDR entry (I-0138 tranche 5).
    Require(stage.OpenHdr(DXGI_FORMAT_R16G16B16A16_FLOAT,reason),reason.c_str());
    const D3D12_CLEAR_VALUE float_sentinel{DXGI_FORMAT_R16G16B16A16_FLOAT,{-4.0f,-4.0f,-4.0f,-4.0f}};
    const auto float_target=gpu.Texture(DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,&float_sentinel);
    ComPtr<ID3D12DescriptorHeap> float_rtv;Check(gpu.device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&float_rtv)),"control float rtv heap");
    gpu.device->CreateRenderTargetView(float_target.Get(),nullptr,float_rtv->GetCPUDescriptorHandleForHeapStart());
    const UINT float_pitch=W*8;const auto float_readback=gpu.Buffer(D3D12_HEAP_TYPE_READBACK,UINT64(float_pitch)*H,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto from_half=[](uint16_t h){const uint32_t exponent=(h>>10)&31u,mantissa=h&0x3FFu;const float sign=(h&0x8000u)?-1.0f:1.0f;
        if(exponent==31u)return mantissa?std::numeric_limits<float>::quiet_NaN():sign*std::numeric_limits<float>::infinity();
        return exponent?sign*std::ldexp(float(mantissa|0x400u),int(exponent)-25):sign*std::ldexp(float(mantissa),-24);};
    PostToneParams hdr_block;   // the block of the last HDR draw (the model's)
    const auto draw_hdr=[&](const Inputs& in){
        const auto float_handle=float_rtv->GetCPUDescriptorHandleForHeapStart();gpu.list->ClearRenderTargetView(float_handle,float_sentinel.Color,0,nullptr);
        const auto t=yebis::BuildTonemap(d,in.record);Require(t.matrix_supported && t.guarded,"the record is served");
        const auto uv=vignette_of(in.vignette_power);hdr_block=BuildPostTone(t,0.0f<in.vignette_power?&uv:nullptr,N);ApplyToneLook(hdr_block,in.look,false,true,in.er_vivid);
        if(in.er_plain)for(float& word:hdr_block.er_dispersion)word=0.0f;
        if(in.grain)SetFilmGrain(hdr_block,in.grain,H,in.grain_seed);
        ID3D12Resource* adapted=nullptr;if(in.factor!=0.0f){kept.push_back(factor_texture(in.factor));adapted=kept.back().Get();}
        const PostToneLut lut{in.strip,DXGI_FORMAT_R8G8B8A8_UNORM,in.strip_generation};
        stage.Record(gpu.list.Get(),in.scene,in.scene_generation,float_handle,hdr_block,[&](uint64_t bytes){const auto m=allocate(bytes);return PostToneConstantMemory{m.first,m.second};},
            in.glare,in.glare?1u:0u,adapted,in.strip?&lut:nullptr,true);
        auto barrier=Transition(float_target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);gpu.list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION out{float_readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{float_target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        out.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,1,float_pitch};gpu.list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
        barrier=Transition(float_target.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);gpu.list->ResourceBarrier(1,&barrier);gpu.Run();constant_at=0;
        std::vector<float> texels(size_t(W)*H*4);uint16_t* memory{};const D3D12_RANGE read{0,SIZE_T(float_pitch)*H};
        Check(float_readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control float readback");
        for(size_t i=0;i<texels.size();++i)texels[i]=from_half(memory[i]);const D3D12_RANGE none{};float_readback->Unmap(0,&none);return texels;};
    uint64_t hdr_draws{},hdr_under{},hdr_over{},hdr_modelled{},hdr_graded{},hdr_graded_under{},hdr_clamped{},hdr_at_knee{};double hdr_worst_graded{};double hdr_worst_under{},hdr_worst_lut{},hdr_worst_model{},hdr_lut_bound{};bool hdr_ok=true;
    struct HdrCase {const char* name;bool glare;float vignette,factor;bool lut;};
    static constexpr HdrCase hdr_cases[]{{"plain",false,0.0f,0.0f,false},{"glare",true,0.0f,0.0f,false},{"vignette, factor 0.61",true,0.3f,0.61f,false},{"LUT",true,0.0f,0.0f,true}};
    for(const bool synthetic:{false,true})for(const auto& c:hdr_cases)for(const float mapping:{0.0f,32.0f}){
        Inputs in{synthetic?synthetic_scene.Get():capture_scene.Get(),++generation,c.glare?glare.Get():nullptr,c.factor,record,c.vignette,nullptr,0};
        if(mapping!=0.0f){in.record.tone_type=3u;in.record.mapping_factor=mapping;}
        if(c.lut){in.strip=synthetic?strip2.Get():strip.Get();in.strip_generation=++strip_generation;}
        const auto bytes=draw_new(in);in.scene_generation=++generation;if(c.lut)in.strip_generation=++strip_generation;
        // With a LUT the graded value does not say where the knee is: the same inputs drawn by the entry without the LUT do.
        std::vector<float> ungraded;if(c.lut){auto plain=in;plain.strip=nullptr;plain.strip_generation=0;ungraded=draw_hdr(plain);in.scene_generation=++generation;}
        const auto floats=draw_hdr(in);++hdr_draws;
        // The gain is 1 where the curve's largest channel is under the knee. The entry's own output says where: decoded (the power
        // 1 / inv_gamma) and taken back through the colour matrix's inverse, it is the curve's output where the gain is 1 and none
        // of its channels was clamped, and a larger one where the gain is above 1 (the gain scales it). A texel is « under » when
        // no channel is at a clamp and that value's largest channel is under 0.97 of the knee; any other texel is only required
        // not to be under the 8-bit draw.
        const double knee=double(hdr_block.hdr_knee);double inverse[3][3]{};
        {const auto& q=hdr_block.colour;   // the 3x3 part's inverse by its cofactors
            const auto cofactor=[&](int r,int k){const int r1=(r+1)%3,r2=(r+2)%3,k1=(k+1)%3,k2=(k+2)%3;return double(q[r1][k1])*double(q[r2][k2])-double(q[r1][k2])*double(q[r2][k1]);};
            const double determinant=double(q[0][0])*cofactor(0,0)+double(q[0][1])*cofactor(0,1)+double(q[0][2])*cofactor(0,2);
            Require(std::abs(determinant)>1e-6,"the HDR case's colour matrix has no inverse");
            for(int r=0;r<3;++r)for(int k=0;k<3;++k)inverse[r][k]=cofactor(k,r)/determinant;}
        // The LUT's bound (the header): one level of input, the two reads' filter weights (8 fractional bits each: 1/256 of a node
        // step of 17 levels), the two-draw form's rounding (0.5) and a half float's last place under 1 (255 / 2048 = 0.12, an upper
        // value: half of it is what round to nearest needs). StripSlope sums a node's three forward differences: a strip whose
        // steepest edges do not meet at one node could pass it (a false failure then, never a false pass).
        const double bound=c.lut?StripSlope(synthetic?synthetic_strip_bytes:strip_bytes)*(1.0+2.0*17.0/256.0)+0.62:1.1;hdr_lut_bound=std::max(hdr_lut_bound,c.lut?bound:0.0);
        const uint8_t* scene_texels=synthetic?synthetic_bytes.data():scene_bytes.data();
        for(size_t texel=0;texel<size_t(W)*H;++texel){const float* h=floats.data()+texel*4;const uint8_t* s=bytes.data()+texel*4;
            if(!std::isfinite(h[0]) || !std::isfinite(h[1]) || !std::isfinite(h[2]) || !std::isfinite(h[3])){hdr_ok=false;continue;}
            // A clamp of the colour matrix's output: exactly 0 (the programme's saturate, then its power), or 1 and above.
            const float* const where=c.lut?ungraded.data()+texel*4:h;bool under=true;double decoded[3]{};
            for(int k=0;k<3 && under;++k){decoded[k]=std::pow(std::max(double(where[k]),0.0),1.0/double(hdr_block.inv_gamma));under=where[k]>0.0f && decoded[k]<0.9999;
                decoded[k]-=double(hdr_block.colour[k][3])+2.384185791015625e-7;}
            if(!under)++hdr_clamped;
            else{for(int r=0;r<3 && under;++r)under=inverse[r][0]*decoded[0]+inverse[r][1]*decoded[1]+inverse[r][2]*decoded[2]<0.97*knee;
                if(!under)++hdr_at_knee;}
            if(c.lut)++(under?hdr_graded_under:hdr_graded);
            // Under the knee the entry is the 8-bit draw within the bound, graded or not (the final review's L2: a graded texel
            // was only held from below); elsewhere it is never under it by more than the bound.
            for(int k=0;k<3;++k){const double difference=double(h[k])*255.0-double(s[k]);
                if(under){double& worst=c.lut?hdr_worst_graded:hdr_worst_under;worst=std::max(worst,std::abs(difference));if(std::abs(difference)>bound)hdr_ok=false;}
                else{if(c.lut)hdr_worst_lut=std::max(hdr_worst_lut,-difference);if(difference<-bound)hdr_ok=false;}}
            if(std::abs(double(h[3])*255.0-double(s[3]))>0.65)hdr_ok=false;   // the 8-bit rounding and the half float's last place
            ++(under?hdr_under:hdr_over);
            // The model: the plain sensitometric case, a finite scene texel.
            if(c.glare || c.lut || c.vignette!=0.0f || c.factor!=0.0f || in.record.tone_type==3u)continue;   // also a capture whose own record is the Reinhard curve's (the opening scene's)
            double x[3]{},y[4]{0,0,0,1},colour[3]{};bool finite=true;
            for(int k=0;k<3;++k){uint16_t half{};std::memcpy(&half,scene_texels+texel*8+size_t(k)*2,2);const float value=from_half(half);finite=finite && std::isfinite(value);
                x[k]=std::max(double(value)*double(hdr_block.exposure),0.00006103515625);const double e=std::exp(-x[k]*double(hdr_block.curve_a)),shoulder=1.0-e*double(hdr_block.curve_b);
                y[k]=std::min(std::max((1.0-e)*shoulder*shoulder,0.0),1.0);}
            if(!finite)continue;
            for(int r=0;r<3;++r){double v=2.384185791015625e-7;for(int k=0;k<4;++k)v+=y[k]*double(hdr_block.colour[r][k]);colour[r]=std::min(std::max(v,0.0),1.0);}
            const double m=std::max(y[0],std::max(y[1],y[2])),gain=m>knee?hdr::Highlight(m,knee)/m:1.0;   // the curve's output (no glare in this case)
            if(1.0-m<1e-4)continue;   // so close to 1 that the programme's single-precision value decides the logarithm: left to the checks above
            for(int k=0;k<3;++k){const double expected=std::pow(colour[k],double(hdr_block.inv_gamma))*std::pow(gain,1.0/double(hdr::kSdrGamma)),error=std::abs(double(h[k])-expected);
                hdr_worst_model=std::max(hdr_worst_model,error/std::max(expected,0.05));if(error>expected*2e-3+1e-4)hdr_ok=false;}
            ++hdr_modelled;}}
    std::printf("the HDR entry: %llu draws; %llu texels under the knee (%llu of them graded) within %.3f/255 of the 8-bit draw, graded within %.3f/255 of the two-draw form (the bound from the strips' slope %.3f); "
        "%llu only required not to be under it (%llu with a channel at a clamp, %llu at or above 0.97 of the knee; %llu of them graded, never under by more than %.3f/255); %llu texels against the model, worst %.5f of the value\n",
        static_cast<unsigned long long>(hdr_draws),static_cast<unsigned long long>(hdr_under),static_cast<unsigned long long>(hdr_graded_under),hdr_worst_under,hdr_worst_graded,hdr_lut_bound,
        static_cast<unsigned long long>(hdr_over),static_cast<unsigned long long>(hdr_clamped),static_cast<unsigned long long>(hdr_at_knee),static_cast<unsigned long long>(hdr_graded),hdr_worst_lut,
        static_cast<unsigned long long>(hdr_modelled),hdr_worst_model);
    Require(hdr_ok,"the HDR entry differs from the 8-bit draws or from its model beyond its bounds");
    Require(hdr_under>0 && hdr_graded_under>0 && hdr_over>0 && (hdr_modelled>0 || record.tone_type==3u),"the HDR entry's checks did not all run");   // no model of the Reinhard curve here: a capture of a Reinhard record has none
    // 4. The candidate looks (I-0138 tranche 6).
    uint64_t look_draws{},look_texels{},look_gained{};double look_worst_float{},look_worst_byte{};bool look_ok=true;
    uint64_t texels_of[4]{},gained_of[4]{},er_differing[4]{},er_dispersed{};double er_worst_dispersed{};
    const auto dispersion=look::ErDispersionWords();
    for(const auto candidate:{look::ToneLook::Agx,look::ToneLook::Aces,look::ToneLook::Er}){
        Require(stage.OpenLook(uint32_t(candidate),false,reason) && stage.OpenLook(uint32_t(candidate),true,reason),reason.c_str());
        for(const bool synthetic:{false,true}){
            Inputs in{synthetic?synthetic_scene.Get():capture_scene.Get(),++generation,nullptr,0.0f,record,0.0f,nullptr,0};in.look=candidate;in.er_plain=true;
            const auto bytes=draw_new(in);in.scene_generation=++generation;
            std::vector<uint8_t> with;   // the candidate after Elden Ring with its dispersion
            if(candidate==look::ToneLook::Er){auto dispersed=in;dispersed.er_plain=false;with=draw_new(dispersed);in.scene_generation=++generation;
                for(size_t i=0;i<bytes.size();++i)if(with[i]!=bytes[i])++er_differing[i&3];}
            const auto floats=draw_hdr(in);++look_draws;
            Require(((hdr_block.flags&kPostToneLookMask)>>kPostToneLookShift)==uint32_t(candidate) && hdr_block.look_scale>0.0f,"the look's block");
            const double knee=double(hdr_block.hdr_knee);const uint8_t* scene_texels=synthetic?synthetic_bytes.data():scene_bytes.data();
            const look::Tangent tangent{double(hdr_block.look_a),double(hdr_block.look_b)};   // the block's own words, as the programme reads them
            Require(tangent.slope>0.0 && std::abs(look::Grey(candidate,tangent.input)-knee)<1e-5,"the look's tangent");
            for(size_t texel=0;texel<size_t(W)*H;++texel){const float* h=floats.data()+texel*4;const uint8_t* s=bytes.data()+texel*4;
                look::Vec x{};bool finite=true;
                for(int k=0;k<3;++k){uint16_t half{};std::memcpy(&half,scene_texels+texel*8+size_t(k)*2,2);const float value=from_half(half);finite=finite && std::isfinite(value);
                    x[k]=std::min(std::max(double(value)*double(hdr_block.exposure),0.00006103515625)*double(hdr_block.look_scale),65504.0);}
                if(!finite)continue;
                if(!std::isfinite(h[0]) || !std::isfinite(h[1]) || !std::isfinite(h[2])){look_ok=false;continue;}
                look::Vec y{};
                // The candidate after Elden Ring with its dispersion: the green and the blue bytes against the model of the
                // channel's read at its place (the header).
                if(candidate==look::ToneLook::Er){
                    const double u=(double(texel%W)+0.5)/double(W),v=(double(texel/W)+0.5)/double(H),gamma=double(hdr_block.inv_gamma);
                    for(int channel=1;channel<3;++channel){
                        const double pu=u+double(float(dispersion[channel==1?0:2]))*(2.0*u-1.0),pv=v+double(float(dispersion[channel==1?1:3]))*(2.0*v-1.0);
                        const double ax=pu*double(W)-0.5,ay=pv*double(H)-0.5,bx=std::floor(ax),by=std::floor(ay),fx=ax-bx,fy=ay-by;double blend{};
                        for(int j=0;j<2;++j)for(int i=0;i<2;++i){
                            const size_t at=size_t(std::min<long long>(std::max<long long>(static_cast<long long>(by)+j,0),H-1))*W+size_t(std::min<long long>(std::max<long long>(static_cast<long long>(bx)+i,0),W-1));
                            uint16_t half{};std::memcpy(&half,scene_texels+at*8+size_t(channel)*2,2);const double exposed=double(from_half(half))*double(hdr_block.exposure);
                            const double c=std::min((std::isnan(exposed)?0.00006103515625:std::max(exposed,0.00006103515625))*double(hdr_block.look_scale),65504.0);
                            blend+=(i?fx:1.0-fx)*(j?fy:1.0-fy)*std::pow(look::ErLift(look::ErCurve(c)),gamma);}
                        const double encoded=std::pow(std::min(std::max(std::pow(blend,1.0/gamma)+2.384185791015625e-7,0.0),1.0),gamma),error=std::abs(double(with[texel*4+size_t(channel)])-255.0*encoded);
                        er_worst_dispersed=std::max(er_worst_dispersed,error);if(error>1.2+255.0*(encoded*5e-3+5e-4))look_ok=false;}
                    ++er_dispersed;}
                if(candidate==look::ToneLook::Er){for(int k=0;k<3;++k)y[k]=look::ErLift(look::ErCurve(x[k]));}
                else y=candidate==look::ToneLook::Agx?look::Agx(x):look::Aces(x);
                const double y1[4]{y[0],y[1],y[2],1.0};
                const double gain=look::HighlightGain(candidate,tangent,knee,std::max(x[0],std::max(x[1],x[2])));if(gain>1.0){++look_gained;++gained_of[size_t(candidate)];}
                for(int r=0;r<3;++r){double v=2.384185791015625e-7;for(int k=0;k<4;++k)v+=y1[k]*double(hdr_block.colour[r][k]);
                    const double encoded=std::pow(std::min(std::max(v,0.0),1.0),double(hdr_block.inv_gamma)),expected=encoded*std::pow(gain,1.0/double(hdr::kSdrGamma));
                    const double float_error=std::abs(double(h[r])-expected),byte_error=std::abs(double(s[r])-255.0*encoded);
                    look_worst_float=std::max(look_worst_float,float_error-expected*5e-3);look_worst_byte=std::max(look_worst_byte,byte_error);
                    if(float_error>expected*5e-3+5e-4 || byte_error>1.1+255.0*(encoded*5e-3+5e-4))look_ok=false;}
                ++look_texels;++texels_of[size_t(candidate)];}}}
    std::printf("the candidate looks: %llu draw pairs, %llu texels against the models (%llu above the knee's tangent point); the HDR entries' largest error beyond 0.5 %% of the value %.6f (5e-4 allowed), the 8-bit entries' largest %.3f/255\n",
        static_cast<unsigned long long>(look_draws),static_cast<unsigned long long>(look_texels),static_cast<unsigned long long>(look_gained),look_worst_float,look_worst_byte);
    Require(look_ok,"a candidate look's entry differs from its model beyond its bounds");
    Require(look_draws==6,"the candidate looks' draws did not all run");
    for(const auto candidate:{look::ToneLook::Agx,look::ToneLook::Aces,look::ToneLook::Er})
        Require(texels_of[size_t(candidate)]>size_t(W)*H && gained_of[size_t(candidate)]>0,"a candidate look's checks did not all run");   // over its two scenes: more than one scene's worth
    std::printf("the candidate after Elden Ring: %llu texels against its model, %llu above its tangent point; with its dispersion %llu texels' green and blue bytes against the model, largest %.3f/255; "
        "against the draws without it %llu red, %llu green, %llu blue and %llu alpha bytes differ\n",static_cast<unsigned long long>(texels_of[3]),static_cast<unsigned long long>(gained_of[3]),
        static_cast<unsigned long long>(er_dispersed),er_worst_dispersed,static_cast<unsigned long long>(er_differing[0]),static_cast<unsigned long long>(er_differing[1]),
        static_cast<unsigned long long>(er_differing[2]),static_cast<unsigned long long>(er_differing[3]));
    Require(er_differing[0]==0 && er_differing[3]==0 && er_differing[1]>0 && er_differing[2]>0 && er_dispersed>size_t(W)*H,"the lateral dispersion: the red or the alpha moved, or the green or the blue did not, or its model did not run");
    {   // I-0138 tranche 7b: the vivid variant's 8-bit entry (without the dispersion) against its model, look::ErVivid on the encoded
        // value the section's model gives, both scenes; its bytes differ from the draw without it (the bound 1.6 levels: section 4's 1.1,
        // with room for the steps' slope on the model's float error, the contrast's up to 1.2 and the vibrance's 1.25;
        // observed 1.04 on the card, 0.98 on WARP). Its HDR entry (the step review's M1) against the same model times
        // the highlights' gain, every value finite, some texels above the knee.
        uint64_t vivid_texels{},vivid_moved{},vivid_hdr{},vivid_gained{};double vivid_worst{},vivid_worst_hdr{};bool vivid_ok=true;
        for(const bool synthetic:{false,true}){
            Inputs in{synthetic?synthetic_scene.Get():capture_scene.Get(),++generation,nullptr,0.0f,record,0.0f,nullptr,0};in.look=look::ToneLook::Er;in.er_plain=true;
            const auto plain=draw_new(in);in.scene_generation=++generation;in.er_vivid=true;const auto bytes=draw_new(in);
            in.scene_generation=++generation;const auto floats=draw_hdr(in);
            Require(hdr_block.er_lift[1]==float(look::kErVividContrast),"the vivid variant's HDR block");
            const look::Tangent tangent{double(hdr_block.look_a),double(hdr_block.look_b)};const double knee=double(hdr_block.hdr_knee);
            const auto t=yebis::BuildTonemap(d,record);auto block=BuildPostTone(t,nullptr,N);ApplyToneLook(block,look::ToneLook::Er,false);
            const uint8_t* scene_texels=synthetic?synthetic_bytes.data():scene_bytes.data();
            for(size_t texel=0;texel<size_t(W)*H;++texel){look::Vec x{};bool finite=true;
                for(int k=0;k<3;++k){uint16_t half{};std::memcpy(&half,scene_texels+texel*8+size_t(k)*2,2);const float value=from_half(half);finite=finite && std::isfinite(value);
                    x[k]=std::min(std::max(double(value)*double(block.exposure),0.00006103515625)*double(block.look_scale),65504.0);}
                if(!finite)continue;
                look::Vec e{};for(int k=0;k<3;++k)e[k]=std::pow(std::min(std::max(look::ErLift(look::ErCurve(x[k]))+2.384185791015625e-7,0.0),1.0),double(block.inv_gamma));
                const auto v=look::ErVivid(e);
                for(int k=0;k<3;++k){const double error=std::abs(double(bytes[texel*4+size_t(k)])-255.0*v[k]);vivid_worst=std::max(vivid_worst,error);
                    if(error>1.6+255.0*(v[k]*5e-3+5e-4))vivid_ok=false;}
                const float* h=floats.data()+texel*4;
                if(!std::isfinite(h[0]) || !std::isfinite(h[1]) || !std::isfinite(h[2]))vivid_ok=false;
                else{const double gain=look::HighlightGain(look::ToneLook::Er,tangent,knee,std::max(x[0],std::max(x[1],x[2])));if(gain>1.0)++vivid_gained;
                    for(int k=0;k<3;++k){const double expected=v[k]*std::pow(gain,1.0/double(hdr::kSdrGamma)),error=std::abs(double(h[k])-expected);
                        vivid_worst_hdr=std::max(vivid_worst_hdr,error-expected*5e-3);if(error>expected*5e-3+5e-4)vivid_ok=false;}
                    ++vivid_hdr;}
                if(bytes[texel*4]!=plain[texel*4] || bytes[texel*4+1]!=plain[texel*4+1] || bytes[texel*4+2]!=plain[texel*4+2])++vivid_moved;
                ++vivid_texels;}}
        std::printf("the vivid variant: %llu texels against its model, largest %.3f/255; %llu texels moved from the draw without it\n",
            static_cast<unsigned long long>(vivid_texels),vivid_worst,static_cast<unsigned long long>(vivid_moved));
        std::printf("the vivid variant's HDR entry: %llu texels against its model (%llu above the knee), largest error beyond 0.5 %% of the value %.6f\n",
            static_cast<unsigned long long>(vivid_hdr),static_cast<unsigned long long>(vivid_gained),vivid_worst_hdr);
        Require(vivid_ok && vivid_texels>size_t(W)*H && vivid_moved*2>vivid_texels && vivid_hdr>size_t(W)*H && vivid_gained>0,"the vivid variant differs from its model, or did not run, or moved too little");}
    // I-0141 tranche 11 (Thomas 2026-11-17: DS3's vignette by area in the Rekindled look, Elden Ring's dropped): the area's own vignette in
    // the ER candidate's body, on the exposed scene before the curve as the original's. On the capture's scene, the candidate without its
    // dispersion (every channel in place): with the record's vignette (power 0.3) against the same draw without one, no texel brighter
    // (the factor is at most 1 and the curve rises; the same dither), many darker, the centre unchanged; with the vignette's coordinates
    // but bit 0 cleared, the draw without one (the bit alone draws it: the player's row).
    {Inputs in{capture_scene.Get(),++generation,nullptr,0.0f,record,0.0f,nullptr,0};in.look=look::ToneLook::Er;in.er_plain=true;
        const auto without=draw_new(in);in.scene_generation=++generation;in.vignette_power=0.3f;const auto with=draw_new(in);
        uint64_t darker{},brighter{};for(size_t i=0;i<size_t(W)*H*4;++i){if(i%4==3)continue;if(with[i]<without[i])++darker;else if(with[i]>without[i])++brighter;}
        const size_t centre=(size_t(H/2)*W+W/2)*4;bool centre_same=true;for(int k=0;k<3;++k)centre_same=centre_same && std::abs(int(with[centre+k])-int(without[centre+k]))<=1;
        in.no_vignette_bit=true;in.scene_generation=++generation;const auto cleared=draw_new(in);
        std::printf("the area's vignette in the ER body: %llu channel values darker, %llu brighter; its coordinates without the bit %s\n",static_cast<unsigned long long>(darker),
            static_cast<unsigned long long>(brighter),cleared==without?"draw none":"DRAW ONE");
        Require(brighter==0 && darker*20>size_t(W)*H*3 && centre_same && cleared==without,"the area's vignette in the ER body: not darker toward the edges only, or drawn without the bit");}
    // 5. I-0138 tranche 7: the film grain. Off (every draw above), the entries' bytes are the ones checked there. At 100 % on the
    // capture's scene, against the same draw without it: in the 8-bit entry alone, with the synthetic LUT (the grain added after the
    // LUT's read: a grain before it would leave the LUT's bend in its three channels' differences) and in the HDR entry; where no
    // channel is within the grain's reach of a clamp (its bound below; the HDR entry's values up to 0.94), each pixel's three channels moved by one value
    // (monochrome: within the dither's and the rounding's one level on WARP, two on the card, whose float to UNORM conversion is not
    // a rounding to the nearest (shaders/post_tone.hlsl's head: INFERRED the cause, 2.00 the card's observed maximum); 1.5/255 in HDR), the mean move
    // under half a level (no tint, no darkening), at most the strength's own bound (kFilmGrainMost and two levels), and more than
    // half the pixels moved; a second seed moves another grain.
    {struct GrainCase {const char* name;bool lut,hdr;};uint64_t grain_checked{};
        for(const GrainCase c:{GrainCase{"8-bit",false,false},GrainCase{"8-bit with the LUT",true,false},GrainCase{"HDR",false,true}}){
            Inputs in{capture_scene.Get(),++generation,glare.Get(),0.0f,record,0.0f,nullptr,0};
            if(c.lut){in.strip=strip2.Get();in.strip_generation=++strip_generation;}
            std::vector<double> plain,grained,other;
            const auto draw=[&](uint32_t grain,uint32_t seed){Inputs g=in;g.grain=grain;g.grain_seed=seed;std::vector<double> v(size_t(W)*H*4);
                if(c.hdr){const auto f=draw_hdr(g);for(size_t i=0;i<v.size();++i)v[i]=double(f[i])*255.0;}
                else{const auto b=draw_new(g);for(size_t i=0;i<v.size();++i)v[i]=double(b[i]);}
                return v;};
            plain=draw(0,0);grained=draw(100,1);other=draw(100,2);
            const double most=255.0*double(kFilmGrainMost)+2.0,low=most,high=c.hdr?0.94*255.0:255.0-most,spread=c.hdr?1.5:warp?1.0:2.0;
            double sum[3]{},worst_spread{},worst{};uint64_t inside{},moved{},seeded{};
            for(size_t t=0;t<size_t(W)*H;++t){const double* p=plain.data()+t*4;const double* g=grained.data()+t*4;
                bool in_range=true;for(int k=0;k<3;++k)in_range=in_range && std::isfinite(p[k]) && p[k]>=low && p[k]<=high && std::isfinite(g[k]);
                if(!in_range)continue;++inside;
                const double dr=g[0]-p[0],dg=g[1]-p[1],db=g[2]-p[2];sum[0]+=dr;sum[1]+=dg;sum[2]+=db;
                worst_spread=std::max(worst_spread,std::max(std::abs(dr-dg),std::max(std::abs(dg-db),std::abs(dr-db))));
                worst=std::max(worst,std::max(std::abs(dr),std::max(std::abs(dg),std::abs(db))));
                if(dr!=0.0 || dg!=0.0 || db!=0.0)++moved;if(other[t*4+1]!=g[1])++seeded;
                Require(g[3]==p[3],"the film grain moved the alpha");}
            Require(inside*4>uint64_t(W)*H,"the film grain's check: under a quarter of the pixels away from a clamp");
            const double mean[3]{sum[0]/double(inside),sum[1]/double(inside),sum[2]/double(inside)};
            std::printf("the film grain at 100 %%, %s: %llu pixels away from a clamp, %llu moved, %llu other with a second seed; mean move %.3f %.3f %.3f, largest %.2f, channels apart by at most %.2f (levels of 255)\n",
                c.name,static_cast<unsigned long long>(inside),static_cast<unsigned long long>(moved),static_cast<unsigned long long>(seeded),mean[0],mean[1],mean[2],worst,worst_spread);
            Require(worst_spread<=spread,"the film grain is not monochrome (or the LUT bends it: added before the LUT)");
            Require(std::abs(mean[0])<0.5 && std::abs(mean[1])<0.5 && std::abs(mean[2])<0.5,"the film grain moves the mean");
            Require(worst<=most && moved*2>inside && seeded*2>inside,"the film grain's strength, its share of moved pixels or its seed");
            ++grain_checked;}
        Require(grain_checked==3,"the film grain's cases did not all run");}
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(gpu.device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t messages{};
    for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::fprintf(stderr,"%s\n",message->pDescription);}}
    Require(messages==0,"debug-layer messages");
    Require(unequal==0,"the pass differs from the stages it replaces");
    Require(reinhard_compared==2*std::size(cases)*3,"the Reinhard cases were not all compared");
    CloseHandle(gpu.event);
    const std::string chained=t16.empty()?std::string():", its LUT draw the original's grading output, its volume bit for bit, the two draws chained within "+std::to_string(chain_diff.largest)+"/255"+(same?"":" (reported)");
    std::printf("PASS post tone stage %s: %s: the original's tone-map inputs through our pass equal its output within %d/255 each draw%s (%s); the %llu draws against the stages it "
        "replaces byte for byte; no debug-layer message\n",argv[1],dir.substr(dir.find_last_of("/\\")+1).c_str(),std::max(tone_diff.largest,lut_diff.largest),chained.c_str(),
        same?"the capture's adapter and driver: exactness required":"another adapter, driver or WARP: each draw within 1/255",static_cast<unsigned long long>(compared));
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
