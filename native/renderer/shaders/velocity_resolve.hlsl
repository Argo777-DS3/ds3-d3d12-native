// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The velocity resolve (scene_pass_velocity.cpp): VS and PS, vs_5_0 / ps_5_0.
cbuffer Resolve : register(b0) { row_major float4x4 current_to_previous; float4 scale; float4 screen; };
Texture2D<float4> velocity : register(t0);
Texture2D<float> depth : register(t1);
float4 VS(uint id : SV_VertexID) : SV_Position { const float2 t=float2((id<<1)&2,id&2); return float4(t*float2(2,-2)+float2(-1,1),0,1); }
float2 PS(float4 position : SV_Position) : SV_Target {
    const int3 pixel=int3(position.xy,0);
    const float4 encoded=velocity.Load(pixel);
    const float2 uv=position.xy*screen.zw;
    float2 previous;
    if(encoded.w>0.5) previous=uv-float2(0.5,-0.5)*((encoded.xy-0.5)*2.0/scale.xy);
    else { const float4 h=mul(float4(uv,depth.Load(pixel),1),current_to_previous); previous=h.xy/h.w; }
    return (previous-uv)*screen.xy;
}
