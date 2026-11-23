// W10 under D-0175 (W10-NATIVE-PASSES 2, 3.1): the native tone-map constants against the original's captured $Globals of
// `TonemapHDR_Dither_Exposure_Matrix_Gamma` at m37 (Reference1632 frame 1200 draw 45), from the m37 Tone Map record (Exposure
// 1.52, Gamma 2.2, FilmicContrast 0.3, ColorH 0, ColorS 0.9, ColorV 1) and the executable's data words (DS3_EXE for Rdata).
// Words compared bit for bit; each one that differs is printed with its distance in ulps.
#include "../world/yebis_tonemap.h"
#include "../world/light_producers.h"
#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <utility>

using namespace ds3native::yebis;
namespace {
int failures{},checks{};
uint32_t Bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
// tolerance: the ulps allowed (0 = bit for bit).
void Word(const char* what,float got,uint32_t expected,int tolerance=0) {
    ++checks;const uint32_t g=Bits(got);const int ulps=static_cast<int>(g)-static_cast<int>(expected);if(g==expected)return;
    if(std::abs(ulps)<=tolerance){std::printf("near %s: %d ulps (within %d)\n",what,ulps,tolerance);return;}++failures;
    std::printf("FAIL %s: %08X (%g), captured %08X, %d ulps\n",what,g,got,expected,ulps);}
}
int main(int argc,char** argv) {
    if(argc>=2)_putenv_s("DS3_EXE",argv[1]);
    TonemapData d;
    if(!ds3light::Rdata(0x2B60C40,d.curve.data(),84) || !ds3light::Rdata(0x2B60CD0,&d.tx_scale,4) || !ds3light::Rdata(0x3D7EDA4,&d.third,4) ||
       !ds3light::Rdata(0x3D7EE10,&d.one,4) || !ds3light::Rdata(0x3D7EF70,&d.steps,4)){
        std::printf("yebis_tonemap_test: skipped (no executable)\n");return 0;}
    if(!LoadColorData(d.color,[](uint32_t rva,void* out,size_t bytes){return ds3light::Rdata(rva,out,bytes);})){std::printf("FAIL colour data words\n");return 1;}
    TonemapInputs in;std::memcpy(&in.exposure,"\x5C\x8F\xC2\x3F",4);std::memcpy(&in.gamma,"\xCD\xCC\x0C\x40",4);
    std::memcpy(&in.filmic_contrast,"\x9A\x99\x99\x3E",4);in.color_h=0.0f;std::memcpy(&in.color_s,"\x66\x66\x66\x3F",4);in.color_v=1.0f;
    const auto t=BuildTonemap(d,in);
    Word("exposure (616)",t.exposure,0x3FC28F5Cu);
    Word("gamma correction (808)",t.gamma_correction,0x3EE8BA2Eu);
    Word("dither scale (809)",t.dither[0],0x3B808081u);Word("dither offset (810)",t.dither[1],0xBB008081u);
    Word("curve T.x (832)",t.curve[0],0x40198186u);Word("curve T.y (833)",t.curve[1],0x3F039F8Eu);
    // I-0138 part 4c: the Reinhard curve's c208 (YebisTonemapCurveConstants_141718390 case 2, a = F, b = 1): F = 32 gives
    // (4/1024 - 2, 1, 1/1024, 512); F = 0.36 (s40_00_0040 id 1) its own words; the sensitometric record keeps .zw 0.
    {const auto r=ds3native::yebis::ReinhardCurve(32.0f);
        Word("Reinhard F 32 x",r[0],0xBFFF8000u);Word("Reinhard F 32 y",r[1],0x3F800000u);Word("Reinhard F 32 z",r[2],0x3A800000u);Word("Reinhard F 32 w",r[3],0x44000000u);
        // F 0.36 against the original's captured words (i138-4b-opening2-orig-1500, its draw 27's $Globals 832-835).
        const auto q=ds3native::yebis::ReinhardCurve(std::bit_cast<float>(0x3EB851ECu));
        Word("Reinhard F 0.36 x",q[0],0x41E6E9E0u);Word("Reinhard F 0.36 y",q[1],0x3F800000u);Word("Reinhard F 0.36 z",q[2],0x40F6E9E0u);Word("Reinhard F 0.36 w",q[3],0x3D84B5DDu);
        Word("sensitometric curve .z",t.curve[2],0u);Word("sensitometric curve .w",t.curve[3],0u);}
    const uint32_t matrix[16]{0x3F6BD7B2u,0x3D927918u,0x3BEC95C0u,0,0x3CAE2978u,0x3F78B589u,0x3BEC95C0u,0,0x3CAE2978u,0x3D927918u,0x3F683F92u,0,0,0,0,0x3F800000u};
    for(int i=0;i<16;++i){const std::string what="colour matrix word "+std::to_string(840+i);Word(what.c_str(),t.color_matrix[size_t(i)],matrix[i]);}   // TM-3: (L - s L) + s on the diagonal, bit for bit
    if(!t.matrix_supported){++checks;++failures;std::printf("FAIL the m37 matrix is in the supported case\n");}
    // GL-4: E69A0's matrix (hue rotation, saturation, value) against the captured tone-map words of the other zones (the tone-map
    // draws of i66-reference-1650 m31 ColorS 0.85, 1651 m50 ColorH 1 and ColorS 0.9, 1652 m45 ColorS 0.85; ColorV 1 everywhere).
    {struct Zone {const char* name;float h,s;uint32_t words[16];};
        const Zone zones[]{{"m31",0.0f,0.85f,{0x3F61C38B,0x3DDBB598,0x3C317050,0,0x3D029F14,0x3F75104E,0x3C317050,0,0x3D029F14,0x3DDBB598,0x3F5C5F5B,0,0,0,0,0x3F800000}},
            {"m50",1.0f,0.9f,{0x3F6AF563,0x3D775851,0x3CB2A311,0,0x3CC0CDCA,0x3F79430F,0x3B3682CE,0,0x3C124699,0x3DA9AC52,0x3F68815B,0,0,0,0,0x3F800000}},
            {"m45",0.0f,0.85f,{0x3F61C38B,0x3DDBB598,0x3C317050,0,0x3D029F14,0x3F75104E,0x3C317050,0,0x3D029F14,0x3DDBB598,0x3F5C5F5B,0,0,0,0,0x3F800000}}};
        for(const auto& z:zones){TonemapInputs zi=in;zi.color_h=z.h;std::memcpy(&zi.color_s,&z.s,4);if(z.s==0.85f){const uint32_t b=0x3F59999Au;std::memcpy(&zi.color_s,&b,4);}
            if(z.s==0.9f){const uint32_t b=0x3F666666u;std::memcpy(&zi.color_s,&b,4);}
            const auto zt=BuildTonemap(d,zi);
            for(int i=0;i<16;++i){const std::string what=std::string(z.name)+" colour matrix word "+std::to_string(840+i);Word(what.c_str(),zt.color_matrix[size_t(i)],z.words[i]);}}}
    // TM-2: the bound block holds, at every word the two programmes read, the captured value.
    {std::array<uint8_t,tonemap_globals_bytes> block{};TonemapGlobals(t,block.data());
        const auto at=[&](uint32_t w){float f;std::memcpy(&f,block.data()+4*w,4);return f;};
        const std::pair<uint32_t,uint32_t> captured[]{{20,0x3F800000u},{21,0xBF800000u},{616,0x3FC28F5Cu},{617,0x3F800000u},{618,0x3F800000u},{619,0x3F800000u},
            {620,0},{621,0},{622,0},{623,0x3F800000u},{744,0x40000000u},{745,0xBF800000u},{746,0},{747,0},{748,0x3F000000u},{749,0},{750,0},{751,0},
            {752,0x40000000u},{753,0xBF800000u},{754,0},{755,0x3F800000u},{756,0x3F000000u},{757,0},{758,0},{759,0},{808,0x3EE8BA2Eu},{809,0x3B808081u},
            {810,0xBB008081u},{811,0},{832,0x40198186u},{833,0x3F039F8Eu},{834,0},{835,0}};
        for(const auto& [w,bits]:captured){const std::string what="globals word "+std::to_string(w);Word(what.c_str(),at(w),bits);}
        for(uint32_t i=0;i<16;++i){const std::string what="globals word "+std::to_string(840+i);Word(what.c_str(),at(840+i),matrix[i]);}}
    // TM-2: the dither texture from the executable's Bayer rows (141719770) against the original's t3 captured at m37
    // (i66-reference-1637-m37-tonemapcapture, reference-tonemap-t3.raw), all 64 bytes.
    {std::array<float,16> rows{};float scale{},bias{},to_byte{};
        if(!ds3light::Rdata(0x2B60D60,rows.data(),16) || !ds3light::Rdata(0x2B60D40,rows.data()+4,16) || !ds3light::Rdata(0x2B60D50,rows.data()+8,16) ||
           !ds3light::Rdata(0x2B60D30,rows.data()+12,16) || !ds3light::Rdata(0x3D7ED5C,&scale,4) || !ds3light::Rdata(0x2A5F3CC,&bias,4) || !ds3light::Rdata(0x2B60CE4,&to_byte,4)){
            ++checks;++failures;std::printf("FAIL the dither data words\n");}
        else{const auto texels=TonemapDither(rows,scale,bias,to_byte);
            constexpr uint8_t captured[64]{7,7,7,7,135,7,7,135,39,7,7,39,167,7,7,167,199,199,199,199,71,199,199,71,231,199,199,231,103,199,199,103,
                55,55,55,55,183,55,55,183,23,55,55,23,151,55,55,151,247,247,247,247,119,247,247,119,215,247,247,215,87,247,247,87};
            for(size_t i=0;i<64;++i){++checks;if(texels[i]!=captured[i]){++failures;std::printf("FAIL dither byte %zu: %u, captured %u\n",i,texels[i],captured[i]);}}}}
    // I-0113 tranche 20: the value scale and the contrast (E69A0's FUN_1416CBC30 then FUN_1416CBC90, each post-multiplied), and the
    // tone setter's guards (1416F71B0, FUN_141753F00). The data words first: half 0.5, FLT_MIN, 2.2.
    {++checks;if(Bits(d.color.half)!=0x3F000000u || Bits(d.color.min_exposure)!=0x00800000u || Bits(d.color.default_gamma)!=0x400CCCCDu){++failures;
            std::printf("FAIL the guard data words: %08X %08X %08X\n",Bits(d.color.half),Bits(d.color.min_exposure),Bits(d.color.default_gamma));}
        TonemapInputs vi=in;const float v=0.7f,c=0.98f;vi.color_v=v;const auto tv=BuildTonemap(d,vi);vi.contrast=c;const auto tc=BuildTonemap(d,vi);
        ++checks;if(!tv.matrix_supported || !tc.matrix_supported){++failures;std::printf("FAIL ColorV and Contrust served with the colour data\n");}
        // diag(v) after the saturation: every colour word of the m37 matrix times v (one product each), the last row and column kept.
        for(int i=0;i<16;++i){const bool colour=i%4<3 && i/4<3;
            Word(("ColorV 0.7 word "+std::to_string(840+i)).c_str(),tv.color_matrix[size_t(i)],colour?Bits(t.color_matrix[size_t(i)]*v):Bits(t.color_matrix[size_t(i)]));}
        // the contrast after it: colour words times c, the offsets (1 - c) 0.5 in words 843, 847, 851 (the transpose of row 3).
        const float offset=(1.0f-c)*0.5f;
        for(int i=0;i<16;++i){const bool colour=i%4<3 && i/4<3,shift=i%4==3 && i<12;
            Word(("Contrust 0.98 word "+std::to_string(840+i)).c_str(),tc.color_matrix[size_t(i)],colour?Bits(tv.color_matrix[size_t(i)]*c):shift?Bits(offset):Bits(tv.color_matrix[size_t(i)]));}
        TonemapInputs gi=in;gi.exposure=0.0f;gi.gamma=0.0f;auto tg=BuildTonemap(d,gi);
        Word("exposure 0: FLT_MIN",tg.exposure,0x00800000u);Word("gamma 0: 1 / 2.2",tg.gamma_correction,0x3EE8BA2Eu);
        gi.exposure=-1.0f;gi.gamma=-3.0f;tg=BuildTonemap(d,gi);Word("exposure -1: FLT_MIN",tg.exposure,0x00800000u);Word("gamma -3: 1 / 2.2",tg.gamma_correction,0x3EE8BA2Eu);
        gi.exposure=std::nanf("");gi.gamma=std::nanf("");tg=BuildTonemap(d,gi);Word("exposure NaN: FLT_MIN",tg.exposure,0x00800000u);Word("gamma NaN: 1 / 2.2",tg.gamma_correction,0x3EE8BA2Eu);
        ++checks;if(!tg.guarded){++failures;std::printf("FAIL the guards served with the colour data\n");}
        Word("exposure 1.52 kept",t.exposure,0x3FC28F5Cu);}
    // I-0113 tranche 20: the vignette's fov term (FUN_141751A60: (tan(pi/8) (1 - d) + d tan(fov / 2)) power). d = 0 keeps the
    // captured m31 rectangle whatever the fov; d = 1 at a 45-degree fov (tan(fov / 2) = tan(pi/8)) gives the same v; d = 0.5 at a
    // wider fov widens it by the term's mean; a non-zero d with a zero fov term scales v by (1 - d).
    {VignetteData vd;const uint32_t tan_bits=0x3ED413CDu,k_bits=0x3FAAAAA8u,m_bits=0x3F2AAAE3u;std::memcpy(&vd.tan_eighth_pi,&tan_bits,4);
        std::memcpy(&vd.k,&k_bits,4);std::memcpy(&vd.m,&m_bits,4);const float aspect=1280.0f/720.0f,power=0.4f;
        const auto base=VignetteRect(vd,power,aspect),zero_d=VignetteRect(vd,power,aspect,0.0f,0.9f),full=VignetteRect(vd,power,aspect,1.0f,vd.tan_eighth_pi);
        for(size_t i=0;i<4;++i){Word(("vignette d 0, rect "+std::to_string(i)).c_str(),zero_d[i],Bits(base[i]));Word(("vignette d 1 at 45 degrees, rect "+std::to_string(i)).c_str(),full[i],Bits(base[i]));}
        const float wide=0.7f,half=VignetteRect(vd,power,aspect,0.5f,wide)[3],none=VignetteRect(vd,power,aspect,0.25f,0.0f)[3];
        Word("vignette d 0.5, v",half,Bits((vd.tan_eighth_pi*(1.0f-0.5f)+0.5f*wide)*power));
        Word("vignette d 0.25 and no fov, v",none,Bits((vd.tan_eighth_pi*0.75f)*power));
        ++checks;if(!(base[3]<half)){++failures;std::printf("FAIL a wider fov must widen the vignette's rectangle\n");}
        // +0x138 from the camera's fov (VignetteFovTerm): tan(fov / 2); tan(pi/8) for a zero or negative tangent; NaN kept (refused).
        Word("fov term at 90 degrees",VignetteFovTerm(vd,1.5707964f),Bits(std::tan(1.5707964f*0.5f)));
        Word("fov term at 0",VignetteFovTerm(vd,0.0f),tan_bits);Word("fov term at -1",VignetteFovTerm(vd,-1.0f),tan_bits);
        ++checks;if(!std::isnan(VignetteFovTerm(vd,std::nanf("")))){++failures;std::printf("FAIL a NaN fov must stay NaN (the vignette refused)\n");}}
    std::printf("yebis_tonemap_test: %s (%d checks, %d failures)\n",failures?"FAILED":"passed",checks,failures);
    return failures?1:0;
}
