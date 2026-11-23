// I-0125 R6b (R6-PLAN §3, on with the rays): the deferred surfaces' specular ambient traced. In place of the
// region draws (GXEnvMapRegion and _Pcc: one box a region blending cube(R, min(6.5 − 6.5 g, 6)) × E × g_envMapColorScale ×
// min(N·V + 1, 1)² with alpha smoothstep(fade / g_envMapFadeDistance) × g_envMapFadeRate into SpecularAcc, E = F0 + (1 − F0)
// (1 − sat(N·V))⁵ g²: R6-PLAN §1), one full-screen triangle over the same pixels (the G-buffer stencil bit 0x08) that walks the
// frame's regions in their order: where a ray is traced, its hit's radiance takes every region's cube sample's place (the regions'
// weights kept: the result times their coverage 1 − Π(1 − w)); where it misses without the forward sky, black. I-0132 R11d (D-0262: no
// lighting cheat): every surface's GGX lobe is sampled at both levels (R6b's roughness cut, the RT level's rough surfaces taking the
// cubes, is gone); a frame without a top level gives nothing (R11e: its cubes along R, the fallback, removed; it is not shown). The
// ray: one GGX sample of the surface's lobe (α = r², the RR guide's roughness; R itself for a mirror's sharp lobe), its noise left to
// Ray Reconstruction on the frames it denoises. The hit's radiance as DS3's deferred chain would give that surface: albedo × (its
// ambient (HitAmbient: the radiance cache's answer alone since I-0132 R11d; R6a's baked RtAmbientOf and R7b's seed removed)
// + its light set's directional lights + the local lights) + their specular; the set's first light (the sun) seen as `s`, one ray along
// the frame's sun (the traced shadow's direction; 1 on a frame without it; no FC_ShadowColor floor since R11d; step review M2); no AO and no mask's z (no screen-space term off screen: t32.x and t32.z
// taken 1), no translucency (the rays' materials read none), I-0126 R7b its emission (g_EmissiveTexture × FC_EmissiveColor, GB5's), no fog, no specular ambient at the hit (one bounce). A hit on an instance without its records (its set not ready, a
// view refused) or on a record without its material is taken as a miss, not drawn grey (step review M4, the final review's H2:
// D-0197). (R4's "nothing traced where no region covers the pixel" is gone since I-0132 R11d.) The local lights: every light of the list
// (R4-PLAN §1: no frustum cull) unshadowed at the RT level; at the PT level one chosen in proportion to its unshadowed terms over
// them all (R4c's estimator) and its ray. R6c (R6-PLAN §6): on the frames RR denoises, `trace_hit` also writes the specular hit
// distance (its second target); `water` makes the water's g_ReflectionTexture (t54) by rays in place of the mirror view's draws.
// I-0126 R7a: `gi`, the diffuse ambient traced (at the end of this file). R7b: the hits' ambient from NVIDIA's radiance cache (SHaRC,
// `cache_update` and `cache_resolve`, at the end of this file) on the GI frames, and the hits' emission.
// Compiled by dxc at build time (Build-GameBootstrap.ps1, with 16-bit types and SHaRC's headers from third-party/, outside the
// repository: D-0252): vs_6_6 `vs`, ps_6_6 `trace`, `trace_hit`, `water`, `gi`, `nrd_signal`, `gi_compose`, `spec_compose` and `cache_update`
// (SHARC_UPDATE 1), cs_6_6 `cache_resolve`; I-0126 R7d: ps_6_6 `gi_temporal`, `gi_spatial`, `gi_shade`. This software contains source code
// provided by NVIDIA Corporation (SHaRC's headers, I-0126 R7c NRD's front end NRD.hlsli, R7d RTXDI's GI reservoirs, included).
#define ReflectRoot "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), CBV(b0), SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), SRV(t5), SRV(t6), "     "UAV(u0), UAV(u1), UAV(u2), UAV(u3), UAV(u4), SRV(t7), "     "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_WRAP)"
#include "rt_lights.hlsli"
#include "rt_material.hlsli"
// R7b (R7-PLAN §3): SHaRC 1.6.5, its update compiled with SHARC_UPDATE 1, the other programmes with 0. Its voxels hold a surface's
// ambient (its diffuse indirect light over its albedo: the material demodulation on, each path vertex's own direct light kept out of
// its voxel, given only to the vertices before it: cache_update), so a query is a hit's whole ambient (HitAmbient; R7b's baked
// RtAmbientOf, its seed, removed by I-0132 R11d).
#ifndef SHARC_UPDATE
#define SHARC_UPDATE 0
#endif
#define SHARC_ENABLE_64_BIT_ATOMICS 1
#define SHARC_MATERIAL_DEMODULATION 1
#include "SharcCommon.h"
RWStructuredBuffer<uint64_t> cache_keys:register(u0);                     // the hash entries
RWStructuredBuffer<SharcAccumulationData> cache_accumulated:register(u1);   // this frame's samples (the update's, cleared by the resolve)
RWStructuredBuffer<SharcPackedData> cache_resolved:register(u2);            // the cells' ambient across frames (the resolve's)
#define RtMaskViewBits 1u    // RtMaskView (rt_scene.h): what the eye sees (the control view's rays)
#define RtMaskSunBits 2u
#define RtMaskLocalBits 4u
#define RtMaskWaterBits 0x20u   // R6c: the water slices (rt_scene.h's RtMaskWater)
#define RtLightRadiusMax 0.5   // metres: a local light's sphere cap (rt_lights.hlsl's)
#define RT_REFLECT_SALT 0x1000u   // the pass's own draws (step review L3: apart from the lights' and the sun's salts)
#define RT_REFLECT_TRACED 1u   // a top level this frame
#define RT_REFLECT_PT 4u       // the PT level: the local lights sampled with their ray, the paths past their first hit (every surface traced at both levels since I-0132 R11d)
#define RT_REFLECT_SUN 8u      // the frame traces the sun's shadow (`sun` its direction)
#define RT_REFLECT_SKY_WITNESS 16u   // R6d (rt-sky-witness.flag, a diagnostic): the water's texels coloured by what their ray met (water())
// I-0126 R7a's diagnostics (rt-gi-no-ambient, rt-gi-no-local flags; the GI pass's and, R7b, the cache's update's constants only):
// the GI's hits without their ambient, without the local lights (the sets' fill lights are removed on the GI frames, D-0262).
#define RT_REFLECT_GI_NO_AMBIENT 64u
#define RT_REFLECT_GI_NO_LOCAL 128u
#define RT_REFLECT_CACHE 256u         // R7b: the cache updated and resolved this frame (a GI frame): the hits' ambient its own (HitAmbient)
#define RT_REFLECT_CACHE_CLEAR 512u   // R7b: cache_resolve zeroes every entry (the buffers' first frame)
#define RT_REFLECT_CACHE_WITNESS 2048u   // R7b's diagnostic (rt-cache-witness.flag; the GI pass): a hit's ambient its cell's answer, red none to green full
#define RT_REFLECT_GI_NO_SKY 1024u    // R7b's diagnostic (rt-gi-no-sky.flag; the GI and the cache's update): a miss brings nothing (the misses' share)
#define RT_REFLECT_GI_MEAN 4096u      // I-0126 R7d's bias check (rt-gi-mean.flag; the GI's passes): the pixel's estimate added into `mean` (GiMean)
#define RT_REFLECT_GI_MEAN_RESTART 8192u   // its first frame (the camera's rows or the extent changed, a GI frame missed): written, not added
#define RT_REFLECT_GI_NO_GLOSSY 16384u   // R7d's diagnostic (rt-gi-no-glossy.flag): the GI's hits without their specular (HitRadiance)
#define RT_REFLECT_PATH_NO_CACHE 32768u  // I-0128 R8b's reference (rt-pt-no-cache.flag): the PT level's paths 8 vertices, no cache (PathVertices)
#define RT_REFLECT_LIGHT_GRID 65536u     // I-0128 R8c: the hits' local lights from their cell of the frame's grid (light_grid; no-rt-light-grid.flag off)
#define RT_REFLECT_ROULETTE 131072u      // I-0132 R11a: Russian roulette past a path's first vertex (PathFrom; the default, no-rt-roulette.flag off; never the reference)
#define RT_REFLECT_EMISSIVE_X4 262144u   // I-0132 R11d's test tools (Thomas's sheets): the hits' emission x4 (rt-emissive-x4.flag)
#define RT_REFLECT_EMISSIVE_X16 524288u  // and x16 (rt-emissive-x16.flag); neither: DS3's own
#define RT_REFLECT_HIT_NO_FILL 1048576u   // I-0141 tranche 28 (a bounded run's share variant, bit 128): the sets' lights 1 to 3 out of the hits' light
#define RT_REFLECT_HIT_NO_SUN 2097152u    // and bit 256: the set's light 0 (the sun) out of the hits' light; the pixels' own direct light kept
#define RT_GI_SALT 0x2000u   // the GI's ray's own random salt (`gi`, nrd_signal, ReSTIR's initial sample; HitRadiance's diagnostic)
#define RT_REFLECT_FAR 1.0e4   // R6c: the specular hit distance of a miss and of the cubes along R (their environment at infinity)
#define RT_DENOISE_SKY_Z 2.0e5   // R7c: a pixel without a surface, past NRD's range (rt_denoise.h's kRtDenoiseSkyZ, kRtDenoiseRange 1e5)
// RtReflectConstants (rt_scene.h): the camera as RtViewConstants' first rows, the extent, the slots of the depth (R32 of the D32S8),
// GB0, GB1, GB3; whether a material table is bound, the frame's seed, the regions', lights' and light sets' counts, the mode
// (RT_REFLECT_*), the raster's material samplers' anisotropy and mip bias, `gi_paths` (I-0132 R12g: the GI's paths a pixel at the PT level, 1 to 4;
// in the place of R6b's roughness cut, unused since I-0132 R11d, every surface traced), the sun's cone (cosine; 0 sharp), the shadow rays' offset (kRtShadowRayOffset), the frame's dominant rows and
// FC_ShadowColor (both unused by the shader since I-0132 R11d: RtAmbientOf and the sun's floor removed); the frame's sun (toward it, world: the traced shadow's) and in its w a local light's sphere as a share of its range;
// R6c: the water's plane (n·X + w = 0, n the water's up) and the reflection target's extent (the `water` programme's); R7a: GB2's slot
// (the `gi` programme's); R7b: the camera's position at the cache's last resolve (`previous`, its xyz; cache_resolve's); R7c: the
// velocity target's slot, its rows' scale and the camera's current-to-previous screen rows (nrd_signal's motion, the velocity resolve's
// formula), the denoised radiance's slot (gi_compose's; spec_compose's the specular's, I-0132 R11), REBLUR's hit distance parameters; I-0128 R8b: the PT level's path vertices
// (`bounces`, PathVertices).
struct Reflect {float4 right;float4 up;float4 forward;float4 position;float4 jitter;uint width;uint height;uint depth;uint gb0;uint gb1;uint gb3;
    uint materials;uint seed;uint regions;uint lights;uint sets;uint mode;uint anisotropy;float texture_bias;uint gi_paths;float cone;float4 offset;
    float4 dominant;float4 shadow_colour;float4 sun;float4 plane;uint water_width;uint water_height;float time;uint gb2;float4 previous;
    uint velocity;uint denoised;uint mean;uint bounces;float4 velocity_scale;float4 current_to_previous[4];float4 hit_distance;
    float4 eye_previous;uint4 restir;uint4 probe;   // I-0129 R9a: the probes' requests' and results' slots, their count (`probe`)
    uint4 ser;};   // I-0132 R11b: the reordered programmes' outputs: the colour's and the distance's UAV slots, then their SRV slots (ser_apply)
ConstantBuffer<Reflect> frame:register(b0);
RaytracingAccelerationStructure scene:register(t0);
StructuredBuffer<RtMaterialRecord> table:register(t1);
ByteAddressBuffer rows:register(t2);       // RtInstanceRow by instance index, then the skinned instances' slice bases, then the forward blocks (RtForwardRow)
StructuredBuffer<RtLight> lights:register(t3);
ByteAddressBuffer regions:register(t4);    // a region every 416 bytes: CB_ENVMAP_REGION (400), its cube's slot, 1 when parallax-corrected
ByteAddressBuffer sets:register(t5);       // cbLight's FC_DirLight sets, 288 bytes a set (the light programmes' 18 rows)
ByteAddressBuffer fogs:register(t6);       // R6d: cbFog's FC_FogParamArray sets, 96 bytes a set (GXLightComposite's 6 rows), the same index
// I-0128 R8c (R8-PLAN §6, rt_light_grid.h): the frame's lights binned in a world grid about the camera, with RT_REFLECT_LIGHT_GRID:
// its header (the origin and the cell's size as floats, the cells along x, y, z and their count), each cell's {first word, count},
// the light indices (ascending in each cell).
ByteAddressBuffer light_grid:register(t7);
SamplerState wrap:register(s0);
struct Out {float4 position:SV_Position;};
[RootSignature(ReflectRoot)]
Out vs(uint id:SV_VertexID){Out o;const float2 uv=float2((id<<1)&2,id&2);o.position=float4(uv*float2(2,-2)+float2(-1,1),0,1);return o;}
// I-0132 R11e (D-0262, no fallback): the region cubes' sampling (RegionWeight, RegionCube, Coverage, Regions, Cubes), read only on a
// frame without a top level since R11d, removed: such a frame is never shown (its world black, magenta in the test runs).
// A set's row r (FC_DirLight[set], 18 rows: Vec_0..3, Col_0..3, Spc_0..3, AmbientSH[3], AmbientReflection, bLocalLight, FC_SsaoWeight).
float4 SetRow(uint set,uint r){return asfloat(sets.Load4(288*set+16*r));}
// One directional light's specular as GXLightAcc_DDirLightShadowed1 makes it: sat(N·H)^n' × (F0 + (1 − F0)(1 − sat(V·H))⁵) ×
// sat(N·L) / max(N·V, N·L, 1e-4) × (n' / 8 + 1 / 4), n' = max(1, n w / (0.01 n + w)), n = 2^(13 g), w the light's Vec.w.
float3 DirSpecular(float4 l,float3 n,float3 v,float3 f0,float g,float nl){
    const float3 h=normalize(v+l.xyz);const float e=exp2(13*g);const float lobe=max(e*l.w/(e*0.01+l.w),1);
    const float f=pow(1-saturate(dot(v,h)),5);
    return RtPow(saturate(dot(h,n)),lobe)*(f0+(1-f0)*f)*(saturate(nl)/max(max(dot(v,n),nl),0.0001))*(lobe*0.125+0.25);}
// The set's directional lights at a surface (DDirLightShadowed1's terms, its translucency left out, the SSAO weights at AO 1):
// `seen` the first light's mask term (`s`, the traced sun's visibility; I-0132 R11d (D-0262): no lerp(1 − FC_ShadowColor, 1, s) floor).
// I-0141 tranche 28's share variant (test tooling): RT_REFLECT_HIT_NO_SUN takes light 0, RT_REFLECT_HIT_NO_FILL lights 1 to 3, out of
// the hits' light (the passes the GI's diagnostic bits reach: the GI's rays, the specular rays on NRD's frames, the cache's update).
void DirLights(uint set,float3 n,float3 v,float3 f0,float g,float3 seen,out float3 diffuse,out float3 specular){
    const float4 l0=SetRow(set,0),l1=SetRow(set,1);const float nl0=dot(l0.xyz,n),nl1=dot(l1.xyz,n);
    const float sun=(frame.mode&RT_REFLECT_HIT_NO_SUN)?0:1,fill=(frame.mode&RT_REFLECT_HIT_NO_FILL)?0:1;
    diffuse=sun*SetRow(set,4).rgb*seen*(1-f0)*saturate(nl0)+fill*SetRow(set,5).rgb*(1-f0)*saturate(nl1);
    specular=sun*SetRow(set,8).rgb*seen*DirSpecular(l0,n,v,f0,g,nl0)+fill*SetRow(set,9).rgb*DirSpecular(l1,n,v,f0,g,nl1);
    const uint2 local=asuint(SetRow(set,16).xy);
    [unroll]for(uint k=0;k<2;++k){if(!local[k] || !fill)continue;const float4 l=SetRow(set,2+k);const float nl=dot(l.xyz,n);
        diffuse+=SetRow(set,6+k).rgb*(1-f0)*min(pow(nl*0.5+0.5,2),1);specular+=SetRow(set,10+k).rgb*DirSpecular(l,n,v,f0,g,nl);}}
bool Occluded(uint mask,float3 origin,float3 dir,float far){
    RayDesc ray;ray.Origin=origin;ray.Direction=dir;ray.TMin=frame.offset.z;ray.TMax=far;
    return RtOccluded(scene,table,rows,frame.materials!=0,mask,ray,wrap);}
// R7b: the cache's grid (SHaRC's logarithmic levels about the camera: a cell 2^level / RT_CACHE_SCALE metres, the level log2 of its
// distance, at least 1: 4 cm within 2 m, 16 cm at 10 m, 1.3 m at 100 m; SHaRC's samples' scale), its 2^22 entries (kRtCacheEntries,
// 160 MiB: its guide's baseline), the radiance's fixed-point scale (1e4: the dim interiors' ambient a few thousandths, 4e5 a cell a
// frame before the 32-bit sums overflow), the frames a cell accumulates (10) and keeps without a sample (64: about a second).
#define RT_CACHE_SCALE 50.0
#define RT_CACHE_ENTRIES asuint(frame.previous.w)   // the C++ side's kRtCacheEntries (EncodeCache and Bind write it: one source; step review L3)
#define RT_CACHE_RADIANCE_SCALE 1.0e4
#define RT_CACHE_FRAMES 10u
#define RT_CACHE_STALE 64u
#define RT_CACHE_FILLED 4.0          // a cell's samples at which the cache witness reads full green (the diagnostic's only use since I-0132 R11d; R7b's seed fade removed)
#define RT_CACHE_ALBEDO_MIN 0.02     // the demodulation's floor (a black albedo's voxel: no division by zero)
#define RT_CACHE_SAMPLE_MAX 256.0    // a sample's radiance at most (the 32-bit sums of a cell's frame: 1,677 such samples; step review L4, L5)
// What a path gives the cells: finite, not negative (a float to uint conversion of either is undefined: step review L4), at most
// RT_CACHE_SAMPLE_MAX.
float3 CacheSample(float3 x){return all(isfinite(x))?clamp(x,0,RT_CACHE_SAMPLE_MAX):0;}
SharcParameters CacheParameters(){
    SharcParameters s;s.gridParameters.cameraPosition=frame.position.xyz;s.gridParameters.logarithmBase=SHARC_GRID_LOGARITHM_BASE;
    s.gridParameters.sceneScale=RT_CACHE_SCALE;s.gridParameters.levelBias=SHARC_GRID_LEVEL_BIAS;
    s.hashMapData.capacity=RT_CACHE_ENTRIES;s.hashMapData.hashEntriesBuffer=cache_keys;
    // No anti-firefly filter: the demodulated weights (1 / albedo) exceed its threshold by design, and the throughputs (albedos) never grow.
    s.radianceScale=RT_CACHE_RADIANCE_SCALE;s.enableAntiFireflyFilter=false;s.accumulationBuffer=cache_accumulated;s.resolvedBuffer=cache_resolved;
    return s;}
// A hit's ambient (its diffuse indirect light, before its albedo): its cell's answer in the radiance cache (its position and geometric
// normal) on the GI frames, nothing without the cache (no-rt-cache.flag: the hits get no ambient). I-0132 R11d (D-0262: no lighting
// cheat): DS3's baked ambient (RtAmbientOf: the lightmaps, the instances' SH) no longer seeds it (R7b's seed, removed), and the cell
// answers whatever the path's footprint (the `spread` rule went with the seed; a sharp reflection's hit reads its cell's answer):
// an empty cell answers 0 until the cache's paths fill it (SHaRC's own start). SHaRC's other rule kept (R11d's final review M5): a
// segment shorter than the cell (`segment`, the ray's distance to the hit: a corner, a contact) ramps the answer to 0 over one cell,
// so a cell straddling the corner neither leaks into it nor feeds itself. R7a's diagnostic rt-gi-no-ambient: none.
float3 HitAmbient(RtSurface h,float segment){
    if((frame.mode&RT_REFLECT_GI_NO_AMBIENT) || !(frame.mode&RT_REFLECT_CACHE))return 0;
    const SharcParameters s=CacheParameters();
    const SharcVoxelData cell=SharcGetVoxelData(s.resolvedBuffer,HashMapFindEntry(s.hashMapData,h.position,h.geometric,s.gridParameters));
#if !SHARC_UPDATE
    if(frame.mode&RT_REFLECT_CACHE_WITNESS)return lerp(float3(1,0,0),float3(0,1,0),saturate(cell.accumulatedSampleNum/RT_CACHE_FILLED));
#endif
    const float size=HashGridGetVoxelSize(HashGridGetLevel(h.position,s.gridParameters),s.gridParameters);
    return cell.accumulatedRadiance*saturate(segment/size);}
// I-0128 R8c: the cell of `p` in the frame's light grid (rt_light_grid.h's RtLightGridCellOf: floor((p − origin) / cell)), its first
// index word and its count; false without the grid or outside it (a value not finite included): the whole list then.
bool LightCell(float3 p,out uint first,out uint count){
    first=0;count=frame.lights;
    if(!(frame.mode&RT_REFLECT_LIGHT_GRID))return false;
    const float4 head=asfloat(light_grid.Load4(0));const uint4 dims=light_grid.Load4(16);
    const float3 c=floor((p-head.xyz)/head.w);
    if(!all(c>=0) || !all(c<float3(dims.xyz)))return false;
    const uint3 at=uint3(c);const uint2 range=light_grid.Load2(4*(8+2*((at.z*dims.y+at.y)*dims.x+at.x)));   // 8: kRtLightGridHeader (asserted there)
    first=range.x;count=range.y;return true;}
// The hit's direct light: `diffuse` before its albedo (the set's directional lights with the traced sun, the local lights), `specular`
// toward the ray's origin.
void HitLight(RtSurface h,RtInstanceRow row,float3 dir,uint2 p,uint salt,out float3 direct,out float3 glossy){   // salt: the calling pass's (I-0126 R7a step review L2)
    const float3 v=-dir;const float3 origin=h.position+h.geometric*(frame.offset.x+frame.offset.y*length(h.position-frame.position.xyz));
    const uint set=row.lit.w<frame.sets?row.lit.w:0;
    float s=1;
    if((frame.mode&RT_REFLECT_SUN) && !(frame.mode&RT_REFLECT_HIT_NO_SUN)){float3 toward=frame.sun.xyz;
        if(frame.cone>0)toward=RtConeSample(toward,frame.cone,RtRandom2(p,frame.seed,salt+2));
        s=dot(toward,h.geometric)>0 && !Occluded(RtMaskSunBits,origin,toward,1.0e5)?1:0;}
    const float3 seen=s;   // I-0132 R11d (D-0262): no FC_ShadowColor floor under the traced sun (DS3's sky light faked in its shadows: the GI's)
    float3 diffuse,specular;DirLights(set,h.normal,v,h.f0,h.gloss,seen,diffuse,specular);
    // The local lights (rt_lights.hlsli's terms, their common factors: diffuse × (1 − F0 / 2), specular × F0).
    RtLitSurface ls;ls.position=h.position;ls.eye=dir;ls.normal=h.normal;ls.gloss=h.gloss;ls.translucency=0;
    float3 ld=0,lsp=0;
    // I-0128 R8c: the lights of the hit's cell (every light that can reach it, in the list's order), else the whole list; the draws
    // keyed by the light's index, so the sums and the choice are the whole list's with its zero terms left out.
    uint first=0,count=frame.lights;const bool cell=LightCell(h.position,first,count);
    if(frame.mode&RT_REFLECT_PT){float sum=0,chosen=0;uint pick=0;float3 cd=0,cs=0;
        for(uint j=0;j<count;++j){const uint k=cell?light_grid.Load(4*(first+j)):j;float3 dl,tl,sl;RtLocalLight(lights[k],ls,dl,tl,sl);dl=max(dl,0)*(1-h.f0*0.5);sl=max(sl,0)*h.f0;
            const float target=dot(dl+sl,float3(0.2126,0.7152,0.0722));if(!(target>0))continue;
            sum+=target;if(RtRandom2(p,frame.seed,salt+0x100+k).x*sum<target){pick=k;chosen=target;cd=dl;cs=sl;}}
        if(sum>0){const float3 to=lights[pick].position.xyz-origin;const float d=length(to);
            // The ray stops short of the light by its sphere (R4c's rule: a share of its range, capped, at least 0.1 m: its housing).
            const float reach=(lights[pick].kind&RT_LIGHT_PHYSICAL)?f16tof32(lights[pick].kind>>16):(lights[pick].kind&RT_LIGHT_KIND)==RT_LIGHT_SPOT?lights[pick].position.w:rsqrt(max(lights[pick].position.w,1e-12));   // I-0132 R11h: DS3's range
            const float stop=max(min(frame.sun.w*reach,RtLightRadiusMax),0.1);
            const float weight=d>stop && Occluded(RtMaskLocalBits,origin,to/d,d-stop)?0:sum/chosen;ld=cd*weight;lsp=cs*weight;}}
    else if(!(frame.mode&RT_REFLECT_GI_NO_LOCAL))for(uint j=0;j<count;++j){const uint k=cell?light_grid.Load(4*(first+j)):j;float3 dl,tl,sl;RtLocalLight(lights[k],ls,dl,tl,sl);ld+=max(dl,0)*(1-h.f0*0.5);lsp+=max(sl,0)*h.f0;}
    direct=diffuse+ld;glossy=specular+lsp;
}
// One GGX sample of the visible normals (Heitz 2018) around n for the view v (toward the eye), α the roughness squared.
float3 GgxNormal(float3 n,float3 v,float alpha,float2 u){
    float3 t,b;RtBasis(n,t,b);const float3 vl=normalize(float3(dot(v,t)*alpha,dot(v,b)*alpha,dot(v,n)));
    const float3 t1=vl.z<0.9999?normalize(cross(vl,float3(0,0,1))):float3(1,0,0);const float3 t2=cross(t1,vl);
    const float r=sqrt(u.x),phi=6.2831853*u.y;const float p1=r*cos(phi);float p2=r*sin(phi);const float sv=0.5*(1+vl.z);
    p2=(1-sv)*sqrt(1-p1*p1)+sv*p2;const float3 nh=p1*t1+p2*t2+sqrt(max(0,1-p1*p1-p2*p2))*vl;
    const float3 m=normalize(float3(alpha*nh.x,alpha*nh.y,max(0,nh.z)));return normalize(m.x*t+m.y*b+m.z*n);}
// I-0125 R6d (R6-PLAN §7): what a reflection ray that met no surface brings back from the forward slices (the sky's dome, its
// scrolling zenith and its cloud sheets: the forward records, RtForwardAt's three programmes): every forward candidate along `ray`
// (RtMaskForward; each one non-opaque, then RtKeeps' alpha test) as its programme draws it, fogged as the programme fogs it from the
// ray's origin (the reflecting point: the fog along the reflected leg, the first leg's the raster's own, the water's or the composite's),
// composed as the rows blend: the nearest opaque layer (blend ONE, ZERO), then the blended layers in front of it from far to near (the
// RT_SKY_LAYERS nearest). Two traversals (R6d's final review: no blended layer evaluated before the opaque one is known): the first
// finds the nearest opaque layer, the second, up to it, the blended ones, and only when a blended candidate lies in front of it
// (the final review's L5). False with no opaque layer (no dome there: the caller's cubes,
// the clouds left out). `cone` and `start` as TraceRadiance's.
#define RtMaskForwardBits 0x10u
#define RT_SKY_LAYERS 4
float3 SkyLayer(RtMaterialRecord record,RtInstanceRow row,uint geometry,float3x4 world,uint primitive,float2 bary,float width,float3 eye,out float alpha){
    float4 tint,fog[6];RtForwardRowOf(rows,row.forward,tint,fog);float3 at;
    const float4 c=RtForwardAt(table,record,row,rows,geometry,world,primitive,bary,tint,frame.time,width,wrap,at);alpha=c.a;
    return RtForwardFog(c.rgb,at,eye,frame.position.y,fog);}
float SkyFactor(uint factor,float alpha){return factor==RT_FACTOR_ONE?1:factor==RT_FACTOR_SRC_ALPHA?alpha:factor==RT_FACTOR_INV_SRC_ALPHA?1-alpha:0;}
// A forward record of an instance with its forward block (the step review's L1: an instance whose block was not made, its mulCol or its
// fog not read, is refused by the raster's forward rows; a skinned or wind-moved forward instance is not listed: passed over, no unfogged
// stand-in, D-0197).
bool SkyForward(RtMaterialRecord record,RtInstanceRow row){return (record.flags&RT_RECORD_FORWARD) && row.forward!=RT_NONE;}
// The witness (rt-sky-witness.flag, written on its frames only): what the last SkyRadiance met (1 a forward instance's candidate with
// records, 2 a forward record, 4 one with its forward block; 0x80 not called).
static uint sky_seen=0x80;
bool SkyRadiance(RayDesc ray,float start,float cone,out float3 radiance){
    radiance=0;const float3 eye=ray.Origin;const bool witness=(frame.mode&RT_REFLECT_SKY_WITNESS)!=0;if(witness)sky_seen=0;
    // `blended`: the nearest blended forward candidate the first traversal met (every candidate nearer than the final opaque layer is
    // met there, its TMax only shrinking): none nearer than the opaque layer, no second traversal (R6d's final review, L5).
    float blended=ray.TMax;
    RayQuery<RAY_FLAG_FORCE_NON_OPAQUE> opaque;opaque.TraceRayInline(scene,RAY_FLAG_NONE,RtMaskForwardBits,ray);
    while(opaque.Proceed()){
        const RtInstanceRow row=RtRowOf(rows,opaque.CandidateInstanceIndex());
        if(opaque.CandidateGeometryIndex()>=row.count)continue;   // an instance without records (its contribution names another's)
        const RtMaterialRecord record=table[opaque.CandidateInstanceContributionToHitGroupIndex()+opaque.CandidateGeometryIndex()];
        if(witness)sky_seen|=1u|((record.flags&RT_RECORD_FORWARD)?2u:0u)|(SkyForward(record,row)?4u:0u);
        // Before the alpha test (L-a: cheaper): the opaque (ONE, ZERO) layers only, the nearest kept by the query.
        if(!SkyForward(record,row))continue;
        if((record.forward&0xFF)!=RT_FACTOR_ONE){blended=min(blended,opaque.CandidateTriangleRayT());continue;}
        if(RtKeeps(record,row.scroll,opaque.CandidatePrimitiveIndex(),opaque.CandidateTriangleBarycentrics(),wrap))opaque.CommitNonOpaqueTriangleHit();}
    if(opaque.CommittedStatus()!=COMMITTED_TRIANGLE_HIT)return false;
    const float far=opaque.CommittedRayT();float alpha;
    {const RtInstanceRow row=RtRowOf(rows,opaque.CommittedInstanceIndex());
        const RtMaterialRecord record=table[opaque.CommittedInstanceContributionToHitGroupIndex()+opaque.CommittedGeometryIndex()];
        radiance=SkyLayer(record,row,opaque.CommittedGeometryIndex(),opaque.CommittedObjectToWorld3x4(),opaque.CommittedPrimitiveIndex(),
            opaque.CommittedTriangleBarycentrics(),(start+far)*cone,eye,alpha);}
    if(!(blended<far))return true;
    // The blended layers in front of the opaque one.
    float near_t[RT_SKY_LAYERS];float4 near_c[RT_SKY_LAYERS];uint near_b[RT_SKY_LAYERS];uint3 near_id[RT_SKY_LAYERS];uint layers=0;
    RayDesc front=ray;front.TMax=far;
    RayQuery<RAY_FLAG_FORCE_NON_OPAQUE> query;query.TraceRayInline(scene,RAY_FLAG_NONE,RtMaskForwardBits,front);
    while(query.Proceed()){
        const RtInstanceRow row=RtRowOf(rows,query.CandidateInstanceIndex());
        if(query.CandidateGeometryIndex()>=row.count)continue;
        const RtMaterialRecord record=table[query.CandidateInstanceContributionToHitGroupIndex()+query.CandidateGeometryIndex()];
        const float t=query.CandidateTriangleRayT();
        if(!SkyForward(record,row) || (record.forward&0xFF)==RT_FACTOR_ONE || !(t<far) || (layers==RT_SKY_LAYERS && !(t<near_t[RT_SKY_LAYERS-1])) ||
           !RtKeeps(record,row.scroll,query.CandidatePrimitiveIndex(),query.CandidateTriangleBarycentrics(),wrap))continue;
        // The step review's M1: a triangle the traversal reports again (DXR may, the geometries not built NO_DUPLICATE_ANYHIT) is one layer.
        const uint3 id=uint3(query.CandidateInstanceIndex(),query.CandidateGeometryIndex(),query.CandidatePrimitiveIndex());bool again=false;
        for(uint j=0;j<layers;++j)again=again || all(near_id[j]==id);
        if(again)continue;
        const float3 c=SkyLayer(record,row,query.CandidateGeometryIndex(),query.CandidateObjectToWorld3x4(),query.CandidatePrimitiveIndex(),
            query.CandidateTriangleBarycentrics(),(start+t)*cone,eye,alpha);
        // Inserted by distance, nearest first (the farthest dropped when full).
        uint k=min(layers,RT_SKY_LAYERS-1);
        for(;k>0 && near_t[k-1]>t;--k){near_t[k]=near_t[k-1];near_c[k]=near_c[k-1];near_b[k]=near_b[k-1];near_id[k]=near_id[k-1];}
        near_t[k]=t;near_c[k]=float4(c,alpha);near_b[k]=record.forward;near_id[k]=id;layers=min(layers+1,RT_SKY_LAYERS);}
    for(uint k=layers;k>0;--k){const uint l=k-1;
        radiance=SkyFactor(near_b[l]&0xF,near_c[l].a)*near_c[l].rgb+SkyFactor((near_b[l]>>4)&0xF,near_c[l].a)*radiance;}
    return true;}
// One ray's radiance along `ray` (the view's mask, the alpha-tested candidates kept as the raster keeps them); false on a miss or on a
// hit without its records or material (no stand-in surface): the caller brings nothing (I-0132 R11d). `cone` the ray cone's spread a metre along
// the path, `start` the distance the path already covered, `t` the hit's distance along the ray. R6d: a miss takes the forward
// slices' sky (SkyRadiance, `t` then RT_REFLECT_FAR: the far distance for RR's hit distance); none there, false.
// R7b: the first surface along `ray` (the view's mask, the alpha-tested candidates kept as the raster keeps them): RT_MET_SURFACE with
// its material in `h`, its row and distance; RT_MET_NOTHING a miss (`t` RT_REFLECT_FAR); RT_MET_UNREAD a hit without its records or
// material (no stand-in surface, D-0197). TraceRadiance's and the cache's update's.
#define RT_MET_NOTHING 0u
#define RT_MET_SURFACE 1u
#define RT_MET_UNREAD 2u
uint TraceSurface(RayDesc ray,float start,float cone,out RtSurface h,out RtInstanceRow row,out float t){
    h=(RtSurface)0;row=(RtInstanceRow)0;t=0;
    RayQuery<RAY_FLAG_NONE> query;query.TraceRayInline(scene,RAY_FLAG_NONE,RtMaskViewBits,ray);
    while(query.Proceed()){
        if(query.CandidateType()!=CANDIDATE_NON_OPAQUE_TRIANGLE)continue;
        const RtMaterialRecord record=table[query.CandidateInstanceContributionToHitGroupIndex()+query.CandidateGeometryIndex()];
        if(RtAnswers(record,RtMaskViewBits) && RtKeeps(record,RtRowOf(rows,query.CandidateInstanceIndex()).scroll,query.CandidatePrimitiveIndex(),
            query.CandidateTriangleBarycentrics(),wrap))query.CommitNonOpaqueTriangleHit();}
    // I-0132 R11b (R11-PLAN §13): the committed hit read into registers, so that in the ray generation builds (RT_RAYGEN 1) the thread
    // can be reordered by its hit's record (shader execution reordering, shader model 6.9) before the material and the light, the work
    // the hits diverge on; the query itself is not kept across the reorder. A miss takes its own key (the sky's work after it).
    const bool met=query.CommittedStatus()==COMMITTED_TRIANGLE_HIT;
    uint instance=0,geometry=0,slot=0,primitive=0;float2 bary=0;float3x4 world=(float3x4)0;
    if(met){instance=query.CommittedInstanceIndex();geometry=query.CommittedGeometryIndex();slot=query.CommittedInstanceContributionToHitGroupIndex()+geometry;
        primitive=query.CommittedPrimitiveIndex();bary=query.CommittedTriangleBarycentrics();world=query.CommittedObjectToWorld3x4();t=query.CommittedRayT();}
#if RT_RAYGEN
    dx::MaybeReorderThread(met?(slot&0x7FFFu):0x8000u,16);
#endif
    if(!met){t=RT_REFLECT_FAR;return RT_MET_NOTHING;}
    row=RtRowOf(rows,instance);
    if(geometry>=row.count){t=0;return RT_MET_UNREAD;}
    const RtMaterialRecord record=table[slot];
    h=RtMaterialAt(table,record,row,rows,geometry,world,primitive,bary,ray.Direction,(start+t)*cone,frame.anisotropy,frame.texture_bias,wrap);
    return h.material?RT_MET_SURFACE:RT_MET_UNREAD;}
// I-0132 R11d (D-0262): a ray that meets neither a surface nor the forward sky brings nothing (R7a's Unmet: the region cubes or the
// set's hemisphere rows, DS3's baked stand-ins, removed; R11e: the water's cubes on a frame without a top level too).
// One cosine-distributed direction about `n` (a unit normal) from the uniform pair `u`.
float3 Cosine(float3 n,float2 u){float3 t,b;RtBasis(n,t,b);const float s=sqrt(u.x),phi=6.2831853*u.y;
    return normalize(t*(s*cos(phi))+b*(s*sin(phi))+n*sqrt(max(0,1-u.x)));}
// I-0128 R8b (R8-PLAN §3): the PT level's paths past their first hit. The traced vertices of a path: frame.bounces (1 to 4,
// rt-bounces.txt) at the PT level, 8 without the cache for the bias check's reference (rt-pt-no-cache.flag), 1 off the PT level (the
// cache at the first hit, R7's paths). 4 is rt_scene.h's kRtPathVerticesMost (the C++ side clamps to it too).
// I-0128 R8d (R8's final review L8: compiled in, PathFrom cost the RT level 0.7 ms at Firelink 1920x1080, unused): only the programmes
// built with RT_PATHS 1 have it: trace_hit's and gi's `_path` builds, chosen by RtReflections when a frame's paths go past their first
// hit (rt_scene.h's RtPathsOf, this function's condition), and gi_temporal always (ReSTIR's frames are the PT level's; at one vertex
// PathFrom is compiled there unused: the step review's L2, not split); the others' paths stop at their first hit.
#ifndef RT_PATHS
#define RT_PATHS 0
#endif
uint PathVertices(){
    if(!RT_PATHS || !(frame.mode&RT_REFLECT_PT))return 1u;
    return (frame.mode&RT_REFLECT_PATH_NO_CACHE)?8u:clamp(frame.bounces,1u,4u);}
float RtLuma(float3 x){return dot(x,float3(0.2126,0.7152,0.0722));}
// A hit's emission as the rays take it (I-0132 R11d's test tools scale it; DS3's own by default).
float3 HitEmission(RtSurface h){return h.emissive*((frame.mode&RT_REFLECT_EMISSIVE_X16)?16.0:(frame.mode&RT_REFLECT_EMISSIVE_X4)?4.0:1.0);}
// The indirect light a path brings to its vertex `h` (met along the unit `dir`; its row `row`; `start` the distance the path covered
// to it, `cone` the path's spread a metre), from the vertices
// after it: at each vertex one direction drawn from its two lobes in proportion to their weights' luminances, the diffuse (cosine
// about the shading normal, weight albedo × (1 − F0): Lambert's cosine and π cancelled by the cosine draw, the cache update's
// throughput) or the glossy (one GGX sample of the visible normals, α = r², weight DS3's specular ambient term E = F0 + (1 − F0)
// (1 − sat(N·V))⁵ g², the primary surface's in Trace), the chosen weight over its probability (unbiased between the two); the next
// vertex lit as HitLight lights a hit (the sun with its ray, one local light with its ray, its emission), not fogged (R7a: DS3's fog
// is the eye's). The path's last vertex takes its ambient from the cache (HitAmbient: its cell's answer, I-0132 R11d) and no
// specular ambient; a vertex whose diffuse draw goes under its triangle's horizon ends the path there the same way (the cache
// update's rule), a glossy sample under the surface ends it with nothing; a miss brings the forward sky, else nothing (R11d); a hit without its records or material
// ends it with nothing (D-0197). Without the cache (the reference) the last vertex's ambient is 0: the energy left out is the product of
// its eight vertices' albedos. Each vertex's random numbers its own (`salt` the calling pass's: the GI's and the reflections' apart).
#define RT_PATH_SALT 0x8000u
#if RT_PATHS
float3 PathFrom(RtSurface h,RtInstanceRow row,float3 dir,uint2 p,uint salt,float segment,float start,float cone){
    const uint last=PathVertices();const bool cache=!(frame.mode&RT_REFLECT_PATH_NO_CACHE);
    float3 sum=0,through=1;
    [loop]for(uint k=1;k<last;++k){
        const float3 v=-dir;const float nv=saturate(dot(h.normal,v));
        const float3 wd=saturate(h.albedo)*(1-h.f0),ws=h.f0+(1-h.f0)*pow(1-nv,5)*h.gloss*h.gloss;
        const float ld=RtLuma(wd),ls=RtLuma(ws);
        if(!(ld+ls>0))return sum;   // a black vertex sends nothing further
        const uint vs=RT_PATH_SALT+(salt<<8)+(k<<16);   // HitLight's own draws at vs + 2 and vs + 0x100 + its lights (below 0xFF00 lights: the final review's L3)
        const bool diffuse=RtRandom2(p,frame.seed,vs+0x10).x*(ld+ls)<ld;
        const float2 u=RtRandom2(p,frame.seed,vs+0x11);
        float3 next;
        if(diffuse)next=Cosine(h.normal,u);
        else{const float a=h.roughness*h.roughness;const float3 m=GgxNormal(h.normal,v,a,u);next=2*dot(v,m)*m-v;}
        // The final review's L1: only the diffuse draw's horizon end takes the cache term (the cache update's rule); a glossy sample
        // under the surface brings nothing (its lobe's share lost there, as a microfacet sample's), so no diffuse ambient counted on it.
        if(!(dot(next,h.geometric)>0) || !(dot(next,h.normal)>0)){if(cache && diffuse)sum+=through*h.albedo*((ld+ls)/ld)*HitAmbient(h,segment);return sum;}   // over the diffuse draw's probability (its step review L1)
        through*=diffuse?wd*((ld+ls)/ld):ws*((ld+ls)/ls);
        // I-0132 R11a: Russian roulette, the path going on with the probability q of its throughput's largest channel (1 from 1 up),
        // its throughput divided by q, else ending with nothing: the same mean (unbiased), the trace and the vertex's light saved on
        // the dark paths. Its draw at vs + 0x12 (the lobe's at + 0x10 and + 0x11, HitLight's at + 2 and from + 0x100). Never on the
        // reference's paths (`cache` false; the step review's L2), whatever the mode says.
        if(cache && (frame.mode&RT_REFLECT_ROULETTE)){const float q=min(max(through.r,max(through.g,through.b)),1);
            if(!(RtRandom2(p,frame.seed,vs+0x12).x<q))return sum;
            through/=q;}
        RayDesc ray;ray.Origin=h.position+h.geometric*(frame.offset.x+frame.offset.y*length(h.position-frame.position.xyz));ray.Direction=next;
        ray.TMin=frame.offset.z;ray.TMax=1.0e5;
        RtSurface n;RtInstanceRow nrow;float t;
        const uint met=TraceSurface(ray,start,cone,n,nrow,t);
        if(met==RT_MET_NOTHING){float3 sky=0;if(!(frame.mode&RT_REFLECT_GI_NO_SKY))SkyRadiance(ray,start,cone,sky);
            return sum+through*sky;}
        if(met!=RT_MET_SURFACE)return sum;
        start+=t;segment=t;
        float3 direct,glossy;HitLight(n,nrow,next,p,vs,direct,glossy);
        sum+=through*(n.albedo*direct+glossy+HitEmission(n));
        h=n;row=nrow;dir=next;}
    if(cache)sum+=through*h.albedo*HitAmbient(h,segment);
    return sum;}
#endif
// The radiance a ray brings back from its hit `h` (the ray's unit direction `dir`), the hit's instance row `row`: its direct light
// (HitLight), its ambient (HitAmbient) and R7b its emission (GB5's: g_EmissiveTexture × FC_EmissiveColor). I-0128 R8b: with `path`
// (the reflections' and the GI's rays, ReSTIR's initial sample; not the water's target, R8-PLAN §3) and more than one vertex, the hit's
// indirect light from its path (PathFrom, `start` the distance covered to the hit) in place of its cache term.
// I-0126 R7d's diagnostic rt-gi-no-glossy.flag: the GI's hits (salt RT_GI_SALT: `gi`, nrd_signal, ReSTIR's initial sample) without their
// specular toward the receiver (the specular a reused hit carries from the receiver that traced it: the miss fix's review M1).
float3 HitRadiance(RtSurface h,RtInstanceRow row,float3 dir,uint2 p,uint salt,float segment,bool path=false,float start=0,float cone=0){
    float3 direct,glossy;HitLight(h,row,dir,p,salt,direct,glossy);
    if((frame.mode&RT_REFLECT_GI_NO_GLOSSY) && (salt&0xFFFFFu)==RT_GI_SALT)glossy=0;   // R12g: every GI path's
#if RT_PATHS
    if(path && PathVertices()>1u)return h.albedo*direct+glossy+HitEmission(h)+PathFrom(h,row,dir,p,salt,segment,start,cone);
#endif
    return h.albedo*(HitAmbient(h,segment)+direct)+glossy+HitEmission(h);   // R7's form, unchanged (the step review's L1)
}
bool TraceRadiance(RayDesc ray,uint2 p,float start,float cone,out float3 radiance,out float t,uint salt=RT_REFLECT_SALT,bool fogged=true,bool path=false){
    radiance=0;RtSurface h;RtInstanceRow row;
    const uint met=TraceSurface(ray,start,cone,h,row,t);
    if(met==RT_MET_NOTHING)return (frame.mode&RT_REFLECT_GI_NO_SKY) || SkyRadiance(ray,start,cone,radiance);   // R7b's diagnostic: nothing
    if(met!=RT_MET_SURFACE)return false;
    // R6d: the hit fogged along the reflected leg (GXLightComposite_ppo.asm's fog, read 2026-11-06: the same distance and height fog as
    // GXFlver_Col_Fwd's, RtForwardFog, from the set's FC_FogParamArray entry, the index the hit's cbLight set takes), its origin the eye.
    // I-0126 R7a: not along a diffuse bounce (`fogged` false): DS3's fog is the eye's (the composite fogs the pixel once), and its
    // colour added to every bounce leg veiled the GI's surfaces (RESULT « R7a »).
    radiance=HitRadiance(h,row,ray.Direction,p,salt,t,path,start+t,cone);if(!fogged)return true;
    const uint set=row.lit.w<frame.sets?row.lit.w:0;float4 fog[6];[unroll]for(uint k=0;k<6;++k)fog[k]=asfloat(fogs.Load4(96*set+16*k));
    radiance=RtForwardFog(radiance,h.position,ray.Origin,frame.position.y,fog);return true;}
// The pixel's surface as the light passes find it (the jitter undone, the raster depth's distance): the unit direction from the eye
// and the distance along it; false where the pixel has none (depth 1).
bool PixelSurface(uint2 p,out float3 eye,out float r){
    Texture2D<float> depth=ResourceDescriptorHeap[frame.depth];
    const float d=depth.Load(int3(p,0));eye=0;r=0;
    if(d>=1)return false;
    const float2 ndc=float2((p.x+0.5)*frame.jitter.z*2-1,1-(p.y+0.5)*frame.jitter.w*2)-frame.jitter.xy;
    const float3 c=float3(ndc.x/frame.forward.w,ndc.y/frame.position.w,1);const float scale=length(c);
    eye=(c.x*frame.right.xyz+c.y*frame.up.xyz+c.z*frame.forward.xyz)/scale;
    const float a=frame.up.w/(frame.up.w-frame.right.w);r=a*frame.right.w/(a-d)*scale;return true;}
// The pixel's specular term as GXEnvMapRegion weights its cube sample (the programme's E and horizon term, R6-PLAN §1): the
// specular ambient is term × L, L the radiance the lobe's ray brings (I-0132 R11: NRD denoises L, spec_compose applies the term).
// The horizon factor's base saturated (the step review's L2: min(nv + 1, 1) below 0 by rounding would make pow NaN).
float3 SpecularTerm(float3 n,float3 v,float g,float3 f0){const float nv=dot(n,v);return (f0+(1-f0)*pow(1-saturate(nv),5)*g*g)*pow(saturate(nv+1),2);}
// The specular sample at the pixel `p`: false where the pixel has no surface (depth 1). Its unit direction from the eye and distance
// (`eye`, `r`), its normal (GB0), its linear roughness (GB1's gloss, RtRoughnessOf: the lobe's α = r², the RR and NRD guides'), its
// term (SpecularTerm), the radiance L its one lobe ray brings and, R6c, its specular hit distance (RR's and NRD's guide: the world
// distance from the surface to the reflection's hit; RT_REFLECT_FAR for a miss; 0 where nothing is traced: L 0 then).
bool SpecularSample(uint2 p,out float3 eye,out float r,out float3 n,out float roughness,out float3 term,out float3 radiance,out float distance){
    eye=0;r=0;n=float3(0,0,1);roughness=1;term=0;radiance=0;distance=0;
    if(!PixelSurface(p,eye,r))return false;
    const float3 surface=frame.position.xyz+eye*r;
    Texture2D<float4> gb0=ResourceDescriptorHeap[frame.gb0],gb1=ResourceDescriptorHeap[frame.gb1],gb3=ResourceDescriptorHeap[frame.gb3];
    n=normalize(gb0.Load(int3(p,0)).xyz*2-1);const float g=gb1.Load(int3(p,0)).x;const float3 f0=pow(gb3.Load(int3(p,0)).rgb,2);
    const float3 v=-eye;term=SpecularTerm(n,v,g,f0);
    const float3 mirror=2*dot(n,v)*n-v;
    roughness=RtRoughnessOf(g);
    // I-0132 R11d (D-0262): every surface traced at both levels, one sample of its lobe (the RT level's rough surfaces took the region
    // cubes along R, and the specular ambient stopped outside the regions: DS3's baked stand-ins, removed); its noise left to RR, or
    // to NRD on the frames RR does not denoise (I-0132 R11, D-0252).
    if(!(frame.mode&RT_REFLECT_TRACED) || !frame.materials)return true;   // a frame without a top level: nothing (I-0132 R11e: the cubes along R removed; the frame is not shown)
    const float3 m=GgxNormal(n,v,roughness*roughness,RtRandom2(p,frame.seed,RT_REFLECT_SALT+1));float3 dir=2*dot(v,m)*m-v;
    if(dot(dir,n)<=0)dir=mirror;
    // From the surface moved toward the eye by the shadow rays' offset (the depth's precision, the raster's LOD against the rays').
    RayDesc ray;ray.Origin=frame.position.xyz+eye*max(r-(frame.offset.x+frame.offset.y*r),0);ray.Direction=dir;ray.TMin=frame.offset.z;ray.TMax=1.0e5;
    // A miss without the forward sky, or a hit on an instance without its records (no stand-in surface): nothing. The cone: the pixel's
    // angle over the whole path (the mirror's; a sampled lobe's spread left to the denoiser).
    float t;
    if(!TraceRadiance(ray,p,r,2*frame.jitter.w/frame.position.w,radiance,t,RT_REFLECT_SALT,true,true)){radiance=0;distance=RT_REFLECT_FAR;return true;}
    distance=length(ray.Origin+dir*t-surface);
    return true;
}
// The specular ambient at the pixel `p` (term × L) and, R6c, its specular hit distance (SpecularSample's).
float4 Trace(uint2 p,out float distance){
    float3 eye,n,term,radiance;float r,roughness;
    if(!SpecularSample(p,eye,r,n,roughness,term,radiance,distance))return 0;
    return float4(radiance*term,1);
}
[RootSignature(ReflectRoot)]
float4 trace(Out i):SV_Target{float distance;return Trace(uint2(i.position.xy),distance);}
struct HitOut {float4 colour:SV_Target0;float distance:SV_Target1;};
[RootSignature(ReflectRoot)]
HitOut trace_hit(Out i){HitOut o;o.colour=Trace(uint2(i.position.xy),o.distance);return o;}
// R6c: the first water surface along `ray` (the water slices' records, RtMaskWater; none alpha-tested), its distance in `t`.
bool WaterAt(RayDesc ray,out float t){
    t=0;RayQuery<RAY_FLAG_NONE> query;query.TraceRayInline(scene,RAY_FLAG_NONE,RtMaskWaterBits,ray);
    while(query.Proceed()){
        if(query.CandidateType()!=CANDIDATE_NON_OPAQUE_TRIANGLE)continue;
        if(RtAnswers(table[query.CandidateInstanceContributionToHitGroupIndex()+query.CandidateGeometryIndex()],RtMaskWaterBits))query.CommitNonOpaqueTriangleHit();}
    if(query.CommittedStatus()!=COMMITTED_TRIANGLE_HIT)return false;
    t=query.CommittedRayT();return true;}
// R6c (R6-PLAN §6): the water's g_ReflectionTexture, one ray a texel of the reflection target. The water samples t54 at (u, 1 − v) of
// its screen point (WaterBumpTriInteractionFloatObj_Fwd: SV_Position × the reciprocal extent, y flipped, x moved by its bump), so the
// texel (u, v) takes the main camera's ray through the screen point (u, 1 − v), without the jitter (the mirror view never jitters),
// up to the water's surface there (its slices in the rays: each piece seen, several pieces at different heights in one frame), else
// the collect's plane (WT-6's) when the eye is above it; from there the direction mirrored about the plane's normal (the water's up,
// as the mirror view's; the water's bump moves the lookup itself) is traced as R6b's rays and lit as their hits; on a traced frame a
// miss without the forward sky is black (I-0132 R11d, D-0262), and a frame without a top level gives black (R11e). Black where
// neither is ahead of the eye (nothing the water samples: the mirror view's clear).
[RootSignature(ReflectRoot)]
float4 water(Out i):SV_Target{
    const uint2 p=uint2(i.position.xy);
    const float2 uv=(float2(p)+0.5)/float2(frame.water_width,frame.water_height);
    const float2 ndc=uv*2-1;   // x as u; y the screen point 1 − v's NDC, 1 − 2 (1 − v) = 2 v − 1
    const float3 c=float3(ndc.x/frame.forward.w,ndc.y/frame.position.w,1);
    const float3 d=normalize(c.x*frame.right.xyz+c.y*frame.up.xyz+c.z*frame.forward.xyz);
    const float3 n=frame.plane.xyz;const float nd=dot(n,d);
    if(!(nd<-1e-5))return float4(0,0,0,1);   // the mirrored ray would go down: no water below the horizon's other side
    float t;bool found=false;
    if((frame.mode&RT_REFLECT_TRACED) && frame.materials){RayDesc eye;eye.Origin=frame.position.xyz;eye.Direction=d;eye.TMin=0;eye.TMax=1.0e5;found=WaterAt(eye,t);}
    if(!found){const float above=dot(n,frame.position.xyz)+frame.plane.w;if(!(above>0))return float4(0,0,0,1);t=-above/nd;}
    if((frame.mode&RT_REFLECT_SKY_WITNESS) && !found)return float4(1,1,1,1);   // the witness: the eye's ray met no water slice (the plane's point) white
    const float3 at=frame.position.xyz+d*t;const float3 dir=d-2*nd*n;
    if(!(frame.mode&RT_REFLECT_TRACED) || !frame.materials)return float4(0,0,0,1);   // no top level: nothing (I-0132 R11e: the cubes removed)
    RayDesc ray;ray.Origin=at+n*(frame.offset.x+frame.offset.y*t);ray.Direction=dir;ray.TMin=frame.offset.z;ray.TMax=1.0e5;
    // The cone: a texel's angle (two over the target's rows and the lens's vertical scale) over the whole path.
    float3 radiance;float hit;
    if(frame.mode&RT_REFLECT_SKY_WITNESS)sky_seen=0x80;
    if(!TraceRadiance(ray,p,t,2/(float(frame.water_height)*frame.position.w),radiance,hit)){
        // The witness: no forward candidate cyan, forward instances without a forward record yellow, forward records of instances
        // without their forward block (mulCol or fog not read) orange, with it but no opaque layer blue; a surface hit without its records
        // or material (SkyRadiance not called) green. Above: the plane's point (no water slice met) white; below: the forward sky magenta,
        // a surface hit red near to white at 100 m.
        if(frame.mode&RT_REFLECT_SKY_WITNESS)
            return float4(sky_seen==0x80?float3(0,1,0):sky_seen==0?float3(0,1,1):!(sky_seen&2)?float3(1,1,0):!(sky_seen&4)?float3(1,0.5,0):float3(0,0,1),1);
        return float4(0,0,0,1);}   // I-0132 R11d (D-0262): no region cubes as a stand-in
    if((frame.mode&RT_REFLECT_SKY_WITNESS) && hit>=RT_REFLECT_FAR)return float4(1,0,1,1);   // the forward sky composed: magenta
    if(frame.mode&RT_REFLECT_SKY_WITNESS)return float4(1,saturate(hit/100),saturate(hit/100),1);   // a surface hit: red near, white at 100 m
    return float4(radiance,1);
}
// I-0126 R7a (R7-PLAN §1): the deferred surfaces' diffuse ambient traced, in place of pass 0's GB4 × t32.x t32.y (the lightmaps, the
// instance's SH, the decals' SH) and of the SSAO on it. One cosine-distributed ray a pixel about the G-buffer's shading normal (GB0),
// from the surface moved toward the eye by the shadow rays' offset, on the frames Ray Reconstruction denoises; its hit lit as R6b's
// hits are (HitRadiance: the hit's ambient (HitAmbient, the cache's answer alone; R7a's baked RtAmbientOf removed by I-0132 R11d), the
// set's directional lights with the traced sun, the local lights; not fogged along the bounce: the composite fogs the pixel for the eye);
// a miss the forward sky (SkyRadiance, in TraceRadiance), else nothing (I-0132 R11d, D-0262: R7a's region cubes and the pixel's light
// set's hemisphere rows, AmbientSH, removed); a hit without its records or material brings nothing (no stand-in surface, D-0197). The
// result, GB2 × (1 − F0) × L (the cosine pdf cancels Lambert's cosine over π), is added into the CmbAlbedo target (R16G16B16A16),
// before the SSS and the composite; a pixel without a surface (depth 1) adds nothing. GB2 through its sRGB view (linear). Its misses'
// Cosine is above TraceRadiance (I-0128 R8b's paths use it).
// I-0128 R8d (R7d's final review L7: written once; one change for `gi`: a hit without its records or material at 1e4 m or more brings
// nothing, no longer Unmet's cubes, as ReSTIR's initial sample and D-0197 already had it): the GI's ray at `p` from `origin` (the receiver moved toward the eye by the shadow
// rays' offset), one cosine direction about `n`: what it met (RT_MET_*), its radiance L toward the receiver (the hit's, HitRadiance
// unfogged with its path; a miss the forward sky, else nothing (I-0132 R11d: no stand-in); a hit without its
// records or material nothing: no stand-in, D-0197), its distance `t` (TraceSurface's), the hit's position and geometric normal.
// `start` the eye's distance to the receiver. GiSample's (`gi`, nrd_signal) and ReSTIR's initial sample (RtRestirInitial).
// I-0132 R12g: the GI's paths a pixel at the PT level (frame.gi_paths, 1 off it), the j-th path's salt RT_GI_SALT with j in bits 20 and up
// (PathFrom's vertex salts shift it by 8: j lands in bits 28 and up, apart from every other pass's; HitLight's draws from salt + 0x100
// stay under bit 20 below 0xFDF00 lights).
uint GiPaths(){return clamp(frame.gi_paths,1u,4u);}
uint GiSalt(uint j){return RT_GI_SALT|(j<<20);}
// The GI's rays' origin for a pixel at distance `r` along the unit `eye`: the receiver moved toward the eye by the shadow rays' offset
// (GiSample's and the one-ray pass's extra paths, the second step review's L4: one formula).
float3 GiOrigin(float3 eye,float r){return frame.position.xyz+eye*max(r-(frame.offset.x+frame.offset.y*r),0);}
uint GiRay(uint2 p,float3 origin,float start,float3 n,out float3 dir,out float3 radiance,out float t,out float3 hit,out float3 geometric,uint salt){   // R12g: `salt` the path's (GiSalt)
    dir=Cosine(n,RtRandom2(p,frame.seed,salt));radiance=0;hit=0;geometric=0;
    RayDesc ray;ray.Origin=origin;ray.Direction=dir;ray.TMin=frame.offset.z;ray.TMax=1.0e5;
    const float cone=2*frame.jitter.w/frame.position.w;
    RtSurface h;RtInstanceRow row;
    const uint met=TraceSurface(ray,start,cone,h,row,t);
    if(met==RT_MET_SURFACE){radiance=HitRadiance(h,row,dir,p,salt,t,true,start+t,cone);hit=h.position;geometric=h.geometric;}
    else if(met==RT_MET_NOTHING && !(frame.mode&RT_REFLECT_GI_NO_SKY))SkyRadiance(ray,start,cone,radiance);   // R7b's diagnostic: nothing; R11d: no stand-in
    return met;}
// The GI's ray at `p`: its radiance L and its distance (RT_REFLECT_FAR for a miss); false where the pixel has no surface (depth 1) or
// nothing is traced.
bool GiSample(uint2 p,out float3 eye,out float r,out float3 n,out float3 radiance,out float distance){   // the first path's (RT_GI_SALT)
    radiance=0;distance=RT_REFLECT_FAR;n=float3(0,0,1);eye=0;r=0;
    if(!(frame.mode&RT_REFLECT_TRACED) || !frame.materials || !PixelSurface(p,eye,r))return false;
    Texture2D<float4> gb0=ResourceDescriptorHeap[frame.gb0];
    n=normalize(gb0.Load(int3(p,0)).xyz*2-1);   // GB0 as the light passes read it (step review L1: no flip)
    float3 dir,hit,geometric;float t;
    const uint met=GiRay(p,GiOrigin(eye,r),r,n,dir,radiance,t,hit,geometric,RT_GI_SALT);
    distance=met==RT_MET_NOTHING || !(t>0)?RT_REFLECT_FAR:min(t,RT_REFLECT_FAR);   // a hit without its records or material: its distance, its radiance 0
    return true;
}
// I-0126 R7d (R7-PLAN §8), the bias check (rt-gi-mean.flag): the pixel's estimate of L (the demodulated radiance its GI adds, 0 where
// it adds none) added into the R32G32B32A32 accumulation `mean` (its frames in a), written instead on the check's first frame. One
// invocation a pixel a draw (the full-screen triangle), so no two writes meet.
void GiMean(uint2 p,float3 l){
    if(!(frame.mode&RT_REFLECT_GI_MEAN))return;
    RWTexture2D<float4> mean=ResourceDescriptorHeap[frame.mean];
    const float4 sample=float4(all(isfinite(l))?l:0,1);
    mean[p]=(frame.mode&RT_REFLECT_GI_MEAN_RESTART)!=0?sample:mean[p]+sample;}
[RootSignature(ReflectRoot)]
float4 gi(Out i):SV_Target{
    const uint2 p=uint2(i.position.xy);
    Texture2D<float4> gb2=ResourceDescriptorHeap[frame.gb2],gb3=ResourceDescriptorHeap[frame.gb3];
    const float3 albedo=gb2.Load(int3(p,0)).rgb;
    // An unlit colour body (Col, ColSh: GB2 0, its colour in GB5) receives no ambient.
    float3 eye,n,radiance=0;float r,distance;
    const bool lit=any(albedo>0) && GiSample(p,eye,r,n,radiance,distance);
    // I-0132 R12g: at the PT level without ReSTIR (rt-gi-no-restir, a refused ReSTIR) the GI's paths a pixel averaged (each its own salt).
    // The optimisation pass (the step review's L4): the extra paths leave from the first's receiver, its surface read once.
    if(lit){const uint paths=GiPaths();const float3 origin=GiOrigin(eye,r);
        [loop]for(uint j=1;j<paths;++j){float3 dir,more,hit,geometric;float t;GiRay(p,origin,r,n,dir,more,t,hit,geometric,GiSalt(j));radiance+=more;}
        radiance/=float(paths);}
    GiMean(p,lit?radiance:0);
    if(!lit)return 0;
    const float3 f0=pow(gb3.Load(int3(p,0)).rgb,2);
    return float4(albedo*(1-f0)*radiance,0);
}
// I-0126 R7c (R7-PLAN §7), I-0132 R11 (D-0252, Thomas 2026-11-09 09:24: the specular through NRD too): on a GI frame RR does not
// denoise, NRD's inputs (REBLUR_DIFFUSE_SPECULAR; RtDenoiser's kRtDenoiseInputFormats) in place of gi's sum and trace's specular
// ambient, one draw over every pixel before GXLightDeferredAO: the GI ray's radiance L, demodulated (GB2 × (1 − F0) is applied after
// the denoiser, gi_compose), and the specular lobe ray's radiance L (its term applied after the denoiser, spec_compose), each with its
// hit distance normalized by REBLUR's parameters (the diffuse's at roughness 1, the specular's at the pixel's); the normal (GB0) and
// the linear roughness (GB1's gloss, RtRoughnessOf: the lobe's); the view Z (the distance along the camera's forward axis); the
// G-buffer surface's motion in pixels (previous minus current) by the velocity resolve's formula: a pixel a Velo row wrote (alpha 1,
// the rows drawn before the lighting) its rows' NDC delta, any other the camera motion of its depth through the current-to-previous
// rows. Every pixel with a surface is traced, an unlit body and a pixel outside the specular's stencil too (its neighbours' spatial
// filter reads it; the composes add nothing there); a pixel without one writes the sky's view Z (out of NRD's range).
#include "NRD.hlsli"
struct NrdSignal {float4 diffuse:SV_Target0;float4 normal_roughness:SV_Target1;float view_z:SV_Target2;float2 motion:SV_Target3;float4 specular:SV_Target4;};
[RootSignature(ReflectRoot)]
NrdSignal nrd_signal(Out i){
    const uint2 p=uint2(i.position.xy);
    NrdSignal o;o.diffuse=0;o.specular=0;o.normal_roughness=NRD_FrontEnd_PackNormalAndRoughness(float3(0,0,1),1,0);o.view_z=RT_DENOISE_SKY_Z;o.motion=0;
    float3 eye,n,radiance;float r,distance;
    const bool traced=GiSample(p,eye,r,n,radiance,distance);
    GiMean(p,traced?radiance:0);   // R7d's bias check: the estimate NRD is given (before its filter)
    float3 term,specular;float roughness,specular_distance;
    if(!SpecularSample(p,eye,r,n,roughness,term,specular,specular_distance))return o;   // no surface: the sky's guides; a surface not traced (no top level) keeps its guides
    const float view_z=r*dot(eye,frame.forward.xyz);
    o.view_z=view_z;
    o.normal_roughness=NRD_FrontEnd_PackNormalAndRoughness(n,roughness,0);
    o.diffuse=REBLUR_FrontEnd_PackRadianceAndNormHitDist(radiance,REBLUR_FrontEnd_GetNormHitDist(distance,view_z,frame.hit_distance.xyz,1),true);
    o.specular=REBLUR_FrontEnd_PackRadianceAndNormHitDist(specular,REBLUR_FrontEnd_GetNormHitDist(specular_distance,view_z,frame.hit_distance.xyz,roughness),true);
    Texture2D<float4> velocity=ResourceDescriptorHeap[frame.velocity];Texture2D<float> depth=ResourceDescriptorHeap[frame.depth];
    const float4 encoded=velocity.Load(int3(p,0));
    const float2 uv=(float2(p)+0.5)/float2(frame.width,frame.height);
    float2 previous;
    if(encoded.w>0.5)previous=uv-float2(0.5,-0.5)*((encoded.xy-0.5)*2.0/frame.velocity_scale.xy);
    else{const float d=depth.Load(int3(p,0));
        const float4 h=uv.x*frame.current_to_previous[0]+uv.y*frame.current_to_previous[1]+d*frame.current_to_previous[2]+frame.current_to_previous[3];previous=h.xy/h.w;}
    o.motion=(previous-uv)*float2(frame.width,frame.height);
    return o;
}
// R7c: the denoised radiance (NRD's OUT_DIFF_RADIANCE_HITDIST, `denoised`) modulated back, GB2 × (1 − F0) × L, added into the CmbAlbedo
// target where gi adds on RR's frames (the same blend); nothing where the pixel has no surface or no albedo.
[RootSignature(ReflectRoot)]
float4 gi_compose(Out i):SV_Target{
    const uint2 p=uint2(i.position.xy);
    Texture2D<float> depth=ResourceDescriptorHeap[frame.depth];
    if(depth.Load(int3(p,0))>=1)return 0;
    Texture2D<float4> gb2=ResourceDescriptorHeap[frame.gb2],gb3=ResourceDescriptorHeap[frame.gb3],denoised=ResourceDescriptorHeap[frame.denoised];
    const float3 albedo=gb2.Load(int3(p,0)).rgb;
    if(!any(albedo>0))return 0;
    const float3 f0=pow(gb3.Load(int3(p,0)).rgb,2);
    const float3 l=REBLUR_BackEnd_UnpackRadianceAndNormHitDist(denoised.Load(int3(p,0))).rgb;
    return float4(albedo*(1-f0)*max(l,0),0);
}
// I-0132 R11 (D-0252): the denoised specular radiance (NRD's OUT_SPEC_RADIANCE_HITDIST, `denoised` for this draw) times the pixel's
// SpecularTerm, written into SpecularAcc where trace writes on RR's frames (the same pipeline state: the stencil bit 0x08, R11G11B10);
// the same value as trace's term × L with L denoised. Nothing where the pixel has no surface.
[RootSignature(ReflectRoot)]
float4 spec_compose(Out i):SV_Target{
    const uint2 p=uint2(i.position.xy);
    float3 eye;float r;
    if(!PixelSurface(p,eye,r))return 0;
    Texture2D<float4> gb0=ResourceDescriptorHeap[frame.gb0],gb1=ResourceDescriptorHeap[frame.gb1],gb3=ResourceDescriptorHeap[frame.gb3],denoised=ResourceDescriptorHeap[frame.denoised];
    const float3 n=normalize(gb0.Load(int3(p,0)).xyz*2-1);
    const float3 term=SpecularTerm(n,-eye,gb1.Load(int3(p,0)).x,pow(gb3.Load(int3(p,0)).rgb,2));
    const float3 l=REBLUR_BackEnd_UnpackRadianceAndNormHitDist(denoised.Load(int3(p,0))).rgb;
    return float4(term*max(l,0),1);
}
// I-0126 R7d (R7-PLAN §8): ReSTIR GI at the PT level (Ouyang et al. 2021), on NVIDIA's RTXDI library's GI reservoirs (commit f12037fa,
// v3.1.0's pin: their packing, storage and streaming combination; read by the build from its archive outside the repository,
// hash-pinned; this software contains source code provided by NVIDIA Corporation). The resampling is the native's own: RTXDI's
// temporal and spatial functions, measured on still views, brightened the GI by 0.3 to 5.5 % (RESULT « R7d »: their normalization
// counts a domain by its own sample, not the chosen one), so they were replaced by the unbiased form below. On a PT GI frame, in
// place of `gi`'s one draw, three draws over the render extent: `gi_temporal` (the pixel's surface kept for the next frame, the GI's
// ray as the initial sample, combined with the last frame's final reservoir: array 0), `gi_spatial` (array 0's neighbours: array 1,
// the next frame's history), `gi_shade` (GB2 × (1 − F0) × the final sample's estimate, added where `gi` adds).
// The estimator (Bitterli et al. 2020's 1/Z with the reconnection shift, Lin et al. 2022's GRIS; connaissance générale):
// - the target p̂_q(y) = luminance(L) × cos θ × V at the receiver q (the point the rays leave from), visibility included, so every
//   reservoir holds samples its own receiver sees (the initial sample by construction: its own ray's first surface);
// - a candidate y_j from domain j enters q's stream with p̂_q(y_j) × W_j × M_j × J_j, J_j the reconnection's Jacobian from j's receiver
//   to q's; a shift whose Jacobian falls outside the window is invalid (its candidate 0 and, for Z, that domain not counted);
// - W = Σ / (Z × p̂_q(y)), Z the M of the domains whose receiver could have produced the chosen y: its shift valid, above its horizon
//   and seen from it (one ray each), the chosen sample's own domain counted without a ray (its reservoir's sample is in its support);
// - the domains are chosen without looking at their samples (the reprojected pixel, R8d permuted, the neighbours' random offsets, the surface tests
//   on depth, normal and light set: the set is kept in the reservoir's misc bits as a similarity key, although a miss's radiance no
//   longer reads its receiver's set since I-0132 R11d (D-0262: Unmet's hemisphere removed));
//   no age cap, no neighbour search, no boiling filter (each a sample-dependent choice, a bias).
// Biases that stay: a reused hit keeps the radiance it was traced with (zero on still views for the temporal reuse; a reused miss takes
// q's own forward sky, else nothing: RtRestirMissRadiance): its specular toward the receiver that traced it (metres from q on a far
// surface: rt-gi-no-glossy.flag tells its share), the visibility tested in the current scene for last frame's domain, the visibility
// rays stopping short of the sample by 2 cm or 1 % (a thin occluder in that margin; the step review's L1), a domain counted in Z by
// q's miss radiance (where its own sky gives none). (R7d's cache ramp over the tracer's segment, removed by I-0132 R11d.) The motion: the camera's alone (the
// depth through the current-to-previous rows; a moving object's history fails the surface tests and is not reused).
#define RTXDI_GI_RESERVOIR_BUFFER gi_reservoirs
#include "Rtxdi/GI/ReSTIRGIParameters.h"
#include "Rtxdi/Utils/Math.hlsli"
#include "Rtxdi/Utils/Color.hlsli"
#include "Rtxdi/Utils/TypePacking.hlsli"
#include "Rtxdi/Utils/RandomSamplerState.hlsli"
RWStructuredBuffer<RTXDI_PackedGIReservoir> gi_reservoirs:register(u3);   // two arrays (frame.restir.y apart): the frame's, the final
RWStructuredBuffer<uint4> gi_surfaces:register(u4);   // two halves (frame.restir.w apart, by the serial's parity): xyz the surface's bits, w its normal (RTXDI's snorm 2x16); x 0xFFFFFFFF none (the step review's L2: no float path); I-0128 R8d, with a decorrelated share: a third part, the frame's initial estimates (xyz)
#include "Rtxdi/GI/Reservoir.hlsli"
#define RT_RESTIR_TEMPORAL 1u   // frame.restir.z's bits: the temporal resampling on (rt-gi-no-temporal.flag off)
#define RT_RESTIR_SPATIAL 2u    // the spatial resampling on (rt-gi-no-spatial.flag off)
#define RT_RESTIR_HISTORY 4u    // the last frame's reservoirs and surfaces are this extent's and the next serial's (else no temporal reuse)
#define RT_RESTIR_ODD 8u        // this frame's surfaces in the second half
#define RT_RESTIR_WIDE 16u      // the Jacobian's window [0.1, 10] (rt-gi-jacobian-wide.flag, the bias check's), else [0.5, 2]
// I-0128 R8d (R8-PLAN §2): ReSTIR GI's samples decorrelated in time for RR, each choice made without looking at the samples (so the
// 1/Z form below stays unbiased): the temporal neighbour permuted (RTXDI's permutation sampling: the reprojected pixel's 4 x 4 block
// mirrored about a frame's random offset, a bijection, so on a still camera no history feeds two pixels (a moving one's reprojection
// may still map two pixels to one) and a sample moves to another pixel each frame; the reprojected pixel itself where it leaves the screen;
// rt-restir-no-permute.flag R7d's form); the last frame's M capped (bits 8 to 15, rt-restir-history-N.flag; 0 RT_RESTIR_HISTORY_MAX; I-0132
// R12g: in frames, times the GI's paths a pixel);
// a share of the pixels (bits 16 to 23, in 255ths; 102 unless rt-restir-no-decorrelate.flag) shaded from their initial sample (the one ray's
// estimate, kept by gi_temporal in the surfaces' third part) in place of the resampled one, drawn per pixel and frame: a mixture of two
// unbiased estimators (RTXDI's ReSTIR PT « uniform » decorrelation, its stagnancy mode left out: the reservoirs' ages, smoothed over
// a neighbourhood and frames (its duplication map), drive it: a sample-dependent choice).
#define RT_RESTIR_PERMUTE 32u
#define RT_RESTIR_CAP(z) (((z)>>8)&0xFFu)
#define RT_RESTIR_SHARE(z) (float(((z)>>16)&0xFFu)/255.0)
#define RT_RESTIR_SALT 0x5000u   // the permutation's offset (pixel 0's draw: one a frame) and the decorrelation's draw
#define RT_RESTIR_HISTORY_MAX 8u   // the last frame's history at most by default, in frames (RTXDI's GI default M at one path; R12g: x the GI's paths)
#define RT_RESTIR_NEIGHBOURS 2u    // the spatial neighbours (RTXDI's GI default)
#define RT_RESTIR_RADIUS 32.0      // render pixels (RTXDI's GI default; its world footprint follows the render scale: the step review's L4, kept)
#define RT_RESTIR_DEPTH 0.1        // the surface tests: relative depth, normals' cosine (RTXDI's GI defaults)
#define RT_RESTIR_NORMAL 0.6
// A receiver: the G-buffer's position (its depth to its frame's eye, the surface tests'), its shading normal, the point its rays leave
// from (moved toward that eye by the shadow rays' offset, as the GI's ray): the target's, the Jacobian's and the visibility's point
// (the step review's M2).
struct RtRestirSurface {float3 position;float3 normal;float3 origin;float depth;bool valid;};
RtRestirSurface RtRestirNone(){return (RtRestirSurface)0;}
RtRestirSurface RtRestirAt(float3 position,float3 normal,float3 eye){
    RtRestirSurface s;s.position=position;s.normal=normal;s.depth=length(position-eye);s.valid=true;
    s.origin=position+(s.depth>0?(eye-position)/s.depth:0)*min(frame.offset.x+frame.offset.y*s.depth,s.depth);return s;}
uint RtRestirHalf(bool previous){return (((frame.restir.z&RT_RESTIR_ODD)!=0)!=previous)?frame.restir.w:0u;}
RtRestirSurface RtRestirStored(int2 p,bool previous){
    if(any(p<0) || p.x>=int(frame.width) || p.y>=int(frame.height))return RtRestirNone();
    const uint4 v=gi_surfaces[RtRestirHalf(previous)+uint(p.y)*frame.width+uint(p.x)];
    if(v.x==0xFFFFFFFFu)return RtRestirNone();
    return RtRestirAt(asfloat(v.xyz),RTXDI_DecodeNormalizedVectorFromSnorm2x16(v.w),previous?frame.eye_previous.xyz:frame.position.xyz);}
// The surface tests, without the samples: both receivers valid, their normals within RT_RESTIR_NORMAL, the other's depth within
// RT_RESTIR_DEPTH of the expected one.
bool RtRestirSimilar(RtRestirSurface a,RtRestirSurface b,float expected){
    return a.valid && b.valid && dot(a.normal,b.normal)>=RT_RESTIR_NORMAL && abs(expected-b.depth)<=RT_RESTIR_DEPTH*max(expected,b.depth);}
// The pixel's light set (GB3.w, the sets' index) as a reservoir's misc bits (RTXDI's 16 high bits), and back: kept as a similarity key
// (the surface tests' light set) although a miss no longer reads the set since I-0132 R11d (D-0262: Unmet's hemisphere removed).
uint RtRestirSetOf(uint2 p){Texture2D<float4> gb3=ResourceDescriptorHeap[frame.gb3];return min(uint(gb3.Load(int3(p,0)).w*255+0.5),max(frame.sets,1)-1);}
// Whether `s` sees the point `position` (the rays' view mask, alpha tests as the GI's ray keeps them), the ray stopping short of it by
// 1 % of the distance, at least 2 cm (the sample lies on its triangle).
bool RtRestirSees(RtRestirSurface s,float3 position){
    const float3 d=position-s.origin;const float l=length(d);
    const float far=l-max(0.02,0.01*l);
    return !(far>frame.offset.z) || !Occluded(RtMaskViewBits,s.origin,d/l,far);}
// The target without its visibility: luminance(L) × cos θ at `s`'s receiver.
float RtRestirUnseen(RtRestirSurface s,RTXDI_GIReservoir r){
    const float3 d=r.position-s.origin;const float l=length(d);
    return l>0?RTXDI_Luminance(max(r.radiance,0))*saturate(dot(s.normal,d/l)):0;}
// The reconnection's Jacobian of r's sample from `from`'s receiver to `to`'s (RTXDI_CalculateJacobian: the solid angles' ratio), and the
// window that makes a shift valid.
float RtRestirJacobian(RtRestirSurface to,RtRestirSurface from,RTXDI_GIReservoir r){return RTXDI_CalculateJacobian(to.origin,from.origin,r.position,r.normal);}
bool RtRestirValid(float j){return (frame.restir.z&RT_RESTIR_WIDE)!=0?j>=0.1 && j<=10:j>=0.5 && j<=2;}
// Whether domain `l` could have produced the sample `y` seen from `q`: the shift from l to q valid, y above l's horizon with light,
// and seen from l (one ray).
bool RtRestirSupports(RtRestirSurface l,RtRestirSurface q,RTXDI_GIReservoir y){
    return RtRestirValid(RtRestirJacobian(q,l,y)) && RtRestirUnseen(l,y)>0 && RtRestirSees(l,y.position);}
// A candidate's weight into q's stream (p̂_q × J, its W and M applied by RTXDI_CombineGIReservoirs), with the target it had there; 0
// for an invalid shift or a sample q does not see.
float3 RtFinite(float3 x){return all(isfinite(x))?max(x,0):0;}   // not CacheSample's clamp: the estimate `gi`'s, unbounded
// The second bias series (RESULT « R7d »): a miss's radiance is its receiver's (the forward sky fogged from it, else nothing since
// I-0132 R11d, D-0262: R7d's region cubes and set hemisphere removed), so a reused miss takes q's own along the direction from q to the sample (a point RT_REFLECT_FAR
// away): the integrand q's, whoever traced it.
float3 RtRestirMissRadiance(RtRestirSurface q,float3 position){   // a reused miss's (GiRay has the canonical miss's own copy, the same form: change both)
    const float3 d=normalize(position-q.origin);float3 radiance=0;
    if(frame.mode&RT_REFLECT_GI_NO_SKY)return 0;
    RayDesc ray;ray.Origin=q.origin;ray.Direction=d;ray.TMin=frame.offset.z;ray.TMax=1.0e5;
    SkyRadiance(ray,q.depth,2*frame.jitter.w/frame.position.w,radiance);   // I-0132 R11d: else nothing (no stand-in)
    return RtFinite(radiance);}
#define RT_RESTIR_SET(misc) (((misc)>>16)&0xFFu)   // a reservoir's misc bits: its receiver's light set, whether its sample is a miss
#define RT_RESTIR_MISS(misc) ((((misc)>>24)&1u)!=0)
float RtRestirCandidate(RtRestirSurface q,RtRestirSurface from,inout RTXDI_GIReservoir r,bool miss,out float target){
    target=0;if(!RTXDI_IsValidGIReservoir(r) || !(r.weightSum>0))return 0;
    // The cheap tests first (the review's L2): the shift, q's horizon, its visibility, then a miss's radiance at q, the luminance.
    const float j=RtRestirJacobian(q,from,r);if(!RtRestirValid(j))return 0;
    if(!(dot(q.normal,r.position-q.origin)>0) || !RtRestirSees(q,r.position))return 0;
    if(miss)r.radiance=RtRestirMissRadiance(q,r.position);
    const float u=RtRestirUnseen(q,r);if(!(u>0))return 0;
    target=u;return u*j;}
RTXDI_ReservoirBufferParameters RtRestirBuffers(){RTXDI_ReservoirBufferParameters b;b.reservoirBlockRowPitch=frame.restir.x;b.reservoirArrayPitch=frame.restir.y;b.pad1=0;b.pad2=0;return b;}
// The pixel's receiver this frame (the light passes' position, GB0's normal), invalid where it has none or nothing is traced.
RtRestirSurface RtRestirPixel(uint2 p){
    float3 eye;float r;
    if(!(frame.mode&RT_REFLECT_TRACED) || !frame.materials || !PixelSurface(p,eye,r))return RtRestirNone();
    Texture2D<float4> gb0=ResourceDescriptorHeap[frame.gb0];
    return RtRestirAt(frame.position.xyz+eye*r,normalize(gb0.Load(int3(p,0)).xyz*2-1),frame.position.xyz);}
// The GI's ray (GiRay, `gi`'s) as the initial sample: its second vertex (the hit's position and geometric normal, toward the ray's
// origin; a miss a point RT_REFLECT_FAR along the ray facing back), the radiance it sends toward the pixel, its pdf cos θ / π.
RTXDI_GIReservoir RtRestirInitial(uint2 p,RtRestirSurface s,out bool miss,uint salt){
    float3 dir,radiance,hit,geometric;float t;
    const uint met=GiRay(p,s.origin,s.depth,s.normal,dir,radiance,t,hit,geometric,salt);
    const float pdf=saturate(dot(dir,s.normal))*0.31830989;
    miss=met==RT_MET_NOTHING;
    if(met==RT_MET_SURFACE)return RTXDI_MakeGIReservoir(hit,geometric,RtFinite(radiance),pdf);
    return RTXDI_MakeGIReservoir(s.origin+dir*(miss || !(t>0)?RT_REFLECT_FAR:t),-dir,RtFinite(radiance),pdf);}
// The stream's end: W = Σ / (Z × p̂_q(y)) (RTXDI_FinalizeGIResampling's form), 0 without a target or a domain.
void RtRestirFinish(inout RTXDI_GIReservoir r,float target,float z){RTXDI_FinalizeGIResampling(r,1,target*z);}
// I-0132 R11b: the body shared by the pixel shader and the ray generation build (gi_temporal_ser).
void GiTemporal(uint2 p){
    if(any(p>=uint2(frame.width,frame.height)))return;   // the step review's L3: the root UAVs are not bounds-checked
    const RtRestirSurface s=RtRestirPixel(p);
    gi_surfaces[RtRestirHalf(false)+p.y*frame.width+p.x]=s.valid?uint4(asuint(s.position),RTXDI_EncodeNormalizedVectorToSnorm2x16(s.normal)):uint4(0xFFFFFFFFu,0,0,0);
    RTXDI_GIReservoir r=RTXDI_EmptyGIReservoir();
    const uint set=RtRestirSetOf(p);
    bool miss=false;
    if(s.valid){
        // I-0132 R12g: the GI's paths a pixel (GiPaths, 1 off the PT level and by default) as the canonical domain's candidates, each its own
        // salt, streamed by RIS (Talbot et al. 2005; connaissance générale): the first combined as before (taken, its target the chosen one),
        // each other one taken in proportion to its weight (one of no weight never taken, its M counted, as the temporal candidate's). Their
        // M (1 each) all the canonical domain's in Z below. One path: R8d's form exactly.
        const uint paths=GiPaths();RTXDI_RandomSamplerState pick=RTXDI_InitRandomSampler(p,frame.seed,3);
        uint canonical=0;float3 estimate=0;float chosen=0;
        [loop]for(uint j=0;j<paths;++j){bool met_miss=false;const RTXDI_GIReservoir initial=RtRestirInitial(p,s,met_miss,GiSalt(j));
            canonical+=initial.M;estimate+=initial.radiance;
            // The canonical samples (each its own ray's first surface: seen, no ray).
            const float target=RtRestirUnseen(s,initial);
            if(j==0){RTXDI_CombineGIReservoirs(r,initial,0.5,target);chosen=target;miss=met_miss;}
            else if(target*initial.weightSum>0){if(RTXDI_CombineGIReservoirs(r,initial,RTXDI_GetNextRandom(pick),target)){chosen=target;miss=met_miss;}}
            else r.M+=initial.M;}
        // I-0128 R8d: the rays' estimate, L (the cosine pdf cancels cos θ / π; R12g: the paths' mean), for gi_shade's decorrelated pixels.
        if(RT_RESTIR_SHARE(frame.restir.z)>0)gi_surfaces[2*frame.restir.w+p.y*frame.width+p.x]=uint4(asuint(estimate/float(paths)),0);
        if((frame.restir.z&(RT_RESTIR_TEMPORAL|RT_RESTIR_HISTORY))==(RT_RESTIR_TEMPORAL|RT_RESTIR_HISTORY)){
            // Last frame's domain at the camera's reprojection of the pixel's depth (nrd_signal's formula without the Velo rows: they are not
            // drawn yet on RR's frames), its depth from the last frame's eye expected there.
            Texture2D<float> depth=ResourceDescriptorHeap[frame.depth];
            const float2 uv=(float2(p)+0.5)/float2(frame.width,frame.height);
            const float4 h=uv.x*frame.current_to_previous[0]+uv.y*frame.current_to_previous[1]+depth.Load(int3(p,0))*frame.current_to_previous[2]+frame.current_to_previous[3];
            const float2 at=h.xy/h.w*float2(frame.width,frame.height)-0.5;
            int2 last_at=all(isfinite(at))?int2(round(at)):int2(-1,-1);   // off screen: no surface
            // I-0128 R8d: permuted (RTXDI_ApplyPermutationSampling, one offset a frame); where that moves it off screen, the reprojected
            // pixel itself (the step review's L2: no border band without history; a choice by position alone, so no bias).
            if((frame.restir.z&RT_RESTIR_PERMUTE)!=0 && all(isfinite(at))){int2 permuted=last_at;
                RTXDI_ApplyPermutationSampling(permuted,uint(RtRandom2(uint2(0,0),frame.seed,RT_RESTIR_SALT).x*16));
                if(all(permuted>=0) && all(permuted<int2(frame.width,frame.height)))last_at=permuted;}
            const RtRestirSurface t=RtRestirStored(last_at,true);
            uint last_set=0;RTXDI_GIReservoir last=RTXDI_EmptyGIReservoir();
            if(RtRestirSimilar(s,t,length(s.position-frame.eye_previous.xyz)))last=RTXDI_LoadGIReservoir(RtRestirBuffers(),uint2(last_at),1,last_set);
            if(RtRestirSimilar(s,t,length(s.position-frame.eye_previous.xyz)) && RT_RESTIR_SET(last_set)==set){
                // I-0132 R12g (its step review's M2): the cap in frames, so times the GI's paths a pixel (each frame's canonical M), or
                // 4 paths' history would hold two frames and Ultra barely differ from Low on a still view; Z takes the same capped M.
                const uint cap=RT_RESTIR_CAP(frame.restir.z);last.M=min(last.M,(cap?cap:RT_RESTIR_HISTORY_MAX)*paths);
                RTXDI_RandomSamplerState rng=RTXDI_InitRandomSampler(p,frame.seed,1);
                float target;const float weight=RtRestirCandidate(s,t,last,RT_RESTIR_MISS(last_set),target);
                // The step review's L2: a candidate of no weight never taken (its M counted, as RTXDI's combination counts it).
                bool took=false;if(weight>0)took=RTXDI_CombineGIReservoirs(r,last,RTXDI_GetNextRandom(rng),weight);else r.M+=last.M;
                if(took){chosen=target;miss=RT_RESTIR_MISS(last_set);}
                // Z: the canonical domain always (the chosen sample is seen from q); last frame's when it could have produced the chosen one.
                const bool supports=took || RtRestirSupports(t,s,r);
                RtRestirFinish(r,chosen,canonical+(supports?last.M:0));}
            else RtRestirFinish(r,chosen,canonical);}
        else RtRestirFinish(r,chosen,canonical);}
    RTXDI_StoreGIReservoir(r,(set<<16)|(miss?1u<<24:0u),RtRestirBuffers(),p,0);
}
[RootSignature(ReflectRoot)]
void gi_temporal(Out i){GiTemporal(uint2(i.position.xy));}
[RootSignature(ReflectRoot)]
void gi_spatial(Out i){
    const uint2 p=uint2(i.position.xy);
    if(any(p>=uint2(frame.width,frame.height)))return;
    const RtRestirSurface s=RtRestirStored(int2(p),false);
    uint own_set=0;const RTXDI_GIReservoir own=RTXDI_LoadGIReservoir(RtRestirBuffers(),p,0,own_set);
    RTXDI_GIReservoir r=own;bool miss=RT_RESTIR_MISS(own_set);
    if(s.valid && RTXDI_IsValidGIReservoir(own) && (frame.restir.z&RT_RESTIR_SPATIAL)!=0){
        RTXDI_RandomSamplerState rng=RTXDI_InitRandomSampler(p,frame.seed,2);
        r=RTXDI_EmptyGIReservoir();
        float chosen=RtRestirUnseen(s,own);RTXDI_CombineGIReservoirs(r,own,0.5,chosen);   // its own sample: seen from q by construction
        // The neighbours: random offsets in the disc (a random turn of a golden-angle spiral), kept by the surface tests alone.
        const float turn=RTXDI_GetNextRandom(rng)*6.2831853;
        RtRestirSurface ns[RT_RESTIR_NEIGHBOURS];uint nm[RT_RESTIR_NEIGHBOURS];bool kept[RT_RESTIR_NEIGHBOURS];int took=-1;   // the step review's L3: read once
        [unroll]for(uint k=0;k<RT_RESTIR_NEIGHBOURS;++k){
            const float radius=RT_RESTIR_RADIUS*sqrt((float(k)+RTXDI_GetNextRandom(rng))/float(RT_RESTIR_NEIGHBOURS));const float a=turn+2.39996323*float(k);
            const int2 at=clamp(int2(p)+int2(round(float2(cos(a),sin(a))*radius)),int2(0,0),int2(frame.width,frame.height)-1);
            ns[k]=RtRestirStored(at,false);nm[k]=0;
            uint other_set=0;const RTXDI_GIReservoir other=RTXDI_LoadGIReservoir(RtRestirBuffers(),uint2(at),0,other_set);
            kept[k]=any(at!=int2(p)) && RtRestirSimilar(s,ns[k],s.depth) && RT_RESTIR_SET(other_set)==RT_RESTIR_SET(own_set);
            if(!kept[k])continue;
            nm[k]=other.M;
            RTXDI_GIReservoir shifted=other;float target;const float weight=RtRestirCandidate(s,ns[k],shifted,RT_RESTIR_MISS(other_set),target);
            if(weight>0){if(RTXDI_CombineGIReservoirs(r,shifted,RTXDI_GetNextRandom(rng),weight)){took=int(k);chosen=target;miss=RT_RESTIR_MISS(other_set);}}
            else r.M+=other.M;}   // the step review's L2
        // Z: the canonical domain (the chosen sample is seen from q), the chosen sample's own domain, and each other kept neighbour that
        // could have produced it.
        float z=own.M;
        [unroll]for(uint k=0;k<RT_RESTIR_NEIGHBOURS;++k){if(!kept[k])continue;
            if(int(k)==took || RtRestirSupports(ns[k],s,r))z+=nm[k];}
        RtRestirFinish(r,chosen,z);}
    RTXDI_StoreGIReservoir(r,(RT_RESTIR_SET(own_set)<<16)|(miss?1u<<24:0u),RtRestirBuffers(),p,1);
}
// The final sample's estimate of L at the pixel, L(y) cos θ / π × W (its visibility in the target: seen from q), as `gi` adds it:
// GB2 × (1 − F0) × L.
[RootSignature(ReflectRoot)]
float4 gi_shade(Out i):SV_Target{
    const uint2 p=uint2(i.position.xy);
    if(any(p>=uint2(frame.width,frame.height)))return 0;
    const RtRestirSurface s=RtRestirStored(int2(p),false);
    Texture2D<float4> gb2=ResourceDescriptorHeap[frame.gb2],gb3=ResourceDescriptorHeap[frame.gb3];
    const float3 albedo=gb2.Load(int3(p,0)).rgb;
    float3 l=0;
    // I-0128 R8d: a decorrelated pixel (its own draw, the share RT_RESTIR_SHARE) takes its initial sample's estimate.
    const float share=RT_RESTIR_SHARE(frame.restir.z);
    if(s.valid && any(albedo>0) && share>0 && RtRandom2(p,frame.seed,RT_RESTIR_SALT+1).x<share)l=asfloat(gi_surfaces[2*frame.restir.w+p.y*frame.width+p.x].xyz);
    else if(s.valid && any(albedo>0)){const RTXDI_GIReservoir r=RTXDI_LoadGIReservoir(RtRestirBuffers(),p,1);
        const float3 d=r.position-s.origin;const float len=length(d);
        if(RTXDI_IsValidGIReservoir(r) && r.weightSum>0 && len>0)l=r.radiance*saturate(dot(s.normal,d/len))*0.31830989*r.weightSum;}
    if(!all(isfinite(l)))l=0;
    GiMean(p,l);
    if(!any(albedo>0))return 0;
    const float3 f0=pow(gb3.Load(int3(p,0)).rgb,2);
    return float4(albedo*(1-f0)*l,0);
}
// I-0126 R7b (R7-PLAN §3, SHaRC's integration guide): the cache's update, one path a 5 x 5 block of pixels (the draw's grid, a pixel
// of the block drawn at random each frame, so the screen is covered over frames: the guide's 4 %), from the pixel's surface along a
// cosine direction as the GI's ray, then from each hit along a cosine direction about its shading normal, up to RT_CACHE_BOUNCES hits.
// Each hit's direct light (HitLight's diffuse times its albedo, its specular toward the vertex before it, its emission) is given to
// the vertices before it (SharcUpdateMiss: the vertices' weights their throughputs over their albedos), never to
// its own cell, which SharcUpdateHit inserts with no light of its own (its sample counted): a cell gathers its surface's indirect light
// over its albedo, the ambient HitAmbient reads. The throughput a leg carries: the vertex's albedo × (1 − F0) (Lambert's cosine and
// π cancelled by the cosine draw; the GI pass's own weighting). The last hit (RT_CACHE_BOUNCES) gives its direct light and its
// ambient (HitAmbient: the cache's answer, no baked seed since I-0132 R11d) and is not inserted; SHaRC's resampling ends a path early
// at a filled cell (its cached ambient times the albedo, its direct light given already). A miss gives the forward sky, else nothing
// (R11d: no stand-in); a hit without its records or material ends the path with nothing. The pixel's own surface is not a cell (the
// queries are at the rays' hits). The cells' normal: the hit's geometric one (toward the ray's origin: a wall's two sides apart).
#define RT_CACHE_BLOCK 5u
#define RT_CACHE_BOUNCES 3u
#define RT_CACHE_SALT 0x3000u
[RootSignature(ReflectRoot)]
void cache_update(Out i){
#if SHARC_UPDATE
    const uint2 cell=uint2(i.position.xy);
    const uint2 p=min(cell*RT_CACHE_BLOCK+uint2(RtRandom2(cell,frame.seed,RT_CACHE_SALT)*RT_CACHE_BLOCK),uint2(frame.width,frame.height)-1);
    float3 eye;float r;
    if(!(frame.mode&RT_REFLECT_TRACED) || !frame.materials || !PixelSurface(p,eye,r))return;
    Texture2D<float4> gb0=ResourceDescriptorHeap[frame.gb0];
    const float3 n=normalize(gb0.Load(int3(p,0)).xyz*2-1);
    const SharcParameters s=CacheParameters();SharcState state;SharcInit(state);
    RayDesc ray;ray.Origin=frame.position.xyz+eye*max(r-(frame.offset.x+frame.offset.y*r),0);ray.Direction=Cosine(n,RtRandom2(p,frame.seed,RT_CACHE_SALT+1));
    ray.TMin=frame.offset.z;ray.TMax=1.0e5;
    const float cone=2*frame.jitter.w/frame.position.w;float start=r;
    [loop]for(uint k=0;k<=RT_CACHE_BOUNCES;++k){
        RtSurface h;RtInstanceRow row;float t;
        const uint met=TraceSurface(ray,start,cone,h,row,t);
        if(met==RT_MET_NOTHING){float3 sky=0;if(!(frame.mode&RT_REFLECT_GI_NO_SKY))SkyRadiance(ray,start,cone,sky);
            SharcUpdateMiss(s,state,CacheSample(sky));break;}
        if(met!=RT_MET_SURFACE)break;
        float3 direct,glossy;HitLight(h,row,ray.Direction,p,RT_CACHE_SALT+0x400*(k+1),direct,glossy);
        // Its specular too (step review M1): toward the vertex before it, light that vertex receives, whatever the camera.
        const float3 albedo=saturate(h.albedo);   // one albedo for the vertex's light, its ambient, its throughput and demodulation (the fixes' step review L-2)
        const float3 own=albedo*direct+glossy+HitEmission(h);
        // The last hit, or one whose next leg would go past its triangle's horizon (its shading normal tilted: the final review's L1,
        // no cell counted unlit), ends the path: its direct light and its ambient to the vertices before it, no cell of its own.
        const float3 dir=Cosine(h.normal,RtRandom2(p,frame.seed,RT_CACHE_SALT+0x10+k));
        if(k==RT_CACHE_BOUNCES || !(dot(dir,h.geometric)>0)){SharcUpdateMiss(s,state,CacheSample(own+albedo*HitAmbient(h,t)));break;}
        SharcUpdateMiss(s,state,CacheSample(own));
        SharcHitData hit;hit.positionWorld=h.position;hit.normalWorld=h.geometric;hit.materialDemodulation=max(albedo,RT_CACHE_ALBEDO_MIN);
        if(!SharcUpdateHit(s,state,hit,0,RtRandom2(p,frame.seed,RT_CACHE_SALT+2+k).x))break;
        SharcSetThroughput(state,albedo*(1-h.f0));
        start+=t;
        ray.Origin=h.position+h.geometric*(frame.offset.x+frame.offset.y*length(h.position-frame.position.xyz));ray.Direction=dir;}
#endif
}
// R7b: SHaRC's resolve, one thread an entry (kRtCacheEntries / 256 groups): this frame's samples merged into the cells' history
// (RT_CACHE_FRAMES), the cells without a sample for RT_CACHE_STALE frames evicted, the accumulation cleared; with
// RT_REFLECT_CACHE_CLEAR, every entry zeroed instead (the buffers' first frame: SHaRC's guide asks them cleared).
[RootSignature(ReflectRoot)]
[numthreads(256,1,1)]
void cache_resolve(uint3 id:SV_DispatchThreadID){
    if(id.x>=RT_CACHE_ENTRIES)return;
    if(frame.mode&RT_REFLECT_CACHE_CLEAR){cache_keys[id.x]=HASH_GRID_INVALID_HASH_KEY;SharcAccumulationData a;a.data=0;cache_accumulated[id.x]=a;
        SharcPackedData z;z.radianceData=0;z.sampleData=0;z.sampleDataExt=0;cache_resolved[id.x]=z;return;}
    SharcResolveParameters r;r.cameraPositionPrev=frame.previous.xyz;r.accumulationFrameNum=RT_CACHE_FRAMES;r.staleFrameNumMax=RT_CACHE_STALE;r.frameIndex=frame.seed;
    SharcResolveEntry(id.x,CacheParameters(),r);
}
// I-0129 R9a (R9-PLAN §2): the traced probes, one group a request (rt_probes.h): RT_PROBE_RAYS rays, each a uniform direction w
// from a uniform point of the object's box moved along w to where it leaves the box (no ray meets the object itself), its radiance
// L as the GI's ray takes it (HitRadiance unfogged, the cache at the hit; a miss the forward sky, else nothing since I-0132 R11d);
// a ray that meets nothing readable or a triangle's back (the box inside a wall) is not counted. On the
// frames the cache is resolved (a GI frame) only. And one ray a thread toward the
// sun (its cone), from another point of the box, where the frame traces the sun and the request asks it (flags bit 0 clear), a shadowed
// ray counting 0 (no FC_ShadowColor floor since I-0132 R11d). The group's sums go to the results (rt_probes.h's
// RtProbeResult): {L, rays}, {L w red, the sun's visible rays}, {L w green, its traced rays}, {L w blue, 0}.
#define RT_PROBE_RAYS 128u   // rt_probes.h's kRtProbeRays
#define RT_PROBE_SALT 0x4000u
groupshared float4 probe_sums[4][RT_PROBE_RAYS];
float3 ProbeExit(float3 p,float3 w,float3 lo,float3 hi){   // where the ray from p (inside the box) along w leaves it
    const float3 face=select(w>0,hi,lo);float t=1.0e30;
    [unroll]for(uint k=0;k<3;++k)if(abs(w[k])>1e-6)t=min(t,(face[k]-p[k])/w[k]);
    return p+w*max(t,0);}
[RootSignature(ReflectRoot)]
[numthreads(RT_PROBE_RAYS,1,1)]
void probe(uint3 group:SV_GroupID,uint k:SV_GroupIndex){
    ByteAddressBuffer requests=ResourceDescriptorHeap[frame.probe.x];
    const uint at=group.x;
    const float4 a=asfloat(requests.Load4(32*at)),b=asfloat(requests.Load4(32*at+16));
    const float3 lo=min(a.xyz,b.xyz),hi=max(a.xyz,b.xyz),centre=(lo+hi)*0.5;
    const uint2 p=uint2(at,k);
    const float start=length(centre-frame.position.xyz),cone=2*frame.jitter.w/frame.position.w;
    float4 sum=0,red=0,green=0,blue=0;
    const bool traced=(frame.mode&RT_REFLECT_TRACED) && frame.materials && at<frame.probe.z;
    if(traced && (frame.mode&RT_REFLECT_CACHE)){
        const float2 u=RtRandom2(p,frame.seed,RT_PROBE_SALT),v=RtRandom2(p,frame.seed,RT_PROBE_SALT+1);const float z=1-2*u.x,s=sqrt(max(0,1-z*z)),phi=6.2831853*u.y;
        const float3 w=float3(s*cos(phi),z,s*sin(phi));
        RayDesc ray;ray.Origin=ProbeExit(lo+(hi-lo)*float3(v,RtRandom2(p,frame.seed,RT_PROBE_SALT+2).x),w,lo,hi);ray.Direction=w;ray.TMin=frame.offset.z;ray.TMax=1.0e5;
        RtSurface h;RtInstanceRow row;float t;float3 l=0;bool counted=false;
        const uint met=TraceSurface(ray,start,cone,h,row,t);
        if(met==RT_MET_SURFACE && !h.back){l=HitRadiance(h,row,w,p,RT_PROBE_SALT+0x10,t);counted=true;}
        else if(met==RT_MET_NOTHING){if(!(frame.mode&RT_REFLECT_GI_NO_SKY))SkyRadiance(ray,start,cone,l);counted=true;}
        l=RtFinite(l);
        if(counted){sum=float4(l,1);red.xyz=l.r*w;green.xyz=l.g*w;blue.xyz=l.b*w;}}
    // The step review's H1: no sun rays for a request flagged so (rt_probes.h's kRtProbeRequestNoSun). M1: a ray the sun does not reach
    // counts 0, as HitLight's `seen` (I-0132 R11d, D-0262: no FC_ShadowColor floor any more).
    if(traced && (frame.mode&RT_REFLECT_SUN) && !(asuint(b.w)&1u)){float3 toward=frame.sun.xyz;
        if(frame.cone>0)toward=RtConeSample(toward,frame.cone,RtRandom2(p,frame.seed,RT_PROBE_SALT+3));
        const float3 q=lo+(hi-lo)*float3(RtRandom2(p,frame.seed,RT_PROBE_SALT+4),RtRandom2(p,frame.seed,RT_PROBE_SALT+5).x);
        red.w=Occluded(RtMaskSunBits,ProbeExit(q,toward,lo,hi),toward,1.0e5)?0:1;green.w=1;}
    probe_sums[0][k]=sum;probe_sums[1][k]=red;probe_sums[2][k]=green;probe_sums[3][k]=blue;
    GroupMemoryBarrierWithGroupSync();
    [unroll]for(uint step=RT_PROBE_RAYS/2;step>0;step>>=1){
        if(k<step)[unroll]for(uint c=0;c<4;++c)probe_sums[c][k]+=probe_sums[c][k+step];
        GroupMemoryBarrierWithGroupSync();}
    if(k!=0 || at>=frame.probe.z)return;
    RWByteAddressBuffer results=ResourceDescriptorHeap[frame.probe.y];
    [unroll]for(uint c=0;c<4;++c)results.Store4(64*at+16*c,asuint(probe_sums[c][0]));
}
// I-0132 R11b (R11-PLAN §13): shader execution reordering on the PT level's paths. The ray generation builds (lib_6_9, RT_PATHS 1 and
// RT_RAYGEN 1: TraceSurface reorders each thread by its hit's record) of the two programmes the PT level's frames pay for: the
// reflections' lobe with its path (trace_hit's `_path` build) and ReSTIR GI's initial sample with its path (gi_temporal's). One thread a
// pixel over the extent (DispatchRays). trace_hit_ser writes its colour and its hit distance into the scratch targets (frame.ser.x, .y);
// ser_apply, drawn with trace_hit's pipeline state (its stencil test, its targets), copies them where trace_hit would have written:
// every pixel is traced (the stencil is not read here), the stencil's choice is the draw's. gi_temporal_ser writes the reservoirs as
// gi_temporal does (its UAVs only).
#if RT_RAYGEN
[shader("raygeneration")]
void trace_hit_ser(){
    const uint2 p=DispatchRaysIndex().xy;if(any(p>=uint2(frame.width,frame.height)))return;
    float distance;const float4 colour=Trace(p,distance);
    RWTexture2D<float4> c=ResourceDescriptorHeap[frame.ser.x];RWTexture2D<float> d=ResourceDescriptorHeap[frame.ser.y];
    c[p]=colour;d[p]=distance;}
[shader("raygeneration")]
void gi_temporal_ser(){GiTemporal(DispatchRaysIndex().xy);}
#endif
[RootSignature(ReflectRoot)]
HitOut ser_apply(Out i){
    const int3 p=int3(i.position.xy,0);Texture2D<float4> c=ResourceDescriptorHeap[frame.ser.z];Texture2D<float> d=ResourceDescriptorHeap[frame.ser.w];
    HitOut o;o.colour=c.Load(p);o.distance=d.Load(p);return o;}
