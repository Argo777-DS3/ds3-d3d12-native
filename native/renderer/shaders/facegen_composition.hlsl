// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The native FaceGen composition's base layer (game_texture_ingress.cpp): VS and PS, vs_5_0 / ps_5_0.
Texture2D<float4> face:register(t0);SamplerState linearClamp:register(s0);
struct V{float4 position:SV_Position;float2 uv:TEXCOORD0;};
V VS(uint id:SV_VertexID){V o;float2 uv=float2((id<<1)&2,id&2);o.position=float4(uv*float2(2,-2)+float2(-1,1),0,1);o.uv=uv;return o;}
float4 PS(V i):SV_Target{return face.Sample(linearClamp,i.uv);}
