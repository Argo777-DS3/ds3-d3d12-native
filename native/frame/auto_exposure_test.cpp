// I-0132 R11h: the automatic exposure's kernels as built (cs_5_0, both entries; R12h step 2: by fxc at build time) and its math (auto_exposure_math.h, the shader's
// steps on the CPU): the displayed grey rises with the exposure, the kept weights are the pixels between the percentiles, the
// bisection finds the factor whose mean displayed grey is the target, the adaptation is linear beyond the transition and
// exponential within it, its rate continuous there.
#include "../renderer/auto_exposure_math.h"
#include <windows.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include "auto_exposure_histogram.dxbc.h"   // I-0132 R12h step 2: the bytes the module uses (shaders/auto_exposure.hlsl)
#include "auto_exposure_adapt.dxbc.h"
#include <cstddef>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
int failures=0;
void Expect(bool ok,const char* what){std::printf("%s %s\n",ok?"ok  ":"FAIL",what);if(!ok)++failures;}
}
int main(){
    using namespace ds3native::exposure;
    struct Built {const char* entry;const BYTE* bytes;size_t size;};
    for(const Built& kernel:{Built{"Histogram",auto_exposure_histogram_dxbc,sizeof(auto_exposure_histogram_dxbc)},Built{"Adapt",auto_exposure_adapt_dxbc,sizeof(auto_exposure_adapt_dxbc)}}){
        const char* entry=kernel.entry;ID3D12ShaderReflection* shape{};D3D12_SHADER_DESC sd{};
        const bool compute=SUCCEEDED(D3DReflect(kernel.bytes,kernel.size,IID_PPV_ARGS(&shape))) && SUCCEEDED(shape->GetDesc(&sd)) &&
            D3D12_SHVER_GET_TYPE(sd.Version)==D3D12_SHVER_COMPUTE_SHADER && D3D12_SHVER_GET_MAJOR(sd.Version)==5;
        if(shape)shape->Release();
        char what[64]{};std::snprintf(what,sizeof what,"kernel %s built (cs_5_0)",entry);Expect(compute,what);
        // I-0138 tranche 3 (its step review's L1): the kernel's b0 against our tone-map block, its size and the offsets of the
        // five values the solver reads (the kernel that reads them: Adapt).
        if(!std::strcmp(entry,"Adapt")){ID3D12ShaderReflection* reflection{};using ds3native::PostToneParams;
            bool layout=SUCCEEDED(D3DReflect(kernel.bytes,kernel.size,IID_PPV_ARGS(&reflection)));
            if(layout){auto* block=reflection->GetConstantBufferByName("Tone");D3D12_SHADER_BUFFER_DESC desc{};
                layout=SUCCEEDED(block->GetDesc(&desc)) && desc.Size==sizeof(PostToneParams);
                struct Field {const char* name;size_t offset,size;};
                const Field fields[]{{"tone_colour",offsetof(PostToneParams,colour),48},{"tone_exposure",offsetof(PostToneParams,exposure),4},
                    {"tone_curve_a",offsetof(PostToneParams,curve_a),4},{"tone_curve_b",offsetof(PostToneParams,curve_b),4},{"tone_inv_gamma",offsetof(PostToneParams,inv_gamma),4}};
                for(const auto& f:fields){D3D12_SHADER_VARIABLE_DESC v{};
                    layout=layout && SUCCEEDED(block->GetVariableByName(f.name)->GetDesc(&v)) && v.StartOffset==f.offset && v.Size==f.size;}
                reflection->Release();}
            Expect(layout,"the kernels' Tone block is PostToneParams: its size, and the exposure, the curve, the matrix and 1 / gamma at their offsets");}
    }
    // Firelink's words (i132-r11d2 runs, native_yebis_tonemap: exposure 1.52, gamma 2.2, curve 2.39853048 0.514153361), the identity matrix.
    ds3native::PostToneParams block{};block.exposure=1.52f;block.inv_gamma=1.0f/2.2f;block.curve_a=2.39853048f;block.curve_b=0.514153361f;
    block.colour[0][0]=block.colour[1][1]=block.colour[2][2]=1.0f;
    const ToneWords t=ToneWordsOf(block);
    Expect(t.exposure==1.52f && t.rows[1][1]==1.0f && t.rows[0][3]==0.0f,"the tone map's values read from our parameter block");
    bool rising=true;float last=-1.0f;for(int i=-12;i<=8;++i){const float d=DisplayGrey(t,std::exp2(float(i)));if(!(d>=last))rising=false;last=d;}
    Expect(rising && DisplayGrey(t,0.0f)<0.01f && DisplayGrey(t,1e6f)>0.99f,"the displayed grey rises from black to white with the exposed value");
    constexpr float log2_min=-20.0f,bin_width=0.125f;
    std::array<uint32_t,256> bins{};bins[100]=1000;   // every pixel in one bin
    auto w=KeptWeights(bins,0.1f,0.9f);float kept{};for(const auto v:w)kept+=v;
    Expect(std::abs(kept-800.0f)<1e-3f && w[100]==kept,"one bin keeps the pixels between the 10th and the 90th percentile");
    const float goal=SolveLog2(t,w,log2_min,bin_width,0.35f,-8.0f,10.0f);
    const float lum=std::exp2(log2_min+100.5f*bin_width);
    Expect(std::abs(DisplayGrey(t,lum*t.exposure*std::exp2(goal))-0.35f)<1e-3f,"the bisection finds the factor whose displayed grey is the target");
    std::array<uint32_t,256> spread{};spread[10]=100;spread[120]=800;spread[250]=100;   // the darkest and the brightest tenth left out
    w=KeptWeights(spread,0.1f,0.9f);
    Expect(w[10]==0.0f && w[250]==0.0f && w[120]==800.0f,"the percentiles leave the darkest and the brightest tenth out");
    std::array<uint32_t,256> single{};single[120]=1;
    Expect(std::abs(SolveLog2(t,w,log2_min,bin_width,0.35f,-8.0f,10.0f)-SolveLog2(t,KeptWeights(single,0.0f,1.0f),log2_min,bin_width,0.35f,-8.0f,10.0f))<1e-3f,
        "the solved factor follows the kept bins only");
    Expect(SolveLog2(t,w,log2_min,bin_width,1.01f,-8.0f,10.0f)>9.99f && SolveLog2(t,w,log2_min,bin_width,-0.01f,-8.0f,10.0f)<-7.99f,"an unreachable target stops at the bound");
    // I-0138 tranche 5e: with the game's lower bound (log2 a >= 0) a view brighter than the target keeps the area's own exposure
    // (the factor 1) and a darker one is brightened as without the bound.
    // The bright bin solves inside the free range (about -3.9: not the old bound's -8; the step review's L2).
    {std::array<uint32_t,256> bright{};bright[150]=1000;std::array<uint32_t,256> dark{};dark[110]=1000;
        const auto wb=KeptWeights(bright,0.1f,0.9f),wd=KeptWeights(dark,0.1f,0.9f);const float bound=ds3native::kAutoExposureMinLog2;
        const float free_bright=SolveLog2(t,wb,log2_min,bin_width,0.2f,-8.0f,10.0f),free_dark=SolveLog2(t,wd,log2_min,bin_width,0.2f,-8.0f,10.0f);
        const float bound_bright=SolveLog2(t,wb,log2_min,bin_width,0.2f,bound,10.0f),bound_dark=SolveLog2(t,wd,log2_min,bin_width,0.2f,bound,10.0f);
        Expect(bound==0.0f && free_bright<-1.0f && free_bright>-7.0f && std::abs(bound_bright)<1e-4f,"the lower bound: a bright view is no longer darkened (the factor stays 1)");
        Expect(free_dark>1.0f && std::abs(bound_dark-free_dark)<1e-4f,"the lower bound: a dark view is brightened as before");}
    // The adaptation: 3 stops a second towards a smaller factor, 1 towards a larger one, beyond 1.5 stops; exponential within.
    Expect(std::abs(Adapt(0.0f,5.0f,0.1f,3.0f,1.0f,1.5f)-0.1f)<1e-5f,"towards a darker scene, linear at 1 stop a second");
    Expect(std::abs(Adapt(0.0f,-5.0f,0.1f,3.0f,1.0f,1.5f)+0.3f)<1e-5f,"towards a brighter scene, linear at 3 stops a second");
    Expect(std::abs(Adapt(0.0f,1.6f,1.0f,3.0f,1.0f,1.5f)-0.1f)<1e-5f,"the linear step stops at the transition");
    const float inside=Adapt(0.0f,1.0f,0.01f,3.0f,1.0f,1.5f),rate=inside/0.01f;
    Expect(std::abs(rate-1.0f/1.5f)<0.01f,"within the transition, the rate is the distance times the speed over the transition");
    const float edge=Adapt(0.0f,1.5f,0.001f,3.0f,1.0f,1.5f)/0.001f;
    Expect(std::abs(edge-1.0f)<0.01f,"the rate is continuous at the transition (1 stop a second there)");
    Expect(Adapt(1.0f,1.0f,0.1f,3.0f,1.0f,1.5f)==1.0f,"no step at the goal");
    std::printf("auto exposure: %s\n",failures?"FAILED":"passed");
    return failures?1:0;
}
