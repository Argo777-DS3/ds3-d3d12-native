// I-0117 (R2 T2): a skinned instance's world positions for its BLAS, one vertex a thread, one slice a group row (SV_GroupID.y),
// as DS3's FLVER vertex programmes place them (their DXBC, the programme cache of I-0063): rigid (the non-_Skin programmes:
// row NORMAL.w of the palette), blended (the _Skin programmes: the four BLENDINDICES' rows weighted by BLENDWEIGHT over its
// sum, at least 0.001), and for the programmes that read VS b12 (cbClothCancelObjMatrix) the cancel rows blended alike and
// applied first: p = (Σ ŵ·b5[i]) · (Σ ŵ·b12[i]) · v. The palette base (cbInstanceData matricesData.x) is the native's 0. An
// index past a palette's rows reads a zero row, as a bounded CBV read past its view (the native binds each palette at its own
// size, C4-1b). T2c: a cloth mesh's simulated copy (model space, in its slot 0) by the instance's world row, as the _cloth
// programmes take VC_World (cbvModelParam) when VC_isEnableCloth is set. I-0117 T6: then the wind, as the Grass programmes move a
// vertex after placing it (Wind, below). Compiled by dxc at build time (Build-GameBootstrap.ps1).
#define SkinRoot "RootConstants(num32BitConstants=5, b0), SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), UAV(u0)"
// `source`: which of the instance's vertex sources t0 is this dispatch (0 the storage, k a physics stream): its slices only.
struct Rows {uint current;uint auxiliary;uint cancel;uint slices;uint source;};
ConstantBuffer<Rows> rows:register(b0);
ByteAddressBuffer source:register(t0);      // the slices' vertices: the model storage's, or a physics stream's
ByteAddressBuffer slices:register(t1);      // RtSkinRecord, 64 bytes each, then the instance's world row (3x4), then T6 its wind block
ByteAddressBuffer current:register(t2);     // the pose's current rows (3x4, 48 bytes each)
ByteAddressBuffer auxiliary:register(t3);   // its auxiliary rows (the cloth route), or the current rows when not bound
ByteAddressBuffer cancel:register(t4);      // its cancel rows, or the current rows when not bound
RWByteAddressBuffer positions:register(u0); // float3 a vertex, the instance's slices one after the other

struct Matrix34 {float4 r0,r1,r2;};
Matrix34 Row(uint palette,uint index){
    Matrix34 m;m.r0=m.r1=m.r2=float4(0,0,0,0);
    const uint count=palette==0?rows.current:palette==1?rows.auxiliary:rows.cancel;
    if(index>=count)return m;
    const uint at=48*index;
    if(palette==0){m.r0=asfloat(current.Load4(at));m.r1=asfloat(current.Load4(at+16));m.r2=asfloat(current.Load4(at+32));}
    else if(palette==1){m.r0=asfloat(auxiliary.Load4(at));m.r1=asfloat(auxiliary.Load4(at+16));m.r2=asfloat(auxiliary.Load4(at+32));}
    else{m.r0=asfloat(cancel.Load4(at));m.r1=asfloat(cancel.Load4(at+16));m.r2=asfloat(cancel.Load4(at+32));}
    return m;
}
Matrix34 Blend(uint palette,uint4 index,float4 weight){
    const Matrix34 a=Row(palette,index.x),b=Row(palette,index.y),c=Row(palette,index.z),d=Row(palette,index.w);
    Matrix34 m;
    m.r0=a.r0*weight.x+b.r0*weight.y+c.r0*weight.z+d.r0*weight.w;
    m.r1=a.r1*weight.x+b.r1*weight.y+c.r1*weight.z+d.r1*weight.w;
    m.r2=a.r2*weight.x+b.r2*weight.y+c.r2*weight.z+d.r2*weight.w;
    return m;
}
float3 Apply(Matrix34 m,float3 p){const float4 v=float4(p,1);return float3(dot(m.r0,v),dot(m.r1,v),dot(m.r2,v));}
uint4 Indices(uint at,uint format){
    if(format==0){const uint w=source.Load(at);return uint4(w&0xFF,(w>>8)&0xFF,(w>>16)&0xFF,w>>24);}
    const uint2 w=source.Load2(at);return uint4(w.x&0xFFFF,w.x>>16,w.y&0xFFFF,w.y>>16);
}
float4 Weights(uint at,uint format){
    if(format==0){const uint w=source.Load(at);return float4(w&0xFF,(w>>8)&0xFF,(w>>16)&0xFF,w>>24)/255.0;}
    if(format==3)return asfloat(source.Load4(at));
    const uint2 w=source.Load2(at);
    if(format==1)return float4(w.x&0xFFFF,w.x>>16,w.y&0xFFFF,w.y>>16)/65535.0;
    const int4 s=int4(int(w.x<<16)>>16,int(w.x)>>16,int(w.y<<16)>>16,int(w.y)>>16);
    return max(float4(s)/32767.0,-1.0);
}

// I-0117 T6: the wind, as the Grass G-buffer vertex programmes move a vertex after placing it (GXFlver_Grass*_GBuf and _GBuf_Skin,
// Eclipse_grass_Gbuf: one body, read 2026-11-03 with fxc, its literals by their bits): from the world position w and TEXCOORD1 t
// (x a phase, y the bend's weight, z a time scale), cbvModelParam's VC_ForceParam (rows 8 to 72: the loops' counts, then by wind its
// force and time, a point wind's place and time, force, the directional winds' place and range, the gusts' noise, the point winds'
// noise, direction and attraction) and VC_ModelSizeParam (row 75); the block after the world row (RtWindBlock: its row k at
// 16 (k - 8), the size row at 0x410).
float4 WindRow(uint row){return asfloat(slices.Load4(64*rows.slices+48+16*(row-8)));}
float SignedFrac(float x){const float f=frac(abs(x));return x>=-x?f:-f;}
// A gust's strength: its noise row's range (.x to .y) by a smoothed wave of the wind's time (.z its rate) and the distance from the
// world's axis (.w its rate).
float Gust(float4 noise,float time,float distance){
    const float a=SignedFrac((time*noise.z+distance*noise.w)*asfloat(0x39E7C943u))*asfloat(0x40C90FDBu)+asfloat(0xC0490FDBu);
    const float s=sin(a)*0.5+0.5;
    return s*s*s*(s*(s*6-15)+10)*(noise.y-noise.x)+noise.x;
}
float3 Wind(float3 w,float3 t){
    const float two_pi=asfloat(0x40C90FDBu),inverse_two_pi=asfloat(0x3E22F983u);
    const float4 size=asfloat(slices.Load4(64*rows.slices+48+0x410));
    const float scale=1/sqrt(size.y);
    const int2 loops=min(asint(slices.Load2(64*rows.slices+48)),int2(8,8));
    const float distance=sqrt(dot(w.xz,w.xz));
    float2 bend=float2(0,0);
    // The directional winds: a force along its direction, waving with the vertex's phase, faded by its range from its place.
    for(int i=0;i<loops.x;++i){
        const float4 force=WindRow(9+i),place=WindRow(33+i),noise=WindRow(41+i);
        const float3 f=Gust(noise,force.w,distance)*force.xyz;
        const float3 d=w-place.xyz;const float d2=dot(d,d);const float reach=d2>0?sqrt(d2):0;
        const float fade=place.w>0?max(1-reach/max(asfloat(0x38D1B717u),place.w),0):1;
        const float f2=dot(f,f);const float3 along=f2>0?f*rsqrt(f2):float3(0,0,0);
        const float phase=t.z*force.w*10+(t.x*two_pi-dot(along,w));
        const float wave=sin(SignedFrac(phase*inverse_two_pi)*two_pi);
        const float2 o=wave*f.xz*fade+fade*f.xz*t.y*10;
        if(force.w>0)bend+=o*scale;
    }
    // The point winds: a push from its place (or along its direction), faded by its force's .y as a range, waving with the distance.
    for(int j=0;j<loops.y;++j){
        const float4 place=WindRow(17+j),force=WindRow(25+j),noise=WindRow(49+j),aim=WindRow(57+j),attraction=WindRow(65+j);
        const float3 g=Gust(noise,place.w,distance)*force.xyz;
        const float3 d=w-place.xyz;const float d2=dot(d,d);const bool apart=d2>0;
        const float2 flat=apart?d.xz*rsqrt(dot(d.xz,d.xz)):float2(0,0);
        const float3 radial=apart?float3(sqrt(d2),d.xz*rsqrt(d2)):float3(0,d.xz);
        const bool aimed=aim.w>0;
        const float2 aim_xz=dot(aim.xyz,aim.xyz)>0?aim.xz*rsqrt(dot(aim.xz,aim.xz)):float2(0,0);
        const float cone=saturate(dot(aim_xz,flat)*asfloat(0x3F4CCCCDu)+asfloat(0x3E4CCCCDu));
        const float2 push=aimed?aim_xz:flat;
        const float3 lean=aimed?float3(aim_xz,cone):float3(radial.yz,1);
        const float fade=lean.z*(1-min(radial.x/max(g.y,asfloat(0x3A83126Fu)),1));
        const float wave=sin(SignedFrac((10*place.w-2*radial.x)*inverse_two_pi)*two_pi);
        const float2 lean_bend=lean.xy*10*t.y;
        float2 o=push*(g.x*fade);
        o=o*(g.z*(1-wave)+wave)+g.x*lean_bend*fade;
        o=lean_bend*attraction.x*fade+o;
        if(fade>0 && apart && place.w>0)bend+=o*scale;
    }
    // The bend leans the vertex on a sphere: x and z each a / sqrt(a² + 1), y lowered to keep the length, by the weight and size.
    const float2 v=bend/sqrt(bend*bend+1);
    const float y=sqrt(max(1-dot(v,v),0))-1;
    return w+float3(v.x,y,v.y)*(t.y*size.y);
}
// TEXCOORD1 by its format (RtWind*): 1 float3, 2 half4, 3 R8G8B8A8_UNORM, 4 B8G8R8A8_UNORM.
float3 TexCoord1(uint at,uint format){
    if(format==1)return asfloat(source.Load3(at));
    if(format>=3){const uint b=source.Load(at);const float3 v=float3(b&0xFF,(b>>8)&0xFF,(b>>16)&0xFF)/255.0;return format==4?v.zyx:v;}
    const uint2 h=source.Load2(at);return float3(f16tof32(h.x),f16tof32(h.x>>16),f16tof32(h.y));
}
// The placed position, moved by the wind when the slice has one (r3.z its TEXCOORD1's format, r3.w its byte in the vertex); a moved
// position not finite (the programme's own, where the rasterizer drops the triangle) keeps the placed one, which a refit can keep.
void Store(uint vertex,uint at,uint4 r0,uint4 r3,float3 w){
    if(r3.z){const float3 moved=Wind(w,TexCoord1(at+r3.w,r3.z));if(all(isfinite(moved)))w=moved;}
    positions.Store3(12*(r0.w+vertex),asuint(w));
}

[RootSignature(SkinRoot)]
[numthreads(64,1,1)]
void main(uint3 group:SV_GroupID,uint thread:SV_GroupIndex){
    if(group.y>=rows.slices)return;
    // RtSkinRecord: source vertex offset, stride, vertex count, first output vertex, mode (1 rigid, 2 blended), index offset and
    // format, weight offset and format, palette (0 current, 1 auxiliary), cancel (1: the cancel rows), source; T5: the blend elements'
    // stream (offset, stride); T6: the wind (TEXCOORD1's format, its byte in the vertex).
    const uint4 r0=slices.Load4(64*group.y),r1=slices.Load4(64*group.y+16),r2=slices.Load4(64*group.y+32),r3=slices.Load4(64*group.y+48);
    const uint vertex=64*group.x+thread;
    if(vertex>=r0.z || r2.w!=rows.source)return;
    const uint at=r0.x+vertex*r0.y;
    const float3 p=asfloat(source.Load3(at));
    const uint mode=r1.x,palette=r2.y,use_cancel=r2.z;
    if(mode==3){const uint at_world=64*rows.slices;Matrix34 world;world.r0=asfloat(slices.Load4(at_world));world.r1=asfloat(slices.Load4(at_world+16));
        world.r2=asfloat(slices.Load4(at_world+32));Store(vertex,at,r0,r3,Apply(world,p));return;}
    float3 q=p;
    // Rigid: the one row NORMAL.w names, as the programme reads it (no other row read: a row not finite elsewhere changes nothing).
    if(mode==1){const uint bone=Indices(at+r1.y,r1.z).w;if(use_cancel)q=Apply(Row(2,bone),q);
        Store(vertex,at,r0,r3,Apply(Row(palette,bone),q));return;}
    // T5: the blend elements in the second stream (r3: its first vertex's byte, its stride; the storage's, bound as t0 with it).
    const uint blend_at=r3.y?r3.x+vertex*r3.y:at;
    const uint4 index=Indices(blend_at+r1.y,r1.z);float4 weight=Weights(blend_at+r1.w,r2.x);weight/=max(dot(weight,float4(1,1,1,1)),0.001);
    if(use_cancel)q=Apply(Blend(2,index,weight),q);
    Store(vertex,at,r0,r3,Apply(Blend(palette,index,weight),q));
}
