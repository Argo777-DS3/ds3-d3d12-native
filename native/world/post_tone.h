// I-0138 tranche 1 (ROADMAP workstream 11): our tone-map pass's parameter block from the blended Tone Map record. The record's
// semantics are DS3's and stay where they are computed and tested bit for bit on the original's captured words (yebis_tonemap.h:
// the guards, the curve's constants, the colour matrix, the vignette's rectangle); this only lays them out for our programme.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include "yebis_tonemap.h"
#include "../renderer/post_params.h"
#include "../renderer/tone_looks.h"

namespace ds3native {
// I-0138 tranche 6 (K-0495, read in the executable): the ColorGrading LUT's texture name for a record. The original builds
// "%s%04d" from a prefix and « CGrading »: the prefix "m%02d_%02d_" for a map file and for a cutscene's file alike (FUN_140D28490,
// file kinds 7 and 0xB), the empty prefix for a file of another kind. `source`: the record's file as the bank names it: "area" (the
// area's own file: its digits; an area under 10,000,000 has no map file: false), a file's name mAA_BB_NNNN or sAA_BB_NNNN (the
// area's base file, a cutscene's file), or another name (no prefix).
inline bool GradingLutName(const std::string& source,uint32_t area,int32_t id,char (&name)[64]) noexcept {
    const auto digit=[](char c){return c>='0' && c<='9';};
    const bool named_map=source.size()==11 && (source[0]=='m' || source[0]=='M' || source[0]=='s' || source[0]=='S') && source[3]=='_' && source[6]=='_' &&
        std::all_of(source.begin()+1,source.begin()+3,digit) && std::all_of(source.begin()+4,source.begin()+6,digit) && std::all_of(source.begin()+7,source.end(),digit);
    if(named_map)sprintf_s(name,"m%.6sCGrading%04d",source.c_str()+1,id);
    else if(source!="area")sprintf_s(name,"CGrading%04d",id);
    else if(area<10000000u)return false;
    else sprintf_s(name,"m%02u_%02u_CGrading%04d",(area/1000000u)%100u,(area/10000u)%100u,id);
    return true;}
// t: yebis::BuildTonemap's constants. vignette: the three vertices' coordinates (yebis::VignetteCoordinates), or null without one.
// lut_edge: the grading LUT's edge N (the original's afRGBA_Offset[0] and [1] of its grading draw: (N - 1) / N and 1 / 2N).
inline PostToneParams BuildPostTone(const yebis::TonemapConstants& t,const std::array<std::array<float,2>,3>* vignette=nullptr,uint32_t lut_edge=16) {
    PostToneParams p;
    for(size_t r=0;r<3;++r)for(size_t c=0;c<4;++c)p.colour[r][c]=t.color_matrix[r*4+c];
    p.alpha_weights[3]=1.0f;   // the original's afRGBA_Modulate[1]: the output alpha is the scene's
    if(vignette){p.flags|=kPostToneVignette;for(size_t i=0;i<3;++i){p.vignette_uv[i][0]=(*vignette)[i][0];p.vignette_uv[i][1]=(*vignette)[i][1];}}
    p.exposure=t.exposure;p.curve_a=t.curve[0];p.curve_b=t.curve[1];p.curve_c=t.curve[2];p.inv_gamma=t.gamma_correction;
    if(t.reinhard)p.flags|=kPostToneReinhard;
    p.glare_scale=1.0f;        // afRGBA_Modulate[0].y
    p.dither_scale=t.dither[0];p.dither_offset=t.dither[1];
    const float n=float(lut_edge);p.lut_scale=(n-1.0f)/n;p.lut_offset=1.0f/(2.0f*n);
    return p;
}
// I-0138 tranche 6: the block's own curve at a grey scene value after the exposure (shaders/post_tone.hlsl's two curves, in
// double), for the candidates' grey match.
inline double PostToneCurve(const PostToneParams& p,double x) noexcept {
    if(p.flags&kPostToneReinhard){const double s=x*double(p.curve_b);return (std::min)((std::max)(s*(1.0+s*double(p.curve_c))/(1.0+x*double(p.curve_b)),0.0),1.0);}
    const double e=std::exp(-x*double(p.curve_a)),shoulder=1.0-double(p.curve_b)*e;return (std::min)((std::max)((1.0-e)*shoulder*shoulder,0.0),1.0);
}
// I-0138 tranche 6: a candidate look on a block built from the record (tone_looks.h): its bits, and the scale that shows as
// middle grey the scene value the record's own curve shows so. `neutral`: the area's colour matrix replaced by the identity
// (the caller then leaves the area's LUT out too): the candidate's own look, without the area's grade. The HDR entries'
// tangent at the block's knee (set before this call). What the bisections give is kept by their inputs (a frame's block
// seldom differs from the last one's). The candidate after Elden Ring is always neutral and takes its own words: its
// contrast and its dispersion; its vignette is the area's own (the block's vignette bit, I-0141 tranche 11: Thomas 2026-11-17,
// DS3's by area, Elden Ring's dropped). Returns whether the look is drawn without the area's grade (the caller then leaves
// the LUT out); false for the original.
// I-0138 tranche 7: `aberration` false leaves the candidate after Elden Ring without its lateral dispersion (the settings' row), its
// midtone lift (look::kErLift) always with it. Tranche 7b (Thomas, 2026-11-16 12:06, D-0296): its vivid variant's words (look::ErVivid)
// with it too; `vivid` false only for the unit tests' model of the plain candidate.
inline void SetErVivid(PostToneParams& p) noexcept {p.er_lift[1]=float(look::kErVividContrast);p.er_lift[2]=float(look::kErVividVibrance);p.er_lift[3]=float(look::kErVividTint);}
inline bool ApplyToneLook(PostToneParams& p,look::ToneLook candidate,bool neutral,bool aberration=true,bool vivid=true) noexcept {
    if(candidate==look::ToneLook::Original)return false;
    if(candidate==look::ToneLook::Er){neutral=true;p.look_c=float(look::kErContrast);p.er_lift[0]=float(look::kErLift);
        if(vivid)SetErVivid(p);else p.er_lift[1]=p.er_lift[2]=p.er_lift[3]=0.0f;   // written either way (the step review's L1)
        const auto d=look::ErDispersionWords();
        for(size_t i=0;i<4;++i)p.er_dispersion[i]=aberration?float(d[i]):0.0f;}
    struct Kept {look::ToneLook candidate{};float a{},b{},c{},knee{};uint32_t reinhard{};float scale{},input{},slope{};};
    thread_local Kept kept;
    if(kept.candidate!=candidate || kept.a!=p.curve_a || kept.b!=p.curve_b || kept.c!=p.curve_c || kept.knee!=p.hdr_knee || kept.reinhard!=(p.flags&kPostToneReinhard)){
        const auto tangent=look::KneeTangent(candidate,double(p.hdr_knee));
        kept={candidate,p.curve_a,p.curve_b,p.curve_c,p.hdr_knee,p.flags&kPostToneReinhard,
            float(look::GreyScale(candidate,look::GreyInput([&](double x){return PostToneCurve(p,x);}))),float(tangent.input),float(tangent.slope)};}
    p.look_scale=kept.scale;p.look_a=kept.input;p.look_b=kept.slope;
    p.flags=(p.flags&~uint32_t(kPostToneLookMask))|(uint32_t(candidate)<<kPostToneLookShift);
    if(neutral)for(size_t r=0;r<3;++r)for(size_t c=0;c<4;++c)p.colour[r][c]=r==c?1.0f:0.0f;
    return neutral;
}
// I-0138 tranche 7: the film grain's words (shaders/post_tone.hlsl, Grain). percent: the settings' row (0 to 100; 0 none, the
// default); height: the picture's height in pixels (0 not known: a grid of one pixel); seed: the frame's number (the grain moves
// every frame). The strength at 100 % is kFilmGrainMost on the display-encoded value (the noise's corners in (-1, 1), so at most
// about 20 levels of 255 at mid-grey before the interpolation softens it); the grid one pixel up to 1080 lines, then the height
// over 1080 (the grain's size the same share of the picture at 1440p and 4K: one rule for every resolution, D-0151).
inline constexpr float kFilmGrainMost=0.08f;
inline void SetFilmGrain(PostToneParams& p,uint32_t percent,uint32_t height,uint32_t seed) noexcept {
    p.grain[0]=float((std::min)(percent,100u))/100.0f*kFilmGrainMost;
    p.grain[1]=height>1080u?float(height)/1080.0f:1.0f;p.grain[2]=0.0f;p.grain_seed=seed;
}
}
