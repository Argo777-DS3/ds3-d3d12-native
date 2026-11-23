// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The output-size depth (scene_pass_velocity.cpp): VS and PS, vs_5_0 / ps_5_0.
// I-0111 tranche 3 (UPSCALER-DLSS-READY §3.7): the depth at the presentation's extent for the post stages that read one (Alpha_R, the
// depth of field, the motion blur) in a scaled DLSS mode. Each output pixel takes the render depth's texel under its point in the
// jittered render image: the unjittered point (p + 0.5) render / output shifted by the jitter (render pixels, +x right, +y down), the
// texel holding it (point sampling: the nearest). Written as depth (SV_Depth, the test always passing) into a D32S8 target like the
// scene depth, which the stages read through its plane 0.
cbuffer Map : register(b0) { float4 map; };   // render / output (x, y), the jitter (x, y)
Texture2D<float> depth : register(t0);
float4 VS(uint id : SV_VertexID) : SV_Position { const float2 t=float2((id<<1)&2,id&2); return float4(t*float2(2,-2)+float2(-1,1),0,1); }
float PS(float4 position : SV_Position) : SV_Depth {
    uint w,h;depth.GetDimensions(w,h);
    const int2 texel=clamp(int2(floor(position.xy*map.xy+map.zw)),int2(0,0),int2(w,h)-1);
    return depth.Load(int3(texel,0));
}
