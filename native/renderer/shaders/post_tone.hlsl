// I-0138 tranche 1 (ROADMAP workstream 11): the tone-map pass of our own, in place of YEBIS's tone map and colour grading
// programmes. Two draws, as the original's: `ps` from the lit HDR into an 8-bit target (the exposure, the vignette, the
// sensitometric curve, the glare screen-blended after it, the colour matrix, the gamma, the 4x4 dither); with a LUT, `ps_lut` from
// that 8-bit image through the 16x16x16 LUT into the target.
//
// The arithmetic transcribes the original's pixel programmes operation for operation, so that the same inputs give the same
// bytes (post_tone_stage_test against the original's captured draws):
//   ps: ps_TonemapHDR_Dither[_Vignette]_Exposure_Matrix_Gamma (yebis_pins.h; its literals read with fxc /dumpbin /Lx: the floor
//   under the exposed scene 0x38800000 = 2^-14, the term added to the matrix's output before the saturate 0x34800000 = 2^-22);
//   ps_lut: ps_MadColorGradingLutT1_S1: rgb = LUT(t0.rgb x (N - 1) / N + 1 / 2N), a = t0.a.
// The original's adapted-exposure remap of t2 (its Offset words) is not transcribed: in the manual case it is exactly 1 and its
// additive glare term exactly 0; with our automatic exposure's words (1, 0, 1, 0 and 1) it is (f - mean) + mean, exactly f for a
// grey f (the subtraction is exact, Sterbenz).
// Why two draws and not one: the LUT reads the tone map's output as the 8-bit target holds it, and the card's float to UNORM
// conversion is not floor(x 255 + 0.5) (measured on the capture's adapter, I-0138 RESULT « Tranche 1 »: one draw with that rule in
// the programme reads the LUT a level off, mostly higher, on 2.4 % of the colour values). Only a real 8-bit target reproduces it.
// Compiled by fxc as shader model 5 with IEEE strictness (the original's compiler family; its programmes, shader model 4, carry no
// refactoring flag): every operation kept as written here, a mad where the original has one (Build-GameBootstrap.ps1) into
// post_tone_vs.dxbc.h, post_tone_ps.dxbc.h, post_tone_reinhard_ps.dxbc.h (I-0138 part 4c) and post_tone_lut_ps.dxbc.h.
//
// I-0138 tranche 5 (HDR10): ps_hdr and ps_hdr_reinhard, one draw into the float scene of an HDR10 chain (OutputStage): the same
// exposure, vignette, curve, glare and colour matrix (ToneColour, shared with ps), then the record's gamma, the LUT read
// directly (trilinear, no 8-bit image between: flags bit 3), and the highlights' gain: with v the largest channel of the curve's
// output (the glare blended in; before the colour matrix, whose value or contrast may bring a channel to 1 at any luminance:
// the step review's L5), kept up to hdr_knee and continued above it as hdr_knee + (1 - hdr_knee) ln((1 - hdr_knee) / (1 - v))
// (the curve's exponential shoulder continued at the knee's slope: hdr_output.h), the gain is that value over v, applied to the
// three channels in linear light. Written display-encoded (the power 1 / 2.2 of linear light, hdr_output.h's kSdrGamma, 1 the
// paper white), no dither (the output transform's). Where the curve's output is under the knee the picture is ps's before its dither.
// Into post_tone_hdr_ps.dxbc.h and post_tone_hdr_reinhard_ps.dxbc.h.
//
// I-0138 tranche 6 (the « Rekindled » colorimetry's candidates, behind test flags): ps_agx, ps_aces and their HDR entries
// ps_hdr_agx, ps_hdr_aces: the same pass with a candidate's curve in place of the record's (ToneColour's compile-time choice),
// on the exposed scene times look_scale (native/renderer/tone_looks.h: the candidate's middle grey matched to the area's curve;
// the models of the unit test are there). The glare, the colour matrix, the gamma, the dither and the LUT are the pass's own,
// unchanged; the original's entries hold the same instructions as before. The HDR entries' highlights follow the same rule as
// the original curve's, said in the scene's value: above the grey scene value the candidate shows at the knee (look_a), the
// picture rises with the scene along the candidate's tangent there (its slope look_b): the gain is that line over the
// candidate's own value of a grey of the pixel's largest channel. (ps_hdr's logarithm is that tangent exactly for a curve
// y = 1 - exp(-a x), and to the first order at the knee for the sensitometric curve's own shoulder; a candidate's shoulder is
// not an exponential, so the line is taken in the scene's value itself. Two differences from ps_hdr to know on a sheet: the
// candidate's gain reads the scene alone, not the glare blended after the curve, and its line has no ceiling but the output
// pass's roll-off to the peak, where ps_hdr's logarithm ends near 4.4 times the paper white.)
// AgX: after Google Filament's filament/src/ToneMapper.cpp (AgxToneMapper, the look NONE) with the conversions
// filament/src/details/ColorGrading.cpp makes around it, Copyright (C) 2021 The Android Open Source Project, licensed under the
// Apache License, Version 2.0; changed: written again in HLSL, the matrices as rows. ACES: Stephen Hill's fit as in Baking
// Lab's ACES.hlsl (MJP and David Neubelt), MIT license, Copyright (c) 2016 MJP. Notices: native/THIRD-PARTY-NOTICES.md.
// Into post_tone_agx_ps.dxbc.h, post_tone_aces_ps.dxbc.h, post_tone_hdr_agx_ps.dxbc.h and post_tone_hdr_aces_ps.dxbc.h.
//
// The candidate after Elden Ring (ps_er, ps_hdr_er; no code of Elden Ring's is here: its formulas and shipped values as read,
// investigations/I-0138-modern-post-chain/ER-VALUES.md §6 and evidence/I-0138/artifacts/er-tonemap-constants/REPORT.md), a body
// of its own (ToneColourEr), in Elden Ring's order: the bloom added to the exposed scene before the curve (ours is blended
// after it); its curve t = c^contrast, y = t / (t + 1) (ReinhardEx; the contrast look_c); its lateral dispersion
// (er_dispersion: the green and the blue channels read nearer the centre by a share of 2 uv - 1). Elden Ring's dispersion is a
// pass after its tone map, which reads the toned, encoded picture through a linear filter; here it is inside the pass: for the
// green and the blue channel, the four texels around the channel's place are each taken through the exposure, the bloom and the
// curve, encoded by the record's gamma, and then blended by the filter's weights (ErChannel; the step review's M1: the first
// form filtered the scene before the curve, which fringes every bright edge). What still differs from such a pass: the bloom
// is read at the place (it is smooth), the dither is added in place afterwards, and the
// HDR entries' gain is one factor a pixel, from the pixel's own texel (the largest of its three scene values in place: a
// blend of the neighbours' scene values would let a weight of a ten-thousandth times a clamped infinity set the gain). The
// area's colour matrix and LUT are not drawn with it (the block's matrix is the identity). Its vignette is the area's own, DS3's
// (I-0141 tranche 11, Thomas 2026-11-17: « on reprend la vignette de DS3 », it depends on the area; Elden Ring's dropped), with
// the vignette bit as in ToneColour: on the exposed scene before the curve, the bloom not vignetted, its factor taken at the pixel
// for the three channels (not at the green's and blue's dispersed places).
// I-0138 tranche 7: its midtone lift (er_lift.x, a power on the curve's output) is in c but not in the HDR entries' gain, which
// divides the tangent line by ErCurve(top) without the lift: above the knee the output is that line times ErCurve^-(1 - lift),
// continuous and smooth at the knee, but the lifted curve already reaches about 0.79 at the knee 0.75 (the final review's L8).
// Into post_tone_er_ps.dxbc.h and post_tone_hdr_er_ps.dxbc.h.
//
// I-0138 tranche 7: the film grain (the settings' row, none by default: a new effect, PLAN §5), added to the display-encoded
// picture by the pass's last draw, before the UI: ps_lut where a LUT draw follows the tone map (flags bit 6), else the tone map's
// own draw (before the 4x4 dither in the 8-bit entries; after the highlights' gain in the HDR entries). Monochrome: one value a
// pixel added to the three channels, a triangular noise in (-1, 1) at the corners of a grid of grain.y pixels (the picture's
// height over 1080, at least 1: the grain's size follows the resolution), smoothly interpolated between them (the corners'
// values a hash of their place and the frame's seed: Jarzynski and Olano's pcg3d, « Hash Functions for GPU Rendering », JCGT 9(3),
// 2020), times grain.x (the strength) and a weight on the pixel's luma, 1 at mid-grey and 0.25 at black and white (film's grain
// shows most in the midtones). grain.x 0 leaves every entry's bytes as before (the selection is exact). What runs after this pass
// still touches it (the step review's L1): the NIS sharpening (with DLSS or the TAA) sharpens it, the emissive glow adds over it,
// frame generation blends two frames' grains; below 1080 lines its grid stays one pixel (a grain cannot be finer than a pixel).

cbuffer PostToneParams:register(b0){   // native/renderer/post_params.h, the same layout (post_tone_test)
    float4 colour[3];        // the colour matrix's rows: c[i] = dot(float4(y, 1), colour[i])
    float4 alpha_weights;    // the output alpha's weights on the scene texel
    float4 vignette_uv[3];   // .xy: the vignette's coordinates at the three vertices (the original's TEXCOORD2)
    float exposure;
    float curve_a;           // the curve's constants: the sensitometric curve's two, or the Reinhard curve's c208.x and .y
    float curve_b;
    float inv_gamma;
    float glare_scale;
    float dither_scale;
    float dither_offset;
    uint flags;              // bit 0 the vignette, bit 1 the automatic exposure's factor, bit 2 the Reinhard curve (ps_reinhard), bit 3 the LUT in ps_hdr,
                             // bit 6 the grain after a LUT draw (post_params.h; the look's bits 4-5 cleared by the stage)
    float lut_scale;         // (N - 1) / N
    float lut_offset;        // 1 / 2N
    float curve_c;           // the Reinhard curve's third constant (I-0138 part 4c)
    float hdr_knee;          // ps_hdr: the highlights' knee (I-0138 tranche 5)
    float look_scale;        // the candidates' entries (I-0138 tranche 6): on the exposed scene before the candidate's curve
    float look_a;            // the candidates' HDR entries: the grey scene value (after look_scale) shown at hdr_knee
    float look_b;            // and the candidate's slope there (0: no gain)
    float look_c;            // the candidate after Elden Ring: its curve's contrast
    float4 er_dispersion;    // that candidate's lateral dispersion: .xy the green channel's share of 2 uv - 1, .zw the blue's
    float4 er_lift;          // I-0138 tranche 7: its midtone lift, .x the power on the curve's output (1: none); tranche 7b:
                             // .yzw the vivid variant's contrast, vibrance and cool black (ErVivid; 0: none)
    float3 grain;            // I-0138 tranche 7: the film grain, .x its strength (0: none), .y its grid in pixels, .z unused
    uint grain_seed;         // and the frame's seed
};
SamplerState glare_sampler:register(s0);  // trilinear, clamp
SamplerState lut_sampler:register(s1);    // linear with point mips, clamp

struct Vertex {float2 uv:TEXCOORD0;float2 vignette:TEXCOORD1;float4 position:SV_Position;};

// The original's three vertices (vs_2D_Tex2D5 and vs_2D_OffsetTex2D1 from positions (-1, 1), (-1, -3), (3, 1) scaled by (1, -1),
// z 0.1, w 1), in its order: the rasteriser interpolates the texture and vignette coordinates as it does the original's.
Vertex vs(uint id:SV_VertexID){
    static const float2 positions[3]={float2(-1.0,-1.0),float2(-1.0,3.0),float2(3.0,-1.0)};
    static const float2 uvs[3]={float2(0.0,1.0),float2(0.0,-1.0),float2(2.0,1.0)};
    Vertex v;v.uv=uvs[id];v.vignette=vignette_uv[id].xy;v.position=float4(positions[id],0.1,1.0);return v;
}

Texture2D<float4> scene:register(t0);     // the lit HDR at the target's extent
Texture2D<float4> glare:register(t1);     // the glare composite, any extent (a null view: black)
Texture2D<float4> adapted:register(t2);   // the automatic exposure's 1x1 factor
Texture2D<float4> dither:register(t3);    // the 4x4 dither texture, repeated a texel a pixel

// I-0138 tranche 6: the candidates' curves (the head of this file; tone_looks.h holds the same arithmetic in double). Linear
// Rec. 709 in, linear Rec. 709 out, 0 to 1.
// AgX. Filament's ColorSpaceUtils.h: sRGB_to_Rec2020 = XYZ_to_Rec2020 * sRGB_to_XYZ and Rec2020_to_sRGB = XYZ_to_sRGB *
// Rec2020_to_XYZ; its ToneMapper.cpp: AgXInsetMatrix and the inverse of AgXOutsetMatrixInv (Blender's AgX, Rec. 2020 primaries).
static const float3x3 kSrgbToRec2020={0.627507418,0.329277710,0.0433038712, 0.0691077370,0.919504431,0.0113590816, 0.0163964798,0.0880243409,0.895513524};
static const float3x3 kRec2020ToSrgb={1.66021335,-0.587564311,-0.0728275822, -0.124552039,1.13294619,-0.00834833000, -0.0181550265,-0.100604791,1.11883167};
static const float3x3 kAgxInset={0.856627153,0.0951212405,0.0482516061, 0.137318973,0.761241991,0.101439036, 0.111898213,0.0767994186,0.811302368};
static const float3x3 kAgxOutset={1.12710058,-0.110606643,-0.0164939387, -0.141329764,1.15782370,-0.0164939387, -0.141329764,-0.110606643,1.25193641};
static const float kAgxMinEv=-12.47393,kAgxMaxEv=4.026069;
float3 AgxContrast(float3 x){   // agxDefaultContrastApprox
    const float3 x2=x*x,x4=x2*x2,x6=x4*x2;
    return -17.86*x6*x+78.01*x6-126.7*x4*x+92.06*x4-28.72*x2*x+4.361*x2-0.1718*x+0.002857;
}
float3 Agx(float3 v){
    v=mul(kSrgbToRec2020,v);
    v=max(v,0.0);
    v=mul(kAgxInset,v);
    v=log2(max(v,1e-10));
    v=saturate((v-kAgxMinEv)/(kAgxMaxEv-kAgxMinEv));
    v=AgxContrast(v);
    v=mul(kAgxOutset,v);
    v=pow(max(v,0.0),2.2);
    return saturate(mul(kRec2020ToSrgb,v));
}
// ACES by Stephen Hill's fit (Baking Lab's ACES.hlsl): « sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT », RRTAndODTFit,
// « ODT_SAT => XYZ => D60_2_D65 => sRGB ».
static const float3x3 kAcesInput={0.59719,0.35458,0.04823, 0.07600,0.90834,0.01566, 0.02840,0.13383,0.83777};
static const float3x3 kAcesOutput={1.60475,-0.53108,-0.07367, -0.10208,1.10813,-0.00605, -0.00327,-0.07276,1.07602};
float3 Aces(float3 v){
    v=mul(kAcesInput,v);
    const float3 a=v*(v+0.0245786)-0.000090537;
    const float3 b=v*(0.983729*v+0.4329510)+0.238081;
    return saturate(mul(kAcesOutput,a/b));
}

// I-0138 part 4c: ps_reinhard, the record's tone type 3 (the library's TonemapHDR_..._Gamma_REINHARD: the same programme but
// its curve, s = x c208.y, y = sat(s (1 + s c208.z) / (1 + x c208.y)), read in the eight listings,
// evidence/I-0138/artifacts/tranche-4-programmes/reinhard/). One body, the curve a compile-time choice: 0 the sensitometric
// curve, 1 the Reinhard curve, 2 AgX, 3 ACES (I-0138 tranche 6: the candidates; 4, the candidate after Elden Ring, has its own
// body below).
// The colour matrix's output (linear, 0 to 1), the output alpha and the largest channel of the curve's output (with its glare).
// scene_top (the candidates only): the largest channel of the scene value the candidate's curve read.
float3 ToneColour(Vertex v,const int curve,const int2 pixel,out float alpha,out float curve_top,out float scene_top){
    scene_top=0.0;
    float3 k=float3(1.0,1.0,1.0);
    if(flags&2u)k=adapted.Load(int3(0,0,0)).rgb;
    const float3 gl=saturate((glare.SampleLevel(glare_sampler,v.uv,0.0).rgb*k)*glare_scale);
    const float4 texel=scene.Load(int3(pixel,0));
    float3 x=k*texel.rgb;
    alpha=saturate(dot(texel,alpha_weights));
    x=x*exposure;
    if(flags&1u){
        const float d=dot(v.vignette,v.vignette);
        float w=saturate(mad(-d,2.0,d+1.0));
        w=w*w;w=w*w;x=w*x;}
    x=max(x,0.00006103515625);
    float3 y;
    if(curve==1){
        const float3 s=x*curve_b;
        y=saturate((s*mad(s,curve_c,1.0))/mad(x,curve_b,1.0));}
    else if(curve>=2){
        x=min(x*look_scale,65504.0);   // 65504: the largest half float, so an infinite scene texel stays finite (the fit's
        scene_top=max(x.r,max(x.g,x.b));   // quotient of two infinities would be a NaN, drawn black)
        if(curve==2)y=Agx(x);else y=Aces(x);}
    else{
        const float3 e=exp(-x*curve_a);
        const float3 shoulder=mad(-e,curve_b,1.0);
        y=saturate((1.0-e)*(shoulder*shoulder));}
    y=mad(1.0-y,gl,y);
    curve_top=max(y.r,max(y.g,y.b));
    const float4 y1=float4(y,1.0);
    return saturate(float3(dot(y1,colour[0]),dot(y1,colour[1]),dot(y1,colour[2]))+2.384185791015625e-7);
}
// The candidate after Elden Ring (the head of this file).
float3 ErCurve(float3 c){const float3 t=pow(max(c,0.0),look_c);return t/(t+1.0);}
// A scene value of the candidate: the exposed texel with the bloom added, scaled (the grey match), finite. w: the area's own
// vignette on the exposed scene (as ToneColour's, the bloom not vignetted; 1: none).
float ErScene(const float lit,const float k,const float bloom,const float w){
    return min((max(w*k*lit*exposure,0.00006103515625)+max(bloom*k*glare_scale,0.0))*look_scale,65504.0);
}
// One channel at a place of the picture (the head of this file): the four texels around it (loaded, their addresses clamped:
// the linear filter's own footprint, floor(place size - 1/2) and the next), each a toned, encoded value, blended by the
// filter's weights; the blend decoded to linear light.
float ErChannel(const int channel,const float2 place,const float2 size,const float k,const float bloom,const float w){
    const float2 at=place*size-0.5;
    const float2 base=floor(at);
    const float2 f=at-base;
    const int2 low=int2(base),most=int2(size)-1;
    const float4 lit=float4(scene.Load(int3(clamp(low,0,most),0))[channel],scene.Load(int3(clamp(low+int2(1,0),0,most),0))[channel],
        scene.Load(int3(clamp(low+int2(0,1),0,most),0))[channel],scene.Load(int3(clamp(low+int2(1,1),0,most),0))[channel]);
    const float4 weights=float4((1.0-f.x)*(1.0-f.y),f.x*(1.0-f.y),(1.0-f.x)*f.y,f.x*f.y);
    const float4 c=float4(ErScene(lit.x,k,bloom,w),ErScene(lit.y,k,bloom,w),ErScene(lit.z,k,bloom,w),ErScene(lit.w,k,bloom,w));
    const float4 t=pow(c,look_c);
    const float4 y=pow(t/(t+1.0),er_lift.x);
    return pow(dot(pow(y,inv_gamma),weights),1.0/inv_gamma);
}
float3 ToneColourEr(Vertex v,const int2 pixel,out float alpha,out float curve_top,out float scene_top){
    float3 k=float3(1.0,1.0,1.0);
    if(flags&2u)k=adapted.Load(int3(0,0,0)).rgb;
    const float2 centred=v.uv*2.0-1.0;
    const float2 place_g=v.uv+er_dispersion.xy*centred,place_b=v.uv+er_dispersion.zw*centred;
    float2 size;
    scene.GetDimensions(size.x,size.y);
    const float4 texel=scene.Load(int3(pixel,0));   // the red channel and the alpha in place
    alpha=saturate(dot(texel,alpha_weights));
    const float3 bloom=glare.SampleLevel(glare_sampler,v.uv,0.0).rgb;
    float w=1.0;
    if(flags&1u){const float d=dot(v.vignette,v.vignette);w=saturate(mad(-d,2.0,d+1.0));w=w*w;w=w*w;}
    const float red=ErScene(texel.r,k.r,bloom.r,w);
    scene_top=max(red,max(ErScene(texel.g,k.g,bloom.g,w),ErScene(texel.b,k.b,bloom.b,w)));   // the pixel's own texel
    const float3 y=float3(pow(ErCurve(float3(red,red,red)).r,er_lift.x),
        ErChannel(1,place_g,size,k.g,glare.SampleLevel(glare_sampler,place_g,0.0).g,w),ErChannel(2,place_b,size,k.b,glare.SampleLevel(glare_sampler,place_b,0.0).b,w));
    curve_top=max(y.r,max(y.g,y.b));
    const float4 y1=float4(y,1.0);
    return saturate(float3(dot(y1,colour[0]),dot(y1,colour[1]),dot(y1,colour[2]))+2.384185791015625e-7);
}
// I-0138 tranche 7: the film grain (the head of this file).
float GrainHash(uint3 v){   // pcg3d, its first word in [0, 1)
    v=v*1664525u+1013904223u;
    v.x+=v.y*v.z;v.y+=v.z*v.x;v.z+=v.x*v.y;
    v^=v>>16u;
    v.x+=v.y*v.z;v.y+=v.z*v.x;v.z+=v.x*v.y;
    return float(v.x>>8u)*(1.0/16777216.0);
}
float GrainCorner(const int2 corner){   // the sum of two uniforms less 1: triangular in (-1, 1)
    const uint3 v=uint3(asuint(corner),grain_seed);
    return GrainHash(v)+GrainHash(v^uint3(0x9E3779B9u,0x85EBCA6Bu,0xC2B2AE35u))-1.0;
}
float3 Grain(const float3 encoded,const int2 pixel){
    if(grain.x<=0.0)return encoded;
    const float2 p=(float2(pixel)+0.5)/grain.y-0.5;
    const float2 cell=floor(p);
    float2 f=p-cell;f=f*f*(3.0-2.0*f);
    const int2 c=int2(cell);
    const float n=lerp(lerp(GrainCorner(c),GrainCorner(c+int2(1,0)),f.x),lerp(GrainCorner(c+int2(0,1)),GrainCorner(c+int2(1,1)),f.x),f.y);
    const float luma=dot(saturate(encoded),float3(0.2126,0.7152,0.0722));
    return max(encoded+grain.x*mad(3.0*luma,1.0-luma,0.25)*n,0.0);
}
float3 ErVivid(float3 e){   // I-0138 tranche 7b: the vivid variant (tone_looks.h ErVivid, the same arithmetic in double); er_lift.yzw 0: e as it is
    if(er_lift.y<=0.0 && er_lift.z<=0.0 && er_lift.w<=0.0)return e;
    const float3 d=saturate(1.0-e);
    e*=1.0-er_lift.y*d*d*d;
    const float mx=max(e.r,max(e.g,e.b)),mn=min(e.r,min(e.g,e.b));
    const float sat=(mx-mn)/max(mx,1e-4);
    const float skin=saturate(3.0*min(e.g-e.b,e.r-e.g)/max(e.r-e.b,1e-4));
    e=mx-(mx-e)*(1.0+er_lift.z*(1.0-sat)*(1.0-0.5*skin));
    const float w=saturate(1.0-2.0*mx),w2=w*w;
    e.r*=1.0-10.0*er_lift.w*w2;e.b+=er_lift.w*w2;
    return e;
}
float4 Tone(Vertex v,const int curve){
    const int2 pixel=int2(v.position.xy);float alpha,curve_top,scene_top;
    float3 c;
    if(curve==4)c=ToneColourEr(v,pixel,alpha,curve_top,scene_top);else c=ToneColour(v,curve,pixel,alpha,curve_top,scene_top);
    float3 encoded=exp2(log2(c)*inv_gamma);
    if(curve==4)encoded=ErVivid(encoded);   // I-0138 tranche 7b
    if(!(flags&64u))encoded=Grain(encoded,pixel);   // bit 6: a LUT draw follows and adds it
    return float4(mad(dither.Load(int3(pixel&3,0)).r,dither_scale,encoded)+dither_offset,alpha);
}
float4 ps(Vertex v):SV_Target{return Tone(v,0);}
float4 ps_reinhard(Vertex v):SV_Target{return Tone(v,1);}
float4 ps_agx(Vertex v):SV_Target{return Tone(v,2);}
float4 ps_aces(Vertex v):SV_Target{return Tone(v,3);}
float4 ps_er(Vertex v):SV_Target{return Tone(v,4);}

Texture2D<float4> toned:register(t0);     // the tone map's 8-bit output at the target's extent
Texture3D<float4> lut:register(t1);       // the 16x16x16 LUT

float4 ps_lut(Vertex v):SV_Target{
    const float4 texel=toned.Sample(lut_sampler,v.uv);   // the original's filter and coordinates: the texel itself
    return float4(Grain(lut.Sample(lut_sampler,mad(texel.rgb,lut_scale,lut_offset)).rgb,int2(v.position.xy)),texel.a);
}

// I-0138 tranche 5: the HDR entries (the head of this file).
Texture3D<float4> hdr_lut:register(t3);   // ps_hdr: the 16x16x16 LUT in the dither's place (bound with flags bit 3)

float4 ToneHdr(Vertex v,const int curve){
    const int2 pixel=int2(v.position.xy);float alpha,m,top;
    float3 c;
    if(curve==4)c=ToneColourEr(v,pixel,alpha,m,top);else c=ToneColour(v,curve,pixel,alpha,m,top);
    float gain=1.0;
    if(curve>=2){   // a candidate (the head of this file)
        if(top>look_a){
            float grey;
            if(curve==2)grey=Agx(float3(top,top,top)).g;else if(curve==3)grey=Aces(float3(top,top,top)).g;else grey=ErCurve(float3(top,top,top)).g;
            gain=mad(look_b,top-look_a,hdr_knee)/max(grey,1e-4);}}
    else if(m>hdr_knee)gain=(hdr_knee+(1.0-hdr_knee)*log((1.0-hdr_knee)/max(1.0-m,1e-7)))/m;
    float3 encoded=exp2(log2(c)*inv_gamma);
    if(curve==4)encoded=ErVivid(encoded);   // I-0138 tranche 7b: before the gain, which scales the three channels alike (c is at most 1)
    if(flags&8u)encoded=hdr_lut.SampleLevel(lut_sampler,mad(encoded,lut_scale,lut_offset),0.0).rgb;
    return float4(Grain(encoded*pow(gain,1.0/2.2),pixel),alpha);
}
float4 ps_hdr(Vertex v):SV_Target{return ToneHdr(v,0);}
float4 ps_hdr_reinhard(Vertex v):SV_Target{return ToneHdr(v,1);}
float4 ps_hdr_agx(Vertex v):SV_Target{return ToneHdr(v,2);}
float4 ps_hdr_aces(Vertex v):SV_Target{return ToneHdr(v,3);}
float4 ps_hdr_er(Vertex v):SV_Target{return ToneHdr(v,4);}

// I-0138 tranche 8: the LUT strip's copy into the volume, our own in place of GX's LutCopy2DTo3DCs (the gxposteffect bundle's
// compute programme, I-0063 programme cache; read with fxc /dumpbin: [4,16,1] threads, the group's z the slice, each thread four
// columns x, x + 4, x + 8, x + 12 of row y; ld then store_uav_typed, no arithmetic): 3D[x][y][z] = 2D[x][16z + y], the strip's
// texels as the view converts them (R8G8B8A8_UNORM to float), into the R32G32B32A32_FLOAT volume. Dispatch(1, 1, 16).
Texture2D<float4> lut_strip:register(t0);
RWTexture3D<float4> lut_volume:register(u0);
[numthreads(4,16,1)]
void cs_lut_copy(uint3 group:SV_GroupID,uint3 thread:SV_GroupThreadID){
    [unroll] for(uint k=0;k<4;++k){const uint x=thread.x+4*k;
        lut_volume[uint3(x,thread.y,group.z)]=lut_strip.Load(int3(x,16*group.z+thread.y,0));}
}
