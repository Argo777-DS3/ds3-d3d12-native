// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The per-statement scan of the lit HDR's negatives (scene_pass_views.cpp, effect-scan-draws.flag): CS, cs_5_0.
Texture2D<float4> picture:register(t0);RWByteAddressBuffer counts:register(u0);
cbuffer Point:register(b0){uint point_index;uint width;uint height;};
[numthreads(8,8,1)] void CS(uint3 id:SV_DispatchThreadID){
    if(id.x>=width || id.y>=height)return;
    const float4 v=picture.Load(int3(id.xy,0));const uint4 bits=asuint(v);
    const bool4 nan=(bits&0x7FFFFFFF)>0x7F800000;const bool4 neg=v<0;
    const uint at=point_index*64;
    if(any(nan))counts.InterlockedAdd(at+44,1);
    if(!any(neg))return;
    if(neg.x)counts.InterlockedAdd(at+0,1);
    if(neg.y)counts.InterlockedAdd(at+4,1);
    if(neg.z)counts.InterlockedAdd(at+8,1);
    if(neg.w)counts.InterlockedAdd(at+12,1);
    counts.InterlockedAdd(at+16,1);
    if(neg.x)counts.InterlockedMax(at+20,asuint(-v.x));
    if(neg.y)counts.InterlockedMax(at+24,asuint(-v.y));
    if(neg.z)counts.InterlockedMax(at+28,asuint(-v.z));
    counts.InterlockedMax(at+32,width-id.x);counts.InterlockedMax(at+36,height-id.y);counts.InterlockedMax(at+40,id.x+1);counts.InterlockedMax(at+48,id.y+1);
}
