// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The effect layer's programmes (scene_pass_frame.cpp, the layer's pipelines): VS, Downsample, Composite and Bilateral, vs_5_0 / ps_5_0.
Texture2D<float4> source:register(t0);Texture2D<float> fullDepth:register(t1);Texture2D<float> halfDepth:register(t2);
SamplerState pointClamp:register(s0);SamplerState linearClamp:register(s1);
cbuffer SampleOffsets:register(b0){float4 offsets[6];};
struct V{float4 position:SV_Position;float2 uv:TEXCOORD0;};
V VS(uint id:SV_VertexID){V o;const float2 uv=float2((id<<1)&2,id&2);o.position=float4(uv*float2(2,-2)+float2(-1,1),0,1);o.uv=uv;return o;}
float Downsample(V i):SV_Depth{uint w,h;source.GetDimensions(w,h);const int2 last=int2(w,h)-1,at=int2(i.position.xy)*2;
 const float a=source.Load(int3(min(at,last),0)).r,b=source.Load(int3(min(at+int2(1,0),last),0)).r;
 const float c=source.Load(int3(min(at+int2(0,1),last),0)).r,d=source.Load(int3(min(at+int2(1,1),last),0)).r;return min(min(a,b),min(c,d));}
float4 Composite(V i):SV_Target{return source.SampleLevel(linearClamp,i.uv,0);}
float4 Bilateral(V i):SV_Target{const float2 uv=i.uv;
 const float4 bl=source.SampleLevel(linearClamp,uv+offsets[2].xy*0.5,0),tl=source.SampleLevel(linearClamp,uv+offsets[0].xy*0.5,0);
 const float4 tr=source.SampleLevel(linearClamp,uv+offsets[1].xy*0.5,0),br=source.SampleLevel(linearClamp,uv+offsets[3].xy*0.5,0);
 const float inverse_z=1.0/(-offsets[5].y/(offsets[5].x-fullDepth.Sample(pointClamp,uv)));
 const float4 z=-offsets[5].yyyy/(offsets[5].xxxx-halfDepth.GatherRed(linearClamp,uv).wzxy);
 float4 w=1.0/(abs(z*inverse_z-1.0)+0.01);
 const float2 f=frac(uv*offsets[4].xy);
 const float w_tr=w.y/length(float2(f.x,1.0-f.y)),w_tl=w.x/length(1.0-f);
 const float w_bl=w.z/length(float2(1.0-f.x,f.y)),w_br=w.w/length(f);
 return (tl*w_tl+tr*w_tr+bl*w_bl+br*w_br)/(w_tl+w_tr+w_bl+w_br);}
