// I-0113 tranche 27: the menu views' GX tone map (world/menu_tonemap). The second Tone Map block read from the group's entries
// in file order, and TabledToneMapPS's output for default.gparam ids 12 (the Quit Game portrait, mode 0, Gamma 1.8) and 11
// (Status, mode 7), against values computed independently from the disassembled programmes (evidence/I-0113/t27/tonemap).
#include "../world/menu_tonemap.h"
#include <windows.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include "offscreen_composition_vs.dxbc.h"   // I-0132 R12h step 2: the bytes the renderer uses (shaders/offscreen_composition.hlsl)
#include "offscreen_composition_ps.dxbc.h"
#include "offscreen_composition_backgroundps.dxbc.h"
#include <cmath>
#include <utility>
#include <cstdio>
#include <cstring>

using namespace ds3native::menu_tone;
namespace {
int failures{},checks{};
uint32_t Word(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
void Near(const char* what,float got,float expected,float tolerance) {
    ++checks;if(std::fabs(got-expected)<=tolerance)return;++failures;std::printf("FAIL %s: %g, expected %g (+-%g)\n",what,got,expected,tolerance);}
}
int main() {
    // The group's entries in file order, as default.gparam id 12 holds them (evidence/I-0067/queries/status-light/default-gparam.tsv),
    // shortened to the names the reader uses plus decoys of the first block.
    std::vector<Entry> entries{{"Exposure",9,Word(2.0f)},{"Gamma",9,Word(2.2f)},{"ToneType",3,2},{"FilmicContrast",9,Word(0.5f)},
        {"Enable",3,0},{"Exposure",9,Word(2.8f)},{"AvgLuminance",9,Word(3.0f)},{"LuminanceBalance R",9,Word(1.0f)},
        {"Scaler R",9,Word(1.0f)},{"Scaler G",9,Word(1.0f)},{"Scaler B",9,Word(1.0f)},{"ToneMapMode",3,0},{"Contrast",9,Word(0.5f)},
        {"Gamma",9,Word(1.8f)},{"UseTable",3,1},{"Enable AutoExposure",3,0}};
    Params p;std::string refused;
    ++checks;if(!FromEntries(entries,p,refused)){++failures;std::printf("FAIL FromEntries: %s\n",refused.c_str());}
    Near("id 12 exposure (second block)",p.exposure,2.8f,0);Near("id 12 gamma (second block)",p.gamma,1.8f,0);
    Near("id 12 avg luminance",p.avg_luminance,3.0f,0);Near("id 12 mode",static_cast<float>(p.mode),0,0);
    ++checks;if(!Tabled(p)){++failures;std::printf("FAIL id 12 not on the tabled path\n");}
    Near("id 12 scale",Scale(p),std::exp2(2.8f)/3.0f,1e-6f);
    // Outputs x 255 (an independent Python transcription of the two programmes and the 8-bit table, linear filter, clamp).
    const float linear[]{0.0f,0.01f,0.0872f,0.18f,0.5f,1.0f,2.0f};
    const float id12[]{0.0f,20.0f,119.0f,163.04f,212.0f,231.28f,243.0f};
    const float id11[]{2.0f,30.08f,92.0f,142.54f,220.14f,245.0f,253.86f};
    Params s{7,4.60001f,9.0f,0.5f,2.2f,{1,1,1},true};
    for(size_t i=0;i<std::size(linear);++i){char what[64];
        std::snprintf(what,sizeof what,"id 12 at %g",linear[i]);Near(what,Apply(p,linear[i],0)*255.0f,id12[i],0.05f);
        std::snprintf(what,sizeof what,"id 11 at %g",linear[i]);Near(what,Apply(s,linear[i],1)*255.0f,id11[i],0.05f);}
    // A parameter of the wrong gparam type refuses (the step review's L1).
    {auto typed=entries;typed[13].type=3;Params t;++checks;
     if(FromEntries(typed,t,refused) || refused!="type of Gamma"){++failures;std::printf("FAIL wrong type accepted (%s)\n",refused.c_str());}}
    // A missing parameter of the second block refuses.
    entries.pop_back();entries.pop_back();Params q;++checks;
    if(FromEntries(entries,q,refused) || refused!="no UseTable"){++failures;std::printf("FAIL missing UseTable accepted (%s)\n",refused.c_str());}
    // Scaler applies per channel.
    Params r=p;r.scaler={0.5f,1.0f,2.0f};Near("scaler R",Apply(r,0.18f,0)*255.0f,163.04f*0.5f,0.05f);Near("scaler B",Apply(r,0.18f,2)*255.0f,163.04f*2.0f,0.1f);
    // The final review's M4: the renderer's composition programmes, the bytes it uses (I-0132 R12h step 2: built by fxc from
    // shaders/offscreen_composition.hlsl), each of its stage at shader model 5.
    {struct Built {const char* entry;const BYTE* bytes;size_t size;UINT type;};
     for(const Built& b:{Built{"VS",offscreen_composition_vs_dxbc,sizeof(offscreen_composition_vs_dxbc),UINT(D3D12_SHVER_VERTEX_SHADER)},
                         Built{"PS",offscreen_composition_ps_dxbc,sizeof(offscreen_composition_ps_dxbc),UINT(D3D12_SHVER_PIXEL_SHADER)},
                         Built{"BackgroundPS",offscreen_composition_backgroundps_dxbc,sizeof(offscreen_composition_backgroundps_dxbc),UINT(D3D12_SHVER_PIXEL_SHADER)}}){
        ++checks;ID3D12ShaderReflection* shape{};D3D12_SHADER_DESC sd{};
        const bool ok=SUCCEEDED(D3DReflect(b.bytes,b.size,IID_PPV_ARGS(&shape))) && SUCCEEDED(shape->GetDesc(&sd)) &&
            D3D12_SHVER_GET_TYPE(sd.Version)==b.type && D3D12_SHVER_GET_MAJOR(sd.Version)==5;
        if(shape)shape->Release();
        if(!ok){++failures;std::printf("FAIL built %s\n",b.entry);}}}
    // I-0113 tranche 29: the background quad's colour, FUN_140112b40 on the packed BG Color: 18/255 (default.gparam's Setting Info
    // {1, 11} and {1, 12}: 0.070588) gives round(255 (18/255)^2) = round(1.27) = 1, alpha unsquared; 128/255 gives round(64.25) = 64;
    // white stays white, out of range clamps, NaN gives 0.
    {const auto bq=BackgroundQuadColour({0.070588f,0.070588f,0.070588f,1.0f});
     Near("background R",bq[0]*255.0f,1.0f,1e-4f);Near("background B",bq[2]*255.0f,1.0f,1e-4f);Near("background alpha",bq[3],1.0f,0);
     const auto h=BackgroundQuadColour({128.0f/255.0f,1.0f,2.0f,0.5f});
     Near("background 128",h[0]*255.0f,64.0f,1e-4f);Near("background white",h[1],1.0f,0);Near("background clamped",h[2],1.0f,0);Near("background alpha 0.5",h[3]*255.0f,128.0f,1e-4f);
     const auto n=BackgroundQuadColour({std::nanf(""),-1.0f,0.0f,1.0f});Near("background NaN",n[0],0.0f,0);Near("background negative",n[1],0.0f,0);}
    std::printf("menu_tonemap_test: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
