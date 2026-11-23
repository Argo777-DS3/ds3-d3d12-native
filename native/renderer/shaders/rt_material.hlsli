// I-0119 (R3a, R3-PLAN §3): a ray hit's material as the slice's G-buffer programme writes it, and T3's alpha test of a candidate
// (I-0117, R2-PLAN §3). The reference shape (GXFlver_ColDifSpcBumpDOL_Gbuf.ppo, R3-PLAN §1): albedo = g_DiffuseTexture.rgb ×
// FC_DifMapMulCol.rgb (GB2), F0 = g_SpecularTexture.rgb (GB3 holds its square root), gloss = g_BumpmapTexture.b × (g_SpecularPower /
// 256)² (GB1.x), the shading normal from the bump map's rg in the vertex frame (GB0); each texture at the slice's UV0 (scrolled by the
// instance's VC_TexScroll_0) tiled by its FC_TextureTilingScale row. The alpha test as DS3's alpha-tested depth programmes make it
// when they sample one texture (t0, or the material's diffuse at another register: the equipment's; the DptA programmes; I-0117 RESULT
// « T3 »): the texture's alpha against FC_AlphaRef.x, times the vertex
// colour COLOR1.w where the DptA vertex programme reads it; the screen-space dither has no ray analogue (a dither of one half).
// An instance with an alpha-tested slice is FORCE_NON_OPAQUE; its records start at its InstanceContributionToHitGroupIndex, one a
// GeometryIndex(). Included by the traced passes (and the unit test's rt_probe.hlsl); every resource is reached through the
// bindless heap but the table and the rows.
#ifndef RT_MATERIAL_HLSLI
#define RT_MATERIAL_HLSLI
// RtMaterialRecord (rt_scene.h), 192 bytes: the storage buffer's slot; the slice's index start (bytes) and width (2 or 4); the
// ray bits it answers (RtSliceKeep); UV0's first byte, stride and format (RtUv*); its flags (RT_RECORD_*); the positions' first
// byte and stride (a placed instance's: float3 in the model's space); NORMAL's and TANGENT's first byte and stride (R8G8B8A8_UINT);
// the programme family's index; the alpha reference; COLOR1's first byte, stride and format (0 none, 1 UNORM8, 2 float4, 3 BGRA8); R3c:
// a map blend's first layer record (RT_RECORD_LAYERS) or a Mul piece's second layer (RT_RECORD_MUL); the textures' slots by role (diffuse, specular, bump, emissive; ~0 none);
// their tiling rows; FC_DifMapMulCol with the gloss scale in its w; the emissive colour. A map blend's layer record (rt_scene.h's
// RtMaterialSlice::layers): its diffuse, specular and bump in the roles; the first also the mask (the emissive role), the vertex
// weights (its UV element, RtWeight* format), COLOR0 (its colour element) and P0, P1, P2 and the layers' count in its albedo.
// A Mul piece's second layer record: its textures in the roles (the mask the emissive), UV1, TANGENT1, COLOR1, the mask's UV in
// its normal fields and FC_BlendMaskParam's .x and .z in its albedo (rt_scene.h's RtMaterialSlice::mul). I-0125 R6a (208 bytes): its
// ambient (RT_AMBIENT_*) and its lightmap UV's first byte, stride and format (RtUv: shorts or floats, / 2048).
struct RtMaterialRecord {uint buffer;uint index_offset;uint index_width;uint keep;uint uv_offset;uint uv_stride;uint uv_format;uint flags;
    uint position_offset;uint position_stride;uint normal_offset;uint normal_stride;uint tangent_offset;uint tangent_stride;uint family;float ref;
    uint colour_offset;uint colour_stride;uint colour_format;uint extension;uint4 textures;float4 tiling[4];float4 albedo;float4 emissive;
    uint ambient;uint lightmap_offset;uint lightmap_stride;uint lightmap_format;uint forward;uint forward_colour_offset;uint forward_colour_stride;uint forward_colour_format;};   // R6d: a forward slice's blend and COLOR1 (224 bytes)
// RtInstanceRow (rt_scene.h), by InstanceIndex(): the scroll row, a skinned instance's positions slot (~0 a placed instance) and the
// byte of its slices' first vertices in the rows buffer, its first record and its records' count; R3c: the scroll rows VC_TexScroll_1
// and _3 (a Mul piece's second layer and mask), the skin's FC_SkinMatchColor; I-0117 T6: a skinned instance's vertex frame rows
// (a placed model moved by the wind; zero: none). I-0125 R6a: its cbInstanceData ambient rows (atlasUvScaleOffset, vDominantRotateParam,
// the SH rows aSh and ShLightMask), lit.x 1 when its SH rows were read, lit.y and lit.z its t26 and t27 lightmaps' slots (~0 none),
// cbTexHdrScaleParam's rgb (256 bytes); R6d: `forward` its forward block's byte in this buffer (RtForwardRowOf; ~0 none).
struct RtInstanceRow {float4 scroll;uint positions;uint bases;uint first;uint count;float4 scroll1;float4 scroll3;float4 skin;float3x4 frame;
    float4 atlas;float4 rotate;float4 sh[3];uint4 lit;float3 hdr;uint forward;};
#define RT_ROW_BYTES 256
#define RT_NONE 0xFFFFFFFFu
#define RT_RECORD_ALPHA 1u
#define RT_RECORD_ALPHA_TEXTURE 2u
#define RT_RECORD_NORMAL 4u
#define RT_RECORD_TANGENT 8u
#define RT_RECORD_MATERIAL 16u
#define RT_RECORD_LAYERS 32u   // I-0119 R3c: a map blend (its layer records from `extension`)
#define RT_RECORD_UNSCROLLED 64u   // R3c: UV0 without VC_TexScroll (a map blend)
#define RT_RECORD_NORMAL_HALF 128u   // R3c: NORMAL R16G16B16A16_FLOAT, read as is
#define RT_RECORD_TANGENT_HALF 256u
#define RT_RECORD_MUL 512u   // R3c: a two-layer piece (ColDifSpcBumpMul: its second layer the record at `extension`)
#define RT_RECORD_MUL_MASK 1024u   // R3c: MulMask (the second layer record's mask)
#define RT_RECORD_SKIN 2048u   // R3c: the skin's albedo curve (the row's FC_SkinMatchColor)
#define RT_RECORD_HAIR 4096u   // R3c: the hair's (the same colour, multiplied)
#define RT_RECORD_FACE 8192u   // R3c: the FaceGen face (a Mul piece: COLOR0.w the weight, COLOR0.rgb a factor; the skin's curve before the lerp)
#define RT_RECORD_FACE_BOTH 16384u   // R3c: the face image curved too
#define RT_RECORD_FORWARD 32768u   // I-0125 R6d: an unlit forward slice of the sky's programmes (RtForwardAt), its blend and kind in `forward`
#define RT_FACTOR_ZERO 0u          // R6d: a forward slice's blend factors (rt_scene.h's RtFactor*): the source's in bits 0-3,
#define RT_FACTOR_ONE 1u           // the destination's in 4-7
#define RT_FACTOR_SRC_ALPHA 2u
#define RT_FACTOR_INV_SRC_ALPHA 3u
#define RT_FORWARD_OPACITY 2u      // R6d step 2: the cloud sheets' Fujir_Nolight_OpacityBlend_Fwd
#define RT_FORWARD_BLEND_MASK 1u   // R6d step 2: the record's programme in `forward` bits 8-11 (rt_scene.h's RtForward*): 0 GXFlver_Col_Fwd
#define RT_AMBIENT_NONE 0u   // I-0125 R6a: RtMaterialRecord::ambient (rt_scene.h's RtAmbient*)
#define RT_AMBIENT_DOL 1u
#define RT_AMBIENT_SH 2u
#define RT_DIFFUSE 0
#define RT_SPECULAR 1
#define RT_BUMP 2
#define RT_EMISSIVE 3
RtInstanceRow RtRowOf(ByteAddressBuffer rows,uint instance){
    const uint at=RT_ROW_BYTES*instance;const uint4 a=rows.Load4(at+16);RtInstanceRow r;r.scroll=asfloat(rows.Load4(at));
    r.positions=a.x;r.bases=a.y;r.first=a.z;r.count=a.w;r.scroll1=asfloat(rows.Load4(at+32));r.scroll3=asfloat(rows.Load4(at+48));
    r.skin=asfloat(rows.Load4(at+64));r.frame=float3x4(asfloat(rows.Load4(at+80)),asfloat(rows.Load4(at+96)),asfloat(rows.Load4(at+112)));
    r.atlas=asfloat(rows.Load4(at+128));r.rotate=asfloat(rows.Load4(at+144));r.sh[0]=asfloat(rows.Load4(at+160));r.sh[1]=asfloat(rows.Load4(at+176));
    r.sh[2]=asfloat(rows.Load4(at+192));r.lit=rows.Load4(at+224);const uint4 h=rows.Load4(at+240);r.hdr=asfloat(h.xyz);r.forward=h.w;return r;}   // +208 ShLightMask: in the row, read by no hit
// T5: whether the slice answers a ray of `mask` (its keep, RtSliceKeep), for a slice of a non-opaque instance (mixed or alpha-tested):
// a forward-only or overlay slice is passed over by the view's rays, and by the casters' unless it casts (I-0118 R5c: a transparent
// slice).
bool RtAnswers(RtMaterialRecord record,uint mask){return (record.keep&mask)!=0;}
float RtColourAlpha(ByteAddressBuffer vertices,uint at,uint format){
    if(format==2)return asfloat(vertices.Load(at+12));
    return float(vertices.Load(at)>>24)/255.0;
}
// R3c: a colour element's rgb (1 R8G8B8A8_UNORM, 2 float4, 3 B8G8R8A8_UNORM).
float3 RtColourRgb(ByteAddressBuffer vertices,uint at,uint format){
    if(format==2)return asfloat(vertices.Load3(at));
    const uint w=vertices.Load(at);const float3 c=float3(w&0xFF,(w>>8)&0xFF,(w>>16)&0xFF)/255.0;return format==3?c.bgr:c;
}
// RtUv: 0 R16G16_SINT (DS3's UV16: the programme's itof() / 2048), 1 R32G32_FLOAT, 2 R16G16_FLOAT, 3 and 4 the two floats / 2048
// (a map blend's vertex programme scales TEXCOORD0 whatever its format).
float2 RtUv(ByteAddressBuffer vertices,uint at,uint format){
    if(format==1)return asfloat(vertices.Load2(at));
    if(format==3)return asfloat(vertices.Load2(at))*(1.0/2048.0);
    const uint w=vertices.Load(at);
    if(format==2)return float2(f16tof32(w&0xFFFF),f16tof32(w>>16));
    if(format==4)return float2(f16tof32(w&0xFFFF),f16tof32(w>>16))*(1.0/2048.0);   // 4: R16G16_FLOAT / 2048 (a map blend's)
    return float2(int(w<<16)>>16,int(w)>>16)*(1.0/2048.0);
}
uint RtIndex(ByteAddressBuffer indices,uint at,uint width){
    if(width==4)return indices.Load(at);
    const uint w=indices.Load(at&~3u);return (at&2u)?w>>16:w&0xFFFF;
}
// The triangle `primitive`'s three vertex indices in `record`'s slice.
uint3 RtTriangle(ByteAddressBuffer buffer,RtMaterialRecord record,uint primitive){
    const uint first=record.index_offset+3*primitive*record.index_width;
    return uint3(RtIndex(buffer,first,record.index_width),RtIndex(buffer,first+record.index_width,record.index_width),
        RtIndex(buffer,first+2*record.index_width,record.index_width));
}
float3 RtWeights(float2 bary){return float3(1-bary.x-bary.y,bary.x,bary.y);}
// The three vertices' UV0 as the vertex programme makes them (VC_TexScroll_0: uv × scroll.xy + scroll.zw), untiled.
void RtUvs(ByteAddressBuffer buffer,RtMaterialRecord record,uint3 i,float4 scroll,out float2 a,out float2 b,out float2 c){
    a=RtUv(buffer,record.uv_offset+i.x*record.uv_stride,record.uv_format)*scroll.xy+scroll.zw;
    b=RtUv(buffer,record.uv_offset+i.y*record.uv_stride,record.uv_format)*scroll.xy+scroll.zw;
    c=RtUv(buffer,record.uv_offset+i.z*record.uv_stride,record.uv_format)*scroll.xy+scroll.zw;
}
float2 RtTiled(float2 uv,float4 tiling){return uv*tiling.xy+tiling.zw;}   // the pixel programme's FC_TextureTilingScale row
// Whether the candidate triangle `primitive` of `record`, hit at barycentrics `bary`, is kept (its alpha at least the reference); a
// record without the test is opaque. No derivatives in a ray query: the top mip (T3).
bool RtKeeps(RtMaterialRecord record,float4 scroll,uint primitive,float2 bary,SamplerState wrap){
    if(!(record.flags&RT_RECORD_ALPHA))return true;
    // The slots differ from lane to lane (each ray its own hit; T3 step review M2).
    ByteAddressBuffer buffer=ResourceDescriptorHeap[NonUniformResourceIndex(record.buffer)];
    const uint3 i=RtTriangle(buffer,record,primitive);const float3 w=RtWeights(bary);
    float alpha=1;   // a colour-only record (the Emi family): its alpha the colour's, no UV read
    if(record.flags&RT_RECORD_ALPHA_TEXTURE){float2 a,b,c;RtUvs(buffer,record,i,scroll,a,b,c);
        Texture2D<float4> diffuse=ResourceDescriptorHeap[NonUniformResourceIndex(record.textures[RT_DIFFUSE])];
        alpha=diffuse.SampleLevel(wrap,RtTiled(a*w.x+b*w.y+c*w.z,record.tiling[RT_DIFFUSE]),0).w;}
    if(record.colour_format)
        alpha*=RtColourAlpha(buffer,record.colour_offset+i.x*record.colour_stride,record.colour_format)*w.x+
            RtColourAlpha(buffer,record.colour_offset+i.y*record.colour_stride,record.colour_format)*w.y+
            RtColourAlpha(buffer,record.colour_offset+i.z*record.colour_stride,record.colour_format)*w.z;
    return !(alpha<record.ref);
}
// The material at a hit. `world` the instance's ObjectToWorld3x4; `direction` the ray's (unit); `width` the ray cone's width at the
// hit (the texture's level from it and the triangle's texel density; 0: the top mip); `anisotropy`, `bias` the raster's material
// samplers' (R3c: the level they take).
struct RtSurface {
    bool material;      // the record's programme set was read (else a grey albedo and the geometric normal)
    float3 position;    // the hit in the world (the triangle's)
    float3 geometric;   // the triangle's normal, toward the ray's origin
    float3 normal;      // the shading normal (GB0's), toward the side the ray sees
    float3 albedo,f0,emissive;float gloss,roughness;
    bool skinned;       // its vertex frame not read (a skinned instance's positions alone): the shading normal the geometric one
    bool back;          // I-0129 R9a: the ray met the triangle's back (the vertex normal's other side): a probe's ray from inside a wall
    float layer;        // R3c: a two-layer piece's second-layer weight (0 otherwise; the control view's split)
    // I-0125 R6a: the ambient its G-buffer programme writes (GB4), its frame-independent part (RtAmbientOf finishes it): `ambient` the
    // record's form, RT_AMBIENT_MISSING when its inputs are missing (a lightmap without a view, an Ibl instance without its SH rows); a
    // lightmapped surface's t26² × cbTexHdrScaleParam in `light`, sat(D·N) of the rotated dominant direction and t27.a; an Ibl surface's
    // (1 − F0) max(0, SH(N)) in `light`.
    uint ambient;float3 light;float dominant_nl,dominant_a;
};
#define RT_AMBIENT_MISSING 3u
float3 RtSkinCurve(float3 t,float3 colour){t=saturate(sqrt(t)+colour);return t*t;}   // R3c: the skin's albedo curve (GBUF-FAMILIES §17)
// The gloss as a GGX roughness: the light programmes' Blinn-Phong exponent 2^(13 g) (GXLightAcc_DPointLight1.ppo), α² = 2 / (n + 2),
// r = α^½ (INFERRED: the usual equivalence, not DS3's own; RESULT « The gloss »).
float RtRoughnessOf(float gloss){return pow(2.0/(exp2(13*saturate(gloss))+2),0.25);}
float4 RtUint8(uint w){return (float4(w&0xFF,(w>>8)&0xFF,(w>>16)&0xFF,w>>24)-127)/127;}   // the vertex programmes' (v - 127) / 127
// A NORMAL or TANGENT element: R16G16B16A16_FLOAT as is (`half`), else R8G8B8A8_UINT decoded.
float4 RtFrame(ByteAddressBuffer vertices,uint at,bool packed){
    if(packed){const uint2 w=vertices.Load2(at);return float4(f16tof32(w.x&0xFFFF),f16tof32(w.x>>16),f16tof32(w.y&0xFFFF),f16tof32(w.y>>16));}
    return RtUint8(vertices.Load(at));}
// A vertex's four values (RtWeight*: 1 UNORM8, 2 float4, 3 half4, 4 UNORM16).
float4 RtVertex4(ByteAddressBuffer vertices,uint at,uint format){
    if(format==2)return asfloat(vertices.Load4(at));
    if(format==3){const uint2 w=vertices.Load2(at);return float4(f16tof32(w.x&0xFFFF),f16tof32(w.x>>16),f16tof32(w.y&0xFFFF),f16tof32(w.y>>16));}
    if(format==4){const uint2 w=vertices.Load2(at);return float4(w.x&0xFFFF,w.x>>16,w.y&0xFFFF,w.y>>16)/65535.0;}
    const uint w=vertices.Load(at);return float4(w&0xFF,(w>>8)&0xFF,(w>>16)&0xFF,w>>24)/255.0;
}
// A bump texel's normal in the frame (t, B = cross(n, t) × sign, `shading`) as the G-buffer programmes build it, unnormalised.
float3 RtBumped(float4 bump,float3 t,float3 n,float sign,float3 shading){
    const float2 xy=bump.xy*2-1;const float z=sqrt(1-min(dot(xy,xy),1));const float3 bn=cross(n,t)*sign;
    return xy.x*(dot(bn,bn)>0?normalize(bn):0)+xy.y*normalize(t)+z*shading;}
float4 RtSampleCone(Texture2D<float4> texture,SamplerState wrap,float2 uv,float density,float footprint){
    uint w,h;texture.GetDimensions(w,h);
    const float level=footprint>0 && density>0?max(0.5*log2(density*float(w)*float(h))+log2(footprint),0):0;
    return texture.SampleLevel(wrap,uv,level);
}
RtSurface RtMaterialAt(StructuredBuffer<RtMaterialRecord> table,RtMaterialRecord record,RtInstanceRow row,ByteAddressBuffer rows,uint geometry,
    float3x4 world,uint primitive,float2 bary,float3 direction,float width,uint anisotropy,float bias,SamplerState wrap){
    RtSurface s=(RtSurface)0;s.albedo=0.5;s.roughness=1;s.normal=s.geometric=-direction;
    if(record.buffer==RT_NONE)return s;
    ByteAddressBuffer buffer=ResourceDescriptorHeap[NonUniformResourceIndex(record.buffer)];
    const uint3 i=RtTriangle(buffer,record,primitive);const float3 w=RtWeights(bary);
    float3 p0,p1,p2;s.skinned=row.positions!=RT_NONE;
    if(s.skinned){ByteAddressBuffer positions=ResourceDescriptorHeap[NonUniformResourceIndex(row.positions)];const uint base=rows.Load(row.bases+4*geometry);
        p0=asfloat(positions.Load3(12*(base+i.x)));p1=asfloat(positions.Load3(12*(base+i.y)));p2=asfloat(positions.Load3(12*(base+i.z)));}
    else{p0=mul(world,float4(asfloat(buffer.Load3(record.position_offset+i.x*record.position_stride)),1));
        p1=mul(world,float4(asfloat(buffer.Load3(record.position_offset+i.y*record.position_stride)),1));
        p2=mul(world,float4(asfloat(buffer.Load3(record.position_offset+i.z*record.position_stride)),1));}
    s.position=p0*w.x+p1*w.y+p2*w.z;
    const float3 edge=cross(p1-p0,p2-p0);const float area=0.5*length(edge);float3 g=area>0?edge/(2*area):-direction;
    // The vertex frame (a placed instance's): NORMAL and TANGENT rotated by the placement as the vertex programme rotates them by
    // its palette row, then interpolated; B = cross(N, T) × T.w, as the pixel programme makes it. I-0117 T6: a skinned instance with
    // frame rows (a placed model the wind moves: the Grass programmes rotate the frame by the placement alone) by those rows.
    const bool framed=s.skinned && (any(row.frame[0]!=0) || any(row.frame[1]!=0) || any(row.frame[2]!=0));
    const float3x3 rotation=framed?(float3x3)row.frame:(float3x3)world;
    const bool frame=!s.skinned || framed;
    float3 n=g,t=0;float sign=1;
    if(frame && (record.flags&RT_RECORD_NORMAL)){
        const bool packed=(record.flags&RT_RECORD_NORMAL_HALF)!=0;
        n=RtFrame(buffer,record.normal_offset+i.x*record.normal_stride,packed).xyz*w.x+RtFrame(buffer,record.normal_offset+i.y*record.normal_stride,packed).xyz*w.y+
            RtFrame(buffer,record.normal_offset+i.z*record.normal_stride,packed).xyz*w.z;
        n=mul(rotation,n);n=dot(n,n)>0?normalize(n):g;}
    if(frame && (record.flags&RT_RECORD_TANGENT)){
        const bool packed=(record.flags&RT_RECORD_TANGENT_HALF)!=0;
        const float4 a=RtFrame(buffer,record.tangent_offset+i.x*record.tangent_stride,packed),b=RtFrame(buffer,record.tangent_offset+i.y*record.tangent_stride,packed),
            c=RtFrame(buffer,record.tangent_offset+i.z*record.tangent_stride,packed);
        t=mul(rotation,a.xyz*w.x+b.xyz*w.y+c.xyz*w.z);sign=a.w*w.x+b.w*w.y+c.w*w.z;}
    // The facing: the triangle's normal on the vertex normal's side, then a back face (the ray meets its other side) flips the
    // shading normal as the programme flips N on !SV_IsFrontFace (T and B unflipped), whatever the winding or a mirrored placement.
    if(dot(g,n)<0)g=-g;
    const bool back=dot(g,direction)>0;
    s.geometric=back?-g:g;s.back=back;
    const float3 shading=back?-n:n;s.normal=shading;
    s.material=(record.flags&RT_RECORD_MATERIAL)!=0;
    if(!s.material)return s;
    // UV0 and the texel density (the triangle's UV area against its world area; a texture's level from the ray cone's footprint).
    float2 ua=0,ub=0,uc=0;if(record.uv_stride)RtUvs(buffer,record,i,(record.flags&RT_RECORD_UNSCROLLED)?float4(1,1,0,0):row.scroll,ua,ub,uc);
    const float2 uv=ua*w.x+ub*w.y+uc*w.z;const float2 du=ub-ua,dv=uc-ua;const float uv_area=0.5*abs(du.x*dv.y-du.y*dv.x);
    // R3c fourth tranche: the level the raster's sampler takes, an anisotropic one of N the cone's minor axis unless the major is more
    // than N times it (N 1: the major axis), then its mip bias (DLSS's, NativeMaterialBank): the major axis alone blurred the rays past
    // the raster's level at grazing angles (the alpha-tested families darker by up to 13 steps, their transparent texels mixed in: run
    // i119-r3c-bias-m30-on's signed sums).
    const float footprint=width*max(1.0,1.0/(max(float(anisotropy),1.0)*max(abs(dot(g,direction)),1.0/256)))*exp2(bias);
    const float density=area>0?uv_area/area:0;
    float4 texel[4];
    [unroll]for(uint k=0;k<4;++k){texel[k]=float4(1,1,1,1);
        if(record.textures[k]!=RT_NONE){Texture2D<float4> texture=ResourceDescriptorHeap[NonUniformResourceIndex(record.textures[k])];
            texel[k]=RtSampleCone(texture,wrap,RtTiled(uv,record.tiling[k]),density*abs(record.tiling[k].x*record.tiling[k].y),footprint);}}
    // R3c: a map blend (GBUF-FAMILIES §16): layer k's diffuse, specular and bump lerped over the layers before it by
    // w = saturate((W - (mask + 0.01) P0 (1 - W)) (1 + P1 COLOR0.w)), W the vertex weights (one a layer after the first), the mask at
    // UV0 x P2 (none read: 0);
    // the result in the reference roles (the blend's albedo scale and gloss scale 1: the collect's).
    uint present=(record.textures[RT_SPECULAR]!=RT_NONE?2u:0u)|(record.textures[RT_BUMP]!=RT_NONE?4u:0u);   // the roles any layer gives
    if(record.flags&RT_RECORD_LAYERS){
        const RtMaterialRecord e=table[record.extension];const uint layers=min(uint(e.albedo.w),4);
        float3 weights=0;
        if(e.uv_stride)weights=RtVertex4(buffer,e.uv_offset+i.x*e.uv_stride,e.uv_format).xyz*w.x+RtVertex4(buffer,e.uv_offset+i.y*e.uv_stride,e.uv_format).xyz*w.y+
            RtVertex4(buffer,e.uv_offset+i.z*e.uv_stride,e.uv_format).xyz*w.z;
        float colour=0;
        if(e.colour_format)colour=RtColourAlpha(buffer,e.colour_offset+i.x*e.colour_stride,e.colour_format)*w.x+RtColourAlpha(buffer,e.colour_offset+i.y*e.colour_stride,e.colour_format)*w.y+
            RtColourAlpha(buffer,e.colour_offset+i.z*e.colour_stride,e.colour_format)*w.z;
        float mask=0;
        if(e.textures[RT_EMISSIVE]!=RT_NONE){Texture2D<float4> texture=ResourceDescriptorHeap[NonUniformResourceIndex(e.textures[RT_EMISSIVE])];
            mask=RtSampleCone(texture,wrap,RtTiled(uv*e.albedo.z,e.tiling[RT_EMISSIVE]),density*abs(e.tiling[RT_EMISSIVE].x*e.tiling[RT_EMISSIVE].y)*e.albedo.z*e.albedo.z,footprint).x;}
        const float3 blend=saturate((weights-(mask+0.01)*e.albedo.x*(1-weights))*(1+e.albedo.y*colour));   // the blend review's M1: the + 0.01
        for(uint layer=1;layer<layers;++layer){RtMaterialRecord l=e;if(layer>1)l=table[record.extension+layer-1];
            [unroll]for(uint k=0;k<3;++k){if(l.textures[k]==RT_NONE)continue;
                Texture2D<float4> texture=ResourceDescriptorHeap[NonUniformResourceIndex(l.textures[k])];
                const float4 value=RtSampleCone(texture,wrap,RtTiled(uv,l.tiling[k]),density*abs(l.tiling[k].x*l.tiling[k].y),footprint);
                // A role no layer before gave (pending there): this layer's value, not a lerp from white (the final review's L1).
                texel[k]=(present&(1u<<k))||k==0?lerp(texel[k],value,layer==1?blend.x:layer==2?blend.y:blend.z):value;present|=1u<<k;}}}
    // R3c: a two-layer piece (ColDifSpcBumpMul and MulMask, GBUF-FAMILIES §3 and §4): its second layer (the record at `extension`)
    // at UV1 (TEXCOORD0.zw, VC_TexScroll_1) in TANGENT1's frame, lerped by w = COLOR1.w; MulMask: k = mask.r FC_BlendMaskParam.x + .z,
    // the mask at TEXCOORD1 (VC_TexScroll_3) tiled by its row, u = k > 0 ? saturate(c (1 + k)) : saturate(c + k (1 - c)) for c =
    // COLOR1.w, w = u² (3 - 2u). The albedo, F0 and gloss lerped; each layer's normal built in its own frame, the two lerped, then
    // normalised (the programme normalises neither before).
    float4 second_bump=0;float3 second_tangent=0;float second_sign=1;float second_weight=0;bool second=false,second_has_bump=false,faced=false;
    if(record.flags&RT_RECORD_MUL){const RtMaterialRecord e=table[record.extension];second=true;
        float2 va=0,vb=0,vc=0;if(e.uv_stride)RtUvs(buffer,e,i,row.scroll1,va,vb,vc);
        const float2 uv1=va*w.x+vb*w.y+vc*w.z;const float2 d1=vb-va,e1=vc-va;const float density1=area>0?0.5*abs(d1.x*e1.y-d1.y*e1.x)/area:0;
        float c=0;
        if(e.colour_format)c=RtColourAlpha(buffer,e.colour_offset+i.x*e.colour_stride,e.colour_format)*w.x+RtColourAlpha(buffer,e.colour_offset+i.y*e.colour_stride,e.colour_format)*w.y+
            RtColourAlpha(buffer,e.colour_offset+i.z*e.colour_stride,e.colour_format)*w.z;
        // A MulMask whose mask has no heap view: layer 0 alone (its fixes' step review, 2), never the plain Mul weight.
        second_weight=(record.flags&RT_RECORD_MUL_MASK) && e.textures[RT_EMISSIVE]==RT_NONE?0:c;
        if((record.flags&RT_RECORD_MUL_MASK) && e.textures[RT_EMISSIVE]!=RT_NONE && e.normal_stride){
            const float2 ma=RtUv(buffer,e.normal_offset+i.x*e.normal_stride,0)*row.scroll3.xy+row.scroll3.zw,mb=RtUv(buffer,e.normal_offset+i.y*e.normal_stride,0)*row.scroll3.xy+row.scroll3.zw,
                mc=RtUv(buffer,e.normal_offset+i.z*e.normal_stride,0)*row.scroll3.xy+row.scroll3.zw;
            const float2 dm=mb-ma,em=mc-ma;const float density_mask=area>0?0.5*abs(dm.x*em.y-dm.y*em.x)/area:0;
            Texture2D<float4> texture=ResourceDescriptorHeap[NonUniformResourceIndex(e.textures[RT_EMISSIVE])];
            const float m=RtSampleCone(texture,wrap,RtTiled(ma*w.x+mb*w.y+mc*w.z,e.tiling[RT_EMISSIVE]),density_mask*abs(e.tiling[RT_EMISSIVE].x*e.tiling[RT_EMISSIVE].y),footprint).x;
            const float k=m*e.albedo.x+e.albedo.y;const float u=saturate(k>0?c*(1+k):c+k*(1-c));second_weight=u*u*(3-2*u);}
        [unroll]for(uint k=0;k<3;++k){if(e.textures[k]==RT_NONE)continue;
            Texture2D<float4> texture=ResourceDescriptorHeap[NonUniformResourceIndex(e.textures[k])];
            const float4 value=RtSampleCone(texture,wrap,RtTiled(uv1,e.tiling[k]),density1*abs(e.tiling[k].x*e.tiling[k].y),footprint);
            if(k==RT_BUMP){second_bump=value;second_has_bump=true;}
            // The face (FgFace*): the skin's curve on layer 0 (and on the face image, the _Bloody bodies) before the lerp, then the
            // lerped albedo times COLOR0.rgb (the second layer's colour element, its .w the weight).
            else if(k==RT_DIFFUSE && (record.flags&RT_RECORD_FACE)){
                float3 base=texel[k].rgb,face=value.rgb;
                if(record.flags&RT_RECORD_SKIN)base=RtSkinCurve(base,row.skin.rgb);
                if(record.flags&RT_RECORD_FACE_BOTH)face=RtSkinCurve(face,row.skin.rgb);
                float3 tint=1;
                if(e.colour_format)tint=RtColourRgb(buffer,e.colour_offset+i.x*e.colour_stride,e.colour_format)*w.x+RtColourRgb(buffer,e.colour_offset+i.y*e.colour_stride,e.colour_format)*w.y+
                    RtColourRgb(buffer,e.colour_offset+i.z*e.colour_stride,e.colour_format)*w.z;
                texel[k].rgb=lerp(base,face,second_weight)*tint;faced=true;}
            else texel[k]=lerp(texel[k],value,second_weight);}
        if(frame && e.tangent_stride){
            const float4 a=RtFrame(buffer,e.tangent_offset+i.x*e.tangent_stride,false),b=RtFrame(buffer,e.tangent_offset+i.y*e.tangent_stride,false),
                c3=RtFrame(buffer,e.tangent_offset+i.z*e.tangent_stride,false);
            second_tangent=mul(rotation,a.xyz*w.x+b.xyz*w.y+c3.xyz*w.z);second_sign=a.w*w.x+b.w*w.y+c3.w*w.z;}}
    // R3c: the skin (GXFlver_SkinIbl, SkinIbl_Bloody, SkinSSSIbl_Bloody; GBUF-FAMILIES §17): saturate(sqrt(t0) + FC_SkinMatchColor)²
    // before FC_DifMapMulCol (the _Bloody bodies' wet and blood blocks not applied: neutral where the control view met them).
    float3 diffuse=record.textures[RT_DIFFUSE]!=RT_NONE?texel[RT_DIFFUSE].rgb:1;
    // The face's above (a face image without a heap view: layer 0's curve here, the final review's L3).
    if((record.flags&RT_RECORD_SKIN) && !faced)diffuse=RtSkinCurve(diffuse,row.skin.rgb);
    // R3c: the hair (GXFlver_HairIbl and its variants): saturate(sqrt(t0) × FC_SkinMatchColor)² before FC_DifMapMulCol.
    if(record.flags&RT_RECORD_HAIR){diffuse=saturate(sqrt(diffuse)*(row.skin.w<0?1:row.skin.rgb));diffuse*=diffuse;}   // w < 0: the colour unread
    s.albedo=diffuse*record.albedo.rgb;
    s.f0=(present&2u)?texel[RT_SPECULAR].rgb:0;
    s.emissive=record.textures[RT_EMISSIVE]!=RT_NONE?texel[RT_EMISSIVE].rgb*record.emissive.rgb:0;
    if(present&4u){
        s.gloss=(second && second_has_bump?lerp(texel[RT_BUMP].b,second_bump.b,second_weight):texel[RT_BUMP].b)*record.albedo.w;
        if(dot(t,t)>0){float3 bump=RtBumped(texel[RT_BUMP],t,n,sign,shading);
            // A second bump without a view (the heap refused it): layer 0's normal unchanged (the final review's L2).
            if(second && second_has_bump)bump=lerp(bump,dot(second_tangent,second_tangent)>0?RtBumped(second_bump,second_tangent,n,second_sign,shading):shading,second_weight);
            s.normal=dot(bump,bump)>0?normalize(bump):shading;}}
    s.roughness=RtRoughnessOf(s.gloss);s.layer=second?second_weight:0;
    // I-0125 R6a (R6-PLAN §1; GXFlver_ColDifSpcBumpDOL_Gbuf.ppo, ColDifSpcBumpIbl_Gbuf.ppo): the ambient's inputs at the final normal.
    s.ambient=record.ambient;
    if(record.ambient==RT_AMBIENT_SH){
        if(row.lit.x==0)s.ambient=RT_AMBIENT_MISSING;
        const float4 n4=float4(s.normal,1);s.light=(1-s.f0)*max(0,float3(dot(row.sh[0],n4),dot(row.sh[1],n4),dot(row.sh[2],n4)));}
    else if(record.ambient==RT_AMBIENT_DOL){
        if(row.lit.y==RT_NONE || row.lit.z==RT_NONE || !record.lightmap_stride)s.ambient=RT_AMBIENT_MISSING;
        else{// The lightmap UV: its element / 2048 × atlasUvScaleOffset.xy + .zw (the .vpo: TEXCOORD0.zw, TEXCOORD1.xy in the
            // two-layer bodies and the lightmapped blends; no scroll), its own texel density.
            const float2 la=RtUv(buffer,record.lightmap_offset+i.x*record.lightmap_stride,record.lightmap_format)*row.atlas.xy+row.atlas.zw,
                lb=RtUv(buffer,record.lightmap_offset+i.y*record.lightmap_stride,record.lightmap_format)*row.atlas.xy+row.atlas.zw,
                lc=RtUv(buffer,record.lightmap_offset+i.z*record.lightmap_stride,record.lightmap_format)*row.atlas.xy+row.atlas.zw;
            const float2 lu=la*w.x+lb*w.y+lc*w.z;const float2 ld=lb-la,le=lc-la;const float lm_density=area>0?0.5*abs(ld.x*le.y-ld.y*le.x)/area:0;
            Texture2D<float4> t26=ResourceDescriptorHeap[NonUniformResourceIndex(row.lit.y)],t27=ResourceDescriptorHeap[NonUniformResourceIndex(row.lit.z)];
            const float4 l=RtSampleCone(t26,wrap,lu,lm_density,footprint),d=RtSampleCone(t27,wrap,lu,lm_density,footprint);
            s.light=l.rgb*l.rgb*row.hdr.rgb;s.dominant_a=d.w;
            // The dominant direction t27.rgb × 2 − 1 normalised, rotated by vDominantRotateParam q: the programme's instructions in order
            // (its registers' names kept).
            float4 r3=(d.yzzx-0.5)*2;r3*=rsqrt(max(dot(r3.xzw,r3.xzw),1e-12));const float4 q=row.rotate;
            float3 r4=r3.x*-q.xyz;float r2w=r3.w*q.x-r4.y;r4.x=r3.w*-q.y-r4.x;r4.x=r3.z*q.w+r4.x;r2w=-r3.z*-q.z+r2w;r4.y=r2w*q.x;
            r4.z=r3.w*q.w+r4.z;r3.y=-r3.y*-q.y+r4.z;r4.y=q.w*r3.y+r4.y;r4.y=q.y*r4.x+r4.y;const float x0=r4.x;r4.x=x0*q.x;r4.z=x0*q.w;
            r3.x=r3.w*q.z+r3.x*q.w;r3.x=r3.z*-q.x+r3.x;float3 r5;r5.x=-q.z*r3.x+r4.y;r3.z=q.w*r3.x-r4.x;r3.x=q.x*r3.x+r4.z;r3.x=-q.y*r3.y+r3.x;
            r5.z=q.z*r2w+r3.x;r2w=q.y*r2w+r3.z;r5.y=q.z*r3.y+r2w;
            s.dominant_nl=saturate(dot(r5,s.normal));}}
    return s;
}
// I-0125 R6a: the surface's GB4 as its programme writes it, with the frame's dominant rows (`dominant`: FC_DominantRgbScale ×
// FC_DominantColorScale in xyz, FC_DominantSharpnesOffset in w): a lightmapped surface's (1 − F0) L (s (1 − F0) sat(D·N) + 1 − s),
// s = sat(t27.a + offset); an Ibl surface's as read; 0 without a form or with its inputs missing.
float3 RtAmbientOf(RtSurface s,float4 dominant){
    if(s.ambient==RT_AMBIENT_SH)return s.light;
    if(s.ambient!=RT_AMBIENT_DOL)return 0;
    const float3 l=s.light*dominant.xyz;const float sharp=saturate(s.dominant_a+dominant.w);
    return (1-s.f0)*(l*sharp*(1-s.f0)*s.dominant_nl+l*(1-sharp));
}
// I-0125 R6d (R6-PLAN §7): an instance's forward block (rt_scene.h's RtForwardRow, at its row's `forward`, RT_NONE: none, the
// instance's mulCol or fog not read and its forward slices passed over): its mulCol and its model's forward fog entry (cbFog's first six
// rows: FogParam, FogCol, ExtFogParam, HeightFogParam, HeightFogCol, FogType).
void RtForwardRowOf(ByteAddressBuffer rows,uint at,out float4 colour,out float4 fog[6]){
    colour=asfloat(rows.Load4(at));
    [unroll]for(uint k=0;k<6;++k)fog[k]=asfloat(rows.Load4(at+16+16*k));}
// R6d (GXFlver_Col_Fwd.ppo, read 2026-11-06): an unlit forward slice's colour at a hit, before its fog: g_DiffuseTexture at UV0
// (VC_TexScroll_0, its row's tiling) × (1, 1, 1, COLOR1.w), its rgb × FC_DynamicDiffuseColor.rgb (the record's albedo), the whole ×
// mulCol (`tint`, the instance's: the vertex programme passes instance 0's); `position` the hit in the world (a skinned instance's from
// its positions, as RtMaterialAt's). `width` the ray cone's width at the hit (the texture's level from it and the triangle's texel
// density, as RtMaterialAt's without the samplers' anisotropy).
// R6d step 2 (MultiBlend2_et1_nolight_MuitiBlendMask_Fwd.ppo, read 2026-11-06), the scrolling zenith, T the frame's FC_GlobalTime, the
// textures k at role k tiled by row k (`record`'s), its layer record `e` (12_scene_collect.inc): m = sat((v − (t0(uv0 + T f2_1).x +
// t3(uv0 + T f2_2).x + 0.01) f0 (1 − v)) (1 + COLOR0.w f1)), v = TEXCOORD1.x; rgb = lerp(t1(uv0 + T f2_4).rgb, t2(uv1 + T f2_3).rgb, m)
// × colour_0 × FC_DifMapMulCol (the record's albedo.rgb); alpha 1 (the whole times mulCol below, as the programme's).
float4 RtForwardBlendMaskAt(RtMaterialRecord record,RtMaterialRecord e,RtInstanceRow row,ByteAddressBuffer buffer,uint3 i,float3 w,float area,float time,float width,
    SamplerState wrap){
    float2 a0=0,b0=0,c0=0,a1=0,b1=0,c1=0;RtUvs(buffer,record,i,row.scroll,a0,b0,c0);RtUvs(buffer,e,i,row.scroll1,a1,b1,c1);
    const float2 uv0=a0*w.x+b0*w.y+c0*w.z,uv1=a1*w.x+b1*w.y+c1*w.z;
    const float2 d0=b0-a0,e0=c0-a0,d1=b1-a1,e1=c1-a1;
    const float density0=area>0?0.5*abs(d0.x*e0.y-d0.y*e0.x)/area:0,density1=area>0?0.5*abs(d1.x*e1.y-d1.y*e1.x)/area:0;
    float4 t[4];
    [unroll]for(uint k=0;k<4;++k){Texture2D<float4> texture=ResourceDescriptorHeap[NonUniformResourceIndex(record.textures[k])];
        const float2 uv=k==0?uv0+time*e.albedo.xy:k==3?uv0+time*e.albedo.zw:k==2?uv1+time*e.emissive.xy:uv0+time*e.emissive.zw;
        t[k]=RtSampleCone(texture,wrap,RtTiled(uv,record.tiling[k]),(k==2?density1:density0)*abs(record.tiling[k].x*record.tiling[k].y),width);}
    const float v=RtVertex4(buffer,e.lightmap_offset+i.x*e.lightmap_stride,e.lightmap_format).x*w.x+RtVertex4(buffer,e.lightmap_offset+i.y*e.lightmap_stride,e.lightmap_format).x*w.y+
        RtVertex4(buffer,e.lightmap_offset+i.z*e.lightmap_stride,e.lightmap_format).x*w.z;
    const float colour=RtColourAlpha(buffer,record.forward_colour_offset+i.x*record.forward_colour_stride,record.forward_colour_format)*w.x+
        RtColourAlpha(buffer,record.forward_colour_offset+i.y*record.forward_colour_stride,record.forward_colour_format)*w.y+
        RtColourAlpha(buffer,record.forward_colour_offset+i.z*record.forward_colour_stride,record.forward_colour_format)*w.z;
    const float mask=(t[0].x+t[3].x+0.01)*record.albedo.w;
    const float m=saturate((v-mask*(1-v))*(colour*record.emissive.x+1));
    return float4(lerp(t[1].rgb,t[2].rgb,m)*record.albedo.rgb,1);
}
// R6d step 2 (Fujir_Nolight_OpacityBlend_Fwd.ppo, read 2026-11-06), the cloud sheets: rgb = t3(uv0 + T f2_2).rgb × colour_0 (the
// record's albedo), a = t1(uv2 + T f2_0).x × t2(uv1 + T f2_1).x × TEXCOORD3.x; the roles 0, 1, 2 t3, t2, t1 with their rows; UV0 the
// record's, UV1 its layer's UV and UV2 its layer's lightmap element, all / 2048 and unscrolled; TEXCOORD3 the record's lightmap element.
float4 RtForwardOpacityAt(RtMaterialRecord record,RtMaterialRecord e,ByteAddressBuffer buffer,uint3 i,float3 w,float area,float time,float width,SamplerState wrap){
    float2 a0=0,b0=0,c0=0,a1=0,b1=0,c1=0;RtUvs(buffer,record,i,float4(1,1,0,0),a0,b0,c0);RtUvs(buffer,e,i,float4(1,1,0,0),a1,b1,c1);
    const float2 a2=RtUv(buffer,e.lightmap_offset+i.x*e.lightmap_stride,e.lightmap_format),b2=RtUv(buffer,e.lightmap_offset+i.y*e.lightmap_stride,e.lightmap_format),
        c2=RtUv(buffer,e.lightmap_offset+i.z*e.lightmap_stride,e.lightmap_format);
    const float2 uv[3]={a0*w.x+b0*w.y+c0*w.z+time*e.emissive.xy,a1*w.x+b1*w.y+c1*w.z+time*e.albedo.zw,a2*w.x+b2*w.y+c2*w.z+time*e.albedo.xy};
    const float2 dv[3][2]={{b0-a0,c0-a0},{b1-a1,c1-a1},{b2-a2,c2-a2}};
    float4 t[3];
    [unroll]for(uint k=0;k<3;++k){Texture2D<float4> texture=ResourceDescriptorHeap[NonUniformResourceIndex(record.textures[k])];
        const float density=area>0?0.5*abs(dv[k][0].x*dv[k][1].y-dv[k][0].y*dv[k][1].x)/area:0;
        t[k]=RtSampleCone(texture,wrap,RtTiled(uv[k],record.tiling[k]),density*abs(record.tiling[k].x*record.tiling[k].y),width);}
    const float v=RtVertex4(buffer,record.lightmap_offset+i.x*record.lightmap_stride,record.lightmap_format).x*w.x+
        RtVertex4(buffer,record.lightmap_offset+i.y*record.lightmap_stride,record.lightmap_format).x*w.y+RtVertex4(buffer,record.lightmap_offset+i.z*record.lightmap_stride,record.lightmap_format).x*w.z;
    return float4(t[0].rgb*record.albedo.rgb,t[2].x*t[1].x*v);
}
float4 RtForwardAt(StructuredBuffer<RtMaterialRecord> table,RtMaterialRecord record,RtInstanceRow row,ByteAddressBuffer rows,uint geometry,float3x4 world,uint primitive,
    float2 bary,float4 tint,float time,float width,SamplerState wrap,out float3 position){
    ByteAddressBuffer buffer=ResourceDescriptorHeap[NonUniformResourceIndex(record.buffer)];
    const uint3 i=RtTriangle(buffer,record,primitive);const float3 w=RtWeights(bary);
    float3 p0,p1,p2;
    if(row.positions!=RT_NONE){ByteAddressBuffer positions=ResourceDescriptorHeap[NonUniformResourceIndex(row.positions)];const uint base=rows.Load(row.bases+4*geometry);
        p0=asfloat(positions.Load3(12*(base+i.x)));p1=asfloat(positions.Load3(12*(base+i.y)));p2=asfloat(positions.Load3(12*(base+i.z)));}
    else{p0=mul(world,float4(asfloat(buffer.Load3(record.position_offset+i.x*record.position_stride)),1));
        p1=mul(world,float4(asfloat(buffer.Load3(record.position_offset+i.y*record.position_stride)),1));
        p2=mul(world,float4(asfloat(buffer.Load3(record.position_offset+i.z*record.position_stride)),1));}
    position=p0*w.x+p1*w.y+p2*w.z;
    const float area=0.5*length(cross(p1-p0,p2-p0));
    if((record.forward>>8)==RT_FORWARD_BLEND_MASK)return RtForwardBlendMaskAt(record,table[record.extension],row,buffer,i,w,area,time,width,wrap)*tint;
    if((record.forward>>8)==RT_FORWARD_OPACITY)return RtForwardOpacityAt(record,table[record.extension],buffer,i,w,area,time,width,wrap)*tint;
    float2 ua=0,ub=0,uc=0;if(record.uv_stride)RtUvs(buffer,record,i,row.scroll,ua,ub,uc);
    const float2 du=ub-ua,dv=uc-ua;const float density=area>0?0.5*abs(du.x*dv.y-du.y*dv.x)/area:0;
    const float4 tiling=record.tiling[RT_DIFFUSE];
    Texture2D<float4> diffuse=ResourceDescriptorHeap[NonUniformResourceIndex(record.textures[RT_DIFFUSE])];
    float4 c=RtSampleCone(diffuse,wrap,RtTiled(ua*w.x+ub*w.y+uc*w.z,tiling),density*abs(tiling.x*tiling.y),width);
    c.a*=RtColourAlpha(buffer,record.forward_colour_offset+i.x*record.forward_colour_stride,record.forward_colour_format)*w.x+
        RtColourAlpha(buffer,record.forward_colour_offset+i.y*record.forward_colour_stride,record.forward_colour_format)*w.y+
        RtColourAlpha(buffer,record.forward_colour_offset+i.z*record.forward_colour_stride,record.forward_colour_format)*w.z;
    c.rgb*=record.albedo.rgb;
    return c*tint;
}
// R6d: GXFlver_Col_Fwd's fog of the colour `c` at `p` seen from `eye` (the programme's camera, FC_MatrixInvView's translation): the
// distance fog d = sat(E.x − E.x 2^(−(|eye.xz − p.xz| − E.y) E.z)) toward FogCol.rgb × (FogCol.w (ℓ − 1) + 1), then the height fog
// h = sat(sat((p.y − eye.y + H.w − H.y) H.z) H.x) toward HeightFogCol.rgb × (HeightFogCol.w (ℓ − 1) + 1), ℓ the unfogged colour's
// luminance; E ExtFogParam (fog[2]), H HeightFogParam (fog[3]), FogCol fog[1], HeightFogCol fog[4]. A zero entry leaves `c`. The
// height term the programmes' (p − C).y + H.w − H.y with H.w the camera's y (14E900 writes the view's +0x34 there: the fixes' step review
// M1), so `camera_y` the frame's camera whatever `eye` (the horizontal distance's origin: the reflecting point for a reflected leg).
float3 RtForwardFog(float3 c,float3 p,float3 eye,float camera_y,float4 fog[6]){
    const float l=dot(c,float3(0.29891,0.58661,0.11448))-1;
    const float h=saturate(saturate((p.y-camera_y+fog[3].w-fog[3].y)*fog[3].z)*fog[3].x);
    const float d=saturate(fog[2].x-fog[2].x*exp2(-(length(eye.xz-p.xz)-fog[2].y)*fog[2].z));
    const float3 near=c+d*(fog[1].rgb*(fog[1].w*l+1)-c);
    return near+h*(fog[4].rgb*(fog[4].w*l+1)-near);
}
// I-0122 R4b (final review L4): a shadow ray's query, the traced sun's and the local lights': whether `ray` meets a caster of `mask`
// (the instances' mask, then each non-opaque candidate's slice answering it and kept by its alpha test); no table (`materials`
// false), every instance opaque.
bool RtOccluded(RaytracingAccelerationStructure scene,StructuredBuffer<RtMaterialRecord> table,ByteAddressBuffer rows,bool materials,uint mask,RayDesc ray,
    SamplerState wrap){
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    query.TraceRayInline(scene,materials?RAY_FLAG_NONE:RAY_FLAG_FORCE_OPAQUE,mask,ray);
    while(query.Proceed()){
        if(query.CandidateType()!=CANDIDATE_NON_OPAQUE_TRIANGLE)continue;
        const RtMaterialRecord record=table[query.CandidateInstanceContributionToHitGroupIndex()+query.CandidateGeometryIndex()];
        if(RtAnswers(record,mask) && RtKeeps(record,RtRowOf(rows,query.CandidateInstanceIndex()).scroll,query.CandidatePrimitiveIndex(),
            query.CandidateTriangleBarycentrics(),wrap))
            query.CommitNonOpaqueTriangleHit();}
    return query.CommittedStatus()==COMMITTED_TRIANGLE_HIT;
}
// I-0123 RR-b (RR-PLAN §3): the penumbrae's samples, on the frames Ray Reconstruction denoises. White noise, independent across pixels
// and frames as RR's guide asks (§3.5: a good hash, no screen-space pattern): PCG's 3D hash (Jarzynski and Olano, 2020) of the pixel and
// the frame's seed, `salt` keeping each light's draw apart; two uniforms in [0, 1).
float2 RtRandom2(uint2 pixel,uint seed,uint salt){
    uint3 v=uint3(pixel,seed^(salt*0x9E3779B9u))*1664525u+1013904223u;
    v.x+=v.y*v.z;v.y+=v.z*v.x;v.z+=v.x*v.y;v^=v>>16u;v.x+=v.y*v.z;v.y+=v.z*v.x;v.z+=v.x*v.y;
    return float2(v.xy>>8)*(1.0/16777216.0);
}
// Two unit vectors completing `axis` (unit) to an orthonormal frame.
void RtBasis(float3 axis,out float3 t,out float3 b){t=normalize(abs(axis.z)<0.999?cross(axis,float3(0,0,1)):cross(axis,float3(1,0,0)));b=cross(axis,t);}
// A direction uniform in the cone of `axis` whose half-angle has the cosine `cos_max`.
float3 RtConeSample(float3 axis,float cos_max,float2 u){
    const float c=1-u.x*(1-cos_max),s=sqrt(max(0,1-c*c)),phi=6.2831853*u.y;float3 t,b;RtBasis(axis,t,b);
    return normalize(t*(s*cos(phi))+b*(s*sin(phi))+axis*c);
}
// A point uniform on the disc of radius `radius` around `centre`, facing `axis` (the sphere light as the surface sees it).
float3 RtDiscSample(float3 centre,float3 axis,float radius,float2 u){
    const float r=radius*sqrt(u.x),phi=6.2831853*u.y;float3 t,b;RtBasis(axis,t,b);
    return centre+t*(r*cos(phi))+b*(r*sin(phi));
}
#endif
