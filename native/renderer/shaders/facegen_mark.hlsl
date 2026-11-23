// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The native FaceGen composition's marks (game_texture_ingress.cpp): VS and PS, vs_5_0 / ps_5_0.
Texture2D<float4> mark:register(t0);SamplerState linearWrap:register(s0);
cbuffer Layer:register(b0){float4 corners[2];float4 colour;};
struct V{float4 position:SV_Position;float2 uv:TEXCOORD0;};
V VS(uint id:SV_VertexID){V o;const float4 pair=corners[id>>1];o.position=float4((id&1)?pair.zw:pair.xy,0,1);o.uv=float2(id&1,id>>1);return o;}
float4 PS(V i):SV_Target{return mark.Sample(linearWrap,i.uv)*colour;}
