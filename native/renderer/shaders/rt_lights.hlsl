// I-0122 R4a (R4-PLAN §4; on with the rays): the frame's local lights shaded natively from the one list, in place of the original's
// tile batches of GXLightAcc_DPointLightN, LegacyDPointLightN and DSpotLightN: one full-screen triangle, each pixel summing the lights
// its 128-pixel tile holds (the batches' binning, in their order), added (blend ONE/ONE, as the programmes) into the diffuse and
// specular accumulation targets. The programmes' common factors: the diffuse × (1 − F0 / 2), the specular × F0 (F0 = GB3²), both
// (I-0126 R7c, D-0251: no SSAO term since the SSAO's removal). R4b (R4-PLAN §3, the plan's R5b): a shadowed light (RT_LIGHT_SHADOWED) as the `Shadowed` programmes
// take it, its visibility v = 1 + strength × (s − 1) on the diffuse and the specular (not the translucency), s from one ray toward
// the light through the frame's top level (the local casters' mask RtMaskLocal and the alpha test, as the sun's) instead of the
// local atlas's four taps; with no top level this frame (`traced` 2) the shadowed lights are shaded unshadowed (I-0131 R10a: the
// local atlas and the shadowed batches are removed). Compiled by dxc at build time
// (Build-GameBootstrap.ps1): vs_6_6 `vs`, ps_6_6 `shade` and `pt`.
// R4c (R4-PLAN §7, the PT level: rt-level.txt « pt », on a frame Ray Reconstruction denoises with a top level): every light of the
// tile casts, `pt` in place of `shade`: one light a pixel chosen in proportion to its unshadowed terms, its ray, its terms weighted
// as an unbiased estimate of them all, added with the tile's unshadowed translucency.
#define LightsRoot "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), CBV(b0), SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), "     "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_WRAP)"
#include "rt_lights.hlsli"
#include "rt_material.hlsli"
#define RtMaskLocalBits 4u   // RtMaskLocal (rt_scene.h): the local lights' casters (the engine's category 0x04)
// RtLightsConstants (rt_scene.h): the camera as RtViewConstants' first rows (the rows with the near and far planes, the lens scales,
// the jitter and the reciprocal extent), the extent, the slots of the depth (R32 of the D32S8), GB0, GB1, GB3, a word unused since
// R7c (the SSAO term's slot, D-0251), the tiles' columns and edge; R4b: the shadowed lights traced (1: a top level
// bound) or shaded unshadowed (2), whether a material table is bound, the shadow ray's offset (metres, a
// share of the distance, its TMin; the sun's).
// I-0123 RR-b: `seed` the frame's, offset.w a traced light's sphere radius as a share of its range (0: the sharp ray to its centre).
struct Lights {float4 right;float4 up;float4 forward;float4 position;float4 jitter;uint width;uint height;uint depth;uint gb0;uint gb1;uint gb3;uint pad;uint columns;
    uint tile;uint traced;uint materials;uint seed;float4 offset;};
#define RtLightRadiusMax 0.5   // metres: the sphere's radius cap (rt_scene.h kRtLightRadiusShare's comment)
ConstantBuffer<Lights> frame:register(b0);
StructuredBuffer<RtLight> lights:register(t0);
ByteAddressBuffer tiles:register(t1);   // a tile's {first word, count} at 8 × tile, then the lights' indices
RaytracingAccelerationStructure scene:register(t2);   // R4b: the frame's top level (read only when `traced`)
StructuredBuffer<RtMaterialRecord> table:register(t3);
ByteAddressBuffer rows:register(t4);   // RtInstanceRow by instance index
SamplerState wrap:register(s0);         // SS_WrapLinear (the alpha-tested casters' textures)
struct Out {float4 position:SV_Position;};
struct Targets {float4 diffuse:SV_Target0;float4 specular:SV_Target1;};
// R4b: whether `light` is seen from `origin` (the surface moved off itself toward the light's side): one ray toward it through the
// local casters with the alpha test (RtOccluded, the sun's query), stopping where the atlas's view of the light starts (nothing
// nearer the light than its near plane was drawn into the atlas). On the segment to the light, a spot's q.z is affine (0 at its near
// plane) and every component of p − light scales by (1 − t / d), so a point's major-axis distance, taken on the view's axes as the
// programmes pick the cube's face (DPointLightShadowed1: ShadowCenterPos − P in view space), meets its faces' near plane at
// d (1 − near / m). A point's near plane unknown (trans.w 0): seen, rather than a ray ending inside the light's own housing.
// I-0123 RR-b: the ray aims at `aim`, the light's position or a point of its sphere; the cut stays the light's own: a spot's plane is
// affine (its crossing exact toward any aim; an aim in front of the plane, possible on the sphere, traces the whole segment: the fixes'
// review L-A), a point's cube around its position met by a slab test on the view's axes (step review L1). 1 seen, 0 hidden.
// R4c: `stop` for a point whose near plane is unknown (a light the original did not shadow, at the PT level): the ray ends that far
// short of the aim (0: seen, R4b's rule).
float RtLightSeen(float3 origin,RtLight light,float3 aim,float stop){
    const float3 to=aim-origin;const float d=length(to);const float3 dir=to/d;
    float far;
    if((light.kind&RT_LIGHT_KIND)==RT_LIGHT_SPOT){
        const float zs=dot(light.to_light[2],float4(origin,1)),zl=dot(light.to_light[2],float4(aim,1));
        if(!(zs>0))return 1;
        far=zl<0?d*zs/(zs-zl):d;   // the plane crossed before the aim; else no cut (the segment in front of it)
        if(stop>0 && (light.kind&RT_LIGHT_SHADOWED)==0)far=min(far,d-stop);}   // R4c: an unshadowed spot's housing too (final review L3)
    else{const float n=(light.kind&RT_LIGHT_SHADOWED)!=0?light.trans.w:0;
        if(!(n>0)){if(!(stop>0))return 1;far=d-stop;}
        else{
            const float3 o=origin-light.position.xyz;
            const float3 oa=float3(dot(o,frame.right.xyz),dot(o,frame.up.xyz),dot(o,frame.forward.xyz));
            if(!(max(abs(oa.x),max(abs(oa.y),abs(oa.z)))>n))return 1;   // the surface within the near cube
            const float3 inv=1/float3(dot(dir,frame.right.xyz),dot(dir,frame.up.xyz),dot(dir,frame.forward.xyz));
            const float3 t1=(-n-oa)*inv,t2=(n-oa)*inv,lo=min(t1,t2),hi=max(t1,t2);
            const float enter=max(lo.x,max(lo.y,lo.z)),leave=min(hi.x,min(hi.y,hi.z));
            far=enter<=leave && enter>0?enter:d;}}   // where the segment enters the cube, else its end
    if(!(far>frame.offset.z))return 1;
    RayDesc ray;ray.Origin=origin;ray.Direction=dir;ray.TMin=frame.offset.z;ray.TMax=min(far,d);
    return RtOccluded(scene,table,rows,frame.materials!=0,RtMaskLocalBits,ray,wrap)?0:1;
}
[RootSignature(LightsRoot)]
Out vs(uint id:SV_VertexID){Out o;const float2 uv=float2((id<<1)&2,id&2);o.position=float4(uv*float2(2,-2)+float2(-1,1),0,1);return o;}
// The unit direction from the camera through pixel p and its distance at depth d.
float3 RtEyeAt(uint2 p,float d,out float r){
    const float2 ndc=float2((p.x+0.5)*frame.jitter.z*2-1,1-(p.y+0.5)*frame.jitter.w*2)-frame.jitter.xy;
    const float3 v=float3(ndc.x/frame.forward.w,ndc.y/frame.position.w,1);
    const float scale=length(v);const float a=frame.up.w/(frame.up.w-frame.right.w);r=a*frame.right.w/(a-d)*scale;
    return (v.x*frame.right.xyz+v.y*frame.up.xyz+v.z*frame.forward.xyz)/scale;
}
// The pixel's surface as the traced sun finds it (rt_shadow.hlsl): the ray through the pixel's centre, the jitter undone, at the
// distance the raster depth gives (r); N from GB0, gloss and translucency from GB1, F0² from GB3. False on the sky.
bool RtSurfaceAt(uint2 p,out RtLitSurface s,out float r,out float3 reflectance){
    s=(RtLitSurface)0;r=0;reflectance=0;
    Texture2D<float> depth=ResourceDescriptorHeap[frame.depth];
    const float d=depth.Load(int3(p,0));
    if(d>=1)return false;
    const float3 eye=RtEyeAt(p,d,r);
    Texture2D<float4> gb0=ResourceDescriptorHeap[frame.gb0],gb1=ResourceDescriptorHeap[frame.gb1],gb3=ResourceDescriptorHeap[frame.gb3];
    s.position=frame.position.xyz+eye*r;s.eye=eye;s.normal=normalize(gb0.Load(int3(p,0)).xyz*2-1);
    const float2 g=gb1.Load(int3(p,0)).xy;s.gloss=g.x;s.translucency=g.y;
    const float3 f0=gb3.Load(int3(p,0)).rgb;reflectance=f0*f0;
    return true;
}
// R4b: the shadow rays leave from the surface moved off it by the sun's offset (the depth's precision and the raster's LOD against the
// rays' LOD 0): toward the eye, as the sun's; for a translucent surface whose light and eye are on opposite sides of it, along the
// normal toward the light (final review M1: a thin translucent surface lit from behind, pulled toward the eye, would hide the light
// behind itself; the atlas sees it at its own depth). An opaque surface keeps the eye's side (no specular through a thin wall: the
// fixes' review L1); the sides by the eye too (a bump normal turned from the eye near a silhouette: L2).
float3 RtRayOrigin(RtLitSurface s,float r,float3 light){
    const float back=frame.offset.x+frame.offset.y*r;
    const float lit_side=dot(s.normal,light-s.position);
    return s.translucency>0 && dot(s.normal,-s.eye)*lit_side<0?s.position+(lit_side<0?-s.normal:s.normal)*back:frame.position.xyz+s.eye*max(r-back,0);
}
// I-0123 RR-b: a traced light's sphere on a frame Ray Reconstruction denoises: a share of its range, capped (0: the sharp ray).
float RtLightSphere(RtLight light){
    const float reach=(light.kind&RT_LIGHT_PHYSICAL)?f16tof32(light.kind>>16):(light.kind&RT_LIGHT_KIND)==RT_LIGHT_SPOT?light.position.w:rsqrt(max(light.position.w,1e-12));   // I-0132 R11h: a physical light's DS3 range
    return frame.offset.w>0?min(frame.offset.w*reach,RtLightRadiusMax):0;
}
// I-0122 R4c (the fixes' review M-a): the surface's geometric normal from the depth (the bump map's tilts the shading normal off the
// wall), on each axis the neighbour nearer to the pixel's distance (not across an edge); the shading normal where none can be read.
float3 RtPlaneNormal(uint2 p,RtLitSurface s){
    Texture2D<float> depth=ResourceDescriptorHeap[frame.depth];
    float3 edge[2];
    for(uint axis=0;axis<2;++axis){float best=1e30;edge[axis]=0;
        for(int side=-1;side<=1;side+=2){const int2 q=int2(p)+(axis==0?int2(side,0):int2(0,side));
            if(any(q<0) || q.x>=int(frame.width) || q.y>=int(frame.height))continue;
            const float d=depth.Load(int3(q,0));if(d>=1)continue;
            float r;const float3 to=frame.position.xyz+RtEyeAt(uint2(q),d,r)*r-s.position;
            if(dot(to,to)<best){best=dot(to,to);edge[axis]=to*side;}}}
    const float3 n=cross(edge[0],edge[1]);const float len=length(n);
    if(!(len>0))return s.normal;
    return dot(n,s.normal)<0?-n/len:n/len;
}
// The ray toward a point of the light's sphere (its disc as the surface sees it), a new sample a pixel a frame and a light (`salt`);
// `radius` 0 for the sharp ray. I-0122 R4c (final review M1): the radius at most the light's height above the surface's planes (its
// geometric one, `plane`, and its shading one), so that no point of the disc falls behind the surface (a wall lit by its own sconce
// would dim evenly instead of showing a penumbra).
float3 RtLightAim(RtLight light,RtLitSurface s,float3 plane,float3 origin,uint2 p,uint salt,out float radius){
    radius=RtLightSphere(light);float3 aim=light.position.xyz;
    if(radius>0){const float3 to=aim-s.position;radius=min(radius,min(abs(dot(plane,to)),abs(dot(s.normal,to))));
        aim=RtDiscSample(aim,normalize(aim-origin),radius,RtRandom2(p,frame.seed,salt));}
    return aim;
}
[RootSignature(LightsRoot)]
Targets shade(Out i){
    const uint2 p=uint2(i.position.xy);
    const uint tile=(p.y/frame.tile)*frame.columns+p.x/frame.tile;
    const uint2 range=tiles.Load2(8*tile);
    if(range.y==0)discard;
    RtLitSurface s;float r;float3 reflectance;
    if(!RtSurfaceAt(p,s,r,reflectance))discard;
    const float3 plane=frame.offset.w>0 && frame.traced==1?RtPlaneNormal(p,s):s.normal;   // the spheres' cap (read only on traced RR frames)
    float3 diffuse=0,specular=0;
    for(uint k=0;k<range.y;++k){const RtLight light=lights[tiles.Load(4*(range.x+k))];
        const bool shadowed=(light.kind&RT_LIGHT_SHADOWED)!=0;
        float3 dl,tl,sl;RtLocalLight(light,s,dl,tl,sl);
        // The ray only where the shadowed terms are lit (no ray behind a spot, past a range or on a surface facing away) and the
        // strength is not 0.
        float seen=1;
        if(shadowed && frame.traced==1 && light.trans.z!=0 && any(dl+sl>0)){
            const float3 origin=RtRayOrigin(s,r,light.position.xyz);float radius;
            seen=1+light.trans.z*(RtLightSeen(origin,light,RtLightAim(light,s,plane,origin,p,k+1,radius),0)-1);}
        diffuse+=dl*seen+tl;specular+=sl*seen;}
    Targets o;o.diffuse=float4(diffuse*(1-reflectance*0.5),1);o.specular=float4(specular*reflectance,1);return o;
}
// R4c, the PT level (R4-PLAN §7): every light of the tile casts. The target p̂: the light's unshadowed diffuse and specular at the
// surface with the programmes' common factors (the diffuse × (1 − F0 / 2), the specular × F0), as luminance; the translucency, summed
// unshadowed (§1), not sampled (`translucent`).
float RtTarget(RtLight light,RtLitSurface s,float3 reflectance,out float3 diffuse,out float3 specular,out float3 translucent){
    RtLocalLight(light,s,diffuse,translucent,specular);diffuse=max(diffuse,0)*(1-reflectance*0.5);specular=max(specular,0)*reflectance;
    return dot(diffuse+specular,float3(0.2126,0.7152,0.0722));
}
// `pt`: one light a pixel chosen among the tile's in proportion to p̂ (resampled importance sampling with every light a candidate,
// streamed: a candidate kept with the probability p̂ / Σp̂ so far), its ray (`RtLightSeen`, 1 or 0: every light casts whole, the
// `Shadowed` programmes' strength, a raster's partial shadow, not kept; a point without a near plane stops short of the aimed point by its
// sphere's radius, at least 0.1 m: its own housing, INFERRED), its terms × Σp̂ / p̂(y) × v: an unbiased estimate of every light's
// shadowed terms, one ray a pixel a frame for Ray Reconstruction to denoise; added with every tile light's translucency, unshadowed.
// No reuse between pixels or frames (R4-PLAN §7: ReSTIR's reuse, written first, darkened the lit pixels beside a shadow and lagged a
// moving caster; with every tile light already a candidate, it would only share visibility).
[RootSignature(LightsRoot)]
Targets pt(Out i){
    const uint2 p=uint2(i.position.xy);
    const uint2 range=tiles.Load2(8*((p.y/frame.tile)*frame.columns+p.x/frame.tile));
    if(range.y==0)discard;
    RtLitSurface s;float r;float3 reflectance;
    if(!RtSurfaceAt(p,s,r,reflectance))discard;
    float sum=0,chosen=0;uint light=0;float3 diffuse=0,specular=0,translucent=0;
    for(uint k=0;k<range.y;++k){const uint index=tiles.Load(4*(range.x+k));float3 dl,sl,tl;
        const float target=RtTarget(lights[index],s,reflectance,dl,sl,tl);translucent+=tl;
        if(!(target>0))continue;
        sum+=target;if(RtRandom2(p,frame.seed,0x100+k).x*sum<target){light=index;chosen=target;diffuse=dl;specular=sl;}}
    if(sum>0){const RtLight l=lights[light];const float3 origin=RtRayOrigin(s,r,l.position.xyz);float radius;
        const float3 aim=RtLightAim(l,s,frame.offset.w>0?RtPlaneNormal(p,s):s.normal,origin,p,0x51,radius);
        // The stop the light's own sphere, not the disc's capped radius (the fixes' review M-b: a fixture's housing beside its wall).
        const float weight=sum/chosen*RtLightSeen(origin,l,aim,max(RtLightSphere(l),0.1));
        diffuse*=weight;specular*=weight;}
    Targets o;o.diffuse=float4(diffuse+translucent*(1-reflectance*0.5),1);o.specular=float4(specular,1);return o;
}
