// I-0138 tranche 1 (ROADMAP workstream 11): our tone-map pass's parameter block (world/post_tone.h, renderer/post_params.h).
//  1. The block built from the m37 Tone Map record (Exposure 1.52, Gamma 2.2, FilmicContrast 0.3, ColorH 0, ColorS 0.9, ColorV 1)
//     and the executable's data words holds, bit for bit, the words the original's tone-map draw bound at m37 (Reference1632 frame
//     1200 draw 45, the goldens of yebis_tonemap_test: `$Globals` words 616, 617, 620-623, 808-810, 832, 833, 840-851) and its
//     grading draw's LUT scale and offset (Reference1696 draw 50, bytes 2976 and 2992: 15/16 and 1/32, the values T17 compares).
//  2. The C++ block's layout is the compiled programmes' cbuffer (D3DReflect on the build's post_tone_vs, post_tone_ps and
//     post_tone_lut_ps).
//  3. I-0138 tranche 6: the candidate looks' models (renderer/tone_looks.h) against values computed apart (a Python script
//     of the published arithmetic in double, I-0138 RESULT « Tranche 6 »): AgX after Filament and ACES by Stephen Hill's fit,
//     at a grey, two saturated colours, a bright grey and black, within 1e-8; their grey inputs; and ApplyToneLook's bits,
//     scale (the record's curve and the candidate both at 0.18 for one scene value, within 1e-6) and neutral matrix.
// Argument: the executable (DS3_EXE for Rdata); without it part 1 is skipped with a message.
#include "../world/post_tone.h"
#include "../world/light_producers.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "post_tone_vs.dxbc.h"
#include "post_tone_ps.dxbc.h"
#include "post_tone_lut_ps.dxbc.h"
#include "post_tone_hdr_ps.dxbc.h"   // I-0138 tranche 5
#include "post_tone_hdr_reinhard_ps.dxbc.h"
#include "post_tone_reinhard_ps.dxbc.h"
#include "post_tone_agx_ps.dxbc.h"   // I-0138 tranche 6: the candidate looks' entries
#include "post_tone_aces_ps.dxbc.h"
#include "post_tone_hdr_agx_ps.dxbc.h"
#include "post_tone_hdr_aces_ps.dxbc.h"
#include "post_tone_er_ps.dxbc.h"
#include "post_tone_hdr_er_ps.dxbc.h"

using namespace ds3native;
namespace {
int failures{},checks{};
uint32_t Bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
void Word(const char* what,float got,uint32_t expected){++checks;if(Bits(got)==expected)return;++failures;std::printf("FAIL %s: %08X (%g), captured %08X\n",what,Bits(got),got,expected);}
void True(const char* what,bool value){++checks;if(value)return;++failures;std::printf("FAIL %s\n",what);}
// The programme's cbuffer against the C++ block: its size and each variable's offset and size.
void Layout(const char* programme,const void* code,size_t bytes){
    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
    if(FAILED(D3DReflect(code,bytes,IID_PPV_ARGS(&reflection)))){True((std::string(programme)+": reflection").c_str(),false);return;}
    auto* block=reflection->GetConstantBufferByName("PostToneParams");D3D12_SHADER_BUFFER_DESC desc{};
    if(FAILED(block->GetDesc(&desc))){True((std::string(programme)+": no PostToneParams cbuffer").c_str(),false);return;}
    True((std::string(programme)+": the cbuffer's size").c_str(),desc.Size==sizeof(PostToneParams));
    struct Field {const char* name;size_t offset,size;};
    const Field fields[]{{"colour",offsetof(PostToneParams,colour),48},{"alpha_weights",offsetof(PostToneParams,alpha_weights),16},
        {"vignette_uv",offsetof(PostToneParams,vignette_uv),48},{"exposure",offsetof(PostToneParams,exposure),4},{"curve_a",offsetof(PostToneParams,curve_a),4},
        {"curve_b",offsetof(PostToneParams,curve_b),4},{"inv_gamma",offsetof(PostToneParams,inv_gamma),4},{"glare_scale",offsetof(PostToneParams,glare_scale),4},
        {"dither_scale",offsetof(PostToneParams,dither_scale),4},{"dither_offset",offsetof(PostToneParams,dither_offset),4},{"flags",offsetof(PostToneParams,flags),4},
        {"lut_scale",offsetof(PostToneParams,lut_scale),4},{"lut_offset",offsetof(PostToneParams,lut_offset),4},{"curve_c",offsetof(PostToneParams,curve_c),4},{"hdr_knee",offsetof(PostToneParams,hdr_knee),4},   // curve_c: I-0138 part 4c; hdr_knee: tranche 5 (the block's last word, `pad` until then)
        {"look_scale",offsetof(PostToneParams,look_scale),4},{"look_a",offsetof(PostToneParams,look_a),4},{"look_b",offsetof(PostToneParams,look_b),4},{"look_c",offsetof(PostToneParams,look_c),4},
        {"er_dispersion",offsetof(PostToneParams,er_dispersion),16},
        {"er_lift",offsetof(PostToneParams,er_lift),16},{"grain",offsetof(PostToneParams,grain),12},{"grain_seed",offsetof(PostToneParams,grain_seed),4}};   // tranche 7: four words more (224 bytes), the film grain's four (240); I-0141 tranche 11: the ER vignette's four dropped (224)   // tranche 6: twelve words more (the block 160 to 208 bytes)
    True((std::string(programme)+": the cbuffer's variable count").c_str(),desc.Variables==sizeof(fields)/sizeof(fields[0]));
    for(const auto& f:fields){D3D12_SHADER_VARIABLE_DESC v{};
        const bool ok=SUCCEEDED(block->GetVariableByName(f.name)->GetDesc(&v)) && v.StartOffset==f.offset && v.Size==f.size;
        True((std::string(programme)+": "+f.name+" at its offset").c_str(),ok);}
}
}
int main(int argc,char** argv) {
    Layout("post_tone_ps",post_tone_ps_dxbc,sizeof post_tone_ps_dxbc);Layout("post_tone_vs",post_tone_vs_dxbc,sizeof post_tone_vs_dxbc);
    Layout("post_tone_lut_ps",post_tone_lut_ps_dxbc,sizeof post_tone_lut_ps_dxbc);
    Layout("post_tone_hdr_ps",post_tone_hdr_ps_dxbc,sizeof post_tone_hdr_ps_dxbc);   // I-0138 tranche 5: the HDR entries read the same block
    Layout("post_tone_hdr_reinhard_ps",post_tone_hdr_reinhard_ps_dxbc,sizeof post_tone_hdr_reinhard_ps_dxbc);
    Layout("post_tone_reinhard_ps",post_tone_reinhard_ps_dxbc,sizeof post_tone_reinhard_ps_dxbc);   // part 4c's programme, unchecked until the fifth step review
    Layout("post_tone_agx_ps",post_tone_agx_ps_dxbc,sizeof post_tone_agx_ps_dxbc);Layout("post_tone_aces_ps",post_tone_aces_ps_dxbc,sizeof post_tone_aces_ps_dxbc);   // I-0138 tranche 6
    Layout("post_tone_hdr_agx_ps",post_tone_hdr_agx_ps_dxbc,sizeof post_tone_hdr_agx_ps_dxbc);Layout("post_tone_hdr_aces_ps",post_tone_hdr_aces_ps_dxbc,sizeof post_tone_hdr_aces_ps_dxbc);
    Layout("post_tone_er_ps",post_tone_er_ps_dxbc,sizeof post_tone_er_ps_dxbc);Layout("post_tone_hdr_er_ps",post_tone_hdr_er_ps_dxbc,sizeof post_tone_hdr_er_ps_dxbc);
    True("the block's HDR knee is the default",BuildPostTone(yebis::TonemapConstants{}).hdr_knee==kPostToneHdrKnee);
    True("the block asks no look and scales nothing by default",(BuildPostTone(yebis::TonemapConstants{}).flags&kPostToneLookMask)==0u && BuildPostTone(yebis::TonemapConstants{}).look_scale==1.0f);
    {   // I-0138 tranche 7: the film grain's words.
        const auto none=BuildPostTone(yebis::TonemapConstants{});
        True("no film grain by default",none.grain[0]==0.0f && none.grain[1]==1.0f && (none.flags&kPostToneGrainLater)==0u);
        auto p=none;SetFilmGrain(p,0,1080,7);True("the grain's row at 0 adds none",p.grain[0]==0.0f && p.grain_seed==7u);
        SetFilmGrain(p,100,1080,8);True("the grain at 100 %: its most, a grid of one pixel at 1080 lines",p.grain[0]==kFilmGrainMost && p.grain[1]==1.0f && p.grain_seed==8u);
        SetFilmGrain(p,50,2160,9);True("the grain at 50 % on a 4K picture: half its most, a grid of two pixels",p.grain[0]==0.5f*kFilmGrainMost && p.grain[1]==2.0f);
        SetFilmGrain(p,250,720,1);True("a strength over 100 % kept at 100 %, a grid of one pixel under 1080 lines",p.grain[0]==kFilmGrainMost && p.grain[1]==1.0f);
        SetFilmGrain(p,30,0,1);True("an unknown height: a grid of one pixel",p.grain[1]==1.0f);}
    {   // 3. the candidate looks' models
        using look::ToneLook;using look::Vec;
        const auto equal=[](const Vec& got,const Vec& expected){for(int k=0;k<3;++k)if(!(std::abs(got[k]-expected[k])<=1e-8))return false;return true;};
        True("AgX at a middle grey",equal(look::Agx({0.18,0.18,0.18}),{0.2148365921,0.2148509740,0.2148514499}));
        True("AgX at a saturated red",equal(look::Agx({1.0,0.1,0.05}),{0.6723479852,0.1967019538,0.1280584862}));
        True("AgX at a bright blue",equal(look::Agx({0.05,0.3,2.0}),{0.2029570527,0.4142053494,0.8243123861}));
        True("AgX at a bright grey",equal(look::Agx({8.0,8.0,8.0}),{0.9234204151,0.9236122464,0.9236339597}));
        True("AgX at black",equal(look::Agx({0.0,0.0,0.0}),{0.0000025289,0.0000025294,0.0000025295}));
        True("ACES at a middle grey",equal(look::Aces({0.18,0.18,0.18}),{0.1055912472,0.1055912472,0.1055901913}));
        True("ACES at a saturated red",equal(look::Aces({1.0,0.1,0.05}),{0.6859766718,0.0580443222,0.0258980975}));
        True("ACES at a bright blue",equal(look::Aces({0.05,0.3,2.0}),{0.0711214423,0.2234294517,0.8124382034}));
        True("ACES at a bright grey",equal(look::Aces({8.0,8.0,8.0}),{0.9630393942,0.9630393942,0.9630297638}));
        True("ACES at black",equal(look::Aces({0.0,0.0,0.0}),{0.0,0.0,0.0}));
        True("the candidates' grey inputs",std::abs(look::GreyInput(ToneLook::Agx)-0.145152979706)<1e-9 && std::abs(look::GreyInput(ToneLook::Aces)-0.263146682169)<1e-9 &&
            look::GreyInput(ToneLook::Original)==0.0);
        // A rising ramp stays rising (grey, 0 to 64), both candidates.
        {bool rising=true;double last_agx=-1.0,last_aces=-1.0;for(int i=0;i<=4096;++i){const double s=64.0*double(i)/4096.0;const double a=look::Agx({s,s,s})[1],c=look::Aces({s,s,s})[1];
            rising=rising && a>=last_agx && c>=last_aces;last_agx=a;last_aces=c;}
            True("the candidates rise with a grey ramp",rising);}
        // The block: both of the pass's own curves (the sensitometric curve with m37's constants, a Reinhard curve), each candidate.
        // The candidate after Elden Ring: its curve at three values, its grey input ((0.18 / 0.82)^(1 / 1.4)), its dispersion's words.
        True("the ER curve",std::abs(look::ErCurve(0.18)-0.0831179770811011)<1e-12 && std::abs(look::ErCurve(1.0)-0.5)<1e-12 && std::abs(look::ErCurve(8.0)-0.9483982126113827)<1e-12 &&
            look::ErCurve(-1.0)==0.0);
        True("the ER candidate's grey input",std::abs(look::GreyInput(ToneLook::Er)-0.33854259853215823)<1e-9);
        {const auto d=look::ErDispersionWords();
            True("the ER dispersion's words",d[0]==-0.0006 && d[1]==-0.00005 && d[2]==-0.0012 && d[3]==-0.0001);}
        {PostToneParams p;p.curve_a=2.3985305f;p.curve_b=0.5141534f;for(int r=0;r<3;++r){p.colour[r][r]=0.9f;p.colour[r][3]=0.01f;}
            auto er=p;const bool er_neutral=ApplyToneLook(er,ToneLook::Er,false);bool identity=er_neutral;   // always neutral, whatever is asked
            for(int r=0;r<3;++r)for(int c=0;c<4;++c)identity=identity && er.colour[r][c]==(r==c?1.0f:0.0f);
            True("the ER candidate's block",identity && er.look_c==float(look::kErContrast) && ((er.flags&kPostToneLookMask)>>kPostToneLookShift)==uint32_t(ToneLook::Er) &&
                er.er_dispersion[0]==-0.0006f && er.er_dispersion[2]==-0.0012f && er.look_b>0.0f);
            // I-0141 tranche 11: its vignette the area's own, the block's bit 0 as the record set it (DS3's, by area; Elden Ring's dropped).
            auto lit=p;lit.flags|=kPostToneVignette;ApplyToneLook(lit,ToneLook::Er,false);auto unlit=p;ApplyToneLook(unlit,ToneLook::Er,false);
            True("the ER candidate keeps the area's vignette bit",(lit.flags&kPostToneVignette)!=0 && (unlit.flags&kPostToneVignette)==0);
            const double grey=look::GreyInput([&](double x){return PostToneCurve(p,x);});
            True("the ER candidate's scale matches the middle grey",std::abs(look::ErCurve(grey*double(er.look_scale))-look::kMiddleGrey)<1e-6);
            auto agx=p;True("AgX with the grade is not neutral",!ApplyToneLook(agx,ToneLook::Agx,false));
            True("another candidate leaves the ER words at none",agx.er_dispersion[2]==0.0f && agx.look_c==0.0f && agx.er_lift[0]==1.0f &&
                agx.er_lift[1]==0.0f && agx.er_lift[2]==0.0f && agx.er_lift[3]==0.0f);
            // I-0138 tranche 7b (D-0296): the ER candidate with its vivid words by default, without them only when asked (the plain model's form).
            // bare: over a block that had the words, written to none.
            {auto bare=er;ApplyToneLook(bare,ToneLook::Er,false,true,false);auto original=p;ApplyToneLook(original,ToneLook::Original,false);
                True("the ER candidate's vivid words by default, none when not asked nor for the original",er.er_lift[1]==float(look::kErVividContrast) &&
                    er.er_lift[2]==float(look::kErVividVibrance) && er.er_lift[3]==float(look::kErVividTint) && bare.er_lift[1]==0.0f && bare.er_lift[2]==0.0f &&
                    bare.er_lift[3]==0.0f && original.er_lift[1]==0.0f && original.er_lift[2]==0.0f && original.er_lift[3]==0.0f);}
            // I-0138 tranche 7: the midtone lift always with the ER candidate (136 of 255 to about 154), its aberration the settings' row.
            auto still=p;ApplyToneLook(still,ToneLook::Er,false,false);
            True("the ER candidate's lift, and its aberration off",er.er_lift[0]==float(look::kErLift) && still.er_lift[0]==float(look::kErLift) &&
                still.er_dispersion[0]==0.0f && still.er_dispersion[1]==0.0f && still.er_dispersion[2]==0.0f && still.er_dispersion[3]==0.0f);
            const double node8=std::pow(look::ErLift(std::pow(136.0/255.0,2.2)),1.0/2.2)*255.0;
            True("the lift sends the grey node 8 near ER's median",node8>150.0 && node8<158.0);}
        {   // I-0138 tranche 7b: the vivid variant's model against the Fable subagent's table (values in, out, to four places: within 2e-4) and its words.
            struct Row {look::Vec in,out;};
            static const Row rows[]{{{0,0,0},{0,0,0.0400}},{{0.1,0.1,0.1},{0.0277,0.0417,0.0753}},{{0.25,0.25,0.25},{0.1360,0.1656,0.1835}},
                {{0.5,0.5,0.5},{0.4482,0.4500,0.4504}},{{0.6,0.6,0.6},{0.5693,0.5693,0.5693}},{{1,1,1},{1,1,1}},{{1,0,0},{1,0,0}},{{0,1,0},{0,1,0}},
                {{0,0,1},{0,0,1}},{{1,1,0},{1,1,0}},{{0.42,0.40,0.37},{0.3424,0.3284,0.2933}},{{0.72,0.55,0.45},{0.7074,0.4963,0.3682}},
                {{0.12,0.18,0.32},{0.0393,0.0927,0.2504}},{{1,0.62,0.15},{1,0.5889,0.0675}},{{0.1333,0,0},{0.0444,0,0.0304}},
                {{1,1,0.5},{1,1,0.3881}},{{1.5,1.2,0.6},{1.5,1.1853,0.5237}}};
            bool table=true;for(const auto& r:rows){const auto o=look::ErVivid(r.in);for(int k=0;k<3;++k)table=table && std::abs(o[k]-r.out[k])<2e-4;}
            True("the vivid variant's model gives its table",table);
            bool ordered=true,monotonic=true,positive=true;uint32_t seed=0x7B7B7B7Bu;const auto u=[&]{seed=seed*1664525u+1013904223u;return double(seed>>8)/16777216.0*1.6;};
            for(int i=0;i<20000;++i){look::Vec a{u(),u(),u()};auto b=a;const int k=i%3;b[k]+=0.01;const auto fa=look::ErVivid(a),fb=look::ErVivid(b);
                monotonic=monotonic && fb[k]>=fa[k]-1e-9;positive=positive && fa[0]>=0.0 && fa[1]>=0.0 && fa[2]>=0.0;
                const double mx=(std::max)(a[0],(std::max)(a[1],a[2]));
                if(mx>=0.6)for(int x=0;x<3;++x)   // above the cool black's reach (its weight 0 once the contrast step's largest is 0.5)
                    for(int y=0;y<3;++y)if(a[x]>a[y]+1e-6)ordered=ordered && fa[x]>=fa[y]-1e-9;}
            True("the vivid variant: each channel monotonic, never negative, the channels' order kept where the largest is at least 0.6 (no tint there)",monotonic && positive && ordered);
            PostToneParams p;SetErVivid(p);
            True("the vivid variant's words",p.er_lift[1]==float(look::kErVividContrast) && p.er_lift[2]==float(look::kErVividVibrance) && p.er_lift[3]==float(look::kErVividTint) &&
                PostToneParams{}.er_lift[1]==0.0f && PostToneParams{}.er_lift[2]==0.0f && PostToneParams{}.er_lift[3]==0.0f);}
        for(const bool reinhard:{false,true})for(const auto candidate:{ToneLook::Agx,ToneLook::Aces}){
            PostToneParams p;p.curve_a=2.3985305f;p.curve_b=0.5141534f;p.exposure=1.52f;for(int r=0;r<3;++r){p.colour[r][r]=0.9f;p.colour[r][3]=0.01f;}
            if(reinhard){p.flags|=kPostToneReinhard;p.curve_a=0.0f;p.curve_b=1.0f;p.curve_c=1.0f/1024.0f;}
            p.flags|=kPostToneVignette;auto kept=p;ApplyToneLook(kept,candidate,false);
            True("a look's bits beside the block's flags",kept.flags==(p.flags|(uint32_t(candidate)<<kPostToneLookShift)) && kept.colour[0][0]==0.9f && kept.colour[0][3]==0.01f);
            const double grey=look::GreyInput([&](double x){return PostToneCurve(p,x);});
            const double through=candidate==ToneLook::Agx?look::Agx({grey*kept.look_scale,grey*kept.look_scale,grey*kept.look_scale})[1]:look::Aces({grey*kept.look_scale,grey*kept.look_scale,grey*kept.look_scale})[1];
            True("a look's scale matches the middle grey",grey>0.0 && std::abs(PostToneCurve(p,grey)-look::kMiddleGrey)<1e-6 && std::abs(through-look::kMiddleGrey)<1e-6);
            auto neutral=p;const bool said_neutral=ApplyToneLook(neutral,candidate,true);bool identity=said_neutral;
            for(int r=0;r<3;++r)for(int c=0;c<4;++c)identity=identity && neutral.colour[r][c]==(r==c?1.0f:0.0f);
            True("a neutral look's matrix is the identity",identity && neutral.look_scale==kept.look_scale);
            // The HDR entries' tangent at the block's knee: the candidate's grey value there is the knee, the gain 1 at it and rising.
            const auto tangent=look::KneeTangent(candidate,double(p.hdr_knee));
            True("a look's tangent at the knee",kept.look_a==float(tangent.input) && kept.look_b==float(tangent.slope) && tangent.slope>0.0 &&
                std::abs(look::Grey(candidate,tangent.input)-double(p.hdr_knee))<1e-9 && look::HighlightGain(candidate,tangent,double(p.hdr_knee),tangent.input)==1.0 &&
                std::abs(look::HighlightGain(candidate,tangent,double(p.hdr_knee),tangent.input*1.0001)-1.0)<1e-6 &&
                look::HighlightGain(candidate,tangent,double(p.hdr_knee),4.0*tangent.input)>look::HighlightGain(candidate,tangent,double(p.hdr_knee),2.0*tangent.input) &&
                look::HighlightGain(candidate,tangent,double(p.hdr_knee),2.0*tangent.input)>1.0);
            auto flat=p;flat.hdr_knee=1.0f;ApplyToneLook(flat,candidate,false);
            True("a knee of 1: no gain",flat.look_b==0.0f && flat.look_a>1.0e38f && look::HighlightGain(candidate,look::KneeTangent(candidate,1.0),1.0,1.0e6)==1.0);}
        {PostToneParams p;p.curve_a=2.3985305f;p.curve_b=0.5141534f;const auto before=p;const bool original_neutral=ApplyToneLook(p,ToneLook::Original,true);
            True("the original look changes nothing",!original_neutral && !std::memcmp(&p,&before,sizeof p));}
    }
    if(argc>=2)_putenv_s("DS3_EXE",argv[1]);
    yebis::TonemapData d;
    const bool data=ds3light::Rdata(0x2B60C40,d.curve.data(),84) && ds3light::Rdata(0x2B60CD0,&d.tx_scale,4) && ds3light::Rdata(0x3D7EDA4,&d.third,4) &&
        ds3light::Rdata(0x3D7EE10,&d.one,4) && ds3light::Rdata(0x3D7EF70,&d.steps,4);
    if(data){
        if(!yebis::LoadColorData(d.color,[](uint32_t rva,void* out,size_t bytes){return ds3light::Rdata(rva,out,bytes);})){std::printf("FAIL colour data words\n");return 1;}
        yebis::TonemapInputs in;std::memcpy(&in.exposure,"\x5C\x8F\xC2\x3F",4);std::memcpy(&in.gamma,"\xCD\xCC\x0C\x40",4);
        std::memcpy(&in.filmic_contrast,"\x9A\x99\x99\x3E",4);in.color_h=0.0f;std::memcpy(&in.color_s,"\x66\x66\x66\x3F",4);in.color_v=1.0f;
        const auto p=BuildPostTone(yebis::BuildTonemap(d,in));
        Word("exposure (616)",p.exposure,0x3FC28F5Cu);Word("glare scale (617)",p.glare_scale,0x3F800000u);
        const uint32_t alpha[4]{0,0,0,0x3F800000u};for(int i=0;i<4;++i)Word(("alpha weight (62"+std::to_string(i)+")").c_str(),p.alpha_weights[i],alpha[i]);
        Word("gamma correction (808)",p.inv_gamma,0x3EE8BA2Eu);Word("dither scale (809)",p.dither_scale,0x3B808081u);Word("dither offset (810)",p.dither_offset,0xBB008081u);
        Word("curve T.x (832)",p.curve_a,0x40198186u);Word("curve T.y (833)",p.curve_b,0x3F039F8Eu);
        const uint32_t matrix[12]{0x3F6BD7B2u,0x3D927918u,0x3BEC95C0u,0,0x3CAE2978u,0x3F78B589u,0x3BEC95C0u,0,0x3CAE2978u,0x3D927918u,0x3F683F92u,0};
        for(int i=0;i<12;++i)Word(("colour matrix word "+std::to_string(840+i)).c_str(),p.colour[i/4][i%4],matrix[i]);
        Word("LUT scale (grading byte 2976)",p.lut_scale,0x3F700000u);Word("LUT offset (grading byte 2992)",p.lut_offset,0x3D000000u);
        True("no flag without a vignette",p.flags==0u);
        // The vignette's coordinates laid out a vertex a row, and its flag.
        const std::array<std::array<float,2>,3> uv{{{-0.25f,0.125f},{-0.25f,-0.375f},{0.75f,0.125f}}};const auto v=BuildPostTone(yebis::BuildTonemap(d,in),&uv);
        True("the vignette flag",v.flags==kPostToneVignette);
        for(size_t i=0;i<3;++i)True("the vignette coordinates",v.vignette_uv[i][0]==uv[i][0] && v.vignette_uv[i][1]==uv[i][1] && v.vignette_uv[i][2]==0.0f && v.vignette_uv[i][3]==0.0f);}
    else std::printf("post_tone_test: the record's words skipped (no executable)\n");
    // I-0138 tranche 6 (K-0495; its final review's L2): the ColorGrading LUT's name from the record's file.
    {char name[64]{};const auto is=[&](const char* source,uint32_t area,int32_t id,const char* want){
            name[0]=0;const bool made=GradingLutName(source,area,id,name);return want?made && !std::strcmp(name,want):!made;};
        True("the LUT's name: an area's own file",is("area",130000000u,3,"m30_00_CGrading0003") && is("area",151010000u,12,"m51_01_CGrading0012"));
        True("the LUT's name: the area's base file",is("m45_00_0000",145000010u,7,"m45_00_CGrading0007"));
        True("the LUT's name: a cutscene's file takes the map prefix",is("s30_00_0030",230000030u,1,"m30_00_CGrading0001") && is("area",230000030u,2,"m30_00_CGrading0002"));
        True("the LUT's name: a file of another kind has no prefix",is("default",1u,7,"CGrading0007") && is("mapbase",130000000u,0,"CGrading0000") && is("m45_00_000",145000000u,7,"CGrading0007"));
        True("the LUT's name: no map file under area 10,000,000",is("area",1u,7,nullptr) && is("area",9999999u,7,nullptr));}
    if(failures){std::printf("FAIL post tone parameters: %d of %d checks\n",failures,checks);return 1;}
    std::printf("PASS post tone parameters: %d checks: the cbuffer's layout in the twelve programmes, the candidate looks' models%s\n",checks,data?", the m37 record's block equal to the original's captured words bit for bit":"");
    return 0;
}
