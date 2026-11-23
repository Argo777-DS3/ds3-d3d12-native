// I-0123 RR-a (RR-PLAN §1): DLSS Ray Reconstruction's guides from the G-buffer, on the frames RR evaluates. One full-screen triangle
// into three targets at the render extent: the diffuse albedo (GB2 through its sRGB view, linear), the specular albedo (the RR guide's
// EnvBRDFApprox2 of F0 = GB3², α = r², N·V), the world normal (GB0 × 2 − 1, as the native's light pass reads it) with the roughness
// r = (2 / (2^(13 g) + 2))^¼ in w (g = GB1.x, the programmes' exponent 2^(13 g); R3-PLAN §2's Blinn-Phong to GGX equivalence α² =
// 2 / (n + 2), INFERRED). A pixel without a surface (the depth at 1) gets the guide's sky values: the diffuse albedo 0.5, the rest 0.
// Compiled by dxc at build time (Build-GameBootstrap.ps1): vs_6_6 `vs`, ps_6_6 `guide`.
#define GuidesRoot "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), CBV(b0)"
// RrGuidesConstants (rt_scene.h): the camera as RtViewConstants' first rows (the rows with the near and far planes, the lens scales,
// the jitter and the reciprocal extent), the extent, the slots of the depth (R32 of the D32S8) and GB0 to GB3 (rt_main_views_).
struct Guides {float4 right;float4 up;float4 forward;float4 position;float4 jitter;uint width;uint height;uint depth;uint gb0;uint gb1;uint gb2;uint gb3;uint pad;};
ConstantBuffer<Guides> frame:register(b0);
struct Out {float4 position:SV_Position;};
struct Targets {float4 albedo:SV_Target0;float4 specular:SV_Target1;float4 normal:SV_Target2;};
// The RR guide's fit of the environment BRDF (Ray Tracing Gems, chapter 32), as its integration guide gives it.
float3 EnvBRDFApprox2(float3 specular,float alpha,float nv){
    nv=abs(nv);
    const float4 x=float4(1,nv,nv*nv,nv*nv*nv);const float4 y=float4(1,alpha,alpha*alpha,alpha*alpha*alpha);
    const float2x2 m1=float2x2(0.99044,-1.28514,1.29678,-0.755907);
    const float3x3 m2=float3x3(1,2.92338,59.4188,20.3225,-27.0302,222.592,121.563,626.13,316.627);
    const float2x2 m3=float2x2(0.0365463,3.32707,9.0632,-9.04756);
    const float3x3 m4=float3x3(1,3.59685,-1.36772,9.04401,-16.3174,9.22949,5.56589,19.7886,-20.2123);
    float bias=dot(mul(m1,x.xy),y.xy)*rcp(dot(mul(m2,x.xyw),y.xyw));
    const float scale=dot(mul(m3,x.xy),y.xy)*rcp(dot(mul(m4,x.xzw),y.xyw));
    bias*=saturate(specular.g*50);   // the guide's: no bias for a specular reflectance of 0
    return mad(specular,max(0,scale),max(0,bias));
}
[RootSignature(GuidesRoot)]
Out vs(uint id:SV_VertexID){Out o;const float2 uv=float2((id<<1)&2,id&2);o.position=float4(uv*float2(2,-2)+float2(-1,1),0,1);return o;}
[RootSignature(GuidesRoot)]
Targets guide(Out i){
    const uint2 p=uint2(i.position.xy);
    Texture2D<float> depth=ResourceDescriptorHeap[frame.depth];
    Targets o;
    if(depth.Load(int3(p,0))>=1){o.albedo=float4(0.5,0.5,0.5,1);o.specular=0;o.normal=0;return o;}
    Texture2D<float4> gb0=ResourceDescriptorHeap[frame.gb0],gb1=ResourceDescriptorHeap[frame.gb1],gb2=ResourceDescriptorHeap[frame.gb2],gb3=ResourceDescriptorHeap[frame.gb3];
    // The eye's ray through the pixel's centre (the jitter undone), as the native's light pass and the traced sun find it.
    const float2 ndc=float2((p.x+0.5)*frame.jitter.z*2-1,1-(p.y+0.5)*frame.jitter.w*2)-frame.jitter.xy;
    const float3 v=float3(ndc.x/frame.forward.w,ndc.y/frame.position.w,1);
    const float3 eye=normalize(v.x*frame.right.xyz+v.y*frame.up.xyz+v.z*frame.forward.xyz);
    const float3 raw=gb0.Load(int3(p,0)).xyz*2-1;
    const float3 n=dot(raw,raw)>1e-8?normalize(raw):-eye;
    const float g=gb1.Load(int3(p,0)).x;
    const float alpha2=2/(exp2(13*g)+2);   // α² = 2 / (n + 2)
    const float roughness=sqrt(sqrt(alpha2));   // r = α^½
    const float3 f0=gb3.Load(int3(p,0)).rgb;
    o.albedo=float4(gb2.Load(int3(p,0)).rgb,1);
    o.specular=float4(EnvBRDFApprox2(f0*f0,roughness*roughness,dot(n,-eye)),1);
    o.normal=float4(n,roughness);
    return o;
}
