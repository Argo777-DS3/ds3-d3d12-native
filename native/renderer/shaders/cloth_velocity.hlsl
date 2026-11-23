// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The cloth Velo rows' vertex programme (scene_pass_velocity.cpp): VS, vs_5_0.
// I-0134: the cloth Velo rows' vertex programme. The game's 192 cloth Velo programmes (fxc /dumpbin of the I-0063 cache) take,
// with VC_isEnableCloth, VC_World (b7 rows 0-2) for the current position and VC_PrevWorld (rows 3-5) for the previous one, both
// from the same POSITION: the simulated copy's own motion is lost. Here the previous position is the previous copy's (slot 2,
// PREVPOSITION). The outputs are theirs, in their order: SV_Position, TEXCOORD0 the current clip position, TEXCOORD1 the previous
// one, TEXCOORD2 the normal (no Velo pixel programme reads it: zero), TEXCOORD3 the texture coordinate as they make it
// (itof, x VC_TexScroll_0.xy, x 1/2048, + .zw), which the alpha-tested _Velo_[A] programmes read.
cbuffer cbMatrixWVP : register(b0) { float4 VC_MatrixWVP[4]; float4 VC_PrevMatrixWVP[4]; };
cbuffer cbvModelParam : register(b7) { float4 VC_World[3]; float4 VC_PrevWorld[3]; };
cbuffer cbTexScroll : register(b11) { float4 VC_TexScroll_0; };
struct Out { float4 position : SV_Position; float4 current : TEXCOORD0; float4 previous : TEXCOORD1; float3 normal : TEXCOORD2; float2 uv : TEXCOORD3; };
Out VS(float3 position : POSITION0, int2 uv : TEXCOORD0, float3 previous : PREVPOSITION0) {
    const float4 p=float4(position,1),q=float4(previous,1);
    const float4 w=float4(dot(p,VC_World[0]),dot(p,VC_World[1]),dot(p,VC_World[2]),1);
    const float4 v=float4(dot(q,VC_PrevWorld[0]),dot(q,VC_PrevWorld[1]),dot(q,VC_PrevWorld[2]),1);
    Out o;
    o.position=float4(dot(w,VC_MatrixWVP[0]),dot(w,VC_MatrixWVP[1]),dot(w,VC_MatrixWVP[2]),dot(w,VC_MatrixWVP[3]));
    o.current=o.position;
    o.previous=float4(dot(v,VC_PrevMatrixWVP[0]),dot(v,VC_PrevMatrixWVP[1]),dot(v,VC_PrevMatrixWVP[2]),dot(v,VC_PrevMatrixWVP[3]));
    o.normal=0;
    o.uv=float2(uv)*VC_TexScroll_0.xy*(1.0/2048.0)+VC_TexScroll_0.zw;
    return o;
}
