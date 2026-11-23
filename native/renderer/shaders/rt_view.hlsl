// I-0117 (R2 T4, R2-PLAN §3): the control view (rt-view.flag, a test tool). One primary ray a pixel of the main view through the
// frame's top level, from the camera as the raster's projection makes it (BuildSceneView's lens, the upscaler's jitter undone),
// against the raster depth right after the G-buffer (the deferred rows only: no transparent, effect or forward row has written it
// yet). Each pixel is classed: the traced distance within 2 cm + 1 % of the raster's (agree), within 5 % (close), nearer, farther,
// no hit where the raster has a surface (missed), no surface in either (sky), a hit where the raster has none (extra); the counts
// go to a raw buffer (one word a class) and two pictures: the traced scene (I-0119 R3a: each hit's albedo from its material record,
// rt_material.hlsli, at the level the primary ray's cone gives; a slice without a material its instance's colour) and the classes.
// I-0119 R3b: at the agreeing pixels whose hit has a material, its albedo, F0, gloss and normal against the G-buffer the raster wrote
// there, counted by programme family, and a third picture of the differences (red albedo, green gloss, blue normal). Every resource
// but the scene, the material table, the rows and the counts is reached through the bindless heap. Compiled by dxc at build time (Build-GameBootstrap.ps1).
#define ViewRoot "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), CBV(b0), SRV(t0), SRV(t1), SRV(t2), UAV(u0), "     "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_WRAP)"
#include "rt_material.hlsli"
// RtViewConstants (rt_scene.h): the camera's rows (world space) with the near and far planes, the lens scales sx, sy, the jitter in
// NDC and the reciprocal extent; the extent, the slots (depth SRV, the two pictures' UAVs), the pass (1: clear the counts), the
// instance mask and whether a material table is bound; I-0119 R3b: the slots of the materials picture's UAV and of GB0 to GB3's SRVs
// (~0: no comparison).
struct View {float4 right;float4 up;float4 forward;float4 position;float4 jitter;uint width;uint height;uint depth;uint traced;uint classes;uint clear;uint mask;uint materials;
    uint differences;uint gb0;uint gb1;uint gb2;uint gb3;uint anisotropy;float texture_bias;uint pad2;   // R3c: the raster's samplers' anisotropy and mip bias
    uint gb4;uint3 pad3;float4 dominant;};   // I-0125 R6a: GB4's SRV, the frame's dominant rows (RtAmbientOf)
ConstantBuffer<View> view:register(b0);
RaytracingAccelerationStructure scene:register(t0);
StructuredBuffer<RtMaterialRecord> table:register(t1);
ByteAddressBuffer rows:register(t2);       // RtInstanceRow by instance index, then the skinned instances' slice bases
// RtViewCounts: agree, close, nearer, farther, missed, sky, extra, nearer_cw, alpha_met; then (R3b) from byte 64 one RtMaterialCheck
// of 16 words a programme family (RT_FAMILIES): the compared pixels, those within tolerance by channel (albedo, F0, gloss, normal), the
// summed errors (albedo and gloss in 8-bit steps, the normal in tenths of a degree); R3c: the signed albedo errors by channel and the
// signed gloss error (traced less raster, in steps, rounded; two's complement), the pixels whose second-layer weight is at least one
// half and their summed albedo error; two words unused. I-0125 R6a (20 words a family): from word 14 the ambient against GB4 at the
// same pixels, for the families that write one (RtMaterialCheck): compared, within 5 %, the signed and absolute sums of 256 log2 of the
// luminances' ratio (traced over raster), the pixels whose ambient's inputs were missing.
RWByteAddressBuffer counts:register(u0);
#define RT_FAMILIES 256
#define RT_FAMILY_BYTES 80
#define RT_COUNT_BYTES (64+RT_FAMILY_BYTES*RT_FAMILIES)
SamplerState wrap:register(s0);
float3 Hue(uint key){key=(key^61u)^(key>>16);key*=9u;key^=key>>4;key*=0x27d4eb2du;key^=key>>15;
    return float3(key&0xFF,(key>>8)&0xFF,(key>>16)&0xFF)/255.0*0.75+0.25;}
float3 Srgb(float3 c){c=saturate(c);return select(c<=0.0031308,12.92*c,1.055*pow(c,1/2.4)-0.055);}
[RootSignature(ViewRoot)]
[numthreads(8,8,1)]
void main(uint2 p:SV_DispatchThreadID){
    if(view.clear){for(uint w=16*(p.y*8+p.x);w<RT_COUNT_BYTES;w+=16*64)counts.Store4(w,uint4(0,0,0,0));return;}   // one group of 64
    uint kind=8;bool cw=false,met=false;float met_t=3.4e38;   // outside the extent: counted nowhere
    if(p.x<view.width && p.y<view.height){
        Texture2D<float> depth=ResourceDescriptorHeap[view.depth];
        const float d=depth.Load(int3(p,0));
        // The pixel's centre in NDC less the jitter the raster's projection added (m20 = 2 jx / W, m21 = -2 jy / H).
        const float2 ndc=float2((p.x+0.5)*view.jitter.z*2-1,1-(p.y+0.5)*view.jitter.w*2)-view.jitter.xy;
        const float3 v=float3(ndc.x/view.forward.w,ndc.y/view.position.w,1);   // view space at z = 1
        const float scale=length(v);
        RayDesc ray;ray.Origin=view.position.xyz;ray.Direction=(v.x*view.right.xyz+v.y*view.up.xyz+v.z*view.forward.xyz)/scale;
        ray.TMin=view.right.w*scale;ray.TMax=view.up.w*scale;   // between the raster's near and far planes
        RayQuery<RAY_FLAG_NONE> query;query.TraceRayInline(scene,view.materials?RAY_FLAG_NONE:RAY_FLAG_FORCE_OPAQUE,view.mask&0xFF,ray);
        while(query.Proceed()){
            if(query.CandidateType()!=CANDIDATE_NON_OPAQUE_TRIANGLE)continue;
            const RtMaterialRecord record=table[query.CandidateInstanceContributionToHitGroupIndex()+query.CandidateGeometryIndex()];
            // T5: the nearest alpha-tested candidate met (final review M3: counted only when not past the surfaces compared, below).
            if(RtAnswers(record,view.mask) && (record.flags&RT_RECORD_ALPHA))met_t=min(met_t,query.CandidateTriangleRayT());
            if(RtAnswers(record,view.mask) && RtKeeps(record,RtRowOf(rows,query.CandidateInstanceIndex()).scroll,query.CandidatePrimitiveIndex(),
                query.CandidateTriangleBarycentrics(),wrap))query.CommitNonOpaqueTriangleHit();
        }
        const bool hit=query.CommittedStatus()==COMMITTED_TRIANGLE_HIT;const float t=hit?query.CommittedRayT():0;
        float3 traced=0,classes=0,differences=0;RtSurface s=(RtSurface)0;uint family=0;
        if(hit){
            // R3a: the hit's albedo (the primary ray's cone: a pixel's angle 2 / (sy H) at the distance); a slice without a material
            // its instance's colour, darkened with the distance.
            if(view.materials){const RtInstanceRow row=RtRowOf(rows,query.CommittedInstanceIndex());
                if(query.CommittedGeometryIndex()<row.count){
                    const RtMaterialRecord record=table[query.CommittedInstanceContributionToHitGroupIndex()+query.CommittedGeometryIndex()];family=min(record.family,RT_FAMILIES-1);
                    s=RtMaterialAt(table,record,row,rows,query.CommittedGeometryIndex(),query.CommittedObjectToWorld3x4(),query.CommittedPrimitiveIndex(),
                        query.CommittedTriangleBarycentrics(),ray.Direction,t*2*view.jitter.w/view.position.w,view.anisotropy,view.texture_bias,wrap);}}
            if(s.material)traced=sqrt(saturate(s.albedo));
            else traced=Hue(query.CommittedInstanceID()*131u+query.CommittedGeometryIndex())*(0.45+0.55/(1+t/80));}
        if(d<1){
            const float a=view.up.w/(view.up.w-view.right.w);const float r=a*view.right.w/(a-d)*scale;   // z from the depth, then along the ray
            const float tolerance=0.02+0.01*r;
            kind=!hit?4:abs(t-r)<=tolerance?0:abs(t-r)<=0.02+0.05*r?1:t<r?2:3;
            // T5: a nearer hit on a triangle clockwise as DXR sees it (its front face by default; the raster's back face, its front
            // being counter-clockwise, for an unmirrored instance): counted again in word 7, drawn cyan.
            cw=kind==2 && query.CommittedTriangleFrontFace();
            met=met_t<=max(hit?t:0,r)*1.01+0.02;   // an alpha test met no farther than the farther of the two surfaces (its tolerance)
            const float grey=0.2+0.6*exp(-r/60);
            classes=kind==0?float3(grey,grey,grey):kind==1?float3(1,0.9,0.1):kind==2?(cw?float3(0,0.9,0.9):float3(0.15,0.35,1)):kind==3?float3(1,0.5,0):float3(1,0,0);
            // R3b: where the traced surface is the raster's (agree) and has a material, its channels against the G-buffer the raster
            // wrote there (GB0 the normal × 0.5 + 0.5, GB1.x the gloss, GB2 the albedo through its sRGB view, GB3 sqrt F0): within
            // 8 steps of 255 (albedo and F0 compared as stored: sRGB, square root), the normal within 10 degrees; by family.
            if(kind==0 && s.material && view.gb0!=RT_NONE){
                Texture2D<float4> gb0=ResourceDescriptorHeap[view.gb0],gb1=ResourceDescriptorHeap[view.gb1],gb2=ResourceDescriptorHeap[view.gb2],gb3=ResourceDescriptorHeap[view.gb3];
                const float3 n=gb0.Load(int3(p,0)).xyz*2-1;const float gloss=gb1.Load(int3(p,0)).x;const float3 albedo=gb2.Load(int3(p,0)).rgb,f0=gb3.Load(int3(p,0)).rgb;
                const float3 da=abs(Srgb(albedo)-Srgb(s.albedo))*255,df=abs(f0-sqrt(saturate(s.f0)))*255;
                const float ea=max(da.x,max(da.y,da.z)),ef=max(df.x,max(df.y,df.z)),eg=abs(gloss-saturate(s.gloss))*255;
                const float en=dot(n,n)>0?degrees(acos(clamp(dot(normalize(n),s.normal),-1,1))):180;
                const uint at=64+RT_FAMILY_BYTES*family;
                counts.InterlockedAdd(at,1);counts.InterlockedAdd(at+4,ea<=8?1:0);counts.InterlockedAdd(at+8,ef<=8?1:0);counts.InterlockedAdd(at+12,eg<=8?1:0);
                counts.InterlockedAdd(at+16,en<=10?1:0);counts.InterlockedAdd(at+20,uint(min(ea,255)));counts.InterlockedAdd(at+24,uint(min(eg,255)));counts.InterlockedAdd(at+28,uint(min(en,180)*10));
                // R3c: the bias apart from the spread (a scale or a weight off shows in the signed sums; texel speckle cancels there).
                const int3 sa=int3(round((Srgb(s.albedo)-Srgb(albedo))*255));
                counts.InterlockedAdd(at+32,asuint(sa.x));counts.InterlockedAdd(at+36,asuint(sa.y));counts.InterlockedAdd(at+40,asuint(sa.z));
                counts.InterlockedAdd(at+44,asuint(int(round((saturate(s.gloss)-gloss)*255))));
                if(s.layer>=0.5){counts.InterlockedAdd(at+48,1);counts.InterlockedAdd(at+52,uint(min(ea,255)));}
                // I-0125 R6a: the hit's ambient (RtAmbientOf) against GB4 by luminance (1/1024 apart allowed).
                if(s.ambient==RT_AMBIENT_MISSING)counts.InterlockedAdd(at+72,1);
                else if(s.ambient!=RT_AMBIENT_NONE && view.gb4!=RT_NONE){Texture2D<float4> gb4=ResourceDescriptorHeap[view.gb4];
                    const float3 luma=float3(0.2126,0.7152,0.0722);
                    const float raster=dot(gb4.Load(int3(p,0)).rgb,luma),ambient=dot(RtAmbientOf(s,view.dominant),luma);
                    const int ratio=clamp(int(round(256*log2((ambient+1.0/1024)/(raster+1.0/1024)))),-4096,4096);   // clamped: a sample's sums in 32 bits up to 524 k saturated pixels a family (signed), 1 M (absolute)
                    counts.InterlockedAdd(at+56,1);counts.InterlockedAdd(at+60,abs(ambient-raster)<=0.05*max(ambient,raster)+1.0/1024?1:0);
                    counts.InterlockedAdd(at+64,asuint(ratio));counts.InterlockedAdd(at+68,uint(abs(ratio)));}
                differences=float3(saturate(ea/32),saturate(eg/32),saturate(en/30));}}
        else{kind=hit?6:5;classes=hit?float3(1,0,1):float3(0,0,0);}
        RWTexture2D<float4> traced_out=ResourceDescriptorHeap[view.traced];RWTexture2D<float4> classes_out=ResourceDescriptorHeap[view.classes];
        traced_out[p]=float4(traced,1);classes_out[p]=float4(classes,1);
        if(view.differences!=RT_NONE){RWTexture2D<float4> differences_out=ResourceDescriptorHeap[view.differences];differences_out[p]=float4(differences,1);}}
    // One atomic a class a wave.
    [unroll]for(uint k=0;k<7;++k){const uint n=WaveActiveCountBits(kind==k);if(n && WaveIsFirstLane())counts.InterlockedAdd(4*k,n);}
    {const uint n=WaveActiveCountBits(cw);if(n && WaveIsFirstLane())counts.InterlockedAdd(28,n);}
    {const uint n=WaveActiveCountBits(met && kind>=1 && kind<=4);if(n && WaveIsFirstLane())counts.InterlockedAdd(32,n);}
}
