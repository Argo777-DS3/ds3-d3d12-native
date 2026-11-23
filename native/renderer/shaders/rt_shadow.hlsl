// I-0117 R5 (RT-PT-PLAN §8, D-0237): the sun's shadow traced (on with the rays). In place of the original's mask layer
// (GXScreenSpaceCascadedShadow_Soft from the cascade atlas, then its blur passes: SunShadowStage), one ray a G-buffer pixel from
// the surface the raster depth holds toward the sun, through the frame's top level with the sun casters' mask (RtMaskSun) and
// the alpha-tested slices' test (rt_material.hlsli): the shadow term in the mask's w (1 lit, 0 in shadow). On the main view by
// default: sharp (one ray; the `cone` below on frames Ray Reconstruction denoises, D-0237), xyz the « no shadow » packed depth
// (0, 0, 1) as the original's OutputColor (the blur that reads it is not run; the composite then writes the mask's RGB); the
// Player menu views' xyz and blur by `neutral` (below). Pixel programmes
// over one full-screen triangle, by the G-buffer stencil bit 0x08 as the original's: `fill` (bit clear: OutputColor's value)
// and `trace` (bit set). I-0140 tranche 35: `blur`, the original's GXScreenSpaceShadow_Blur on a Player menu view's traced term
// (the menu views' penumbra, below). Compiled by dxc at build time (Build-GameBootstrap.ps1): vs_6_6 `vs`, ps_6_6 `fill`, `trace`
// and `blur`.
#define SunRoot "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), CBV(b0), SRV(t0), SRV(t1), SRV(t2), "     "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_WRAP)"
#include "rt_material.hlsli"
#define RtMaskSunBits 2u   // RtMaskSun (rt_scene.h): the sun casters
// RtSunConstants (rt_scene.h): the camera as RtViewConstants (rows with the near and far planes, the lens scales, the jitter and
// the reciprocal extent), the depth's slot, whether a material table is bound (I-0119 R3a); the direction toward the sun (world, unit) and the
// ray's offset (metres, then a share of the distance, the ray's TMin; w 1: the orientation witness, rt-shadow-witness.flag).
// I-0123 RR-b: `cone` the cosine of the sun's angular radius (0: the sharp ray), `seed` the frame's.
// I-0140 tranche 31: `neutral` 1, the mask's xyz white (a Player menu view's: its lighting reads them as the neutral white), else (0, 0, 1).
// I-0140 tranche 35: `neutral` 2, a menu view's target that a blur pass reads next: xyz white where the G-buffer stencil bit 0x08 is
// set (`trace`, `blur`) and black where it is clear (`fill`), the blur's stand-in for the original's packed depth there (its
// OutputColor's (0, 0, 1), the depth 1, whose weight is 0); `source` the bindless slot of the target a blur pass reads, `blur` its
// scale (the original's [[GXRS+0x5368]+0x128] x the ShadowParam's Blur Radius, in pixels a tap unit).
struct Sun {float4 right;float4 up;float4 forward;float4 position;float4 jitter;uint width;uint height;uint depth;uint materials;float4 sun;float4 offset;float cone;uint seed;uint neutral;float pad3;
    uint source;float blur;uint pad4;uint pad5;};
ConstantBuffer<Sun> sun:register(b0);
RaytracingAccelerationStructure scene:register(t0);
StructuredBuffer<RtMaterialRecord> table:register(t1);
ByteAddressBuffer rows:register(t2);   // RtInstanceRow by instance index
SamplerState wrap:register(s0);
struct Out {float4 position:SV_Position;};
[RootSignature(SunRoot)]
Out vs(uint id:SV_VertexID){Out o;const float2 uv=float2((id<<1)&2,id&2);o.position=float4(uv*float2(2,-2)+float2(-1,1),0,1);return o;}
float3 Rgb(){return sun.neutral!=0?float3(1,1,1):float3(0,0,1);}
[RootSignature(SunRoot)]
float4 fill(Out i):SV_Target{return float4(sun.neutral==2?float3(0,0,0):Rgb(),1);}
[RootSignature(SunRoot)]
float4 trace(Out i):SV_Target{
    const uint2 p=uint2(i.position.xy);
    Texture2D<float> depth=ResourceDescriptorHeap[sun.depth];
    const float d=depth.Load(int3(p,0));
    if(d>=1)return float4(Rgb(),1);
    // The pixel's surface as the control view finds it (rt_view.hlsl): the ray through the pixel's centre, the jitter undone, at
    // the distance the raster depth gives.
    const float2 ndc=float2((p.x+0.5)*sun.jitter.z*2-1,1-(p.y+0.5)*sun.jitter.w*2)-sun.jitter.xy;
    const float3 v=float3(ndc.x/sun.forward.w,ndc.y/sun.position.w,1);
    const float scale=length(v);const float3 eye=(v.x*sun.right.xyz+v.y*sun.up.xyz+v.z*sun.forward.xyz)/scale;
    const float a=sun.up.w/(sun.up.w-sun.right.w);const float r=a*sun.right.w/(a-d)*scale;
    // Pulled back toward the eye by the offset (the depth's precision and the raster's LOD against the rays' LOD 0), then traced
    // toward the sun from a small distance on.
    const float back=sun.offset.x+sun.offset.y*r;
    // I-0118 (rt-shadow-witness.flag, a test tool): a surface facing away from the sun (its normal from the rebuilt positions'
    // screen derivatives) written 0.5 whatever its ray, so the mask's picture tells it apart from a ray's shadow (0) and a lit
    // surface (1). Computed only with the flag (a branch on a constant); the derivatives are undefined in a quad that also holds a
    // sky pixel (returned above), so a silhouette's pixels may be misread.
    if(sun.offset.w!=0){const float3 surface=sun.position.xyz+eye*r;const float3 facing=cross(ddy(surface),ddx(surface));
        if(dot(facing,sun.sun.xyz)*dot(facing,-eye)<0)return float4(Rgb(),0.5);}   // the normal toward the eye, then against the sun
    // I-0123 RR-b: on a frame Ray Reconstruction denoises, toward a point of the sun's disc (a new sample a pixel a frame), else sharp.
    const float3 toward=sun.cone>0?RtConeSample(sun.sun.xyz,sun.cone,RtRandom2(p,sun.seed,0)):sun.sun.xyz;
    RayDesc ray;ray.Origin=sun.position.xyz+eye*max(r-back,0);ray.Direction=toward;ray.TMin=sun.offset.z;ray.TMax=1.0e5;
    return float4(Rgb(),RtOccluded(scene,table,rows,sun.materials!=0,RtMaskSunBits,ray,wrap)?0:1);
}
// I-0140 tranche 35: the original's screen-space shadow blur (GXScreenSpaceShadow_Blur, ps_5_0, read with fxc /dumpbin from the
// I-0063 programme cache; its block cbBlurParam from 1D7850, SUN-SHADOWS-C4-STATIC-PREP 3.2) on a pixel with the stencil bit
// 0x08: the centre with the weight 1, then 12 taps at (x, y) * `blur` pixels from it (ClampPoint: the texel holding the point,
// the edge's beyond the view), each weighted by G = exp(-(x x + y y) / 2) / sqrt(2 pi) times exp(-100 (z - z0)^2 / z0^2), z the
// tap's view depth and z0 the centre's (the original decodes them from the source's packed xyz and linearises them by
// g_vDepthComputeParam; this pass reads the depth plane and linearises it by the camera's planes, as `trace` does: the ratio is
// the same); w the weighted mean of the taps' w. A tap the source marks off the G-buffer (xyz black, `fill` above) weighs 0, as
// the original's packed depth 1 there. xyz Rgb(): white (the last pass's mask) or the flag of a further pass's source.
static const float2 kBlurTaps[12]={float2(0,-2),float2(-1,-1),float2(0,-1),float2(1,-1),float2(-2,0),float2(-1,0),float2(1,0),float2(2,0),
    float2(-1,1),float2(0,1),float2(1,1),float2(0,2)};
float ViewDepth(float d){const float a=sun.up.w/(sun.up.w-sun.right.w);return a*sun.right.w/(a-d);}
[RootSignature(SunRoot)]
float4 blur(Out i):SV_Target{
    Texture2D<float4> source=ResourceDescriptorHeap[sun.source];
    Texture2D<float> depth=ResourceDescriptorHeap[sun.depth];
    const float2 centre=i.position.xy;const int2 last=int2(sun.width,sun.height)-1;
    const float z0=ViewDepth(depth.Load(int3(int2(centre),0)));
    float weights=1,sum=source.Load(int3(int2(centre),0)).w;
    [unroll] for(uint k=0;k<12;++k){
        const int2 q=clamp(int2(floor(centre+kBlurTaps[k]*sun.blur)),int2(0,0),last);
        const float4 tap=source.Load(int3(q,0));
        const float dz=ViewDepth(depth.Load(int3(q,0)))-z0;
        const float w=tap.x>0.5?exp(-0.5*dot(kBlurTaps[k],kBlurTaps[k]))*0.39894228*exp(-100*dz*dz/(z0*z0)):0;
        weights+=w;sum+=w*tap.w;}
    return float4(Rgb(),sum/weights);
}
